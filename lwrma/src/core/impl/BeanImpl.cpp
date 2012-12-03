#include "BeanImpl.h"
#include <LenovoCore/Logger.h>
#include <qdebug.h>

DEFINE_LOGGER(Bean);

static const QString NS_WLANConfiguration = QLatin1String("WLANConfiguration");

BeanImpl::BeanImpl(Bean *intf, const QString& ident)
	: m_intf(intf)
{
	m_system = new System(ident);
	m_soapCore = new SoapCore();
}

BeanImpl::~BeanImpl()
{
}

Bean *BeanImpl::q_ptr() const
{
	return m_intf;
}
	
bool BeanImpl::init()
{
	if (!m_system->init()) {
		return false;
	}
	return true;
}

AsyncOp *BeanImpl::changeRouterWifiPassword(const QString& password, const QString& ssid, bool configStart, bool configFinish)
{
	ChangeRouterWifiPasswordOp *op = new ChangeRouterWifiPasswordOp(this, password, ssid, configStart, configFinish);
	op->start();
	return op;
}

AsyncOp *BeanImpl::changeRouterPassword(const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword)
{
	ChangeRouterPasswordOp *op = new ChangeRouterPasswordOp(this, wifiPassword, oldWifiPassword, adminPassword, oldAdminPassword);
	op->start();
	return op;
}

AsyncOp *BeanImpl::changeRouterWanSettings(const QVariantMap& settings)
{
	ChangeRouterWanSettingsOp *op = new ChangeRouterWanSettingsOp(this, settings);
	op->start();
	return op;
}

AsyncOp *BeanImpl::ensureSoap(int delay, int retryCount)
{
	EnsureSoapOp *op = new EnsureSoapOp(this, delay, retryCount);
	op->start();
	return op;
}

AsyncOp *BeanImpl::discoverRouterSoap(int timeout, const QString& mac, int maxRetryCount, int retryDelay)
{
//	DiscoverRouterSoapOp *op = new DiscoverRouterSoapOp(this, timeout, mac);
//	op->start();
	MasterDiscoverRouterSoapOp *op = new MasterDiscoverRouterSoapOp(this, timeout, mac, maxRetryCount, retryDelay);
	op->start();
	return op;
}

AsyncOp *BeanImpl::reconnectRouter(int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect)
{
	ReconnectRouterOp *op = new ReconnectRouterOp(this, delay, maxRetryCount, mac, wifiName, reconnect);
	op->start();
	return op;
}

AsyncOp *BeanImpl::restartRouter()
{
	RestartRouterOp *op = new RestartRouterOp(this);
	op->start();
	return op;
}

AsyncOp *BeanImpl::checkInternet(const QString& host, int delay, int maxRetryCount)
{
	CheckInternetOp *op = new CheckInternetOp(this, host, delay, maxRetryCount);
	op->start();
	return op;
}

ChangeRouterWifiPasswordOp::ChangeRouterWifiPasswordOp(BeanImpl *bean, const QString& password, const QString& ssid, bool configStart, bool configFinish)
	: m_bean(bean), m_password(password), m_ssid(ssid), m_configStart(configStart), m_configFinish(configFinish)
{
}

ChangeRouterWifiPasswordOp::~ChangeRouterWifiPasswordOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void ChangeRouterWifiPasswordOp::start()
{
	LOG_DEBUG(QString::fromUtf8("ChangeRouterWifiPasswordOp::start"));
	m_bean->m_soapCore->setWrappedMode(false);
	if (m_configStart) {
		m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationStarted"), QStringList(QLatin1String("NewSessionID")), QStringList(m_bean->m_soapCore->sessionId()));
		connect(m_op, SIGNAL(finished()), SLOT(onConfigStarted()));
	} else {
		m_op = m_bean->m_soapCore->invoke(NS_WLANConfiguration, QLatin1String("GetInfo"));
		connect(m_op, SIGNAL(finished()), SLOT(onGetInfoFinished()));
	}
}

void ChangeRouterWifiPasswordOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void ChangeRouterWifiPasswordOp::onConfigStarted()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onConfigStarted %1").arg(result));
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	m_op = m_bean->m_soapCore->invoke(NS_WLANConfiguration, QLatin1String("GetInfo"));
	connect(m_op, SIGNAL(finished()), SLOT(onGetInfoFinished()));
}

