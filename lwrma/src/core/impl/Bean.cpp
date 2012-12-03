#include "BeanImpl.h"

Bean::Bean(const QString& ident, QObject *parent)
	: QObject(parent)
{
	m_impl = new BeanImpl(this, ident);
}

Bean::~Bean()
{
	delete m_impl;
}

BeanImpl *Bean::d_ptr() const
{
	return m_impl;
}

System *Bean::system() const
{
	return m_impl->m_system;
}

SoapCore *Bean::soapCore() const
{
	return m_impl->m_soapCore;
}

bool Bean::init()
{
	return m_impl->init();
}

AsyncOp *Bean::changeRouterWifiPassword(const QString& password, const QString& ssid, bool configStart, bool configFinish)
{
	return m_impl->changeRouterWifiPassword(password, ssid, configStart, configFinish);
}

AsyncOp *Bean::changeRouterPassword(const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword)
{
	return m_impl->changeRouterPassword(wifiPassword, oldWifiPassword, adminPassword, oldAdminPassword);
}

AsyncOp *Bean::changeRouterWanSettings(const QVariantMap& settings)
{
	return m_impl->changeRouterWanSettings(settings);
}

AsyncOp *Bean::ensureSoap(int delay, int retryCount)
{
	return m_impl->ensureSoap(delay, retryCount);
}

AsyncOp *Bean::discoverRouterSoap(int timeout, const QString& mac, int maxRetryCount, int retryDelay)
{
	return m_impl->discoverRouterSoap(timeout, mac, maxRetryCount, retryDelay);
}

AsyncOp *Bean::reconnectRouter(int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect)
{
	return m_impl->reconnectRouter(delay, maxRetryCount, mac, wifiName, reconnect);
}

AsyncOp *Bean::restartRouter()
{
	return m_impl->restartRouter();
}

AsyncOp *Bean::checkInternet(const QString& host, int delay, int maxRetryCount)
{
	return m_impl->checkInternet(host, delay, maxRetryCount);
}
