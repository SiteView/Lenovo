#include "pages.h"
#include <LenovoCore/LenovoCore.h>
#include <QtGui/QScrollArea>
#include <stdlib.h>
#include <qdebug.h>

DEFINE_LOGGER(Pages);

// TODO: initialize these
bool g_lanMode;
QString g_targetMac;

bool checkSoapResponse(AsyncOp *op, int& responseCode)
{
	QVariant var1 = op->value("responseCode");
	QVariant var2 = op->value("response");

	LOG_DEBUG(QString::fromUtf8("checkSoapResponse begin"));

	if (!var1.isValid() && !var2.isValid()) {
		LOG_DEBUG(QString::fromUtf8("checkSoapResponse invalid op result!"));
		return false;
	}

	if (var1.isValid()) {
		bool ok;
		responseCode = var1.toInt(&ok);
		if (ok) {
			LOG_DEBUG(QString::fromUtf8("responseCode = %1").arg(responseCode));
		} else {
			LOG_DEBUG(QString::fromUtf8("responseCode NAN %1").arg(var1.toString()));
			return false;
		}
	}

	if (var2.isValid() && var2.type() == QVariant::Map) {
		QVariantMap m1 = qvariant_cast<QVariantMap>(var2);
		QVariantMap::const_iterator it = m1.begin();
		QVariantMap::const_iterator ie = m1.end();
		for (; it != ie; ++it) {
			LOG_DEBUG(QString::fromUtf8("responseMap: %1=[%2]").arg(it.key()).arg(it.value().toString()));
		}
	}

	LOG_DEBUG(QString::fromUtf8("checkSoapResponse end OK!"));
	return true;
}

bool validateResponseCode(int responseCode)
{
	bool retval;
	switch (responseCode) {
	case 0:
	case 1:
		retval = true;
		break;
	default:
		retval = false;
		break;
	}
	return retval;
}

/*
** UIAppMessage
*/

void UIAppMessage::onLoad(const QVariantMap& params)
{
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	QAbstractButton *okButton = findChild<QAbstractButton*>(QString::fromUtf8("okButton"));
	connect(okButton, SIGNAL(clicked()), SLOT(onOkButtonClicked()));
}

void UIAppMessage::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(params().value(QString::fromUtf8("stringId")).toInt()));
}

void UIAppMessage::onOkButtonClicked()
{
	app()->closePage();
}

/*
** UITestMain
*/

void UITestMain::onLoad(const QVariantMap& params)
{
	QAbstractButton *closeButton = findChild<QAbstractButton*>(QString::fromUtf8("closeButton"));
	m_speedLabel = findChild<QLabel*>(QString::fromUtf8("speedLabel"));
	m_wifiNameLabel = findChild<QLabel*>(QString::fromUtf8("wifiNameLabel"));
	m_wifiSecurityLabel = findChild<QLabel*>(QString::fromUtf8("wifiSecurityLabel"));
	m_uptimeLabel = findChild<QLabel*>(QString::fromUtf8("uptimeLabel"));
	QAbstractButton *testButton1 = findChild<QAbstractButton*>(QString::fromUtf8("testButton1"));
	QAbstractButton *testButton2 = findChild<QAbstractButton*>(QString::fromUtf8("testButton2"));
	QAbstractButton *testButton3 = findChild<QAbstractButton*>(QString::fromUtf8("testButton3"));

	connect(closeButton, SIGNAL(clicked()), SIGNAL(closeButtonClicked()));
	connect(testButton1, SIGNAL(clicked()), SLOT(onTestButton1Clicked()));
	connect(testButton2, SIGNAL(clicked()), SLOT(onTestButton2Clicked()));
	connect(testButton3, SIGNAL(clicked()), SLOT(onTestButton3Clicked()));

	connect(app(), SIGNAL(incomingBPSChanged()), SLOT(updateSpeed()));
	connect(app(), SIGNAL(outgoingBPSChanged()), SLOT(updateSpeed()));
	connect(app(), SIGNAL(uptimeMinutesChanged()), SLOT(updateTime()));
	connect(app(), SIGNAL(wifiNameChanged()), SLOT(updateName()));
	connect(app(), SIGNAL(wifiSecurityChanged()), SLOT(updateSecurity()));
}

void UITestMain::onTranslate()
{
	updateSpeed();
	updateTime();
	updateName();
	updateSecurity();
}

void UITestMain::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UITestMain::updateSpeed()
{
	m_speedLabel->setText(app()->translateUIText(120030).arg(app()->incomingBPS() / 1024, 0, 'f', 2).arg(app()->outgoingBPS() / 1024, 0, 'f', 2));
}

void UITestMain::updateTime()
{
	div_t d = div(app()->uptimeMinutes(), 60);
	m_uptimeLabel->setText(app()->translateUIText(120028).arg(d.quot).arg(d.rem));
}

void UITestMain::updateName()
{
	m_wifiNameLabel->setText(app()->wifiName());
}

void UITestMain::updateSecurity()
{
	m_wifiSecurityLabel->setText(app()->translateUIText(app()->wifiSecurity() ? 110005 : 110006));
}

void UITestMain::onTestButton1Clicked()
{
	QString wifiPassword, oldWifiPassword;
	QString adminPassword, oldAdminPassword;

	wifiPassword = QLatin1String("siteviewtest");
	oldWifiPassword = QLatin1String("siteviewtest");
	adminPassword = QLatin1String("lenovot61p");
	oldAdminPassword = QLatin1String("lenovot61p");

	m_op = app()->bean()->changeRouterPassword(wifiPassword, oldWifiPassword, adminPassword, oldAdminPassword);
	app()->wait(m_op, this, "onChangePasswordFinished");
}

void UITestMain::onChangePasswordFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;
	if (op->result() == AsyncOp::NoError) {
		app()->showMessage(130004);
	} else {
		app()->showMessage(110214);
	}
}

void UITestMain::onTestButton2Clicked()
{
/*
	app()->configSet(QString::fromUtf8("MinimizeOnClose"), true);
	app()->configSet(QString::fromUtf8("AutoStart"), true);
	app()->configSet(QString::fromUtf8("AutoUpdate"), true);
	app()->configSet(QString::fromUtf8("EnablePopup"), true);
*/
//	app()->navigateTo(QString::fromUtf8("RouterSetupWelcome"));
	app()->showMessage(130004);
}

void UITestMain::onTestButton3Clicked()
{
	QVariantMap params1;
	params1.insert(QString::fromUtf8("popupMode"), QString::fromUtf8("1"));
	app()->navigateTo(QString::fromUtf8("RouterWlanSecurity"), params1);
}

/*
** UINewRouterConfirm
*/

void UINewRouterConfirm::onLoad(const QVariantMap& params)
{
	QAbstractButton *yesButton = findChild<QAbstractButton*>(QString::fromUtf8("yesButton"));
	QAbstractButton *noButton = findChild<QAbstractButton*>(QString::fromUtf8("noButton"));

	connect(yesButton, SIGNAL(clicked()), SLOT(onYesButtonClicked()));
	connect(noButton, SIGNAL(clicked()), SLOT(onNoButtonClicked()));
}