void ChangeRouterWifiPasswordOp::onGetInfoFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onGetInfoFinished %1").arg(result));
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	QVariant varResponse = op->value("response");
	if (!varResponse.isValid()) {
		return notifyFinished(UnknownError);
	}

	QVariantMap response = qvariant_cast<QVariantMap>(varResponse);

	QString ssid = m_ssid.isNull() ? response.value(QLatin1String("NewSSID")).toString() : m_ssid;

	QStringList paramNames;
	QStringList paramValues;

	paramNames << QLatin1String("NewSSID") << QLatin1String("NewRegion") << QLatin1String("NewChannel") << QLatin1String("NewWirelessMode");
	paramValues << ssid << response.value(QLatin1String("NewRegion")).toString() << response.value(QLatin1String("NewChannel")).toString() << response.value(QLatin1String("NewWirelessMode")).toString();

	QString actionName;
	if (m_password.isNull()) {
		actionName = QLatin1String("SetWLANNoSecurity");
	} else {
		actionName = QLatin1String("SetWLANWPAPSKByPassphrase");
		paramNames << QLatin1String("NewWPAEncryptionModes") << QLatin1String("NewWPAPassphrase");
		paramValues << QLatin1String("WPA2-PSK") << m_password;
	}
	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(NS_WLANConfiguration, actionName, paramNames, paramValues);
	connect(m_op, SIGNAL(finished()), SLOT(onSetWLANFinished()));
}

void ChangeRouterWifiPasswordOp::onSetWLANFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onSetWLANFinished %1").arg(result));
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	if (m_configFinish) {
		m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationFinished"), QLatin1String("NewStatus"), QLatin1String("ChangesApplied"));
		connect(m_op, SIGNAL(finished()), SLOT(onCommitFinished()));

		connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout1()));
		m_timer1.setSingleShot(true);
		m_timer1.start(5000);
	} else {
		notifyFinished(NoError);
	}
}

void ChangeRouterWifiPasswordOp::onTimeout1()
{
	LOG_DEBUG(QString::fromUtf8("ChangeRouterWifiPasswordOp::onTimeout1"));
	if (m_op) {
		m_op->abort();
	}
	notifyFinished(NoError);
}

void ChangeRouterWifiPasswordOp::onCommitFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	LOG_DEBUG(QString::fromUtf8("onCommitFinished %1").arg(op->result()));

	notifyFinished(NoError);
}

ChangeRouterWanSettingsOp::ChangeRouterWanSettingsOp(BeanImpl *bean, const QVariantMap& settings)
	: m_bean(bean), m_settings(settings)
{
}

ChangeRouterWanSettingsOp::~ChangeRouterWanSettingsOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void ChangeRouterWanSettingsOp::start()
{
	QString actionName;
	QStringList paramNames;
	QStringList paramValues;

	QString type = m_settings.value(QLatin1String("ConnectionType")).toString();
	if (type.compare(QLatin1String("static"), Qt::CaseInsensitive) == 0) {
		actionName = QLatin1String("SetIPInterfaceInfo");
		paramNames << QLatin1String("NewAddressingType")
			<< QLatin1String("NewExternalIPAddress")
			<< QLatin1String("NewSubnetMask")
			<< QLatin1String("NewDefaultGateway")
			<< QLatin1String("NewPrimaryDNS")
			<< QLatin1String("NewSecondaryDNS");

		paramValues << QLatin1String("Static")
			<< m_settings.value(QLatin1String("ExternalIPAddress")).toString()
			<< m_settings.value(QLatin1String("SubnetMask")).toString()
			<< m_settings.value(QLatin1String("DefaultGateway")).toString()
			<< m_settings.value(QLatin1String("PrimaryDNS")).toString()
			<< m_settings.value(QLatin1String("SecondaryDNS")).toString();

	} else {
/*
		actionName = QLatin1String("SetConnectionType2");
		paramNames << QLatin1String("NewConnectionType")
			<< QLatin1String("NewPrimaryDNS")
			<< QLatin1String("NewSecondaryDNS")
			<< QLatin1String("NewAccountName")
			<< QLatin1String("NewISPLoginname")
			<< QLatin1String("NewISPPassword")
			<< QLatin1String("NewConnectionMode");

		if (type.compare(QLatin1String("pppoe"), Qt::CaseInsensitive) == 0) {
			paramValues << QLatin1String("PPPoE")
				<< QString()
				<< QString()
				<< QString()
				<< m_settings.value(QLatin1String("ISPLoginname")).toString()
				<< m_settings.value(QLatin1String("ISPPassword")).toString()
				<< QLatin1String("AlwaysOn");
		} else {
			paramValues << QLatin1String("DHCP")
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString();
		}
/*/
		actionName = QLatin1String("SetConnectionType");
		paramNames << QLatin1String("NewConnectionType")
			<< QLatin1String("NewISPLoginname")
			<< QLatin1String("NewISPPassword")
			<< QLatin1String("NewConnectionMode")
			<< QLatin1String("Newidletimer")
			<< QLatin1String("NewConnectionID")
			<< QLatin1String("NewPPTPMyIP")
			<< QLatin1String("NewPPTPServerIP")
			<< QLatin1String("NewBigpondAuthServer")
			<< QLatin1String("NewPrimaryDNS")
			<< QLatin1String("NewSecondaryDNS")
			<< QLatin1String("NewNATEnable");

		if (type.compare(QLatin1String("pppoe"), Qt::CaseInsensitive) == 0) {
			paramValues << QLatin1String("PPPoE")
				<< m_settings.value(QLatin1String("ISPLoginname")).toString()
				<< m_settings.value(QLatin1String("ISPPassword")).toString()
				<< QLatin1String("AlwaysOn")
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString();
		} else {
			paramValues << QLatin1String("DHCP")
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString()
				<< QString();
		}
//*/
	}

	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("WANIPConnection"), actionName, paramNames, paramValues);
	connect(m_op, SIGNAL(finished()), SLOT(onSoapFinished()));
}

void ChangeRouterWanSettingsOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void ChangeRouterWanSettingsOp::onSoapFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	notifyFinished(NoError);
}

ChangeRouterPasswordOp::ChangeRouterPasswordOp(BeanImpl *bean, const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword)
	: m_bean(bean), m_wifiPassword(wifiPassword), m_oldWifiPassword(oldWifiPassword), m_adminPassword(adminPassword), m_oldAdminPassword(oldAdminPassword)
{
}

ChangeRouterPasswordOp::~ChangeRouterPasswordOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void ChangeRouterPasswordOp::start()
{
	if (!m_adminPassword.isNull() || !m_wifiPassword.isNull()) {
		LOG_DEBUG(QString::fromUtf8("changeRouterPassword %1 %2").arg(m_adminPassword.isNull()).arg(m_wifiPassword.isNull()));
		m_bean->m_soapCore->setWrappedMode(false);
		m_op = m_bean->m_soapCore->invoke(NS_WLANConfiguration, QLatin1String("GetInfo"));
		connect(m_op, SIGNAL(finished()), SLOT(onGetInfoFinished()));
	} else {
		LOG_DEBUG(QString::fromUtf8("changeRouterPassword nothing to do"));
		QMetaObject::invokeMethod(this, "onNothingToDo", Qt::QueuedConnection);
	}
}

void ChangeRouterPasswordOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void ChangeRouterPasswordOp::onNothingToDo()
{
	notifyFinished(NoError);
}

void ChangeRouterPasswordOp::onGetInfoFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	QVariant varResponse = op->value("response");
	if (!varResponse.isValid()) {
		return notifyFinished(UnknownError);
	}

	m_infoResp = qvariant_cast<QVariantMap>(varResponse);

	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("WLANConfiguration"), QLatin1String("GetWPASecurityKeys"));
	connect(m_op, SIGNAL(finished()), SLOT(onGetKeysFinished()));
}

void ChangeRouterPasswordOp::onGetKeysFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	QVariant varResponse = op->value("response");
	if (!varResponse.isValid()) {
		return notifyFinished(UnknownError);
	}

	QVariantMap resp = qvariant_cast<QVariantMap>(varResponse);
	m_currentWifiKey = resp.value(QLatin1String("NewWPAPassphrase")).toString();

	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("LANConfigSecurity"), QLatin1String("GetInfo"));
	connect(m_op, SIGNAL(finished()), SLOT(onGetLanInfoFinished()));
}

void ChangeRouterPasswordOp::onGetLanInfoFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	QVariant varResponse = op->value("response");
	if (!varResponse.isValid()) {
		return notifyFinished(UnknownError);
	}

	QVariantMap resp = qvariant_cast<QVariantMap>(varResponse);

