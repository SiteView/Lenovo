#include "WlanCoreImpl.h"
#include <QDebug>
//#include <QMessageBox>

WlanCoreImpl::WlanCoreImpl(WlanCore *intf)
	: m_intf(intf), m_wlanapi(NULL), m_wlanHandle(NULL)
	, m_deviceOk(false)
	, m_softwareEnabled(false), m_hardwareEnabled(false), m_enabled(false)
	, m_connected(false), m_signalQuality(0)
{
}

WlanCoreImpl::~WlanCoreImpl()
{
	if (m_wlanHandle) {
//		m_wlanapi->WlanRegisterNotification(m_wlanHandle, WLAN_NOTIFICATION_SOURCE_NONE, TRUE, NULL, NULL, NULL, NULL);
		m_wlanapi->WlanCloseHandle(m_wlanHandle, NULL);
		m_wlanHandle = NULL;
	}

	if (m_wlanapi) {
		delete m_wlanapi;
		m_wlanapi = NULL;
	}
}

WlanCore *WlanCoreImpl::q_ptr() const
{
	return m_intf;
}

bool WlanCoreImpl::init()
{
	m_wlanapi = new WlanLib();
	if (!m_wlanapi->init()) {
		return false;
	}

	OSVERSIONINFOEX osver;
	osver.dwOSVersionInfoSize = sizeof(osver);
	if (!GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&osver))) {
		return false;
	}

	m_wxp = osver.dwMajorVersion <= 5;

	DWORD status;
	DWORD actualVersion;
	status = m_wlanapi->WlanOpenHandle(1, NULL, &actualVersion, &m_wlanHandle);
	if (status != ERROR_SUCCESS) {
		return false;
	}

	cacheInterfaceInfo();

	status = m_wlanapi->WlanRegisterNotification(m_wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, &WlanCoreImpl::wlanCallback, this, NULL, NULL);
	if (status != ERROR_SUCCESS) {
		return false;
	}

	return true;
}

void WlanCoreImpl::wlanCallback(PWLAN_NOTIFICATION_DATA data, PVOID ctx)
{
	WlanCoreImpl *that = static_cast<WlanCoreImpl*>(ctx);
//	switch (data->NotificationCode) {
//	case wlan_notification_acm_interface_arrival:
//	case wlan_notification_acm_interface_removal:
		QMetaObject::invokeMethod(that, "cacheInterfaceInfo", Qt::QueuedConnection);
//		break;
//	default:
//		qDebug() << data->NotificationCode;
//		break;
//	}
	//emit static_cast<WlanCoreImpl*>(ctx)->q_ptr()->statusChanged();
}

void WlanCoreImpl::cacheInterfaceInfo()
{
	m_deviceOk = false;
	memset(&m_deviceId, 0, sizeof(m_deviceId));
	PWLAN_INTERFACE_INFO_LIST intfList;
	if (ERROR_SUCCESS == m_wlanapi->WlanEnumInterfaces(m_wlanHandle, NULL, &intfList)) {
		if (intfList->dwNumberOfItems > 0) {
			const WLAN_INTERFACE_INFO *intf = intfList->InterfaceInfo + 0;
			m_deviceOk = true;
			m_deviceId = intf->InterfaceGuid;
			m_deviceState = intf->isState;
			m_deviceDesc = QString::fromWCharArray(intf->strInterfaceDescription);
		}
		m_wlanapi->WlanFreeMemory(intfList);
	}

	if (m_deviceOk) {
		cacheRadioState();
		cacheConnectionInfo();
	} else {
		setSoftwareEnabled(false);
		setHardwareEnabled(false);
		setEnabled(false);
		setSsid(QString());
		setMacAddress(QByteArray());
		setSignalQuality(0);
		setProfileName(QString());
		setConnected(false);
	}
}

void WlanCoreImpl::cacheRadioState()
{
	// TODO: XP support
	bool hwOn = false;
	bool swOn = false;

	if (m_deviceOk) {
		PWLAN_RADIO_STATE state;
		DWORD cb = sizeof(state);
		WLAN_OPCODE_VALUE_TYPE vt;
		if (ERROR_SUCCESS == m_wlanapi->WlanQueryInterface(m_wlanHandle, &m_deviceId, wlan_intf_opcode_radio_state, NULL, &cb, reinterpret_cast<PVOID*>(&state), &vt)) {
			for (DWORD i = 0; i < state->dwNumberOfPhys; i++) {
				if (state->PhyRadioState[i].dot11HardwareRadioState == dot11_radio_state_on) {
					hwOn = true;
				}
				if (state->PhyRadioState[i].dot11SoftwareRadioState == dot11_radio_state_on) {
					swOn = true;
				}
			}
			m_wlanapi->WlanFreeMemory(state);
		}
	}

	setSoftwareEnabled(swOn);
	setHardwareEnabled(hwOn);
	setEnabled(swOn && hwOn);
}

