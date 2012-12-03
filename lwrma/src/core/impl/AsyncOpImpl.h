#ifndef __AsyncOpImpl_h__
#define __AsyncOpImpl_h__

#include <LenovoCore/AsyncOp.h>

class AsyncOpImpl
	: public QObject
{
	Q_OBJECT

public:
	AsyncOpImpl(AsyncOp *intf);
	virtual ~AsyncOpImpl();
	AsyncOp *q_ptr() const;

	void setValue(const char *name, const QVariant& value);
	void setValues(const QVariantMap& values);
	QVariant value(const char *name) const;
	void copyValues(const AsyncOpImpl *other);
	void abort();
	void notifyFinished(int result);

	AsyncOp *m_intf;
	QVariantMap m_valueMap;
	int m_result;
	bool m_aborted;
	bool m_finished;
};

#endif // __AsyncOpImpl_h__