/*	if (!m_wifiPassword.isNull()) {
		if (m_currentWifiKey != m_oldWifiPassword) {
			return notifyFinished(InvalidWifiPasswordError);
		}
	}
*/
	if (!m_adminPassword.isNull()) {
		QString oldAdminPassword = resp.value(QLatin1String("NewPassword")).toString();
		if (oldAdminPassword != m_oldAdminPassword) {
			return notifyFinished(InvalidAdminPasswordError);
		}
	}

	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationStarted"), QStringList(QLatin1String("NewSessionID")), QStringList(m_bean->m_soapCore->sessionId()));
	connect(m_op, SIGNAL(finished()), SLOT(onConfigInFinished()));
}

void ChangeRouterPasswordOp::onConfigInFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	if (!m_wifiPassword.isNull()) {
		startWifiConfig();
	} else {
		startAdminConfig();
	}
}

void ChangeRouterPasswordOp::startWifiConfig()
{
	QStringList paramNames;
	QStringList paramValues;

	paramNames << QLatin1String("NewSSID")
		<< QLatin1String("NewRegion")
		<< QLatin1String("NewChannel")
		<< QLatin1String("NewWirelessMode")
		<< QLatin1String("NewWPAEncryptionModes")
		<< QLatin1String("NewWPAPassphrase");

	paramValues << m_infoResp.value(QLatin1String("NewSSID")).toString()
		<< m_infoResp.value(QLatin1String("NewRegion")).toString()
		<< m_infoResp.value(QLatin1String("NewChannel")).toString()
		<< m_infoResp.value(QLatin1String("NewWirelessMode")).toString()
		<< QLatin1String("WPA2-PSK")
		<< m_wifiPassword;

	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(NS_WLANConfiguration, QLatin1String("SetWLANWPAPSKByPassphrase"), paramNames, paramValues);
	connect(m_op, SIGNAL(finished()), SLOT(onWifiConfigFinished()));
}

void ChangeRouterPasswordOp::startAdminConfig()
{
	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("LANConfigSecurity"), QLatin1String("SetConfigPassword"), QLatin1String("NewPassword"), m_adminPassword);
	connect(m_op, SIGNAL(finished()), SLOT(onAdminConfigFinished()));
}

void ChangeRouterPasswordOp::startConfigOut()
{
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout1()));
	m_timer1.setSingleShot(true);
	m_timer1.start(5000);
	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationFinished"), QLatin1String("NewStatus"), QLatin1String("ChangesApplied"));
	connect(m_op, SIGNAL(finished()), SLOT(onConfigOutFinished()));
}

void ChangeRouterPasswordOp::onTimeout1()
{
	m_op->abort();
	delete m_op;
	m_op = NULL;

	updateProfile();
}

void ChangeRouterPasswordOp::onWifiConfigFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	if (!m_adminPassword.isNull()) {
		startAdminConfig();
	} else {
		startConfigOut();
	}
}

void ChangeRouterPasswordOp::onAdminConfigFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	startConfigOut();
}

void ChangeRouterPasswordOp::onConfigOutFinished()
{
	m_timer1.stop();

	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		return;
	}

	if (isAborted()) {
		return;
	}

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

//	if (responseCode != 0) {
//		return notifyFinished(UnknownError);
//	}

//	notifyFinished(NoError);

	updateProfile();
}

void ChangeRouterPasswordOp::updateProfile()
{
	if (!m_wifiPassword.isNull()) {
		QString profileName(m_infoResp.value(QLatin1String("NewSSID")).toString());
		LOG_DEBUG(QString::fromUtf8("Now trying to update profile %1").arg(profileName));
		m_op = m_bean->m_system->createWlanProfile(profileName, m_wifiPassword);
		connect(m_op, SIGNAL(finished()), SLOT(onCreateProfileFinished()));
	} else {
		notifyFinished(NoError);
	}
}

void ChangeRouterPasswordOp::onCreateProfileFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("createWlanProfile result: %1").arg(result));
	if (result == WlanRadioOffError || result == WlanServiceDownError || result == WlanNoDeviceError) {
		return notifyFinished(NoError);
	}

	if (result != AsyncOp::NoError) {
		return notifyFinished(result);
	}

	m_op = m_bean->reconnectRouter(3000, 10, m_infoResp.value(QLatin1String("NewWLANMACAddress")).toString(), m_infoResp.value(QLatin1String("NewSSID")).toString(), true);
	connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
}

