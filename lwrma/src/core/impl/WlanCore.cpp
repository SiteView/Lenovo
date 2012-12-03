#include "WlanCoreImpl.h"
#include <QtCore/QDir>

WlanCore::WlanCore(QObject *parent)
	: QObject(parent)
{
	m_impl = new WlanCoreImpl(this);
}

WlanCore::~WlanCore()
{
	delete m_impl;
}

WlanCoreImpl *WlanCore::d_ptr() const
{
	return m_impl;
}

bool WlanCore::init()
{
	return d_ptr()->init();
}

bool WlanCore::softwareEnabled() const
{
	return m_impl->m_softwareEnabled;
}

bool WlanCore::hardwareEnabled() const
{
	return m_impl->m_hardwareEnabled;
}

bool WlanCore::enabled() const
{
	return m_impl->m_enabled;
}

QString WlanCore::ssid() const
{
	return m_impl->m_ssid;
}

QByteArray WlanCore::macAddress() const
{
	return m_impl->m_macAddress;
}

int WlanCore::signalQuality() const
{
	return m_impl->m_signalQuality;
}

QString WlanCore::profileName() const
{
	return m_impl->m_profileName;
}

bool WlanCore::connected() const
{
	return m_impl->m_connected;
}

QList<WlanCore::Network> WlanCore::snapshotNetworkList() const
{
	return m_impl->snapshotNetworkList();
}

QString WlanCore::getProfileContent(const QString& profileName) const
{
	return m_impl->getProfileContent(profileName);
}

void WlanCore::connectWithProfile(const QString& profileName, int bssType)
{
	m_impl->connectWithProfile(profileName, bssType);
}

void WlanCore::connectWithSsid(const QString& ssid, bool securityEnabled)
{
	m_impl->connectWithSsid(ssid, securityEnabled);
}

void WlanCore::connectForce(const QString& ssid, const QString& profileName, bool securityEnabled)
{
	m_impl->connectForce(ssid, profileName, securityEnabled);
}