void UINewRouterConfirm::onYesButtonClicked()
{
	QVariantMap params1;
	params1.insert(QLatin1String("mode"), 1);
	app()->navigateTo(QString::fromUtf8("RouterSetupWelcome"), params1);
}

void UINewRouterConfirm::onNoButtonClicked()
{
	app()->navigateTo(QString::fromUtf8("PurchaseInfo"));
}

/*
** UIInstallConfirm
*/

void UIInstallConfirm::onLoad(const QVariantMap& params)
{
	QAbstractButton *yesButton = findChild<QAbstractButton*>(QString::fromUtf8("yesButton"));
	QAbstractButton *noButton = findChild<QAbstractButton*>(QString::fromUtf8("noButton"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));

	connect(yesButton, SIGNAL(clicked()), SLOT(onYesButtonClicked()));
	connect(noButton, SIGNAL(clicked()), SLOT(onNoButtonClicked()));
}

void UIInstallConfirm::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(110220).arg(app()->wifiName()));
}

void UIInstallConfirm::onYesButtonClicked()
{
	//QVariantMap params1;
	//params1.insert(QLatin1String("mode"), 1);
	app()->navigateTo(QString::fromUtf8("RouterSetupWelcome")/*, params1*/);
}

void UIInstallConfirm::onNoButtonClicked()
{
	app()->closePage();
}

/*
** UIPurchaseInfo
*/

void UIPurchaseInfo::onLoad(const QVariantMap& params)
{
	QAbstractButton *quitButton = findChild<QAbstractButton*>(QString::fromUtf8("quitButton"));
	connect(quitButton, SIGNAL(clicked()), app(), SLOT(quit()));
}

/*
** UIRouterSetupWelcome
*/

void UIRouterSetupWelcome::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	m_backButton = findChild<QAbstractButton*>(QString::fromUtf8("backButton"));

	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
	connect(m_backButton, SIGNAL(clicked()), SLOT(onBackButtonClicked()));

	if (params.value(QLatin1String("mode"), 0).toInt() != 0) {
		m_backButton->setVisible(false);
	}
}

void UIRouterSetupWelcome::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape) {
		if (m_backButton->isVisible()) {
			m_backButton->click();
		}
		e->accept();
	}
}

void UIRouterSetupWelcome::onNextButtonClicked()
{
	app()->configSet(QLatin1String("WlanProfileName"), QString());
	app()->configSet(QLatin1String("RouterMac"), QString());
	app()->navigateTo(QString::fromUtf8("RouterSetupDetect"));
}

void UIRouterSetupWelcome::onBackButtonClicked()
{
	app()->closePage();
}

/*
** UIRouterSetupDetect
*/

void UIRouterSetupDetect::onLoad(const QVariantMap& params)
{
	m_retryCount = 0;

	bool ok;
	m_radioRetryCount = params.value(QLatin1String("radioRetryCount"), QLatin1String("0")).toInt(&ok);
	if (!ok) {
		m_radioRetryCount = 0;
	}

	m_powerRetryCount = params.value(QLatin1String("powerRetryCount"), QLatin1String("0")).toInt(&ok);
	if (!ok) {
		m_powerRetryCount = 0;
	}

	QString action = params.value(QLatin1String("action"), QLatin1String("searchSsid")).toString();

	if (action.compare(QLatin1String("searchSsid")) == 0) {
		searchSsid();
	} else if (action.compare(QLatin1String("connectSsid")) == 0) {
		m_ssid = qvariant_cast<QVariantMap>(params.value(QLatin1String("connectSsid")));
		connectSsid();
	} else if (action.compare(QLatin1String("connectLan")) == 0) {
		connectLan();
	}
}

void UIRouterSetupDetect::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIRouterSetupDetect::searchSsid()
{
	m_op = app()->system()->searchSsid(QLatin1String("lenovo_new"));
	connect(m_op, SIGNAL(finished()), SLOT(onSearchSsidFinished()));
}

void UIRouterSetupDetect::onSearchSsidFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("searchSsid result: %1").arg(result));
	switch (result) {
	case AsyncOp::NoError:
		{
			QVariant varSsidList = op->value("ssidList");
			if (varSsidList.isValid()) {
				QVariantList ssidList = qvariant_cast<QVariantList>(varSsidList);
				int count = ssidList.count();
				if (count == 1) {
					QVariantMap params1;
					params1.insert(QLatin1String("action"), QLatin1String("connectSsid"));
					params1.insert(QLatin1String("connectSsid"), ssidList.at(0));
					app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
				} else if (count > 1) {
					QVariantMap params;
					params.insert(QString::fromUtf8("ssidList"), ssidList);
					app()->navigateTo(QString::fromUtf8("WlanMultipleRouterSelect"), params);
				} else {
					if (m_powerRetryCount == 0) {
						app()->navigateTo(QString::fromUtf8("RouterPowerGuide1"));
					} else if (m_powerRetryCount == 1) {
						app()->navigateTo(QString::fromUtf8("RouterPowerGuide2"));
					} else {
						app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
					}
				}
			} else {
				app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
			}
		}
		break;
	case AsyncOp::WlanRadioOffError:
		{
			if (m_radioRetryCount < 2) {
				QVariantMap params1;
				params1.insert(QString::fromUtf8("retryCount"), m_radioRetryCount);
				params1.insert(QString::fromUtf8("nextPageName"), QString::fromUtf8("RouterSetupDetect"));
				QVariantMap params2;
				params2.insert(QString::fromUtf8("radioRetryCount"), m_radioRetryCount + 1);
				params1.insert(QString::fromUtf8("nextPageParams"), params2);
				app()->navigateTo(QString::fromUtf8("WlanRadioGuide"), params1);
			} else {
				app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
			}
		}
		break;
	case AsyncOp::WlanServiceDownError:
		{
			QVariantMap params1;
			params1.insert(QString::fromUtf8("nextPageName"), QString::fromUtf8("RouterSetupDetect"));
			app()->navigateTo(QString::fromUtf8("WlanServiceGuide"), params1);
		}
		break;
	case AsyncOp::WlanNoDeviceError:
		app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
		break;
	default:
		if (++m_retryCount < 2) {
			LOG_DEBUG(QString::fromUtf8("research ssid %1").arg(m_retryCount));
			searchSsid();
		} else {
			LOG_DEBUG(QString::fromUtf8("fallback to LAN %1").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
		}
		break;
	}
}

void UIRouterSetupDetect::connectSsid()
{
	m_op = app()->system()->connectSsid(m_ssid);
	connect(m_op, SIGNAL(finished()), SLOT(onConnectSsidFinished()));
}

void UIRouterSetupDetect::onConnectSsidFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("connectSsid result: %1").arg(result));
	if (result != AsyncOp::NoError) {
		if (++m_retryCount < 2) {
			LOG_DEBUG(QString::fromUtf8("reconnect ssid %1").arg(m_retryCount));
			connectSsid();
		} else {
			LOG_DEBUG(QString::fromUtf8("fallback to LAN %1").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
		}
		return;
	}

	m_retryCount = 0;
	QString targetMac = op->value("mac").toString();
	m_targetMac1 = targetMac;

	discoverSoap();
}

