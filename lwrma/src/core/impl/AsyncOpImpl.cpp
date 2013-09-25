#include "AsyncOpImpl.h"

#include <QDebug>

AsyncOpImpl::AsyncOpImpl(AsyncOp *intf)
    : m_intf(intf), m_result(0), m_aborted(false), m_finished(false)
{
}

AsyncOpImpl::~AsyncOpImpl()
{
}

AsyncOp *AsyncOpImpl::q_ptr() const
{
    return m_intf;
}

void AsyncOpImpl::setValue(const char *name, const QVariant& value)
{
    QString key = QString::fromUtf8(name).toLower();
    QVariantMap::iterator it = m_valueMap.find(key);
    if (it != m_valueMap.end())
    {
        it->setValue(value);
    }
    else
    {
        m_valueMap.insert(key, value);
    }
}

void AsyncOpImpl::setValues(const QVariantMap& values)
{
    QVariantMap::const_iterator it = values.begin();
    QVariantMap::const_iterator ie = values.end();
    for (; it != ie; ++it)
    {
        setValue(it.key().toUtf8(), it.value());
    }
}

QVariant AsyncOpImpl::value(const char *name) const
{
    QVariant retval;
    QString key = QString::fromUtf8(name).toLower();
    QVariantMap::const_iterator it = m_valueMap.find(key);
    if (it != m_valueMap.end())
    {
        retval = it.value();
    }
    return retval;
}

void AsyncOpImpl::copyValues(const AsyncOpImpl *other)
{
    if (this != other)
    {
        m_valueMap = other->m_valueMap;
    }
}

void AsyncOpImpl::abort()
{
    if (!m_finished && !m_aborted)
    {
        m_aborted = true;
        q_ptr()->onAbort();
    }
}

void AsyncOpImpl::notifyFinished(int result)
{
    if (!m_finished && !m_aborted)
    {
        m_finished = true;
        m_result = result;
        emit q_ptr()->finished();
    }
}