void WlanCoreImpl::cacheConnectionInfo()
{
	QString profileName;
	QString ssid;
	bool connected = false;
	int signalQuality = 0;
	QByteArray macAddress;

	if (m_deviceOk) {
		PWLAN_CONNECTION_ATTRIBUTES conn;
		DWORD cb = sizeof(conn);
		WLAN_OPCODE_VALUE_TYPE vt;
		if (ERROR_SUCCESS == m_wlanapi->WlanQueryInterface(m_wlanHandle, &m_deviceId, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&conn), &vt)) {
			profileName = QString::fromWCharArray(conn->strProfileName).trimmed();
			connected = (conn->isState == wlan_interface_state_connected);
			ssid = QString::fromLatin1(reinterpret_cast<char*>(conn->wlanAssociationAttributes.dot11Ssid.ucSSID), conn->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
			signalQuality = conn->wlanAssociationAttributes.wlanSignalQuality;
			macAddress = QByteArray(reinterpret_cast<char*>(conn->wlanAssociationAttributes.dot11Bssid), 6);
			m_wlanapi->WlanFreeMemory(conn);
		}
	}

	setSsid(ssid);
	setMacAddress(macAddress);
	setSignalQuality(signalQuality);
	setProfileName(profileName);
	setConnected(connected);
}

QList<WlanCore::Network> WlanCoreImpl::snapshotNetworkList() const
{
	QList<WlanCore::Network> ls;
	if (m_deviceOk) {
		PWLAN_AVAILABLE_NETWORK_LIST networkList;
		if (ERROR_SUCCESS == m_wlanapi->WlanGetAvailableNetworkList(m_wlanHandle, &m_deviceId, 0, NULL, &networkList)) {
			for (DWORD i = 0; i < networkList->dwNumberOfItems; i++) {
				WLAN_AVAILABLE_NETWORK *network = networkList->Network + i;
				if (network->bNetworkConnectable) {
					WlanCore::Network item;
					item.ssid = QString::fromLatin1(reinterpret_cast<char*>(network->dot11Ssid.ucSSID), network->dot11Ssid.uSSIDLength);
					item.signalQuality = network->wlanSignalQuality;
					item.bssType = network->dot11BssType;
					item.connected = (network->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED) != 0;
					item.securityEnabled = (network->bSecurityEnabled ? true : false);
					if ((network->dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) != 0) {
						item.profileName = QString::fromWCharArray(network->strProfileName);
					}

					/*if (m_wlanapi->WlanGetNetworkBssList) {
						PWLAN_BSS_LIST bssList;
						if (ERROR_SUCCESS == m_wlanapi->WlanGetNetworkBssList(m_wlanHandle, &m_deviceId, &network->dot11Ssid, network->dot11BssType, network->bSecurityEnabled, NULL, &bssList)) {
							m_wlanapi->WlanFreeMemory(bssList);
						}
					}*/

					ls.append(item);
				}
			}
			m_wlanapi->WlanFreeMemory(networkList);
		}
	}
	return ls;
}

QString WlanCoreImpl::getProfileContent(const QString& profileName) const
{
	QString content;
	if (m_deviceOk) {
		WCHAR profile[256];
		int len = profileName.toWCharArray(profile);
		profile[len] = 0;
		LPWSTR xml;
		DWORD err = m_wlanapi->WlanGetProfile(m_wlanHandle, &m_deviceId, profile, NULL, &xml, NULL, NULL);
		if (ERROR_SUCCESS == err) {
			content = QString::fromWCharArray(xml);
			m_wlanapi->WlanFreeMemory(xml);
		} else {
			content = QString::fromUtf8("%1", err);
		}
	}
	return content;
}

void WlanCoreImpl::connectWithProfile(const QString& profileName, int bssType)
{
	if (m_deviceOk) {
		WCHAR profile[256];
		int len = profileName.toWCharArray(profile);
		profile[len] = 0;
		WLAN_CONNECTION_PARAMETERS cp;
		cp.wlanConnectionMode = wlan_connection_mode_profile;
		cp.strProfile = profile;
		cp.pDot11Ssid = NULL;
		cp.pDesiredBssidList = NULL;
		cp.dot11BssType = static_cast<DOT11_BSS_TYPE>(bssType);
		cp.dwFlags = 0;
		DWORD err = m_wlanapi->WlanConnect(m_wlanHandle, &m_deviceId, &cp, NULL);
		err = 0;
	}
}