void UIRouterSetupDetect::discoverSoap()
{
	LOG_DEBUG(QString::fromUtf8("UIRouterSetupDetect discoverRouterSoap start"));
	m_op = app()->bean()->discoverRouterSoap(30000, m_targetMac1, 4, 5000);
	connect(m_op, SIGNAL(finished()), SLOT(onDiscoveryFinished()));
}

void UIRouterSetupDetect::onDiscoveryFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("discoverRouterSoap result: %1").arg(result));

	int matchIndex;
	bool ssidConnectOk = false;
	if (result == AsyncOp::NoError) {
		matchIndex = op->value("matchIndex").toInt();
		if (matchIndex >= 0) {
			ssidConnectOk = true;
		}
	}

	if (!ssidConnectOk) {
		if (++m_retryCount < 5) {
			LOG_DEBUG(QString::fromUtf8("redo discovery %1").arg(m_retryCount));
			discoverSoap();
		} else {
			LOG_DEBUG(QString::fromUtf8("fallback to LAN %1").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("LanSetupConnectPC"));
		}
		return;
	}

	QVariantList fullList = op->value("fullList").toList();
	QVariantMap varMap = fullList.at(matchIndex).toMap();

	LOG_DEBUG(QString::fromUtf8("EVERYTHING OK!"));
	lockDownRouter(false, varMap.value(QLatin1String("newwlanmacaddress")).toString().toUpper(), varMap.value(QLatin1String("soapHostName")).toString(), varMap);
}

void UIRouterSetupDetect::connectLan()
{
	LOG_DEBUG(QString::fromUtf8("connectLan start"));
	m_op = app()->bean()->discoverRouterSoap(5000, QString());
	connect(m_op, SIGNAL(finished()), SLOT(onDiscoveryLanFinished()));
}

void UIRouterSetupDetect::onDiscoveryLanFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("discoverRouterSoap result: %1").arg(result));
	if (result != AsyncOp::NoError) {
		if (++m_retryCount < 2) {
			LOG_DEBUG(QString::fromUtf8("reconnect LAN %1").arg(m_retryCount));
			connectLan();
		} else {
			LOG_DEBUG(QString::fromUtf8("always failed %1, call help!").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("CallHelp"));
		}
		return;
	}

	int matchIndex = -1;
	QVariantMap varMap;
	QVariantList fullList = op->value("fullList").toList();
	for (int i = 0; i < fullList.count(); i++) {
		varMap = fullList.at(i).toMap();
		QString ssid = varMap.value(QLatin1String("newssid")).toString();
		LOG_DEBUG(QString::fromUtf8("%1 %2").arg(i).arg(ssid));
		if (matchIndex < 0 && ssid.compare(QLatin1String("lenovo_new")) == 0) {
			matchIndex = i;
			break;
		}
	}

	// TODO: BAD!!!!
	//matchIndex = 0;

	if (matchIndex < 0) {
		LOG_DEBUG(QString::fromUtf8("always failed %1, call help!").arg(m_retryCount));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	varMap = fullList.at(matchIndex).toMap();

	LOG_DEBUG(QString::fromUtf8("EVERYTHING OK! 2"));
	lockDownRouter(true, varMap.value(QLatin1String("newwlanmacaddress")).toString().toUpper(), varMap.value(QLatin1String("soapHostName")).toString(), varMap);
}

void UIRouterSetupDetect::lockDownRouter(bool lan, const QString& mac, const QString& host, const QVariantMap& info)
{
	LOG_DEBUG(QString::fromUtf8("LOCKDOWN: LAN ? %1 %2 %3").arg(lan).arg(mac).arg(host));
	m_lanMode = lan;
	m_targetMac = mac;
	g_lanMode = lan;
	g_targetMac = mac;
	app()->bean()->soapCore()->setHost(host);
	app()->generateWifiName();
	app()->configSet(QLatin1String("WlanProfileName"), app()->wifiName());
	app()->configSet(QLatin1String("RouterMac"), g_targetMac);
	m_retryCount = 0;
	changeWifiName();
}

void UIRouterSetupDetect::changeWifiName()
{
	LOG_DEBUG(QString::fromUtf8("changeWifiName start"));
	m_op = app()->bean()->changeRouterWifiPassword(QString(), app()->wifiName(), true, true);
	connect(m_op, SIGNAL(finished()), SLOT(onChangeWifiNameFinished()));
}

void UIRouterSetupDetect::onChangeWifiNameFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("changeWifiName result: %1").arg(result));
	if (result != AsyncOp::NoError) {
		if (++m_retryCount < 2) {
			LOG_DEBUG(QString::fromUtf8("redo changeWifiName %1").arg(m_retryCount));
			changeWifiName();
		} else {
			LOG_DEBUG(QString::fromUtf8("always failed %1, call help!").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("CallHelp"));
		}
		return;
	}

	m_retryCount = 0;
	LOG_DEBUG(QString::fromUtf8("Now try to write WLAN profile %1").arg(app()->wifiName()));
	createProfile();
}

void UIRouterSetupDetect::createProfile()
{
	m_op = app()->system()->createWlanProfile(app()->wifiName(), QString());
	connect(m_op, SIGNAL(finished()), SLOT(onCreateProfileFinished()));
}

void UIRouterSetupDetect::onCreateProfileFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("createWlanProfile result: %1").arg(result));
	if (result != AsyncOp::NoError && !m_lanMode) {
		if (++m_retryCount < 2) {
			LOG_DEBUG(QString::fromUtf8("redo createProfile %1").arg(m_retryCount));
			createProfile();
		} else {
			LOG_DEBUG(QString::fromUtf8("always failed %1, call help!").arg(m_retryCount));
			app()->navigateTo(QString::fromUtf8("CallHelp"));
		}
		return;
	}

	m_op = app()->bean()->reconnectRouter(3000, 10, m_targetMac, m_lanMode ? QString() : app()->wifiName(), true);
	connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
}

void UIRouterSetupDetect::onReconnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("reconnectRouter result: %1").arg(result));

	if (result != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("reconnect failed, need some help?"));
		QVariantMap params1;
		params1.insert(QString::fromUtf8("textId"), 110222);
		app()->navigateTo(QString::fromUtf8("CallHelp"), params1);
		return;
	}

	LOG_DEBUG(QString::fromUtf8("reconnect OK!"));
	app()->navigateTo(QString::fromUtf8("RouterWanDetect"));
}

void UIRouterWanDetect::onLoad(const QVariantMap& params)
{
	m_routerWanGuide = params.value(QLatin1String("routerWanGuide"), 0).toInt();
    m_smartDetectionRetryCount = 0;

	LOG_DEBUG(QString::fromUtf8("start configuration now"));
	m_op = app()->soapCore()->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationStarted"), QLatin1String("NewSessionID"), app()->soapCore()->sessionId());
	connect(m_op, SIGNAL(finished()), SLOT(onConfigStarted()));
}

void UIRouterWanDetect::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIRouterWanDetect::onConfigStarted()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("ConfigurationStarted result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 1, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int responseCode = op->value("responseCode").toInt();
	LOG_DEBUG(QString::fromUtf8("ConfigurationStarted responseCode: %1").arg(responseCode));
	if (responseCode != 0) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 2, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

    startSmartDetection();
}

