#ifndef __WlanCoreImpl_h__
#define __WlanCoreImpl_h__

#include <LenovoCore/WlanCore.h>
#include "wlanlib.h"

class WlanCoreImpl
	: public QObject
{
	Q_OBJECT

public:
	WlanCoreImpl(WlanCore *intf);
	virtual ~WlanCoreImpl();
	WlanCore *q_ptr() const;
	bool init();

	static void WINAPI wlanCallback(PWLAN_NOTIFICATION_DATA, PVOID);

	void setSoftwareEnabled(bool enabled);
	void setHardwareEnabled(bool enabled);
	void setEnabled(bool enabled);
	void setSsid(const QString& ssid);
	void setMacAddress(const QByteArray& macAddress);
	void setSignalQuality(int signalQuality);
	void setProfileName(const QString& profileName);
	void setConnected(bool connected);

	QList<WlanCore::Network> snapshotNetworkList() const;
	QString getProfileContent(const QString& profileName) const;
	void connectWithProfile(const QString& profileName, int bssType);
	void connectWithSsid(const QString& ssid, bool securityEnabled);
	void connectForce(const QString& ssid, const QString& profileName, bool securityEnabled);

public Q_SLOTS:
	void cacheInterfaceInfo();
	void cacheRadioState();
	void cacheConnectionInfo();

public:
	WlanCore *m_intf;
	WlanLib *m_wlanapi;
	HANDLE m_wlanHandle;
	GUID m_deviceId;
	QString m_deviceDesc;
	WLAN_INTERFACE_STATE m_deviceState;
	bool m_deviceOk;
	bool m_wxp;

	bool m_softwareEnabled;
	bool m_hardwareEnabled;
	bool m_enabled;
	bool m_connected;
	QString m_ssid;
	QByteArray m_macAddress;
	int m_signalQuality;
	QString m_profileName;
};

#endif // __WlanCoreImpl_h__
