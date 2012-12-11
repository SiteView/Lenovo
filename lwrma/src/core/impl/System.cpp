#include "SystemImpl.h"

System::System(const QString& ident, QObject *parent)
	: QObject(parent)
{
	m_impl = new SystemImpl(this, ident);
}

System::~System()
{
	delete m_impl;
}

SystemImpl *System::d_ptr() const
{
	return m_impl;
}

bool System::init()
{
	return m_impl->init();
}

bool System::checkInstance(QWidget *mainWindow)
{
	return m_impl->checkInstance(mainWindow);
}

void System::markWindow(QWidget *mainWindow)
{
	m_impl->markWindow(mainWindow);
}

bool System::checkMessage(MSG *msg, long *result)
{
	return m_impl->checkMessage(msg, result);
}

void System::executeAs(QWidget *mainWindow, const QString& path)
{
	m_impl->executeAs(mainWindow, path);
}

void System::broadcastQuitEvent()
{
	m_impl->broadcastQuitEvent();
}

void System::startQuitDog()
{
	m_impl->startQuitDog();
}

void System::stopQuitDog()
{
	m_impl->stopQuitDog();
}

AsyncOp *System::startWlanService()
{
	return m_impl->startWlanService();
}

AsyncOp *System::searchSsid(const QString& ssid)
{
	return m_impl->searchSsid(ssid);
}

AsyncOp *System::connectSsid(const QVariantMap& ssid)
{
	return m_impl->connectSsid(ssid);
}

AsyncOp *System::httpGet(const QString& url)
{
	return m_impl->httpGet(url);
}

AsyncOp *System::createWlanProfile(const QString& ssid, const QString& password)
{
	return m_impl->createWlanProfile(ssid, password);
}

AsyncOp *System::connectWlanProfile(const QString& profile, bool reconnect)
{
	return m_impl->connectWlanProfile(profile, reconnect);
}

bool System::needsElevation() const
{
	return m_impl->m_needsElevation;
}

void System::queryNetworkPerf(System::NetworkPerf& perf)
{
	m_impl->queryNetworkPerf(perf);
}

QIcon System::loadAppIcon(int name, int cx, int cy)
{
	QIcon icon;
	HANDLE hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(name), IMAGE_ICON, cx, cy, 0);
	if (hIcon) {
		icon = QIcon(QPixmap::fromWinHICON(reinterpret_cast<HICON>(hIcon)));
		DestroyIcon(reinterpret_cast<HICON>(hIcon));
	}
	return icon;
}

void System::runUpdateSetup(QWidget *mainWindow, const QString& path)
{
	m_impl->runUpdateSetup(mainWindow, path);
}

bool System::checkRouter(const QString& mac)
{
	return m_impl->checkRouter(mac);
}
bool System::checkRouter2(const QString& ssidName)
{
    return m_impl->checkRouter2(ssidName);
}

bool System::wlanServiceActive() const
{
	return m_impl->m_wlanServiceActive;
}

QUuid System::wlanInterfaceUuid() const
{
	return m_impl->m_wlanInterfaceUuid;
}

QString System::wlanInterfaceDescription() const
{
	return m_impl->m_wlanInterfaceDescription;
}

void System::updateWlanState()
{
	m_impl->updateWlanState();
}

QStringList System::queryGatewayList() const
{
	return m_impl->queryGatewayList();
}