void WlanCoreImpl::connectWithSsid(const QString& ssid, bool securityEnabled)
{
	if (m_deviceOk) {
		QByteArray s = ssid.toLatin1();
		DOT11_SSID ssid;
		ssid.uSSIDLength = s.length();
		memcpy(ssid.ucSSID, s.data(), s.length());
		WLAN_CONNECTION_PARAMETERS cp;
		cp.wlanConnectionMode = securityEnabled ? wlan_connection_mode_discovery_secure : wlan_connection_mode_discovery_unsecure;
		cp.strProfile = NULL;
		cp.pDot11Ssid = &ssid;
		cp.pDesiredBssidList = NULL;
		cp.dot11BssType = dot11_BSS_type_infrastructure;
		cp.dwFlags = 0;
		DWORD err = m_wlanapi->WlanConnect(m_wlanHandle, &m_deviceId, &cp, NULL);
		err = 0;
	}
}

void WlanCoreImpl::connectForce(const QString& ssid, const QString& profileName, bool securityEnabled)
{
	if (m_deviceOk) {
		if (profileName.isEmpty()) {
			const char *tmpl = "<?xml version=\"1.0\" encoding=\"US-ASCII\"?>\r\n<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\r\n<name>%2</name>\r\n<SSIDConfig>\r\n<SSID>\r\n<name>%1</name>\r\n</SSID>\r\n</SSIDConfig>\r\n<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>%3</security></MSM></WLANProfile>";
			QString security;
			if (securityEnabled) {
				security = QString::fromLatin1("<authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>12345678</keyMaterial></sharedKey>");
			} else {
				security = QString::fromLatin1("<authEncryption><authentication>open</authentication><encryption>none</encryption><useOneX>false</useOneX></authEncryption>");
			}
			QString s = QString::fromLatin1(tmpl).arg(ssid).arg(ssid).arg(security);
			WCHAR *xml = new WCHAR[s.length() + 20];
			int len = s.toWCharArray(xml);
			xml[len] = 0;
			WLAN_REASON_CODE reason;
			DWORD err = m_wlanapi->WlanSetProfile(m_wlanHandle, &m_deviceId, 0, xml, NULL, TRUE, NULL, &reason);
			delete[] xml;
			/*if (err != 0) {
				WCHAR buff[2000];
				DWORD len2 = m_wlanapi->WlanReasonCodeToString(reason, 2000, buff, NULL);
				QString text = QString::fromLatin1("%1 %2 [%3]").arg(err).arg(reason).arg(QString::fromWCharArray(buff, -1));
				QMessageBox::warning(NULL, tr("tt"), text);
			}
			err = 0;*/
			if (err != ERROR_SUCCESS) {
				return;
			}
		}
		connectWithProfile(profileName, dot11_BSS_type_infrastructure);
	}
}

void WlanCoreImpl::setSoftwareEnabled(bool enabled)
{
	if (m_softwareEnabled != enabled) {
		m_softwareEnabled = enabled;
		emit q_ptr()->softwareEnabledChanged();
	}
}

void WlanCoreImpl::setHardwareEnabled(bool enabled)
{
	if (m_hardwareEnabled != enabled) {
		m_hardwareEnabled = enabled;
		emit q_ptr()->hardwareEnabledChanged();
	}
}

void WlanCoreImpl::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		emit q_ptr()->enabledChanged();
	}
}

void WlanCoreImpl::setSsid(const QString& ssid)
{
	if (m_ssid != ssid) {
		m_ssid = ssid;
		emit q_ptr()->ssidChanged();
	}
}

void WlanCoreImpl::setMacAddress(const QByteArray& macAddress)
{
	if (m_macAddress != macAddress) {
		m_macAddress = macAddress;
		emit q_ptr()->macAddressChanged();
	}
}

void WlanCoreImpl::setSignalQuality(int signalQuality)
{
	if (m_signalQuality != signalQuality) {
		m_signalQuality = signalQuality;
		emit q_ptr()->signalQualityChanged();
	}
}

void WlanCoreImpl::setProfileName(const QString& profileName)
{
	if (m_profileName != profileName) {
		m_profileName = profileName;
		emit q_ptr()->profileNameChanged();
	}
}

void WlanCoreImpl::setConnected(bool connected)
{
	if (m_connected != connected) {
		m_connected = connected;
		emit q_ptr()->connectedChanged();
	}
}