void ChangeRouterPasswordOp::onReconnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("reconnectRouter result: %1").arg(result));

	if (result != NoError) {
		notifyFinished(result);
	}

	notifyFinished(NoError);
}

EnsureSoapOp::EnsureSoapOp(BeanImpl *bean, int delay, int retryCount)
	: m_bean(bean), m_delay(delay), m_retryCount(retryCount), m_currentCount(0)
{
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout()));
	m_timer1.setSingleShot(true);
}

EnsureSoapOp::~EnsureSoapOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void EnsureSoapOp::start()
{
	restart();
}

void EnsureSoapOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void EnsureSoapOp::restart()
{
	m_bean->m_soapCore->setWrappedMode(false);
	m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceInfo"), QLatin1String("GetInfo"));
	connect(m_op, SIGNAL(finished()), SLOT(onSoapFinished()));
}

void EnsureSoapOp::onSoapFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	if (result != NoError) {
		if (++m_currentCount <= m_retryCount) {
			LOG_DEBUG(QString::fromUtf8("EnsureSoap: failed, retry %1").arg(m_currentCount));
			m_timer1.start(m_delay);
			return;
		}
		LOG_DEBUG(QString::fromUtf8("EnsureSoap: no more chance!"));
		return notifyFinished(result);
	}

	LOG_DEBUG(QString::fromUtf8("EnsureSoap: OK!"));
	notifyFinished(NoError);
}

void EnsureSoapOp::onTimeout()
{
	if (!isAborted()) {
		LOG_DEBUG(QString::fromUtf8("EnsureSoap: delayed retry start"));
		restart();
	}
}

CheckRouterSoapOp::CheckRouterSoapOp(BeanImpl *bean, const QString& host)
	: m_bean(bean), m_host(host)
{
}

CheckRouterSoapOp::~CheckRouterSoapOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void CheckRouterSoapOp::start()
{
	LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::start()"));

	SoapCore *soapCore = m_bean->m_soapCore;
	QString host = soapCore->host();
	soapCore->setHost(m_host);
	m_op = soapCore->invoke(QLatin1String("DeviceInfo"), QLatin1String("GetInfo"));
	soapCore->setHost(host);
	connect(m_op, SIGNAL(finished()), SLOT(onGetInfo1Finished()));
}

void CheckRouterSoapOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void CheckRouterSoapOp::onGetInfo1Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo1Finished() aborted"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo1Finished() %1").arg(result));

	if (result != NoError) {
		return notifyFinished(result);
	}

	bool ok;
	int responseCode = op->value("responseCode").toInt(&ok);
	if (!ok || responseCode != 0) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo1Finished() response error: %1 %2").arg(responseCode).arg(ok));
		return notifyFinished(UnknownError);
	}

	m_values.unite(qvariant_cast<QVariantMap>(op->value("response")));

	SoapCore *soapCore = m_bean->m_soapCore;
	QString host = soapCore->host();
	soapCore->setHost(m_host);
	m_op = soapCore->invoke(QLatin1String("WLANConfiguration"), QLatin1String("GetInfo"));
	soapCore->setHost(host);
	connect(m_op, SIGNAL(finished()), SLOT(onGetInfo2Finished()));
}

void CheckRouterSoapOp::onGetInfo2Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo2Finished() aborted"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo2Finished() %1").arg(result));

	if (result != NoError) {
		return notifyFinished(result);
	}

	bool ok;
	int responseCode = op->value("responseCode").toInt(&ok);
	if (!ok || responseCode != 0) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo2Finished() response error: %1 %2").arg(responseCode).arg(ok));
		return notifyFinished(UnknownError);
	}

	m_values.unite(qvariant_cast<QVariantMap>(op->value("response")));

	SoapCore *soapCore = m_bean->m_soapCore;
	QString host = soapCore->host();
	soapCore->setHost(m_host);
	m_op = soapCore->invoke(QLatin1String("DeviceInfo"), QLatin1String("GetSysUpTime"));
	soapCore->setHost(host);
	connect(m_op, SIGNAL(finished()), SLOT(onGetInfo3Finished()));
}

void CheckRouterSoapOp::onGetInfo3Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo3Finished() aborted"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo3Finished() %1").arg(result));

	if (result != NoError) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = op->value("responseCode").toInt(&ok);
	if (!ok || responseCode != 0) {
		LOG_DEBUG(QString::fromUtf8("CheckRouterSoapOp::onGetInfo3Finished() response error: %1 %2").arg(responseCode).arg(ok));
		return notifyFinished(result);
	}

	m_values.unite(qvariant_cast<QVariantMap>(op->value("response")));
	setValues(m_values);
	notifyFinished(result);
}

