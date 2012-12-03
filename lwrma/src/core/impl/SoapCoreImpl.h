#ifndef __SoapCoreImpl_h__
#define __SoapCoreImpl_h__

#include <LenovoCore/SoapCore.h>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class SoapCoreImpl
	: public QObject
{
	Q_OBJECT

public:
	SoapCoreImpl(SoapCore *intf);
	virtual ~SoapCoreImpl();
	SoapCore *q_ptr() const;

	AsyncOp *invoke(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList);
	AsyncOp *invokeBasic(int port, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);
	AsyncOp *invokeFallback(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);
	AsyncOp *invokeWrapped(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);

	QNetworkAccessManager *networkAccessManager() const;
	QString sessionId() const;
	QList<int> portList() const;
	void setPreferredPort(int port);
	QString wrappedStatus() const;

	SoapCore *m_intf;
	QPointer<QNetworkAccessManager> m_nam;
	QString m_sessionId;
	QList<int> m_portList;
	QString m_wrappedStatus;
	bool m_wrappedMode;
	QString m_routerAddress;
};

class BasicSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	BasicSoapOp(SoapCoreImpl *core, int port, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);
	virtual ~BasicSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onReplyFinished();

private:
	QString buildSoapXml(const QString& sessionId, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList);

private:
	SoapCoreImpl *m_core;
	int m_port;
	QString m_ns;
	QString m_action;
	QStringList m_nameList;
	QStringList m_valueList;
	QPointer<QNetworkReply> m_reply;
	QString m_host;
};

class FallbackSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	FallbackSoapOp(SoapCoreImpl *core, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);
	virtual ~FallbackSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onSoapFinished();

private:
	SoapCoreImpl *m_core;
	QString m_ns;
	QString m_action;
	QStringList m_nameList;
	QStringList m_valueList;
	QList<int> m_portList;
	QPointer<AsyncOp> m_op;
	int m_activePort;
	QString m_host;
};

class WrappedSoapOp
	: public AsyncOp
{
	Q_OBJECT

public:
	WrappedSoapOp(SoapCoreImpl *core, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host);
	virtual ~WrappedSoapOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onSoap1Finished();
	void onSoap2Finished();
	void onSoap3Finished();

private:
	SoapCoreImpl *m_core;
	QString m_ns;
	QString m_action;
	QStringList m_nameList;
	QStringList m_valueList;
	QPointer<AsyncOp> m_op;
	QString m_host;
};

#endif // __SoapCoreImpl_h__
