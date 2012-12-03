#include "AsyncOpImpl.h"

static int regAsyncOpMetaType = qRegisterMetaType<AsyncOp*>();

AsyncOp::AsyncOp(QObject *parent)
	: QObject(parent)
{
	m_impl = new AsyncOpImpl(this);
}

AsyncOp::~AsyncOp()
{
	delete m_impl;
}

AsyncOpImpl *AsyncOp::d_ptr() const
{
	return m_impl;
}

void AsyncOp::setValue(const char *name, const QVariant& value)
{
	d_ptr()->setValue(name, value);
}

QVariant AsyncOp::value(const char *name) const
{
	return d_ptr()->value(name);
}

void AsyncOp::copyValues(const AsyncOp *other)
{
	d_ptr()->copyValues(other->d_ptr());
}

void AsyncOp::setValues(const QVariantMap& values)
{
	d_ptr()->setValues(values);
}

QVariantMap AsyncOp::values() const
{
	return d_ptr()->m_valueMap;
}

void AsyncOp::abort()
{
	d_ptr()->abort();
}

bool AsyncOp::isFinished() const
{
	return d_ptr()->m_finished;
}

bool AsyncOp::isAborted() const
{
	return d_ptr()->m_aborted;
}

int AsyncOp::result() const
{
	return d_ptr()->m_result;
}

void AsyncOp::notifyFinished(int result)
{
	d_ptr()->notifyFinished(result);
}

void AsyncOp::onAbort()
{
}