DiscoverRouterSoapOp::DiscoverRouterSoapOp(BeanImpl *bean, int timeout, const QString& mac)
	: m_bean(bean), m_timeout(timeout), m_mac(mac), m_count(0)
{
	connect(&m_timer, SIGNAL(timeout()), SLOT(onTimeout()));
	m_timer.setSingleShot(true);
	m_timer.setInterval(timeout);
}

DiscoverRouterSoapOp::~DiscoverRouterSoapOp()
{
	qDeleteAll(m_opList);
}

void DiscoverRouterSoapOp::start()
{
	QStringList ls = m_bean->m_system->queryGatewayList();
	//ls.push_front(QLatin1String("routerlogin.net"));
	m_ls = ls;

	if (ls.isEmpty()) {
		LOG_WARNING(QString::fromUtf8("DiscoverRouterSoapOp queryGatewayList result EMPTY, seems DHCP not yet ready!"));
	}

	for (int i = 0; i < ls.count(); i++) {
		CheckRouterSoapOp *op = new CheckRouterSoapOp(m_bean, ls.at(i));
		connect(op, SIGNAL(finished()), SLOT(onSoapFinished()));
		m_opList.append(op);
		op->start();
	}

	m_timer.start();
}

void DiscoverRouterSoapOp::onAbort()
{
}

void DiscoverRouterSoapOp::onSoapFinished()
{
	if (!m_mac.isNull()) {
		AsyncOp *op = qobject_cast<AsyncOp*>(sender());
		if (op) {
			int index = -1;
			for (int i = 0; i < m_opList.count(); i++) {
				if (m_opList.at(i) == op) {
					index = i;
					break;
				}
			}
			if (index >= 0) {
				QString mac = op->values().value(QLatin1String("newwlanmacaddress")).toString().toUpper();
				if (mac.compare(m_mac, Qt::CaseInsensitive) == 0) {
					LOG_DEBUG(QString::fromUtf8("DiscoverRouterSoapOp fast path for %1 %2").arg(mac).arg(m_ls.at(index)));
					for (int i = 0; i < m_opList.count(); i++) {
						AsyncOp *op = m_opList.at(i);
						disconnect(op, SIGNAL(finished()), this, SLOT(onSoapFinished()));
						op->abort();
					}
					QVariantMap varMap = op->values();
					varMap.insert(QLatin1String("soapHostName"), m_ls.at(index));
					QVariantList fullList;
					fullList.push_back(varMap);
					setValue("fullList", fullList);
					setValue("matchIndex", 0);
					return notifyFinished(NoError);
				}
			}
		}
	}

	if (m_count++ == m_opList.count()) {
		process();
	}
}

void DiscoverRouterSoapOp::onTimeout()
{
	for (int i = 0; i < m_opList.count(); i++) {
		m_opList.at(i)->abort();
	}
	process();
}

void DiscoverRouterSoapOp::process()
{
	QStringList ls1;
	QList<QVariantMap> ls2;
	for (int i = 0; i < m_opList.count(); i++) {
		AsyncOp *op = m_opList.at(i);
		if (op->isFinished() && op->result() == NoError) {
			ls1.append(m_ls.at(i));
			ls2.append(op->values());
		}
	}

	if (ls1.isEmpty()) {
		LOG_DEBUG(QString::fromUtf8("No router found!"));
		notifyFinished(UnknownError);
		return;
	}

	QSet<QString> set1;
	int count = ls1.count();
	int i = 0;
	while (i < count) {
		const QVariantMap& varMap = ls2.at(i);
		QString mac = varMap.value(QLatin1String("newwlanmacaddress")).toString().toUpper();
		if (set1.find(mac) == set1.end()) {
			set1.insert(mac);
			i++;
		} else {
			ls1.removeAt(i);
			ls2.removeAt(i);
			count--;
		}
	}

	int matchIndex = -1;
	QVariantList fullList;
	LOG_DEBUG(QString::fromUtf8("found %1 router(s)").arg(ls1.count()));
	for (int i = 0; i < ls1.count(); i++) {
		QString hostName = ls1.at(i);
		QVariantMap varMap = ls2.at(i);
		varMap.insert(QLatin1String("soapHostName"), hostName);
		QString mac = varMap.value(QLatin1String("newwlanmacaddress")).toString().toUpper();
		fullList.push_back(varMap);
		if (!m_mac.isNull() && mac.compare(m_mac, Qt::CaseInsensitive) == 0) {
			matchIndex = i;
		}
		LOG_DEBUG(QString::fromUtf8("%1 [%2]").arg(hostName).arg(mac));
	}

	setValue("fullList", fullList);
	setValue("matchIndex", matchIndex);
	notifyFinished(NoError);
}

