#include "SoapCoreImpl.h"

SoapCore::SoapCore(QObject *parent)
	: QObject(parent)
{
	m_impl = new SoapCoreImpl(this);
}

SoapCore::~SoapCore()
{
	delete m_impl;
}

SoapCoreImpl *SoapCore::d_ptr() const
{
	return m_impl;
}

void SoapCore::setSessionId(const QString& sessionId)
{
	d_ptr()->m_sessionId = sessionId;
}

QString SoapCore::sessionId() const
{
	return d_ptr()->m_sessionId;
}

void SoapCore::setHost(const QString& host)
{
	d_ptr()->m_routerAddress = host;
}

QString SoapCore::host() const
{
	return d_ptr()->m_routerAddress;
}

void SoapCore::setWrappedMode(bool wrappedMode, const QString& newStatus)
{
	if (wrappedMode) {
		d_ptr()->m_wrappedMode = true;
		if (!newStatus.isEmpty()) {
			d_ptr()->m_wrappedStatus = newStatus;
		}
	} else {
		d_ptr()->m_wrappedMode = false;
	}
}

AsyncOp *SoapCore::invoke(const QString& ns, const QString& action)
{
	return d_ptr()->invoke(ns, action, QStringList(), QStringList());
}

AsyncOp *SoapCore::invoke(const QString& ns, const QString& action, const QString& name, const QString& value)
{
	return d_ptr()->invoke(ns, action, QStringList(name), QStringList(value));
}

AsyncOp *SoapCore::invoke(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList)
{
	return d_ptr()->invoke(ns, action, nameList, valueList);
}
