#ifndef __SoapCore_h__
#define __SoapCore_h__

#include <LenovoCore/AsyncOp.h>

class SoapCoreImpl;

class LENOVOCORE_API SoapCore
	: public QObject
{
	Q_OBJECT

public:
	SoapCore(QObject *parent = NULL);
	virtual ~SoapCore();
	SoapCoreImpl *d_ptr() const;
	void setSessionId(const QString& sessionId);
	void setWrappedMode(bool wrappedMode, const QString& newStatus = QString());
	QString sessionId() const;
	void setHost(const QString& host);
	QString host() const;

public Q_SLOTS:
	AsyncOp *invoke(const QString& ns, const QString& action);
	AsyncOp *invoke(const QString& ns, const QString& action, const QString& name, const QString& value);
	AsyncOp *invoke(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList);

private:
	friend class SoapCoreImpl;
	SoapCoreImpl *m_impl;
};

#endif // __SoapCore_h__