MasterDiscoverRouterSoapOp::MasterDiscoverRouterSoapOp(BeanImpl *bean, int timeout, const QString& mac, int maxRetryCount, int retryDelay)
	: m_bean(bean), m_timeout(timeout), m_mac(mac), m_maxRetryCount(maxRetryCount), m_retryDelay(retryDelay), m_retryCount(0)
{
}

MasterDiscoverRouterSoapOp::~MasterDiscoverRouterSoapOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void MasterDiscoverRouterSoapOp::start()
{
	LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp start %1/%2").arg(m_retryCount).arg(m_maxRetryCount));
	DiscoverRouterSoapOp *op = new DiscoverRouterSoapOp(m_bean, m_timeout, m_mac);
	connect(op, SIGNAL(finished()), SLOT(onOpFinished()));
	op->start();
	m_op = op;
}

void MasterDiscoverRouterSoapOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void MasterDiscoverRouterSoapOp::onOpFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp::onOpFinished() aborted"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp::onOpFinished() %1").arg(result));

	if (result != NoError) {
		if (m_retryCount < m_maxRetryCount) {
			LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp Failed, but will retry later"));
			++m_retryCount;
			m_timer1.setSingleShot(true);
			m_timer1.setInterval(m_retryDelay);
			connect(&m_timer1, SIGNAL(timeout()), SLOT(onRetryTimeout()));
			m_timer1.start();
			return;
		}
		LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp Failed, no more retry chance!"));
		return notifyFinished(UnknownError);
	}

	copyValues(op);
	notifyFinished(NoError);
}

void MasterDiscoverRouterSoapOp::onRetryTimeout()
{
	LOG_DEBUG(QString::fromUtf8("MasterDiscoverRouterSoapOp retry now!"));
	disconnect(&m_timer1, SIGNAL(timeout()), this, SLOT(onRetryTimeout()));
	start();
}

ReconnectRouterOp::ReconnectRouterOp(BeanImpl *bean, int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect)
	: m_bean(bean), m_delay(delay), m_maxRetryCount(maxRetryCount), m_mac(mac), m_wifiName(wifiName), m_reconnect(reconnect), m_retryCount(0)
{
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout()));
	m_timer1.setInterval(m_delay);
}

ReconnectRouterOp::~ReconnectRouterOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void ReconnectRouterOp::start()
{
	restart();
}

void ReconnectRouterOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void ReconnectRouterOp::restart()
{
	if (!m_wifiName.isNull() && !m_wifiName.isEmpty()) {
		connectProfile();
	} else {
		discoverRouter();
	}
}

void ReconnectRouterOp::retry()
{
	if (++m_retryCount <= m_maxRetryCount) {
		LOG_DEBUG(QString::fromUtf8("retry %1/%2 after delay %3").arg(m_retryCount).arg(m_maxRetryCount).arg(m_delay));
		m_timer1.start();
	} else {
		LOG_DEBUG(QString::fromUtf8("no more retry chance!"));
		notifyFinished(UnknownError);
	}
}

void ReconnectRouterOp::onTimeout()
{
	m_timer1.stop();
	LOG_DEBUG(QString::fromUtf8("retry %1/%2").arg(m_retryCount).arg(m_maxRetryCount));
	restart();
}

void ReconnectRouterOp::connectProfile()
{
	LOG_DEBUG(QString::fromUtf8("connectProfile %1").arg(m_wifiName));
	m_op = m_bean->m_system->connectWlanProfile(m_wifiName, m_reconnect);
	connect(m_op, SIGNAL(finished()), SLOT(onConnectProfileFinished()));
}

void ReconnectRouterOp::onConnectProfileFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		LOG_DEBUG(QString::fromUtf8("connectProfile self aborted!"));
		return;
	}

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("connectProfile op aborted!"));
		return notifyFinished(AbortedError);
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("connectWlanProfile result: %1").arg(result));
	if (result == WlanProfileNotFound) {
		LOG_DEBUG(QString::fromUtf8("recreate profile?"));
		notifyFinished(WlanProfileNotFound);
	}
	if (result != NoError) {
		return retry();
	}

	discoverRouter();
}