void UIRouterWanDetect::startSmartDetection()
{
    LOG_DEBUG(QString::fromUtf8("SetSmartWizardDetection start"));
    SoapCore *soapCore = app()->soapCore();
    m_op = soapCore->invoke(QLatin1String("WANIPConnection"), QLatin1String("SetSmartWizardDetection"));
    connect(m_op, SIGNAL(finished()), SLOT(onSmartDetectionFinished()));
    connect(&m_smartDetectionTimer, SIGNAL(timeout()), SLOT(onSmartDetectionTimeout()));
    m_smartDetectionTimer.setSingleShot(true);
    m_smartDetectionTimer.start(60000);
}

void UIRouterWanDetect::onSmartDetectionTimeout()
{
    disconnect(&m_smartDetectionTimer, SIGNAL(timeout()), this, SLOT(onSmartDetectionTimeout()));
    LOG_DEBUG(QString::fromUtf8("onSmartDetectionTimeout!!!"));
    if (m_op) {
        m_op->abort();
    }
}

void UIRouterWanDetect::onSmartDetectionFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

    if (op->isAborted()) {
        if (m_smartDetectionRetryCount < 3) {
            m_smartDetectionRetryCount++;
            LOG_DEBUG(QString::fromUtf8("timeout!! retry setSmartDetection! (%1)").arg(m_smartDetectionRetryCount));
            startSmartDetection();
        } else {
            LOG_DEBUG(QString::fromUtf8("No more retry for setSmartDetection! failed"));
            app()->navigateTo(QString::fromUtf8("CallHelp"));
        }
        return;
    }

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("SetSmartWizardDetection result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 3, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
        return;
	}

	int responseCode = op->value("responseCode").toInt();
	LOG_DEBUG(QString::fromUtf8("SetSmartWizardDetection responseCode: %1").arg(responseCode));
	if (responseCode != 0 && responseCode != 1) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 4, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	QVariantMap respMap = qvariant_cast<QVariantMap>(op->value("response"));
	QString conn = respMap.value(QString::fromUtf8("NewConnectionType")).toString();
	LOG_DEBUG(QString::fromUtf8("Conn type: %1").arg(conn));

	if (conn.compare(QString::fromUtf8("Down"), Qt::CaseInsensitive) == 0) {
		if (m_routerWanGuide == 0) {
			app()->navigateTo(QString::fromUtf8("RouterWanGuide1"));
		} else if (m_routerWanGuide == 1) {
			app()->navigateTo(QString::fromUtf8("RouterWanGuide2"));
		} else {
			QVariantMap params1;
			params1.insert(QString::fromUtf8("textId"), 110221);
			app()->navigateTo(QString::fromUtf8("CallHelp"), params1);
		}
	} else if (conn.compare(QString::fromUtf8("Static"), Qt::CaseInsensitive) == 0) {
		app()->navigateTo(QString::fromUtf8("WanStatic"));
	} else if (conn.compare(QString::fromUtf8("DHCP"), Qt::CaseInsensitive) == 0) {
		QVariantMap settings;
		settings.insert(QLatin1String("ConnectionType"), QLatin1String("dhcp"));
		app()->navigateTo(QString::fromUtf8("WanSave"), settings);
	} else {
		app()->navigateTo(QString::fromUtf8("WanDial"));
	}
}

/*
** UIWanSelect
*/

void UIWanSelect::onLoad(const QVariantMap& params)
{
	QAbstractButton *dhcpButton = findChild<QAbstractButton*>(QString::fromUtf8("dhcpButton"));
	QAbstractButton *staticButton = findChild<QAbstractButton*>(QString::fromUtf8("staticButton"));
	QAbstractButton *dialButton = findChild<QAbstractButton*>(QString::fromUtf8("dialButton"));
	connect(dhcpButton, SIGNAL(clicked()), SLOT(onDhcpButtonClicked()));
	connect(staticButton, SIGNAL(clicked()), SLOT(onStaticButtonClicked()));
	connect(dialButton, SIGNAL(clicked()), SLOT(onDialButtonClicked()));
}

void UIWanSelect::onDhcpButtonClicked()
{
	QVariantMap settings;
	settings.insert(QLatin1String("ConnectionType"), QLatin1String("dhcp"));
	app()->navigateTo(QString::fromUtf8("WanSave"), settings);
}

void UIWanSelect::onStaticButtonClicked()
{
	app()->navigateTo(QString::fromUtf8("WanStatic"));
}

void UIWanSelect::onDialButtonClicked()
{
	app()->navigateTo(QString::fromUtf8("WanDial"));
}

/*
** UIWanSave
*/

void UIWanSave::onLoad(const QVariantMap& params)
{
	LOG_DEBUG(QString::fromUtf8("changeRouterWanSettings start"));
	m_op = app()->bean()->changeRouterWanSettings(params);
	connect(m_op, SIGNAL(finished()), SLOT(onSaveFinished()));
}

void UIWanSave::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIWanSave::onSaveFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("changeRouterWanSettings result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 8, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	m_op = app()->soapCore()->invoke(QLatin1String("DeviceConfig"), QLatin1String("ConfigurationFinished"), QLatin1String("NewStatus"), QLatin1String("ChangesApplied"));
	connect(m_op, SIGNAL(finished()), SLOT(onConfigFinished()));
}

void UIWanSave::onConfigFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("ConfigurationFinished result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 9, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int responseCode = op->value("responseCode").toInt();
	LOG_DEBUG(QString::fromUtf8("ConfigurationFinished response: %1").arg(responseCode));
	if (responseCode != 0) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 10, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	LOG_DEBUG(QString::fromUtf8("Delay for apply"));
	m_timer1.setSingleShot(true);
	m_timer1.start(1000);
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout1()));
}

void UIWanSave::onTimeout1()
{
	LOG_DEBUG(QString::fromUtf8("restart router"));
	disconnect(&m_timer1, SIGNAL(timeout()), this, SLOT(onTimeout1()));

	m_op = app()->bean()->restartRouter();
	connect(m_op, SIGNAL(finished()), SLOT(onRestartRouterFinished()));
	// zhouklansman 减少一次重启 
	//m_op = app()->bean()->reconnectRouter(3000, 10, g_targetMac, g_lanMode ? QString() : app()->wifiName(), true);
	//connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
}

void UIWanSave::onRestartRouterFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("onRestartRouterFinished result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 21, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}
	LOG_DEBUG(QString::fromUtf8("Delay for restart"));
	m_timer1.setSingleShot(true);
	m_timer1.start(1000);
	connect(&m_timer1, SIGNAL(timeout()), SLOT(onTimeout2()));
}

void UIWanSave::onTimeout2()
{
	disconnect(&m_timer1, SIGNAL(timeout()), this, SLOT(onTimeout2()));
	LOG_DEBUG(QString::fromUtf8("reconnectRouter start"));
	m_op = app()->bean()->reconnectRouter(3000, 10, g_targetMac, g_lanMode ? QString() : app()->wifiName(), true);
	connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
}

void UIWanSave::onReconnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("reconnectRouter result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Wizard failed 13, fallback to manual select"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	m_op = app()->bean()->checkInternet(QLatin1String("www.netgear.com"), 5000, 8);
	connect(m_op, SIGNAL(finished()), SLOT(onCheckInternetFinished()));
}

