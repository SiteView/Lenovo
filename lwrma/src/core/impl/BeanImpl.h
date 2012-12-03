#ifndef __BeanImpl_h__
#define __BeanImpl_h__

#include <LenovoCore/AsyncOp.h>
#include <LenovoCore/Bean.h>
#include <LenovoCore/SoapCore.h>
#include <LenovoCore/System.h>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

class BeanImpl
	: public QObject
{
	Q_OBJECT

public:
	BeanImpl(Bean *intf, const QString& ident);
	virtual ~BeanImpl();
	Bean *q_ptr() const;
	
	bool init();

	AsyncOp *changeRouterWifiPassword(const QString& password, const QString& ssid, bool configStart, bool configFinish);
	AsyncOp *changeRouterPassword(const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword);
	AsyncOp *changeRouterWanSettings(const QVariantMap& settings);

	AsyncOp *ensureSoap(int delay, int retryCount);
	AsyncOp *discoverRouterSoap(int timeout, const QString& mac, int maxRetryCount, int retryDelay);
	AsyncOp *reconnectRouter(int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect);

	AsyncOp *restartRouter();
	AsyncOp *checkInternet(const QString& host, int delay, int maxRetryCount);

public:
	Bean *m_intf;
	SoapCore *m_soapCore;
	System *m_system;
};

class ChangeRouterWifiPasswordOp
	: public AsyncOp
{
	Q_OBJECT

public:
	ChangeRouterWifiPasswordOp(BeanImpl *bean, const QString& password, const QString& ssid, bool configStart, bool configFinish);
	virtual ~ChangeRouterWifiPasswordOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onConfigStarted();
	void onGetInfoFinished();
	void onSetWLANFinished();
	void onCommitFinished();
	void onTimeout1();

private:

private:
	BeanImpl *m_bean;
	QString m_password;
	QString m_ssid;
	bool m_configStart;
	bool m_configFinish;
	QPointer<AsyncOp> m_op;
	QTimer m_timer1;
};

class ChangeRouterWanSettingsOp
	: public AsyncOp
{
	Q_OBJECT

public:
	ChangeRouterWanSettingsOp(BeanImpl *bean, const QVariantMap& settings);
	virtual ~ChangeRouterWanSettingsOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onSoapFinished();

private:
	BeanImpl *m_bean;
	QVariantMap m_settings;
	QPointer<AsyncOp> m_op;
};

class ChangeRouterPasswordOp
	: public AsyncOp
{
	Q_OBJECT

public:
	ChangeRouterPasswordOp(BeanImpl *bean, const QString& wifiPassword, const QString& oldWifiPassword, const QString& adminPassword, const QString& oldAdminPassword);
	virtual ~ChangeRouterPasswordOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onNothingToDo();
	void onGetInfoFinished();
	void onGetKeysFinished();
	void onGetLanInfoFinished();
	void onConfigInFinished();
	void onWifiConfigFinished();
	void onAdminConfigFinished();
	void onConfigOutFinished();
	void onTimeout1();
	void onCreateProfileFinished();
	void onReconnectFinished();

private:
	void startWifiConfig();
	void startAdminConfig();
	void startConfigOut();
	void updateProfile();

private:
	BeanImpl *m_bean;
	QString m_wifiPassword;
	QString m_oldWifiPassword;
	QString m_adminPassword;
	QString m_oldAdminPassword;
	QPointer<AsyncOp> m_op;
	QVariantMap m_infoResp;
	QString m_currentWifiKey;
	QTimer m_timer1;
};

class EnsureSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	EnsureSoapOp(BeanImpl *bean, int delay, int retryCount);
	virtual ~EnsureSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void restart();
	void onSoapFinished();
	void onTimeout();

private:
	BeanImpl *m_bean;
	int m_delay;
	int m_retryCount;
	int m_currentCount;
	QPointer<AsyncOp> m_op;
	QTimer m_timer1;
};

class CheckRouterSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	CheckRouterSoapOp(BeanImpl *bean, const QString& host);
	virtual ~CheckRouterSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onGetInfo1Finished();
	void onGetInfo2Finished();
	void onGetInfo3Finished();

private:
	BeanImpl *m_bean;
	QPointer<AsyncOp> m_op;
	QString m_host;
	QVariantMap m_values;
};

class MasterDiscoverRouterSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	MasterDiscoverRouterSoapOp(BeanImpl *bean, int timeout, const QString& mac, int maxRetryCount, int retryDelay);
	virtual ~MasterDiscoverRouterSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onOpFinished();
	void onRetryTimeout();

private:
	BeanImpl *m_bean;
	int m_timeout;
	QString m_mac;
	int m_maxRetryCount;
	int m_retryDelay;
	int m_retryCount;
	QPointer<AsyncOp> m_op;
	QTimer m_timer1;
};

class DiscoverRouterSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	DiscoverRouterSoapOp(BeanImpl *bean, int timeout, const QString& mac);
	virtual ~DiscoverRouterSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onSoapFinished();
	void onTimeout();

private:
	void process();

private:
	BeanImpl *m_bean;
	int m_timeout;
	QString m_mac;
	QList<AsyncOp*> m_opList;
	QTimer m_timer;
	int m_count;
	QStringList m_ls;
};

class ReconnectRouterOp
	: public AsyncOp
{
	Q_OBJECT

public:
	ReconnectRouterOp(BeanImpl *bean, int delay, int maxRetryCount, const QString& mac, const QString& wifiName, bool reconnect);
	virtual ~ReconnectRouterOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onConnectProfileFinished();
	void onDiscoverRouterFinished();
	void onTimeout();

private:
	void restart();
	void connectProfile();
	void discoverRouter();
	void retry();

private:
	BeanImpl *m_bean;
	int m_delay;
	int m_maxRetryCount;
	QString m_mac;
	QString m_wifiName;
	bool m_reconnect;
	QPointer<AsyncOp> m_op;
	int m_retryCount;
	QTimer m_timer1;
};

class RestartRouterOp
	: public AsyncOp
{
	Q_OBJECT

public:
	RestartRouterOp(BeanImpl *bean);
	virtual ~RestartRouterOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onConfigStarted();
	void onConfigFinished();

private:
	BeanImpl *m_bean;
	QPointer<AsyncOp> m_op;
};

class CheckInternetOp
	: public AsyncOp
{
	Q_OBJECT

public:
	CheckInternetOp(BeanImpl *bean, const QString& host, int delay, int maxRetryCount);
	virtual ~CheckInternetOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onConnectHostFinished();
	void onTimeout();

private:
	void restart();
	void connectHost();
	void retry();

private:
	BeanImpl *m_bean;
	QString m_host;
	int m_delay;
	int m_maxRetryCount;
	QPointer<AsyncOp> m_op;
	int m_retryCount;
	QTimer m_timer1;
};

#endif // __BeanImpl_h__