void ReconnectRouterOp::discoverRouter()
{
	LOG_DEBUG(QString::fromUtf8("discover router %1").arg(m_mac));
	m_op = m_bean->discoverRouterSoap(5000, m_mac, 12, 5000);
	connect(m_op, SIGNAL(finished()), SLOT(onDiscoverRouterFinished()));
}

void ReconnectRouterOp::onDiscoverRouterFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		LOG_DEBUG(QString::fromUtf8("discoverRouter self aborted!"));
		return;
	}

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("discoverRouter op aborted!"));
		return notifyFinished(AbortedError);
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("discoverRouterSoap result: %1").arg(result));
	if (result != NoError) {
		return retry();
	}

	int matchIndex = op->value("matchIndex").toInt();
	LOG_DEBUG(QString::fromUtf8("matchIndex %1").arg(matchIndex));
	if (matchIndex < 0) {
		return retry();
	}

	copyValues(op);
	notifyFinished(NoError);
}

RestartRouterOp::RestartRouterOp(BeanImpl *bean)
	: m_bean(bean)
{
}

RestartRouterOp::~RestartRouterOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void RestartRouterOp::start()
{
	LOG_DEBUG(QString::fromUtf8("restart router start"));
	m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationStarted"), QStringList(QLatin1String("NewSessionID")), QStringList(m_bean->m_soapCore->sessionId()));
	connect(m_op, SIGNAL(finished()), SLOT(onConfigStarted()));
}

void RestartRouterOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void RestartRouterOp::onConfigStarted()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onConfigStarted %1").arg(result));
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	m_op = m_bean->m_soapCore->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationFinished"), QLatin1String("NewStatus"), QLatin1String("RebootRequired"));
	connect(m_op, SIGNAL(finished()), SLOT(onConfigFinished()));
}

void RestartRouterOp::onConfigFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onConfigFinished %1").arg(result));
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant varResponseCode = op->value("responseCode");
	if (!varResponseCode.isValid()) {
		return notifyFinished(UnknownError);
	}

	bool ok;
	int responseCode = varResponseCode.toInt(&ok);
	if (!ok) {
		return notifyFinished(UnknownError);
	}

	if (responseCode != 0) {
		return notifyFinished(UnknownError);
	}

	notifyFinished(NoError);
}

CheckInternetOp::CheckInternetOp(BeanImpl *bean, const QString& host, int delay, int maxRetryCount)
	: m_bean(bean), m_host(host), m_delay(delay), m_maxRetryCount(maxRetryCount), m_retryCount(0)
{
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout()));
	m_timer1.setInterval(m_delay);
}

CheckInternetOp::~CheckInternetOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void CheckInternetOp::start()
{
	restart();
}

void CheckInternetOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void CheckInternetOp::restart()
{
	connectHost();
}

void CheckInternetOp::retry()
{
	if (++m_retryCount <= m_maxRetryCount) {
		LOG_DEBUG(QString::fromUtf8("retry %1/%2 after delay %3").arg(m_retryCount).arg(m_maxRetryCount).arg(m_delay));
		m_timer1.start();
	} else {
		LOG_DEBUG(QString::fromUtf8("no more retry chance!"));
		notifyFinished(UnknownError);
	}
}

void CheckInternetOp::onTimeout()
{
	m_timer1.stop();
	LOG_DEBUG(QString::fromUtf8("retry %1/%2").arg(m_retryCount).arg(m_maxRetryCount));
	restart();
}

void CheckInternetOp::connectHost()
{
	LOG_DEBUG(QString::fromUtf8("connectHost %1").arg(m_host));
	m_op = m_bean->m_system->httpGet(QString::fromUtf8("http://%1").arg(m_host));
	connect(m_op, SIGNAL(finished()), SLOT(onConnectHostFinished()));
}

void CheckInternetOp::onConnectHostFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (isAborted()) {
		LOG_DEBUG(QString::fromUtf8("connectHost self aborted!"));
		return;
	}

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("connectHost op aborted!"));
		return notifyFinished(AbortedError);
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("connectHost result: %1").arg(result));
	if (result != NoError) {
		return retry();
	}

	notifyFinished(NoError);
}