void UIWanSave::onCheckInternetFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("checkInternet result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Seem internet failed, retry"));
		app()->navigateTo(QString::fromUtf8("RouterWanDetect"));
		return;
	}

	app()->navigateTo(QString::fromUtf8("WlanSetupGuide"));
}

/*
** UIWanStatic
*/

void UIWanStatic::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	m_ipEdit = findChild<QLineEdit*>(QString::fromUtf8("ipEdit"));
	m_maskEdit = findChild<QLineEdit*>(QString::fromUtf8("maskEdit"));
	m_gatewayEdit = findChild<QLineEdit*>(QString::fromUtf8("gatewayEdit"));
	m_dns1Edit = findChild<QLineEdit*>(QString::fromUtf8("dns1Edit"));
	m_dns2Edit = findChild<QLineEdit*>(QString::fromUtf8("dns2Edit"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
	//nextButton->setEnabled(false);
	QMetaObject::invokeMethod(m_ipEdit, "setFocus", Qt::QueuedConnection);
}

void UIWanStatic::onNextButtonClicked()
{
	QVariantMap settings;
	settings.insert(QLatin1String("ConnectionType"), QLatin1String("static"));
	settings.insert(QLatin1String("ExternalIPAddress"), m_ipEdit->text());
	settings.insert(QLatin1String("SubnetMask"), m_maskEdit->text());
	settings.insert(QLatin1String("DefaultGateway"), m_gatewayEdit->text());
	settings.insert(QLatin1String("PrimaryDNS"), m_dns1Edit->text());
	settings.insert(QLatin1String("SecondaryDNS"), m_dns2Edit->text());
	app()->navigateTo(QString::fromUtf8("WanSave"), settings);
}

/*
** UIWanDial
*/

void UIWanDial::onLoad(const QVariantMap& params)
{
	m_nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
    m_editButton = findChild<QAbstractButton*>(QString::fromUtf8("editButton"));
    m_accountEdit = findChild<QLineEdit*>(QString::fromUtf8("accountEdit"));
	m_passwordEdit = findChild<QLineEdit*>(QString::fromUtf8("passwordEdit"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	connect(m_nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(onEditButtonClicked()));
    m_nextButton->setEnabled(false);
	connect(m_accountEdit, SIGNAL(textEdited(const QString&)), SLOT(updateUIState()));
	connect(m_passwordEdit, SIGNAL(textEdited(const QString&)), SLOT(updateUIState()));
	QMetaObject::invokeMethod(m_accountEdit, "setFocus", Qt::QueuedConnection);
	m_textId = 130001;
}

void UIWanDial::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(m_textId));
}

void UIWanDial::onEditButtonClicked()
{
    m_accountEdit->setEnabled(true);
    m_passwordEdit->setEnabled(true);
    m_editButton->setVisible(false);
    m_accountEdit->setFocus();
}

void UIWanDial::onNextButtonClicked()
{
	if (m_textId == 130001) {
		m_textId = 130006;
        m_accountEdit->setEnabled(false);
        m_passwordEdit->setEnabled(false);
        m_editButton->setVisible(true);
        onTranslate();
	} else {
		QVariantMap settings;
		settings.insert(QLatin1String("ConnectionType"), QLatin1String("pppoe"));
		settings.insert(QLatin1String("ISPLoginname"), m_accountEdit->text());
		settings.insert(QLatin1String("ISPPassword"), m_passwordEdit->text());
		app()->navigateTo(QString::fromUtf8("WanSave"), settings);
	}
}

void UIWanDial::updateUIState()
{
	bool fs = !m_accountEdit->text().isEmpty() && !m_accountEdit->text().isEmpty();
	m_nextButton->setEnabled(fs);
}

/*
** UILanSetupConnectPC
*/

void UILanSetupConnectPC::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UILanSetupConnectPC::onNextButtonClicked()
{
/*
	app()->navigateTo(QString::fromUtf8("LanSetupConnectRouter"));
/*/
	QVariantMap params1;
	params1.insert(QLatin1String("action"), QLatin1String("connectLan"));
	app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
//*/
}

/*
** UILanSetupConnectRouter
*/

void UILanSetupConnectRouter::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UILanSetupConnectRouter::onNextButtonClicked()
{
	app()->navigateTo(QString::fromUtf8("LanSetupPowerOn"));
}

/*
** UILanSetupPowerOn
*/

void UILanSetupPowerOn::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UILanSetupPowerOn::onNextButtonClicked()
{
	QVariantMap params1;
	params1.insert(QLatin1String("action"), QLatin1String("connectLan"));
	app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
}

/*
** UIWlanServiceGuide
*/

void UIWlanServiceGuide::onLoad(const QVariantMap& params)
{
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	m_nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));

	connect(m_nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIWlanServiceGuide::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIWlanServiceGuide::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(app()->system()->needsElevation() ? 110209 : 110208));
}

void UIWlanServiceGuide::onNextButtonClicked()
{
	m_nextButton->setEnabled(false);
	m_op = app()->system()->startWlanService();
	connect(m_op, SIGNAL(finished()), SLOT(onStartWlanServiceFinished()));
}

void UIWlanServiceGuide::onStartWlanServiceFinished()
{
	delete m_op;
	m_op = NULL;

	QString nextPageName = params().value(QString::fromUtf8("nextPageName")).toString();
	if (!nextPageName.isEmpty()) {
		QVariant varNextPageParams = params().value(QString::fromUtf8("nextPageParams"));
		if (varNextPageParams.isValid()) {
			app()->navigateTo(nextPageName, qvariant_cast<QVariantMap>(varNextPageParams));
		} else {
			app()->navigateTo(nextPageName);
		}
	}
}

/*
** UIWlanRadioGuide
*/

void UIWlanRadioGuide::onLoad(const QVariantMap& params)
{
	bool ok;
	m_retryCount = params.value(QString::fromUtf8("retryCount")).toInt(&ok);
	if (!ok) {
		m_retryCount = 0;
	}

	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));

	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIWlanRadioGuide::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(m_retryCount == 0 ? 110100 : 110101));
}

void UIWlanRadioGuide::onNextButtonClicked()
{
	QString nextPageName = params().value(QString::fromUtf8("nextPageName")).toString();
	if (!nextPageName.isEmpty()) {
		QVariant varNextPageParams = params().value(QString::fromUtf8("nextPageParams"));
		if (varNextPageParams.isValid()) {
			app()->navigateTo(nextPageName, qvariant_cast<QVariantMap>(varNextPageParams));
		} else {
			app()->navigateTo(nextPageName);
		}
	}
}

/*
** UIWlanMultipleRouterSelect
*/

