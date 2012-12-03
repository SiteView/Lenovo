#include "AppService.h"
#include "MiniApp.h"

AppService::AppService(MiniApp *app)
	: m_app(app)
{
	connect(m_app, SIGNAL(incomingBPSChanged()), SIGNAL(incomingBPSChanged()));
	connect(m_app, SIGNAL(outgoingBPSChanged()), SIGNAL(outgoingBPSChanged()));
	connect(m_app, SIGNAL(uptimeMinutesChanged()), SIGNAL(uptimeMinutesChanged()));
	connect(m_app, SIGNAL(wifiNameChanged()), SIGNAL(wifiNameChanged()));
	connect(m_app, SIGNAL(wifiSecurityChanged()), SIGNAL(wifiSecurityChanged()));
}

AppService::~AppService()
{
}

SoapCore *AppService::soapCore() const
{
	return m_app->soapCore();
}

System *AppService::system() const
{
	return m_app->system();
}

Bean *AppService::bean() const
{
	return m_app->bean();
}

QString AppService::translateUIText(int stringId)
{
	return m_app->translateUIText(stringId, NULL);
}

QString AppService::translateUIText(int stringId, QWidget *widget)
{
	return m_app->translateUIText(stringId, widget);
}

void AppService::navigateTo(const QString& pageName)
{
	m_app->navigateTo(pageName, QVariantMap());
}

void AppService::navigateTo(const QString& pageName, const QVariantMap& params)
{
	m_app->navigateTo(pageName, params);
}

void AppService::setLanguage(const QString& lang)
{
	m_app->setLanguage(lang);
}

void AppService::closePage()
{
	m_app->closePage();
}

void AppService::beginUserMode()
{
	m_app->beginUserMode();
}

void AppService::quit()
{
	QMetaObject::invokeMethod(m_app, "onQuitNow", Qt::QueuedConnection);
}

bool AppService::wait(AsyncOp *op)
{
	return m_app->wait(op, -1);
}

bool AppService::wait(AsyncOp *op, int stringId)
{
	return m_app->wait(op, stringId);
}

AsyncOp *AppService::testRouter(int timeout)
{
	return m_app->testRouter(timeout);
}

void AppService::setWifiPassword(const QString& wifiPassword)
{
	m_app->setWifiPassword(wifiPassword);
}

QString AppService::wifiPassword() const
{
	return m_app->wifiPassword();
}

void AppService::generateWifiName()
{
	m_app->generateWifiName();
}

void AppService::confirmWifiName()
{
	m_app->confirmWifiName();
}

void AppService::setWifiSecurity(bool wifiSecurity)
{
	m_app->setWifiSecurity(wifiSecurity);
}

void AppService::confirmNewWifiPassword(const QString& password)
{
	m_app->confirmNewWifiPassword(password);
}

QString AppService::savedWifiPassword() const
{
	return m_app->savedWifiPassword();
}

double AppService::incomingBPS() const
{
	return m_app->incomingBPS();
}

double AppService::outgoingBPS() const
{
	return m_app->outgoingBPS();
}

int AppService::uptimeMinutes() const
{
	return m_app->uptimeMinutes();
}

QString AppService::wifiName() const
{
	return m_app->wifiName();
}

bool AppService::wifiSecurity() const
{
	return m_app->wifiSecurity();
}

bool AppService::validateWifiPassword(const QString& password) const
{
	return m_app->validateWifiPassword(password);
}

QString AppService::versionString() const
{
	return m_app->versionString();
}

void AppService::wait(AsyncOp *op, QObject *obj, const char *member)
{
	m_app->wait(op, obj, member);
}

void AppService::configSet(const QString& name, const QVariant& value)
{
	m_app->configSet(name, value);
}

QVariant AppService::configGet(const QString& name) const
{
	return m_app->configGet(name);
}

void AppService::showMessage(int stringId)
{
	m_app->showMessage(stringId);
}
