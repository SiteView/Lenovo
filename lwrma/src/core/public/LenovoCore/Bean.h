#ifndef __Bean_h__
#define __Bean_h__

#include <LenovoCore/Base.h>
#include <QtCore/QVariantMap>

class AsyncOp;
class BeanImpl;
class SoapCore;
class System;

class LENOVOCORE_API Bean
	: public QObject
{
	Q_OBJECT

public:
	Bean(const QString& ident, QObject *parent = NULL);
	virtual ~Bean();
	BeanImpl *d_ptr() const;
	System *system() const;
	SoapCore *soapCore() const;

	bool init();

	AsyncOp *changeRouterWifiPassword(const QString& password, const QString& ssid, bool configStart, bool configFinish);
	AsyncOp *changeRouterPassword(const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword);
	AsyncOp *changeRouterWanSettings(const QVariantMap& settings);

	AsyncOp *ensureSoap(int delay, int retryCount);
	AsyncOp *discoverRouterSoap(int timeout, const QString& mac, int maxRetryCount = 0, int retryDelay = 5000);
	AsyncOp *reconnectRouter(int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect);

	AsyncOp *restartRouter();
	AsyncOp *checkInternet(const QString& host, int delay, int maxRetryCount);

private:
	friend class BeanImpl;
	BeanImpl *m_impl;
};

#endif // __Bean_h__