void UIWlanMultipleRouterSelect::onLoad(const QVariantMap& params)
{
	QVariantList ssidList = qvariant_cast<QVariantList>(params.value(QString::fromUtf8("ssidList")));
	for (int i = 0; i < ssidList.count(); i++) {
		QVariantMap entry = qvariant_cast<QVariantMap>(ssidList.at(i));
		QByteArray mac = entry.value(QString::fromUtf8("mac")).toByteArray();
		const quint8 *macData = reinterpret_cast<const quint8*>(mac.data());
		QString s;
		QPushButton *btn = new QPushButton(s.sprintf("%02X:%02X:%02X:%02X:%02X:%02X", macData[0], macData[1], macData[2], macData[3], macData[4], macData[5]), this);
		btn->setProperty("UIClass", QLatin1String("wbutton"));
		btn->setGeometry(80, 130 + 40 * i, 160, 32);
		btn->setProperty("index", i);
		connect(btn, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
	}
}

void UIWlanMultipleRouterSelect::onNextButtonClicked()
{
	int index = sender()->property("index").toInt();
	QVariantList ssidList = qvariant_cast<QVariantList>(params().value(QString::fromUtf8("ssidList")));
	QVariantMap params1;
	params1.insert(QLatin1String("action"), QLatin1String("connectSsid"));
	params1.insert(QLatin1String("connectSsid"), ssidList.at(index));
	app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
}

/*
** UIRouterPowerGuide1
*/

void UIRouterPowerGuide1::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIRouterPowerGuide1::onNextButtonClicked()
{
	QVariant stage2 = params().value(QString::fromUtf8("stage2"));
	if (stage2.isValid()) {
		QVariantMap params1;
		params1.insert(QString::fromUtf8("mac"), g_targetMac);
		app()->navigateTo(QString::fromUtf8("RouterDetect"), params1);
	} else {
		QVariantMap params1;
		params1.insert(QString::fromUtf8("powerRetryCount"), 1);
		app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
	}
}

/*
** UIRouterPowerGuide2
*/

void UIRouterPowerGuide2::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIRouterPowerGuide2::onNextButtonClicked()
{
	QVariantMap params1;
	params1.insert(QString::fromUtf8("powerRetryCount"), 2);
	app()->navigateTo(QString::fromUtf8("RouterSetupDetect"), params1);
}

/*
** UICallHelp
*/

void UICallHelp::onLoad(const QVariantMap& params)
{
	m_stage2 = false;
	app()->configSet(QLatin1String("WlanProfileName"), QString());
	app()->configSet(QLatin1String("RouterMac"), QString());
	m_quitButton = findChild<QAbstractButton*>(QString::fromUtf8("quitButton"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	connect(m_quitButton, SIGNAL(clicked()), SLOT(onQuitButtonClicked()));
}

void UICallHelp::onTranslate()
{
	if (m_stage2) {
		m_promptLabel->setText(app()->translateUIText(110223));
		m_quitButton->setText(app()->translateUIText(110018));
	} else {
		QVariant varTextId = params().value(QString::fromUtf8("textId"));
		if (varTextId.isValid()) {
			m_promptLabel->setText(app()->translateUIText(varTextId.toInt()));
		}
	}
}

void UICallHelp::onQuitButtonClicked()
{
	if (m_stage2) {
		app()->quit();
	} else {
		m_stage2 = true;
		onTranslate();
	}
}

/*
** UIDisconnectPrompt
*/

void UIDisconnectPrompt::onLoad(const QVariantMap& params)
{
	m_quitButton = findChild<QAbstractButton*>(QString::fromUtf8("quitButton"));
	m_reconnectButton = findChild<QAbstractButton*>(QString::fromUtf8("reconnectButton"));
	m_loadingLabel = findChild<QLabel*>(QString::fromUtf8("loadingLabel"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	connect(m_quitButton, SIGNAL(clicked()), app(), SLOT(quit()));
	connect(m_reconnectButton, SIGNAL(clicked()), SLOT(onReconnectButtonClicked()));
	QMovie *circleMovie = new QMovie(QString::fromUtf8(":/images/loading.gif"), "gif", m_loadingLabel);
	m_loadingLabel->setMovie(circleMovie);
	m_loadingLabel->setVisible(false);
	m_promptId = 110015;
}

void UIDisconnectPrompt::onTranslate()
{
	if (m_promptId == 110016) {
		m_promptLabel->setText(app()->translateUIText(m_promptId).arg(app()->wifiName()));
	} else if (m_promptId == 110017) {
		m_promptLabel->setText(app()->translateUIText(m_promptId).arg(app()->wifiName()));
	} else {
		m_promptLabel->setText(app()->translateUIText(m_promptId));
	}
}

void UIDisconnectPrompt::onReconnectButtonClicked()
{
	if (!m_op) {
		m_op = app()->system()->connectWlanProfile(app()->wifiName(), false);
		connect(m_op, SIGNAL(finished()), SLOT(onConnectFinished()));
		m_quitButton->setVisible(false);
		m_reconnectButton->setVisible(false);
		m_loadingLabel->setVisible(true);
		m_loadingLabel->movie()->start();
		m_promptId = 110016;
		onTranslate();
	}
}

void UIDisconnectPrompt::onConnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;
	if (op->result() != AsyncOp::NoError) {
		m_quitButton->setVisible(true);
		m_reconnectButton->setVisible(true);
		m_loadingLabel->setVisible(false);
		m_loadingLabel->movie()->stop();
		m_promptId = 110017;
		onTranslate();
	}
}

void UIDisconnectPrompt::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

/*
** UIRouterWanGuide1
*/

void UIRouterWanGuide1::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIRouterWanGuide1::onNextButtonClicked()
{
	QVariantMap params;
	params.insert(QString::fromUtf8("routerWanGuide"), 1);
	app()->navigateTo(QString::fromUtf8("RouterWanDetect"), params);
}

/*
** UIRouterWanGuide2
*/

void UIRouterWanGuide2::onLoad(const QVariantMap& params)
{
	QAbstractButton *nextButton = findChild<QAbstractButton*>(QString::fromUtf8("nextButton"));
	connect(nextButton, SIGNAL(clicked()), SLOT(onNextButtonClicked()));
}

void UIRouterWanGuide2::onNextButtonClicked()
{
	QVariantMap params;
	params.insert(QString::fromUtf8("routerWanGuide"), 2);
	app()->navigateTo(QString::fromUtf8("RouterWanDetect"), params);
}

/*
** UIWlanSetupGuide
*/

void UIWlanSetupGuide::onLoad(const QVariantMap& params)
{
	QAbstractButton *nowButton = findChild<QAbstractButton*>(QString::fromUtf8("nowButton"));
	QAbstractButton *laterButton = findChild<QAbstractButton*>(QString::fromUtf8("laterButton"));

	connect(nowButton, SIGNAL(clicked()), SLOT(onNowButtonClicked()));
	connect(laterButton, SIGNAL(clicked()), SLOT(onLaterButtonClicked()));
}

void UIWlanSetupGuide::onNowButtonClicked()
{
	app()->navigateTo(QString::fromUtf8("RouterWlanSecurity"));
}

void UIWlanSetupGuide::onLaterButtonClicked()
{
	app()->setWifiSecurity(false);
	app()->confirmNewWifiPassword(QString());
	app()->navigateTo(QString::fromUtf8("RouterSetupComplete"));
}

/*
** UIRouterSetupComplete
*/

void UIRouterSetupComplete::onLoad(const QVariantMap& params)
{
	QAbstractButton *okButton = findChild<QAbstractButton*>(QString::fromUtf8("okButton"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	connect(okButton, SIGNAL(clicked()), SLOT(onOkButtonClicked()));
}

void UIRouterSetupComplete::onOkButtonClicked()
{
	app()->beginUserMode();
	app()->closePage();
}

void UIRouterSetupComplete::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(130012).arg(app()->wifiName()));
}

/*
** UIRouterWlanSecurity
*/

void UIRouterWlanSecurity::onLoad(const QVariantMap& params)
{
	m_okButton = findChild<QAbstractButton*>(QString::fromUtf8("okButton"));
	m_backButton = findChild<QAbstractButton*>(QString::fromUtf8("backButton"));
	m_passwordEdit1 = findChild<QLineEdit*>(QString::fromUtf8("passwordEdit1"));
	m_passwordEdit2 = findChild<QLineEdit*>(QString::fromUtf8("passwordEdit2"));

	connect(m_okButton, SIGNAL(clicked()), SLOT(onOkButtonClicked()));
	connect(m_backButton, SIGNAL(clicked()), SLOT(onBackButtonClicked()));
	connect(m_passwordEdit1, SIGNAL(textEdited(const QString&)), SLOT(updateUIState()));
	connect(m_passwordEdit2, SIGNAL(textEdited(const QString&)), SLOT(updateUIState()));

	m_okButton->setEnabled(false);
	QMetaObject::invokeMethod(m_passwordEdit1, "setFocus", Qt::QueuedConnection);
}

void UIRouterWlanSecurity::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIRouterWlanSecurity::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
	case Qt::Key_Return:
		if (m_okButton->isEnabled()) {
			m_okButton->click();
		}
		e->accept();
		break;
	case Qt::Key_Escape:
		m_backButton->click();
		e->accept();
		break;
	default:
		AppPage::keyPressEvent(e);
	}
}

void UIRouterWlanSecurity::onOkButtonClicked()
{
	QString password = m_passwordEdit1->text();
	if (!app()->validateWifiPassword(password)) {
		QVariantMap params1;
		params1.insert(QLatin1String("CurrentPassword"), password);
		app()->navigateTo(QString::fromUtf8("WlanInvalidPassword"));
		return;
	}

	QVariantMap params1;
	params1.insert(QString::fromUtf8("wifiPassword"), password);
	params1.insert(QString::fromUtf8("popupMode"), params().value(QString::fromUtf8("popupMode")));
	app()->navigateTo(QString::fromUtf8("RouterWlanSave"), params1);
}

void UIRouterWlanSecurity::onBackButtonClicked()
{
	if (params().value(QString::fromUtf8("popupMode")).toString().compare(QLatin1String("1")) == 0) {
		app()->closePage();
	} else {
		app()->navigateTo(QString::fromUtf8("WlanSetupGuide"));
	}
}

void UIRouterWlanSecurity::updateUIState()
{
	bool passwordValid = false;
	if (m_passwordEdit1->text() == m_passwordEdit2->text()) {
		//passwordValid = app()->validateWifiPassword(m_passwordEdit1->text());
		passwordValid = true;
	}
	m_okButton->setEnabled(passwordValid);
}

/*
** UIRouterWlanSave
*/

void UIRouterWlanSave::onLoad(const QVariantMap& params)
{
	QVariant varPassword = params.value(QString::fromUtf8("wifiPassword"));
	QString password;
	if (varPassword.isValid()) {
		password = varPassword.toString();
	}

	app()->setWifiPassword(password);

	LOG_DEBUG(QString::fromUtf8("Now call changeRouterWifiPassword"));
	m_op = app()->bean()->changeRouterWifiPassword(password, app()->wifiName(), true, true);
	connect(m_op, SIGNAL(finished()), SLOT(onChangeWifiFinished()));
}

void UIRouterWlanSave::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIRouterWlanSave::onChangeWifiFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("changeRouterWifiPassword result: %1").arg(result));
	if (op->result() != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("Unexpected failure!"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	app()->confirmNewWifiPassword(params().value(QString::fromUtf8("wifiPassword")).toString());

	LOG_DEBUG(QString::fromUtf8("update profile: %1").arg(app()->wifiName()));
	m_op = app()->system()->createWlanProfile(app()->wifiName(), app()->wifiPassword());
	connect(m_op, SIGNAL(finished()), SLOT(onSaveProfileFinished()));
}

void UIRouterWlanSave::onSaveProfileFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("createWlanProfile result: %1").arg(result));
	if (result != AsyncOp::NoError && !g_lanMode) {
		LOG_DEBUG(QString::fromUtf8("always failed call help!"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	m_op = app()->bean()->reconnectRouter(3000, 10, g_targetMac, g_lanMode ? QString() : app()->wifiName(), true);
	connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
}

void UIRouterWlanSave::onReconnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("reconnectRouter result: %1").arg(result));

	if (result != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("reconnect failed, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	LOG_DEBUG(QString::fromUtf8("reconnect OK!"));
	app()->setWifiSecurity(true);
	app()->navigateTo(QString::fromUtf8("RouterSetupComplete"));
}

/*
** UIRouterDetect
*/

void UIRouterDetect::onLoad(const QVariantMap& params)
{
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));
	QString mac = params.value(QString::fromUtf8("mac")).toString();
	g_targetMac = mac;
	LOG_DEBUG(QString::fromUtf8("discovery for %1").arg(mac));
	m_op = app()->bean()->discoverRouterSoap(5000, mac);
	connect(m_op, SIGNAL(finished()), SLOT(onDiscoveryFinished()));
}

void UIRouterDetect::onTranslate()
{
	int textId = 110215;
	if (params().value(QString::fromUtf8("mode")).toInt() == 1) {
		textId = 110013;
	}
	m_promptLabel->setText(app()->translateUIText(textId));
}

void UIRouterDetect::onUnload()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void UIRouterDetect::onDiscoveryFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("discoverRouterSoap result: %1").arg(result));

	int matchIndex = -1;
	if (result == AsyncOp::NoError) {
		matchIndex = op->value("matchIndex").toInt();
	}

	if (matchIndex >= 0) {
		g_targetMac = params().value(QString::fromUtf8("mac")).toString();
		QVariantMap varMap = op->value("fullList").toList().at(matchIndex).toMap();
		app()->configSet(QLatin1String("WlanProfileName"), varMap.value(QLatin1String("newssid")).toString());
		app()->confirmWifiName();
		QString wpaMode = varMap.value(QLatin1String("newwpaencryptionmodes")).toString();
		if (wpaMode.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
			app()->setWifiSecurity(false);
			app()->confirmNewWifiPassword(QString());
		} else {
			app()->setWifiSecurity(true);
		}

		app()->confirmWifiName();
		app()->soapCore()->setHost(varMap.value(QLatin1String("soapHostName")).toString());

		g_lanMode = false;
		app()->beginUserMode();
		app()->closePage();
	} else {
		app()->confirmWifiName();
		LOG_DEBUG(QString::fromUtf8("connecting to %1").arg(app()->wifiName()));
		m_op = app()->bean()->reconnectRouter(3000, 0, params().value(QString::fromUtf8("mac")).toString(), app()->wifiName(), true);
		connect(m_op, SIGNAL(finished()), SLOT(onReconnectFinished()));
	}
}

void UIRouterDetect::onCreateProfileFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("createProfile result: %1").arg(result));
	if (result != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("reconnect failed, need some help?"));
		QVariantMap param1;
		param1.insert(QString::fromUtf8("ssid"), app()->wifiName());
		app()->navigateTo(QString::fromUtf8("RouterDetectResult"), param1);
		return;
	}

	QString mac = params().value(QString::fromUtf8("mac")).toString();
	g_targetMac = mac;
	LOG_DEBUG(QString::fromUtf8("UIRouterDetect::onCreateProfileFinished() discovery for %1").arg(mac));
	m_op = app()->bean()->discoverRouterSoap(5000, mac, 6, 5000);
	connect(m_op, SIGNAL(finished()), SLOT(onDiscoveryFinished()));

}

void UIRouterDetect::onReconnectFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	if (op->isAborted()) {
		LOG_DEBUG(QString::fromUtf8("aborted, need some help?"));
		app()->navigateTo(QString::fromUtf8("CallHelp"));
		return;
	}

	int result = op->result();
	LOG_DEBUG(QString::fromUtf8("reconnectRouter result: %1").arg(result));

	if (result == AsyncOp::WlanProfileNotFound) {
		LOG_DEBUG(QString::fromUtf8("need to recreate profile"));
		m_op = app()->system()->createWlanProfile(app()->wifiName(), app()->savedWifiPassword());
		connect(m_op, SIGNAL(finished()), SLOT(onCreateProfileFinished()));
		return;
	}

	if (result != AsyncOp::NoError) {
		LOG_DEBUG(QString::fromUtf8("reconnect failed, need some help?"));
		QVariantMap param1;
		param1.insert(QString::fromUtf8("ssid"), app()->wifiName());
		app()->navigateTo(QString::fromUtf8("RouterDetectResult"), param1);
		return;
	}

	LOG_DEBUG(QString::fromUtf8("reconnect OK!"));
	g_targetMac = params().value(QString::fromUtf8("mac")).toString();
//	qDebug() << op->values();
	int matchIndex = op->value("matchIndex").toInt();
	if (matchIndex >= 0) {
		g_targetMac = params().value(QString::fromUtf8("mac")).toString();
		QVariantMap varMap = op->value("fullList").toList().at(matchIndex).toMap();
		QString wpaMode = varMap.value(QLatin1String("newwpaencryptionmodes")).toString();
		if (wpaMode.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
			app()->setWifiSecurity(false);
			app()->confirmNewWifiPassword(QString());
		} else {
			app()->setWifiSecurity(true);
		}
		app()->soapCore()->setHost(varMap.value(QLatin1String("soapHostName")).toString());
	}
	g_lanMode = false;
	app()->beginUserMode();
	app()->closePage();
}

/*
** UIRouterDetectResult
*/

void UIRouterDetectResult::onLoad(const QVariantMap& params)
{
	QAbstractButton *yesButton = findChild<QAbstractButton*>(QString::fromUtf8("yesButton"));
	QAbstractButton *noButton = findChild<QAbstractButton*>(QString::fromUtf8("noButton"));
	m_promptLabel = findChild<QLabel*>(QString::fromUtf8("promptLabel"));

	connect(yesButton, SIGNAL(clicked()), SLOT(onYesButtonClicked()));
	connect(noButton, SIGNAL(clicked()), SLOT(onNoButtonClicked()));
}

void UIRouterDetectResult::onTranslate()
{
	m_promptLabel->setText(app()->translateUIText(110216).arg(params().value(QString::fromUtf8("ssid")).toString()));
}

void UIRouterDetectResult::onYesButtonClicked()
{
	QVariantMap params1;
	params1.insert(QString::fromUtf8("stage2"), g_targetMac);
	app()->navigateTo(QString::fromUtf8("RouterPowerGuide1"), params1);
}

void UIRouterDetectResult::onNoButtonClicked()
{
	QVariantMap params1;
	params1.insert(QLatin1String("mode"), 1);
	app()->navigateTo(QString::fromUtf8("RouterSetupWelcome"), params1);
}

/*
** UIWlanInvalidPassword
*/

void UIWlanInvalidPassword::onLoad(const QVariantMap& params)
{
	m_okButton = findChild<QAbstractButton*>(QString::fromUtf8("okButton"));
	connect(m_okButton, SIGNAL(clicked()), SLOT(onOkButtonClicked()));
}

void UIWlanInvalidPassword::onOkButtonClicked()
{
	QVariantMap params1;
	params1.insert(QLatin1String("CurrentPassword"), params().value(QLatin1String("CurrentPassword")));
	app()->navigateTo(QString::fromUtf8("RouterWlanSecurity"), params1);
}

void UIWlanInvalidPassword::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
	case Qt::Key_Return:
	case Qt::Key_Escape:
		m_okButton->click();
		e->accept();
		break;
	default:
		AppPage::keyPressEvent(e);
	}
}

APP_REGISTER_PAGE(UIAppWait)
APP_REGISTER_PAGE(UIAppMessage)
APP_REGISTER_PAGE(UITestMain)
APP_REGISTER_PAGE(UINewRouterConfirm)
APP_REGISTER_PAGE(UIPurchaseInfo)
APP_REGISTER_PAGE(UIRouterSetupWelcome)
APP_REGISTER_PAGE(UIRouterSetupDetect)
APP_REGISTER_PAGE(UIRouterPowerGuide1)
APP_REGISTER_PAGE(UIRouterPowerGuide2)
APP_REGISTER_PAGE(UIWlanRadioGuide)
APP_REGISTER_PAGE(UILanSetupConnectPC)
//APP_REGISTER_PAGE(UILanSetupConnectRouter)
//APP_REGISTER_PAGE(UILanSetupPowerOn)
//APP_REGISTER_PAGE(UILanSetupConfirm)
APP_REGISTER_PAGE(UIWlanServiceGuide)
APP_REGISTER_PAGE(UIWlanMultipleRouterSelect)
APP_REGISTER_PAGE(UICallHelp)
APP_REGISTER_PAGE(UIRouterDetect)
APP_REGISTER_PAGE(UIRouterDetectResult)
//APP_REGISTER_PAGE(UIRouterDetectFailure)
APP_REGISTER_PAGE(UIRouterWanGuide1)
APP_REGISTER_PAGE(UIRouterWanGuide2)
APP_REGISTER_PAGE(UIRouterWanDetect)
APP_REGISTER_PAGE(UIWlanSetupGuide)
APP_REGISTER_PAGE(UIRouterWlanSecurity)
APP_REGISTER_PAGE(UIRouterWlanSave)
//APP_REGISTER_PAGE(UIRouterWlanReconnect)
APP_REGISTER_PAGE(UIRouterSetupComplete)
APP_REGISTER_PAGE(UIWanSelect)
APP_REGISTER_PAGE(UIWanStatic)
APP_REGISTER_PAGE(UIWanDial)
APP_REGISTER_PAGE(UIWanSave)
APP_REGISTER_PAGE(UIWlanInvalidPassword)
APP_REGISTER_PAGE(UIInstallConfirm)
APP_REGISTER_PAGE(UIDisconnectPrompt)
