#include "SystemImpl.h"
#include <LenovoCore/Logger.h>
#include <QtCore/QCoreApplication>
//#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtTest/QTest>

DEFINE_LOGGER(SystemImpl);

static HANDLE CreateSharedEvent(const WCHAR *name)
{
    SECURITY_DESCRIPTOR sd = { 0 };
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    return CreateEvent(&sa, TRUE, FALSE, name);
}

static long queryPerformanceData(const QString& key, QByteArray& data)
{
    long status;
    DWORD type;
    DWORD cb;
    BYTE *rawData;

    WCHAR keyName[256];
    int len = key.toWCharArray(keyName);
    keyName[len] = 0;

    cb = 4096;
    rawData = static_cast<BYTE*>(malloc(cb));

    for (;;)
    {
        status = RegQueryValueEx(HKEY_PERFORMANCE_DATA, keyName, NULL, &type, rawData, &cb);
        if (status == ERROR_MORE_DATA)
        {
            cb <<= 1;
            rawData = static_cast<BYTE*>(realloc(rawData, cb));
        }
        else
        {
            break;
        }
    }

    if (status == ERROR_SUCCESS)
    {
        data = QByteArray(reinterpret_cast<char*>(rawData), cb);
    }
    free(rawData);
    return status;
}

static int rt_startWlanService()
{
    OSVERSIONINFOEX osver;
    osver.dwOSVersionInfoSize = sizeof(osver);
    if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osver)))
    {
        LOG_DEBUG(QString::fromUtf8("GetVersionEx failed, err=%1").arg(GetLastError()));
        return 40;
    }

    const WCHAR *serviceName;
    if (osver.dwMajorVersion >= 6)
    {
        serviceName = L"wlansvc";
    }
    else
    {
        serviceName = L"wzcsvc";
    }

    LOG_DEBUG(QString::fromUtf8("WINVER %1.%2 service name [%3]").arg(osver.dwMajorVersion).arg(osver.dwMinorVersion).arg(QString::fromWCharArray(serviceName)));

    SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hManager)
    {
        LOG_DEBUG(QString::fromUtf8("OpenSCManager failed, err=%1").arg(GetLastError()));
        return 41;
    }

    SC_HANDLE hService = OpenService(hManager, serviceName, SERVICE_ALL_ACCESS);
    if (!hService)
    {
        LOG_DEBUG(QString::fromUtf8("OpenService failed, err=%1").arg(GetLastError()));
        CloseServiceHandle(hManager);
        return 42;
    }

    if (StartService(hService, NULL, NULL))
    {
        for (;;)
        {
            SERVICE_STATUS_PROCESS serviceStatus;
            DWORD cb;
            if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&serviceStatus), sizeof(serviceStatus), &cb))
            {
                if (serviceStatus.dwCurrentState == SERVICE_START_PENDING)
                {
                    DWORD dwWaitTime = serviceStatus.dwWaitHint / 10;
                    if (dwWaitTime < 1000)
                    {
                        dwWaitTime = 1000;
                    }
                    else if (dwWaitTime > 10000)
                    {
                        dwWaitTime = 10000;
                    }
                    Sleep(dwWaitTime);
                }
                else
                {
                    break;
                }
            }
            else
            {
                LOG_DEBUG(QString::fromUtf8("QueryServiceStatusEx failed, err=%1").arg(GetLastError()));
                break;
            }
        }
    }
    else
    {
        LOG_DEBUG(QString::fromUtf8("StartService failed, err=%1").arg(GetLastError()));
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return 0;
}

bool System::runCommand(int *retval)
{
    QStringList args = qApp->arguments();
    for (int i = 0; i < args.count(); i++)
    {
        LOG_DEBUG(QString::fromUtf8("arg[%1] %2]").arg(i).arg(args.at(i)));
        if (args.at(i).compare(QLatin1String("-cmd=startWlanService")) == 0)
        {
            *retval = rt_startWlanService();
            return true;
        }
    }
    return false;
}

SystemImpl::SystemImpl(System *intf, const QString& ident)
    : m_intf(intf), m_ident(ident), m_quitDog(NULL)
    , m_wlanHandle(NULL)
{
    QString instName = QString::fromUtf8("%1").arg(ident);
    int len = instName.toWCharArray(m_instanceName);
    m_instanceName[len] = 0;

    QString eventName = QString::fromUtf8("Global\\%1").arg(ident);
    len = eventName.toWCharArray(m_globalEventName);
    m_globalEventName[len] = 0;

    m_wlanapi = new WlanLib();
    m_nam = new QNetworkAccessManager();
}

SystemImpl::~SystemImpl()
{
    stopQuitDog();
    if (m_wlanHandle)
    {
        m_wlanapi->WlanCloseHandle(m_wlanHandle, NULL);
    }
    delete m_wlanapi;
    delete m_nam;
}

System *SystemImpl::q_ptr() const
{
    return m_intf;
}

bool SystemImpl::init()
{
    m_osver.dwOSVersionInfoSize = sizeof(m_osver);
    if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&m_osver)))
    {
        LOG_ERROR(QString::fromUtf8("GetVersionEx failed! [err=%1]").arg(GetLastError()));
        return false;
    }

    LOG_INFO(QString::fromUtf8("Windows %1.%2").arg(m_osver.dwMajorVersion).arg(m_osver.dwMinorVersion));

    BOOL admin;
    if (m_osver.dwMajorVersion < 6)
    {
        SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
        PSID AdministratorsGroup;
        if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup))
        {
            if (!CheckTokenMembership(NULL, AdministratorsGroup, &admin))
            {
                admin = FALSE;
            }
            FreeSid(AdministratorsGroup);
        }
    }
    else
    {
        admin = FALSE;
    }

    if (admin)
    {
        m_needsElevation = false;
    }
    else
    {
        m_needsElevation = true;
        LOG_INFO(QString::fromUtf8("Elevation UI needed"));
    }

    if (!m_wlanapi->init())
    {
        LOG_ERROR(QString::fromUtf8("WlanLib::init failed!"));
        return false;
    }

    //connect(&m_processTimer, SIGNAL(timeout()), SLOT(process()));
    //m_processTimer.start(10000);
    m_wlanInit = true;
    //process();
    m_wlanInit = false;
    return true;
}

struct EnumWindowParam
{
    HWND hwnd;
    const WCHAR *tagName;
};

static BOOL CALLBACK __EnumProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowParam *param = reinterpret_cast<EnumWindowParam*>(lParam);
    //LOG_DEBUG(QString().sprintf("target WindowID: %p", hwnd));
    if (GetProp(hwnd, param->tagName))
    {
        param->hwnd = hwnd;
        return FALSE;
    }
    else
    {
        //LOG_DEBUG(QString().sprintf("GetProp WindowID: %p %d", hwnd, GetLastError()));
    }
    return TRUE;
}

bool SystemImpl::checkInstance(QWidget *mainWindow)
{
    EnumWindowParam param;
    param.hwnd = NULL;
    param.tagName = m_instanceName;
    EnumWindows(&__EnumProc, reinterpret_cast<LPARAM>(&param));
    if (param.hwnd)
    {
        SetForegroundWindow(param.hwnd);
        PostMessage(param.hwnd, WM_USER + 400, 0, 0);
        return false;
    }
    HWND hwnd = mainWindow->winId();

    //LOG_DEBUG(QString().sprintf("MyWindowID: %p", hwnd));
    SetProp(hwnd, m_instanceName, reinterpret_cast<HANDLE>(1));
    return true;
}

void SystemImpl::markWindow(QWidget *mainWindow)
{
    HWND hwnd = mainWindow->winId();
    SetProp(hwnd, m_instanceName, reinterpret_cast<HANDLE>(1));
}

bool SystemImpl::checkMessage(MSG *msg, long *result)
{
    if (msg->message == WM_USER + 400)
    {
        *result = 0;
        LOG_DEBUG(QString::fromUtf8("WM_USER success"));
        return true;
    }
    else if (msg->message == WM_POWERBROADCAST)
    {
        LOG_DEBUG(QString::fromUtf8("WM_POWERBROADCAST %1").arg(msg->wParam));
        if (msg->wParam == PBT_APMRESUMEAUTOMATIC)
        {
            LOG_DEBUG(QString::fromUtf8("also notify resume"));
            QMetaObject::invokeMethod(q_ptr(), "systemResumed", Qt::QueuedConnection);
            // bool rMethod= if(rMethod) LOG_DEBUG(QString::fromUtf8("invoke success"));//sleep
        }
        else if(msg->wParam == PBT_APMSUSPEND)
        {
            LOG_DEBUG(QString::fromUtf8("system Sleep or Hibernate"));
            QMetaObject::invokeMethod(q_ptr(), "systemSleep", Qt::QueuedConnection);
        }
    }
    return false;
}

void SystemImpl::runUpdateSetup(QWidget *mainWindow, const QString& path)
{
    QString nativePath = QDir::toNativeSeparators(path);
    LOG_DEBUG(QString::fromUtf8("runUpdateSetup navivePath: %1").arg(nativePath));
    BOOL admin = FALSE;

    SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
    PSID AdministratorsGroup;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup))
    {
        LOG_DEBUG(QString::fromUtf8("runUpdateSetup AllocateAndInitializeSid ok"));
        if (CheckTokenMembership(NULL, AdministratorsGroup, &admin))
        {
            LOG_DEBUG(QString::fromUtf8("runUpdateSetup CheckTokenMembership ok admin=%1").arg(admin));
        }
        else
        {
            admin = FALSE;
            LOG_DEBUG(QString::fromUtf8("runUpdateSetup CheckTokenMembership not ok, assume admin=FALSE"));
        }
        FreeSid(AdministratorsGroup);
    }
    else
    {
        LOG_DEBUG(QString::fromUtf8("runUpdateSetup AllocateAndInitializeSid not ok %1").arg(GetLastError()));
    }

    SHELLEXECUTEINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.hwnd = mainWindow->winId();
    info.lpFile = reinterpret_cast<WCHAR*>(nativePath.data());
    info.nShow = SW_SHOW;
    if (admin)
    {
        info.lpVerb = L"open";
    }
    else
    {
        info.lpVerb = L"runas";
    }

    if (ShellExecuteEx(&info))
    {
        LOG_DEBUG(QString::fromUtf8("runUpdateSetup ShellExecuteEx ok"));
    }
    else
    {
        LOG_DEBUG(QString::fromUtf8("runUpdateSetup ShellExecuteEx not ok, err=%1").arg(GetLastError()));
    }
}

void SystemImpl::executeAs(QWidget *mainWindow, const QString& path)
{
    //QString appPath = qApp->app

    /*	WCHAR fullPath[MAX_PATH * 4];
    	int len = QDir::toNativeSeparators(path).toWCharArray(fullPath);
    	fullPath[len] = 0;

    	BOOL admin = FALSE;

    	SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
    	PSID AdministratorsGroup;
    	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
    		if (!CheckTokenMembership(NULL, AdministratorsGroup, &admin)) {
    			admin = FALSE;
    		}
    		FreeSid(AdministratorsGroup);
    	}

    	if (!admin) {
    		//ShellExecute(NULL, L"runas", fullPath, NULL, NULL, SW_SHOW);
    		SHELLEXECUTEINFO info;
    		memset(&info, 0, sizeof(info));
    		info.cbSize = sizeof(info);
    		info.hwnd = mainWindow->winId();
    		info.lpVerb = L"runas";
    		info.lpFile = fullPath;
    		info.nShow = SW_SHOW;
    		ShellExecuteEx(&info);
    		info.hProcess = 0;
    	}*/
}

void SystemImpl::broadcastQuitEvent()
{
    HANDLE hGlobalQuitEvent = CreateSharedEvent(m_globalEventName);
    SetEvent(hGlobalQuitEvent);
    CloseHandle(hGlobalQuitEvent);
}

void SystemImpl::startQuitDog()
{
    if (!m_quitDog)
    {
        m_quitDog = new QuitDog(this, m_globalEventName);
    }
}

void SystemImpl::stopQuitDog()
{
    if (m_quitDog)
    {
        delete m_quitDog;
        m_quitDog = NULL;
    }
}

AsyncOp *SystemImpl::startWlanService()
{
    StartWlanServiceOp *op = new StartWlanServiceOp(qApp->applicationFilePath(), qApp->applicationDirPath(), m_needsElevation);
    op->start();
    return op;
}

AsyncOp *SystemImpl::searchSsid(const QString& ssid)
{
    SearchSsidOp *op = new SearchSsidOp(this, ssid);
    op->start();
    return op;
}

AsyncOp *SystemImpl::searchSsidList()
{
	SearchSsidListOp *op = new SearchSsidListOp(this);
	op->start();
	return op;
}

AsyncOp *SystemImpl::connectSsid(const QVariantMap& ssid)
{
    ConnectSsidOp *op = new ConnectSsidOp(this, ssid);
    op->start();
    return op;
}

AsyncOp *SystemImpl::httpGet(const QString& url)
{
    HttpGetOp *op = new HttpGetOp(this, url);
    op->start();
    return op;
}

AsyncOp *SystemImpl::createWlanProfile(const QString& ssid, const QString& password)
{
    CreateWlanProfileOp *op = new CreateWlanProfileOp(this, ssid, password);
    op->start();
    return op;
}

AsyncOp *SystemImpl::connectWlanProfile(const QString& profile, bool reconnect)
{
    ConnectProfileOp *op = new ConnectProfileOp(this, profile, reconnect);
    op->start();
    return op;
}

void SystemImpl::queryNetworkPerf(System::NetworkPerf& perf)
{
    perf.instances.clear();

    QByteArray data;
    queryPerformanceData(QLatin1String("510"), data);

    PERF_OBJECT_TYPE *networkObj = NULL;

    PERF_DATA_BLOCK *dataBlock = reinterpret_cast<PERF_DATA_BLOCK*>(data.data());

    perf.perfCounter = dataBlock->PerfTime.QuadPart;
    perf.perfFreq = dataBlock->PerfFreq.QuadPart;

    PERF_OBJECT_TYPE *dataObj = reinterpret_cast<PERF_OBJECT_TYPE*>(reinterpret_cast<BYTE*>(dataBlock) + dataBlock->HeaderLength);
    for (DWORD i = 0; i < dataBlock->NumObjectTypes; i++)
    {
        if (dataObj->ObjectNameTitleIndex == 510)
        {
            networkObj = dataObj;
            break;
        }
        dataObj = reinterpret_cast<PERF_OBJECT_TYPE*>(reinterpret_cast<BYTE*>(dataObj) + dataObj->TotalByteLength);
    }

    PERF_COUNTER_DEFINITION *cdIncoming = NULL;
    PERF_COUNTER_DEFINITION *cdOutgoing = NULL;

    if (networkObj)
    {
        PERF_COUNTER_DEFINITION *counterDef = reinterpret_cast<PERF_COUNTER_DEFINITION*>(reinterpret_cast<BYTE*>(networkObj) + networkObj->HeaderLength);
        for (DWORD i = 0; i < dataObj->NumCounters; i++)
        {
            counterDef = reinterpret_cast<PERF_COUNTER_DEFINITION*>(reinterpret_cast<BYTE*>(counterDef) + counterDef->ByteLength);
            if (counterDef->CounterNameTitleIndex == 264)
            {
                cdIncoming = counterDef;
            }
            else if (counterDef->CounterNameTitleIndex == 506)
            {
                cdOutgoing = counterDef;
            }

            if (cdIncoming && cdOutgoing)
            {
                break;
            }
        }
    }

    if (cdIncoming && cdOutgoing)
    {
        System::NetworkPerfInstance perfInst;

        PERF_INSTANCE_DEFINITION *instDef = reinterpret_cast<PERF_INSTANCE_DEFINITION*>(reinterpret_cast<BYTE*>(networkObj) + networkObj->DefinitionLength);
        for (LONG i = 0; i < dataObj->NumInstances; i++)
        {
            WCHAR *instName = reinterpret_cast<WCHAR*>(reinterpret_cast<BYTE*>(instDef) + instDef->NameOffset);
            PERF_COUNTER_BLOCK *counterBlock = reinterpret_cast<PERF_COUNTER_BLOCK*>(reinterpret_cast<BYTE*>(instDef) + instDef->ByteLength);

            ULONGLONG incomingBytes;
            void *inPtr = reinterpret_cast<BYTE*>(counterBlock) + cdIncoming->CounterOffset;
            if (cdIncoming->CounterSize == 4)
            {
                incomingBytes = *static_cast<DWORD*>(inPtr);
            }
            else
            {
                incomingBytes = *static_cast<ULONGLONG*>(inPtr);
            }

            ULONGLONG outgoingBytes;
            void *outPtr = reinterpret_cast<BYTE*>(counterBlock) + cdOutgoing->CounterOffset;
            if (cdOutgoing->CounterSize == 4)
            {
                outgoingBytes = *static_cast<DWORD*>(outPtr);
            }
            else
            {
                outgoingBytes = *static_cast<ULONGLONG*>(outPtr);
            }

            perfInst.inBytes = incomingBytes;
            perfInst.outBytes = outgoingBytes;
            perfInst.name = QString::fromWCharArray(instName, (instDef->NameLength / sizeof(WCHAR)) - 1);
            perf.instances.append(perfInst);

            instDef = reinterpret_cast<PERF_INSTANCE_DEFINITION*>(reinterpret_cast<BYTE*>(instDef) + instDef->ByteLength + counterBlock->ByteLength);
        }
    }
}

void SystemImpl::process()
{
    updateWlanState();
}

void SystemImpl::updateWlanState()
{
    DWORD status;
    if (!m_wlanHandle)
    {
        HANDLE wlanHandle;
        DWORD wlanVer;
        status = m_wlanapi->WlanOpenHandle(1, NULL, &wlanVer, &wlanHandle);
        if (status == ERROR_SUCCESS)
        {
            status = m_wlanapi->WlanRegisterNotification(wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL, FALSE, &SystemImpl::__wlanNotificationCallback, this, NULL, NULL);
            if (status == ERROR_SUCCESS)
            {
            }
            else
            {
                LOG_DEBUG(QString::fromUtf8("WlanRegisterNotification failed, err=%1").arg(status));
                m_wlanapi->WlanCloseHandle(wlanHandle, NULL);
            }
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("WlanOpenHandle failed, err=%1").arg(status));
        }

        if (status == ERROR_SERVICE_NOT_ACTIVE)
        {
            if (m_wlanInit)
            {
                m_wlanServiceActive = false;
            }
            else
            {
                if (m_wlanServiceActive)
                {
                    LOG_DEBUG(QString::fromUtf8("service down!"));
                    m_wlanServiceActive = false;
                    emit q_ptr()->wlanServiceActiveChanged();
                }
            }
            return;
        }

        if (status != ERROR_SUCCESS)
        {
            return;
        }

        m_wlanHandle = wlanHandle;
    }

    PWLAN_INTERFACE_INFO_LIST intfList;
    status = m_wlanapi->WlanEnumInterfaces(m_wlanHandle, NULL, &intfList);
    if (status == ERROR_SERVICE_NOT_ACTIVE)
    {
        if (m_wlanInit)
        {
            m_wlanServiceActive = false;
        }
        else
        {
            if (m_wlanServiceActive)
            {
                LOG_DEBUG(QString::fromUtf8("service down!"));
                m_wlanServiceActive = false;
                emit q_ptr()->wlanServiceActiveChanged();
            }
        }
        m_wlanapi->WlanCloseHandle(m_wlanHandle, NULL);
        m_wlanHandle = NULL;
        updatwWlanInterface(NULL);
        return;
    }

    if (status == ERROR_INVALID_HANDLE)
    {
        m_wlanHandle = NULL;
        updatwWlanInterface(NULL);
        return;
    }

    if (status != ERROR_SUCCESS)
    {
        LOG_DEBUG(QString::fromUtf8("WlanEnumInterfaces failed, err=%1").arg(status));
        m_wlanapi->WlanCloseHandle(m_wlanHandle, NULL);
        m_wlanHandle = NULL;
        updatwWlanInterface(NULL);
        return;
    }

    if (intfList->dwNumberOfItems == 1)
    {
        LOG_DEBUG(QString::fromUtf8("Found interface: %1 %2").arg(QUuid(intfList->InterfaceInfo[0].InterfaceGuid), QString::fromWCharArray(intfList->InterfaceInfo[0].strInterfaceDescription)));
        updatwWlanInterface(intfList);
    }
    else if (intfList->dwNumberOfItems > 1)
    {
        LOG_DEBUG(QString::fromUtf8("Found %1 interfaces:").arg(intfList->dwNumberOfItems));
        for (DWORD i = 0; i < intfList->dwNumberOfItems; i++)
        {
            const WLAN_INTERFACE_INFO& info = intfList->InterfaceInfo[i];
            LOG_DEBUG(QString::fromUtf8("%1 %2").arg(QUuid(info.InterfaceGuid), QString::fromWCharArray(info.strInterfaceDescription)));
        }
        updatwWlanInterface(intfList);
    }
    else
    {
        LOG_DEBUG(QString::fromUtf8("No interface!"));
        updatwWlanInterface(NULL);
    }

    m_wlanapi->WlanFreeMemory(intfList);

    if (m_wlanInit)
    {
        m_wlanServiceActive = true;
    }
    else
    {
        if (!m_wlanServiceActive)
        {
            m_wlanServiceActive = true;
            LOG_DEBUG(QString::fromUtf8("service started!"));
            emit q_ptr()->wlanServiceActiveChanged();
        }
    }

    updateWlanRadioState();
}

DWORD SystemImpl::selectBestInterface(PWLAN_INTERFACE_INFO_LIST intfList)
{
    return 0;
}

void SystemImpl::updatwWlanInterface(PWLAN_INTERFACE_INFO_LIST intfList)
{
    if (m_wlanInit)
    {
        if (intfList)
        {
            DWORD index = selectBestInterface(intfList);
            const WLAN_INTERFACE_INFO& info = intfList->InterfaceInfo[index];
            m_wlanInterfaceGuid = info.InterfaceGuid;
            m_wlanInterfaceUuid = m_wlanInterfaceGuid;
            m_wlanInterfaceDescription = QString::fromWCharArray(info.strInterfaceDescription);
            LOG_DEBUG(QString::fromUtf8("Initial select interface: %1 %2").arg(m_wlanInterfaceUuid, m_wlanInterfaceDescription));
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("Initial select no interface"));
        }
    }
    else
    {
        if (intfList)
        {
            if (m_wlanInterfaceUuid.isNull())
            {
                DWORD index = selectBestInterface(intfList);
                const WLAN_INTERFACE_INFO& info = intfList->InterfaceInfo[index];
                m_wlanInterfaceGuid = info.InterfaceGuid;
                m_wlanInterfaceUuid = m_wlanInterfaceGuid;
                m_wlanInterfaceDescription = QString::fromWCharArray(info.strInterfaceDescription);
                LOG_DEBUG(QString::fromUtf8("Select interface: %1 %2").arg(m_wlanInterfaceUuid, m_wlanInterfaceDescription));
                emit q_ptr()->wlanInterfaceChanged();
            }
            else
            {
                bool found = false;
                for (DWORD i = 0; i < intfList->dwNumberOfItems; i++)
                {
                    if (IsEqualGUID(m_wlanInterfaceGuid, intfList->InterfaceInfo[i].InterfaceGuid))
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    DWORD index = selectBestInterface(intfList);
                    const WLAN_INTERFACE_INFO& info = intfList->InterfaceInfo[index];
                    m_wlanInterfaceGuid = info.InterfaceGuid;
                    m_wlanInterfaceUuid = m_wlanInterfaceGuid;
                    m_wlanInterfaceDescription = QString::fromWCharArray(info.strInterfaceDescription);
                    LOG_DEBUG(QString::fromUtf8("Change interface: %1 %2").arg(m_wlanInterfaceUuid, m_wlanInterfaceDescription));
                    emit q_ptr()->wlanInterfaceChanged();
                }
            }
        }
        else
        {
            if (m_wlanInterfaceUuid.isNull())
            {
            }
            else
            {
                m_wlanInterfaceUuid = QUuid();
                m_wlanInterfaceGuid = m_wlanInterfaceUuid;
                m_wlanInterfaceDescription.clear();
                LOG_DEBUG(QString::fromUtf8("Removed interface"));
                emit q_ptr()->wlanInterfaceChanged();
            }
        }
    }
}

void SystemImpl::updateWlanRadioState()
{
    /*	if (!m_wlanInterfaceUuid.isNull()) {
    		WLAN_RADIO_STATE *radioState;
    		DWORD cb;
    		DWORD status = m_wlanapi->WlanQueryInterface(m_wlanHandle, &m_wlanInterfaceGuid, wlan_intf_opcode_radio_state, NULL, &cb, reinterpret_cast<PVOID*>(&radioState), NULL);
    		LOG_DEBUG(QString::fromUtf8("WlanQueryInterface wlan_intf_opcode_radio_state %1").arg(status));
    		if (status == ERROR_SUCCESS) {
    			// ERROR_NOT_SUPPORTED
    			m_wlanapi->WlanFreeMemory(radioState);
    		}
    	}*/
}

void SystemImpl::__wlanNotificationCallback(PWLAN_NOTIFICATION_DATA data, PVOID ctx)
{
    LOG_DEBUG(QString::fromUtf8("wlan notification: %1 %2").arg(data->NotificationSource).arg(data->NotificationCode));
}

QStringList SystemImpl::queryGatewayList()
{
    QStringList ls;
    HMODULE hModule = LoadLibraryW(L"iphlpapi");
    if (!hModule)
    {
        hModule = LoadLibraryW(L"iphlpapi");
    }

    if (!hModule)
    {
        LOG_DEBUG(QString::fromUtf8("iphlpapi not available! err=%1").arg(GetLastError()));
        return ls;
    }

    typedef ULONG (WINAPI * PFN_GetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);
    PFN_GetAdaptersInfo pfnGetAdaptersInfo = reinterpret_cast<PFN_GetAdaptersInfo>(GetProcAddress(hModule, "GetAdaptersInfo"));
    if (!pfnGetAdaptersInfo)
    {
        LOG_DEBUG(QString::fromUtf8("iphlpapi [GetAdaptersInfo] not available! err=%1").arg(GetLastError()));
        return ls;
    }

    QByteArray buffer(sizeof(IP_ADAPTER_INFO) * 40, 0);
    IP_ADAPTER_INFO *infoArray = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());
    ULONG cb = buffer.size();
    DWORD err = pfnGetAdaptersInfo(infoArray, &cb);
    if (err != ERROR_SUCCESS)
    {
        LOG_DEBUG(QString::fromUtf8("GetAdaptersInfo failed, err=%1").arg(err));
        return ls;
    }

    LOG_DEBUG(QString::fromUtf8("All gateway list:"));

    IP_ADAPTER_INFO *info = infoArray;
    while (info)
    {
        if (strcmp(info->GatewayList.IpAddress.String, "0.0.0.0") != 0 && strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
        {
            LOG_DEBUG(QString::fromUtf8("\t[%1] [%2] [%3]").arg(QString::fromUtf8(info->AdapterName), QString::fromLocal8Bit(info->Description), QString::fromUtf8(info->GatewayList.IpAddress.String)));
            ls.append(QString::fromLatin1(info->GatewayList.IpAddress.String));
        }
        info = info->Next;
    }
    return ls;
}

bool SystemImpl::checkRouter(const QString& mac)
{
    QByteArray dd = QByteArray::fromHex(mac.toUtf8());
    if (dd.length() != 6)
    {
        return true;
    }
    LOG_DEBUG(QString::fromUtf8("mac length  = %1").arg(dd.length()));
    HMODULE hModule = GetModuleHandleW(L"iphlpapi");
    if (!hModule)
    {
        hModule = LoadLibraryW(L"iphlpapi");
        LOG_DEBUG(QString::fromUtf8("LoadLibraryW  iphlpapi"));
        return true;
    }

    typedef DWORD (WINAPI *PFN_GetIpNetTable)(PMIB_IPNETTABLE, PULONG, BOOL);
    PFN_GetIpNetTable pfnGetIpNetTable = reinterpret_cast<PFN_GetIpNetTable>(GetProcAddress(hModule, "GetIpNetTable"));
    if (!pfnGetIpNetTable)
    {
        LOG_DEBUG(QString::fromUtf8("no pfnGetIpNetTable"));
        return true;
    }

    QVector<uchar> buffer(2048);
    PMIB_IPNETTABLE pNetTable = reinterpret_cast<PMIB_IPNETTABLE>(&buffer[0]);


    DWORD cb = buffer.size();
    DWORD err = (*pfnGetIpNetTable)(pNetTable, &cb, TRUE);
    if (err == ERROR_INSUFFICIENT_BUFFER)
    {
        buffer.resize(cb);
        pNetTable = reinterpret_cast<PMIB_IPNETTABLE>(&buffer[0]);
        err = (*pfnGetIpNetTable)(pNetTable, &cb, TRUE);
    }
    if (err != ERROR_SUCCESS)
    {
        LOG_DEBUG(QString::fromUtf8("pNetTables Error"));
        return true;
    }
    //in_addr ia;
    //ia.S_un.S_addr = row->dwAddr;
    //QString ss=QString(QLatin1String(inet_ntoa(ia)));
    //LOG_DEBUG(QString::fromUtf8("no pfnGetIpNetTable= %1 ").arg(ss));
    //delete pNetTable;
    for (DWORD i = 0; i < pNetTable->dwNumEntries; i++)
    {
        MIB_IPNETROW *row = pNetTable->table + i;
        if (row->dwPhysAddrLen == 6 && memcmp(row->bPhysAddr, dd.data(), 6) == 0)
        {
            return true;
        }
    }
    return false;
}
bool SystemImpl::checkRouter2(const QString& ssidName)
{

    WlanLib *wlanapi = this->m_wlanapi;
    DWORD err;
    DWORD ver;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES connInfo = NULL;
    err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
    if (err != ERROR_SUCCESS)
    {
        return false;
    }
    err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
    if (err != ERROR_SUCCESS)
    {
        return false;
    }
    if(intfList->dwNumberOfItems==0)
    {
        return false;
    }
    //DWORD index = m_system->selectBestInterface(intfList);
    const GUID *intfGuid = &intfList->InterfaceInfo[0].InterfaceGuid;
    DWORD cb;
    WLAN_OPCODE_VALUE_TYPE valueType;
    err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
    if(err == ERROR_SUCCESS)
    {
        if (connInfo->isState == wlan_interface_state_connected && connInfo->wlanConnectionMode == wlan_connection_mode_profile)
        {
            QString SsidName=ssidName;
            LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(SsidName));
            LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(QString::fromWCharArray(connInfo->strProfileName)));
            if(SsidName.compare(QString::fromWCharArray(connInfo->strProfileName))==0)
            {
                LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(SsidName));
                return true;
            }
        }
    }

    return false;
}


QuitDog::QuitDog(SystemImpl *system, const WCHAR *eventName)
    : m_system(system), m_thread(this)
{
    m_hQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hGlobalQuitEvent = CreateSharedEvent(eventName);
    m_hIpChanged = CreateEvent(NULL, FALSE, FALSE, NULL);

    m_pfnNotifyAddrChange = NULL;
    m_pfnCancelIPChangeNotify = NULL;
    HMODULE hModule = LoadLibraryW(L"iphlpapi");
    if (hModule)
    {
        m_pfnNotifyAddrChange = reinterpret_cast<PFN_NotifyAddrChange>(GetProcAddress(hModule, "NotifyAddrChange"));
        m_pfnCancelIPChangeNotify = reinterpret_cast<PFN_CancelIPChangeNotify>(GetProcAddress(hModule, "CancelIPChangeNotify"));
    }

    setupNetworkChangeNotify();
    m_thread.start();
}

QuitDog::~QuitDog()
{
    SetEvent(m_hQuitEvent);
    m_thread.wait();
    if (m_pfnCancelIPChangeNotify)
    {
        (*m_pfnCancelIPChangeNotify)(&m_overlapped);
    }
    CloseHandle(m_hGlobalQuitEvent);
    CloseHandle(m_hQuitEvent);
    CloseHandle(m_hIpChanged);
}

void QuitDog::dogRun()
{
    HANDLE handles[] = { m_hQuitEvent, m_hGlobalQuitEvent, m_hIpChanged };
    bool loopFlag = true;
    while (loopFlag)
    {
        DWORD dwWait = WaitForMultipleObjects(3, handles, FALSE, INFINITE);
        switch (dwWait)
        {
        case WAIT_OBJECT_0:
            LOG_DEBUG(QString::fromUtf8("WAIT_OBJECT_0"));
            loopFlag = false;
            break;
        case WAIT_OBJECT_0 + 1:
            QMetaObject::invokeMethod(m_system->q_ptr(), "quitNow", Qt::QueuedConnection);
            loopFlag = false;
            break;
        case WAIT_OBJECT_0 + 2:
            LOG_DEBUG(QString::fromUtf8("WAIT_OBJECT_0 + 2"));
            QMetaObject::invokeMethod(this, "onAddrChanged", Qt::QueuedConnection);
            break;
        default:
            LOG_DEBUG(QString::fromUtf8("default"));
            loopFlag = false;
            break;
        }
    }
}

void QuitDog::setupNetworkChangeNotify()
{
    if (m_pfnNotifyAddrChange)
    {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_overlapped.hEvent = m_hIpChanged;
        (*m_pfnNotifyAddrChange)(&m_handle, &m_overlapped);
    }
}

bool QuitDog::wlanConnectedStatus()
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    DWORD err;
    DWORD ver;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES connInfo = NULL;
    err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
    if (err != ERROR_SUCCESS)
    {
        return false;
    }
    err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
    if (err != ERROR_SUCCESS)
    {
        return false;
    }
    if(intfList->dwNumberOfItems==0)
    {
        return false;
    }
    DWORD index = m_system->selectBestInterface(intfList);
    const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
    DWORD cb;
    WLAN_OPCODE_VALUE_TYPE valueType;
    err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
    if(err == ERROR_SUCCESS)
    {
        if (connInfo->isState == wlan_interface_state_connected && connInfo->wlanConnectionMode == wlan_connection_mode_profile)
        {
            QString ssidName=m_system->m_wlanSsid;
            LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(ssidName));
            LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(QString::fromWCharArray(connInfo->strProfileName)));
            if(ssidName.compare(QString::fromWCharArray(connInfo->strProfileName))==0)
            {
                LOG_DEBUG(QString::fromUtf8("ssidName = %1").arg(ssidName));
                return true;
            }
        }
    }

    return false;
}

void QuitDog::onAddrChanged()
{
//    if(wlanConnectedStatus())
//    {
//        return;
//    }
    DWORD cb;
    LOG_DEBUG(QString::fromUtf8("onAddrChanged"));
    GetOverlappedResult(m_handle, &m_overlapped, &cb, TRUE);
    QMetaObject::invokeMethod(m_system->q_ptr(), "networkConnectionChanged");
    setupNetworkChangeNotify();
}

QuitDog::DogThread::DogThread(QuitDog *owner)
    : m_owner(owner)
{
}

void QuitDog::DogThread::run()
{
    m_owner->dogRun();
}

ThreadedAsyncOp::ThreadedAsyncOp(QObject *parent)
    : AsyncOp(parent)
{
    m_workerThread = new WorkerThread(this);
}

ThreadedAsyncOp::~ThreadedAsyncOp()
{
    m_workerThread->wait();
    delete m_workerThread;
}

void ThreadedAsyncOp::start()
{
    m_workerThread->start();
}

int ThreadedAsyncOp::process(QVariantMap& result)
{
    return NoError;
}

void ThreadedAsyncOp::onWorkerThreadFinished(int status, const QVariantMap& result)
{
    setValues(result);
    notifyFinished(status);
}

ThreadedAsyncOp::WorkerThread::WorkerThread(ThreadedAsyncOp *owner)
    : m_owner(owner)
{
}

void ThreadedAsyncOp::WorkerThread::run()
{
    QVariantMap varMap;
    int status = m_owner->process(varMap);
    QMetaObject::invokeMethod(m_owner, "onWorkerThreadFinished", Qt::QueuedConnection, Q_ARG(int, status), Q_ARG(QVariantMap, varMap));
}

StartWlanServiceOp::StartWlanServiceOp(const QString& appFilePath, const QString& appDirPath, bool needsElevation)
    : m_appFilePath(appFilePath), m_appDirPath(appDirPath), m_needsElevation(needsElevation)
{
    m_quitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

StartWlanServiceOp::~StartWlanServiceOp()
{
    if (m_workerThread)
    {
        m_workerThread->wait();
        delete m_workerThread;
    }
    CloseHandle(m_quitEvent);
}

void StartWlanServiceOp::start()
{
    m_workerThread = new WorkerThread(this);
    m_workerThread->start();
}

void StartWlanServiceOp::onAbort()
{
    SetEvent(m_quitEvent);
}

void StartWlanServiceOp::workerThreadFinished()
{
    if (!isAborted())
    {
        notifyFinished(m_workSucceeded ? NoError : UnknownError);
    }
}

void StartWlanServiceOp::workerThreadProc()
{
    WCHAR appPath[MAX_PATH];
    WCHAR dirPath[MAX_PATH];

    int len = QDir::toNativeSeparators(m_appFilePath).toWCharArray(appPath);
    appPath[len] = 0;
    len = QDir::toNativeSeparators(m_appDirPath).toWCharArray(dirPath);
    dirPath[len] = 0;

    SHELLEXECUTEINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = m_needsElevation ? L"runas" : L"open";
    info.lpFile = appPath;
    info.lpDirectory = dirPath;
    info.lpParameters = L"-cmd=startWlanService";
    info.nShow = SW_SHOW;
    BOOL b = ShellExecuteEx(&info);
    LOG_DEBUG(QString::fromUtf8("ShellExecuteEx %1 %2").arg(b).arg(GetLastError()));
    if (b)
    {
        if (info.hProcess)
        {
            HANDLE handles[] = { m_quitEvent, info.hProcess };
            if (WAIT_OBJECT_0 + 1 == WaitForMultipleObjects(2, handles, FALSE, INFINITE))
            {
                DWORD code = 0;
                b = GetExitCodeProcess(info.hProcess, &code);
                LOG_DEBUG(QString::fromUtf8("GetExitCodeProcess %1 %2 %3").arg(b).arg(GetLastError()).arg(code));
            }
            CloseHandle(info.hProcess);
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("Seem process failed!"));
        }
        m_workSucceeded = true;
    }
    else
    {
        m_workSucceeded = false;
    }

    QMetaObject::invokeMethod(this, "workerThreadFinished", Qt::QueuedConnection);
}

StartWlanServiceOp::WorkerThread::WorkerThread(StartWlanServiceOp *owner)
    : m_owner(owner)
{
}

void StartWlanServiceOp::WorkerThread::run()
{
    m_owner->workerThreadProc();
}

static int translateWin32Error(DWORD err)
{
    int status;
    switch (err)
    {
    case ERROR_SERVICE_NOT_ACTIVE:
        status = AsyncOp::WlanServiceDownError;
        break;
    case 0x80342002L: //ERROR_NDIS_DOT11_POWER_STATE_INVALID
        status = AsyncOp::WlanRadioOffError;
        break;
//	case ERROR_INVALID_STATE:
//		status = AsyncOp::UnknownError;
//		break;
    default:
        status = AsyncOp::UnknownError;
        break;
    }
    return status;
}

SearchSsidOp::SearchSsidOp(SystemImpl *system, const QString& ssid)
    : m_system(system), m_ssid(ssid)
{
    m_tryCount = 0;
}

void SearchSsidOp::onAbort()
{
}

int SearchSsidOp::process(QVariantMap& result)
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    int status;
    bool foundSSID = false;

    DWORD err;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_AVAILABLE_NETWORK_LIST networkList = NULL;
    PWLAN_BSS_LIST bssList = NULL;

    do
    {
        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD ver;
        err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        LOG_INFO(QString::fromUtf8("Wlan version %1").arg(ver));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        if (intfList->dwNumberOfItems == 0)
        {
            status = WlanNoDeviceError;
            break;
        }

        DWORD index = m_system->selectBestInterface(intfList);
        const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
        LOG_INFO(QString::fromUtf8("Use interface %1 %2").arg(QUuid(*intfGuid).toString()).arg(QString::fromWCharArray(intfList->InterfaceInfo[index].strInterfaceDescription)));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        QByteArray targetSsid = m_ssid.toUtf8();
        QVariantList ls;

        if (m_system->m_osver.dwMajorVersion > 5 && wlanapi->WlanGetNetworkBssList)
        {
            LOG_INFO(QString::fromUtf8("Use WlanGetNetworkBssList"));
            err = wlanapi->WlanGetNetworkBssList(wlanHandle, intfGuid, NULL, dot11_BSS_type_infrastructure, FALSE, NULL, &bssList);
            if (err != ERROR_SUCCESS)
            {
                status = translateWin32Error(err);
                break;
            }

            for (DWORD i = 0; i < bssList->dwNumberOfItems; i++)
            {
                PWLAN_BSS_ENTRY bssEntry = bssList->wlanBssEntries + i;
                QByteArray ssid(reinterpret_cast<char*>(bssEntry->dot11Ssid.ucSSID), bssEntry->dot11Ssid.uSSIDLength);
                if (ssid == targetSsid)
                {
                    QVariantMap varEntry;
                    varEntry.insert(QString::fromUtf8("ssid"), m_ssid);
                    varEntry.insert(QString::fromUtf8("mac"), QByteArray(reinterpret_cast<char*>(bssEntry->dot11Bssid), 6));
                    ls.push_back(varEntry);
                    foundSSID = true;
                    LOG_INFO(QString::fromUtf8("found ssid : %1 for connect set").arg(QString::fromLocal8Bit(ssid.data())));
                    break;
                }
            }

        }
        else
        {
            LOG_INFO(QString::fromUtf8("Use WlanGetAvailableNetworkList"));
            err = wlanapi->WlanGetAvailableNetworkList(wlanHandle, intfGuid, 0, NULL, &networkList);
            if (err != ERROR_SUCCESS)
            {
                status = translateWin32Error(err);
                break;
            }

            for (DWORD i = 0; i < networkList->dwNumberOfItems; i++)
            {
                PWLAN_AVAILABLE_NETWORK network = networkList->Network + i;
                QByteArray ssid(reinterpret_cast<char*>(network->dot11Ssid.ucSSID), network->dot11Ssid.uSSIDLength);
                if (ssid == targetSsid)
                {
                    QVariantMap varEntry;
                    varEntry.insert(QString::fromUtf8("ssid"), m_ssid);
                    varEntry.insert(QString::fromUtf8("mac"), QByteArray(6, 0));
                    ls.push_back(varEntry);
                    foundSSID = true;
                    break;
                }
            }
        }

        result.insert(QString::fromUtf8("ssidList"), ls);
        status = NoError;
    }
    while (false);

    if (bssList)
    {
        wlanapi->WlanFreeMemory(bssList);
    }

    if (networkList)
    {
        wlanapi->WlanFreeMemory(networkList);
    }

    if (intfList)
    {
        wlanapi->WlanFreeMemory(intfList);
    }

    if (wlanHandle)
    {
        wlanapi->WlanCloseHandle(wlanHandle, NULL);
    }
    if(foundSSID)
        return status;
    else
    {
        if(m_tryCount > 4)
            return AsyncOp::NOFound;
        else
        {
            m_tryCount++;
            QTest::qSleep(5000);
            return process(result);
        }
    }
}
SearchSsidListOp::SearchSsidListOp(SystemImpl *system)
    : m_system(system)
{

}

void SearchSsidListOp::onAbort()
{
}

int SearchSsidListOp::process(QVariantMap& result)
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    int status;

    DWORD err;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_AVAILABLE_NETWORK_LIST networkList = NULL;
    PWLAN_BSS_LIST bssList = NULL;
    do
    {
        if (isAborted())
        {
            status = AbortedError;
            break;
        }
        DWORD ver;
        err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }
        LOG_INFO(QString::fromUtf8("Wlan version %1").arg(ver));
        if (isAborted())
        {
            status = AbortedError;
            break;
        }
        err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        if (intfList->dwNumberOfItems == 0)
        {
            status = WlanNoDeviceError;
            break;
        }

        DWORD index = m_system->selectBestInterface(intfList);
        const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
        LOG_INFO(QString::fromUtf8("Use interface %1 %2").arg(QUuid(*intfGuid).toString()).arg(QString::fromWCharArray(intfList->InterfaceInfo[index].strInterfaceDescription)));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        QVariantList ls;

        if (m_system->m_osver.dwMajorVersion > 5 && wlanapi->WlanGetNetworkBssList)
        {
            LOG_INFO(QString::fromUtf8("Use WlanGetNetworkBssList"));
            err = wlanapi->WlanGetNetworkBssList(wlanHandle, intfGuid, NULL, dot11_BSS_type_infrastructure, FALSE, NULL, &bssList);
            if (err != ERROR_SUCCESS)
            {
                status = translateWin32Error(err);
                break;
            }

            for (DWORD i = 0; i < bssList->dwNumberOfItems; i++)
            {
                PWLAN_BSS_ENTRY bssEntry = bssList->wlanBssEntries + i;
                QByteArray ssid(reinterpret_cast<char*>(bssEntry->dot11Ssid.ucSSID), bssEntry->dot11Ssid.uSSIDLength);
                QVariantMap varEntry;
                varEntry.insert(QString::fromUtf8("ssid"), ssid);
                varEntry.insert(QString::fromUtf8("mac"), QByteArray(reinterpret_cast<char*>(bssEntry->dot11Bssid), 6));
                ls.push_back(varEntry);
               // LOG_INFO(QString::fromUtf8("found ssid : %1 for connect set").arg(QString::fromLocal8Bit(ssid.data())));
            }
        }
        else
        {
            LOG_INFO(QString::fromUtf8("Use WlanGetAvailableNetworkList"));
            err = wlanapi->WlanGetAvailableNetworkList(wlanHandle, intfGuid, 0, NULL, &networkList);
            if (err != ERROR_SUCCESS)
            {
                status = translateWin32Error(err);
                break;
            }
            for (DWORD i = 0; i < networkList->dwNumberOfItems; i++)
            {
                PWLAN_AVAILABLE_NETWORK network = networkList->Network + i;
                QByteArray ssid(reinterpret_cast<char*>(network->dot11Ssid.ucSSID), network->dot11Ssid.uSSIDLength);
                QVariantMap varEntry;
                varEntry.insert(QString::fromUtf8("ssid"), ssid);
                varEntry.insert(QString::fromUtf8("mac"), QByteArray(6, 0));
                ls.push_back(varEntry);
            }
        }
        result.insert(QString::fromUtf8("ssidList"), ls);
        status = NoError;
    }
    while (false);
    if (bssList)
    {
        wlanapi->WlanFreeMemory(bssList);
    }
    if (networkList)
    {
        wlanapi->WlanFreeMemory(networkList);
    }

    if (intfList)
    {
        wlanapi->WlanFreeMemory(intfList);
    }

    if (wlanHandle)
    {
        wlanapi->WlanCloseHandle(wlanHandle, NULL);
    }
    return status;
}

ConnectSsidOp::ConnectSsidOp(SystemImpl *system, const QVariantMap& ssid)
    : m_system(system), m_ssid(ssid)
{
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

ConnectSsidOp::~ConnectSsidOp()
{
    CloseHandle(m_hEvent);
    CloseHandle(m_hAbortEvent);
}

void ConnectSsidOp::onAbort()
{
    SetEvent(m_hAbortEvent);
}

int ConnectSsidOp::process(QVariantMap& result)
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    int status;

    DWORD err;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES connInfo = NULL;

    do
    {
        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD ver;
        err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        LOG_INFO(QString::fromUtf8("Wlan version %1").arg(ver));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        if (intfList->dwNumberOfItems == 0)
        {
            status = WlanNoDeviceError;
            break;
        }

        DWORD index = m_system->selectBestInterface(intfList);
        const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
        LOG_INFO(QString::fromUtf8("Use interface %1 %2").arg(QUuid(*intfGuid).toString()).arg(QString::fromWCharArray(intfList->InterfaceInfo[index].strInterfaceDescription)));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        err = wlanapi->WlanRegisterNotification(wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL, FALSE, &ConnectSsidOp::wlanNotify, this, NULL, NULL);
        if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanRegisterNotification failed, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }

        QString sSsid = m_ssid.value(QString::fromUtf8("ssid")).toString();
        QByteArray reqSsid = sSsid.toUtf8();
        QByteArray reqMac = m_ssid.value(QString::fromUtf8("mac")).toByteArray();

        DOT11_SSID dot11Ssid;
        dot11Ssid.uSSIDLength = reqSsid.length();
        memcpy(dot11Ssid.ucSSID, reqSsid.data(), dot11Ssid.uSSIDLength);

        DOT11_BSSID_LIST bssList;

        m_wlanReasonCode = -1;
        if (m_system->m_osver.dwMajorVersion > 5)
        {
            WLAN_CONNECTION_PARAMETERS conn;
            conn.wlanConnectionMode = wlan_connection_mode_discovery_unsecure;
            conn.strProfile = NULL;
            conn.pDot11Ssid = &dot11Ssid;
            conn.pDesiredBssidList = NULL;
            conn.dot11BssType = dot11_BSS_type_infrastructure;
            conn.dwFlags = 0;

            if (reqMac.length() == 6 && reqMac != QByteArray(6, 0))
            {
                bssList.Header.Size = sizeof(bssList);
                bssList.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                bssList.Header.Revision = DOT11_BSSID_LIST_REVISION_1;
                bssList.uNumOfEntries = 1;
                bssList.uTotalNumOfEntries = 1;
                const quint8 *macData = reinterpret_cast<const quint8*>(reqMac.data());
                for (int i = 0; i < 6; i++)
                {
                    bssList.BSSIDs[0][i] = macData[i];
                }
                conn.pDesiredBssidList = &bssList;
            }

            err = wlanapi->WlanConnect(wlanHandle, intfGuid, &conn, NULL);

            if (err != ERROR_SUCCESS)
            {
                LOG_DEBUG(QString::fromUtf8("WlanConnect failed, err=%1").arg(err));
                status = translateWin32Error(err);
                break;
            }
        }
        else
        {
            const char *tmpl = "<?xml version=\"1.0\" encoding=\"US-ASCII\"?>\r\n<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\r\n<name>%2</name>\r\n<SSIDConfig>\r\n<SSID>\r\n<name>%1</name>\r\n</SSID>\r\n</SSIDConfig>\r\n<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>%3</security></MSM></WLANProfile>";
            QString security = QString::fromLatin1("<authEncryption><authentication>open</authentication><encryption>none</encryption><useOneX>false</useOneX></authEncryption>");
            QString s = QString::fromLatin1(tmpl).arg(sSsid).arg(sSsid).arg(security);
            WCHAR *xml = new WCHAR[s.length() + 20];
            int len = s.toWCharArray(xml);
            xml[len] = 0;
            WLAN_REASON_CODE reason;
            err = wlanapi->WlanSetProfile(wlanHandle, intfGuid, 0, xml, NULL, TRUE, NULL, &reason);
            delete[] xml;

            if (err != ERROR_SUCCESS)
            {
                LOG_DEBUG(QString::fromUtf8("WlanSetProfile failed, err=%1").arg(err));
                status = translateWin32Error(err);
                break;
            }

            WLAN_CONNECTION_PARAMETERS conn;
            conn.wlanConnectionMode = wlan_connection_mode_profile;
            conn.strProfile = reinterpret_cast<LPCWSTR>(sSsid.data());
            conn.pDot11Ssid = NULL;
            conn.pDesiredBssidList = NULL;
            conn.dot11BssType = dot11_BSS_type_infrastructure;
            conn.dwFlags = 0;

            err = wlanapi->WlanConnect(wlanHandle, intfGuid, &conn, NULL);

            if (err != ERROR_SUCCESS)
            {
                LOG_DEBUG(QString::fromUtf8("WlanConnect failed, err=%1").arg(err));
                status = translateWin32Error(err);
                break;
            }
        }

        for (;;)
        {
            HANDLE handles[] = { m_hAbortEvent, m_hEvent };
            DWORD dwWait = WaitForMultipleObjects(2, handles, FALSE, 10000);
            LOG_DEBUG(QString::fromUtf8("WaitForMultipleObjects result %1").arg(dwWait));
            if (dwWait == WAIT_OBJECT_0)
            {
                break;
            }
            else if (dwWait == WAIT_OBJECT_0 + 1)
            {
                WCHAR reason[1024];
                memset(reason, 0, sizeof(reason));
                DWORD dd = wlanapi->WlanReasonCodeToString(m_wlanReasonCode, 1023, reason, NULL);
                LOG_DEBUG(QString::fromUtf8("connect ssid reason: %1 %2 [%3]").arg(m_wlanReasonCode).arg(QString::fromWCharArray(reason)).arg(dd));
                break;
            }
            else if (dwWait == WAIT_TIMEOUT)
            {
                break;
            }
            else
            {
                LOG_DEBUG(QString::fromUtf8("WaitForMultipleObjects unexpected result %1").arg(dwWait));
                break;
            }
        }

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD cb;
        WLAN_OPCODE_VALUE_TYPE valueType;
        err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
        if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanQueryInterface failed 1, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }
        /*
        LOG_DEBUG(QString::fromUtf8("conn state: %1").arg(connInfo->isState));
        if (connInfo->isState != wlan_interface_state_connected) {
        	// TODO:
        	LOG_WARNING(QString::fromUtf8("state not match"));
        	status = UnknownError;
        	break;
        }
        */
        if (connInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength != dot11Ssid.uSSIDLength
                || memcmp(connInfo->wlanAssociationAttributes.dot11Ssid.ucSSID, dot11Ssid.ucSSID, dot11Ssid.uSSIDLength) != 0)
        {
            // TODO:
            LOG_WARNING(QString::fromUtf8("ssid not match"));
            status = UnknownError;
            break;
        }

        if (m_system->m_osver.dwMajorVersion > 5 && reqMac.length() == 6 && reqMac != QByteArray(6, 0))
        {
            if (memcmp(connInfo->wlanAssociationAttributes.dot11Bssid, reqMac.data(), 6) != 0)
            {
                // TODO:
                LOG_WARNING(QString::fromUtf8("mac not match"));
                status = UnknownError;
                break;
            }
        }

        QByteArray b1(reinterpret_cast<char*>(connInfo->wlanAssociationAttributes.dot11Bssid), 6);
        QString macText = QString::fromUtf8(b1.toHex()).toUpper();
        LOG_DEBUG(QString::fromUtf8("connect ssid mac: %1").arg(macText));
        result.insert(QLatin1String("mac"), macText);

        status = NoError;
    }
    while (false);

    if (connInfo)
    {
        wlanapi->WlanFreeMemory(connInfo);
    }

    if (intfList)
    {
        wlanapi->WlanFreeMemory(intfList);
    }

    if (wlanHandle)
    {
        wlanapi->WlanCloseHandle(wlanHandle, NULL);
    }

    return status;
}

void ConnectSsidOp::wlanNotify(PWLAN_NOTIFICATION_DATA data, PVOID context)
{
    ConnectSsidOp *that = static_cast<ConnectSsidOp*>(context);
    /*if (data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM && data->NotificationCode == wlan_notification_acm_connection_complete) {
    	PWLAN_CONNECTION_NOTIFICATION_DATA connData = static_cast<PWLAN_CONNECTION_NOTIFICATION_DATA>(data->pData);
    	SetEvent(that->m_hEvent);
    }*/
    if (data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM)
    {
        if (data->NotificationCode == wlan_notification_acm_connection_complete ||
                data->NotificationCode == wlan_notification_acm_disconnected)
        {
            PWLAN_CONNECTION_NOTIFICATION_DATA connData = static_cast<PWLAN_CONNECTION_NOTIFICATION_DATA>(data->pData);
            LOG_DEBUG(QString::fromUtf8("wlanNotify A %1 %2").arg(data->NotificationCode).arg(connData->wlanReasonCode));
            if (data->NotificationCode == wlan_notification_acm_connection_complete)
            {
                that->m_wlanReasonCode = connData->wlanReasonCode;
                LOG_DEBUG(QString::fromUtf8("signal connection complete OK!"));
                SetEvent(that->m_hEvent);
            }
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("wlanNotify B %1").arg(data->NotificationCode));
        }
    }
}

HttpGetOp::HttpGetOp(SystemImpl *system, const QString& url)
    : m_system(system), m_url(url), m_reply(NULL)
{
}

HttpGetOp::~HttpGetOp()
{
    if (m_reply)
    {
        m_reply->abort();
        delete m_reply;
    }
}

void HttpGetOp::start()
{
    QUrl url(m_url);
    QNetworkRequest req(url);
    m_reply = m_system->m_nam->get(req);
    connect(m_reply, SIGNAL(finished()), SLOT(onReplyFinished()));
}

void HttpGetOp::onAbort()
{
    if (m_reply)
    {
        m_reply->abort();
    }
}

void HttpGetOp::onReplyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply->deleteLater();
    m_reply = NULL;

    if (isAborted())
    {
        return;
    }

    if (reply->error() == QNetworkReply::NoError)
    {
        notifyFinished(NoError);
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid())
    {
        notifyFinished(NoError);
    }
    else
    {
        notifyFinished(NetworkError);
    }
}

CreateWlanProfileOp::CreateWlanProfileOp(SystemImpl *system, const QString& ssid, const QString& password)
    : m_system(system), m_ssid(ssid), m_password(password)
{
}

CreateWlanProfileOp::~CreateWlanProfileOp()
{
}

void CreateWlanProfileOp::onAbort()
{
}

int CreateWlanProfileOp::process(QVariantMap& result)
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    int status;

    DWORD err;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;

    do
    {
        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD ver;
        err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        LOG_INFO(QString::fromUtf8("Wlan version %1").arg(ver));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        if (intfList->dwNumberOfItems == 0)
        {
            status = WlanNoDeviceError;
            break;
        }

        DWORD index = m_system->selectBestInterface(intfList);
        const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
        LOG_INFO(QString::fromUtf8("Use interface %1 %2").arg(QUuid(*intfGuid).toString()).arg(QString::fromWCharArray(intfList->InterfaceInfo[index].strInterfaceDescription)));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        const char *tmpl = "<?xml version=\"1.0\" encoding=\"US-ASCII\"?>\r\n<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\r\n<name>%2</name>\r\n<SSIDConfig>\r\n<SSID>\r\n<name>%1</name>\r\n</SSID>\r\n</SSIDConfig>\r\n<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>%3</security></MSM></WLANProfile>";
        QString security;
        if (!m_password.isEmpty())
        {
            security = QString::fromLatin1("<authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption><sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>%1</keyMaterial></sharedKey>").arg(m_password);
        }
        else
        {
            security = QString::fromLatin1("<authEncryption><authentication>open</authentication><encryption>none</encryption><useOneX>false</useOneX></authEncryption>");
        }
        QString s = QString::fromLatin1(tmpl).arg(m_ssid).arg(m_ssid).arg(security);
        WCHAR *xml = new WCHAR[s.length() + 20];
        int len = s.toWCharArray(xml);
        xml[len] = 0;
        WLAN_REASON_CODE reason;
        err = wlanapi->WlanSetProfile(wlanHandle, intfGuid, 0, xml, NULL, TRUE, NULL, &reason);
        delete[] xml;

        if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanSetProfile failed, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }

        status = NoError;
    }
    while (false);

    if (intfList)
    {
        wlanapi->WlanFreeMemory(intfList);
    }

    if (wlanHandle)
    {
        wlanapi->WlanCloseHandle(wlanHandle, NULL);
    }

    return status;
}

ConnectProfileOp::ConnectProfileOp(SystemImpl *system, const QString& profile, bool reconnect)
    : m_system(system), m_profile(profile), m_reconnect(reconnect)
{
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

ConnectProfileOp::~ConnectProfileOp()
{
    CloseHandle(m_hEvent);
    CloseHandle(m_hAbortEvent);
}

void ConnectProfileOp::onAbort()
{
    SetEvent(m_hAbortEvent);
}

int ConnectProfileOp::process(QVariantMap& result)
{
    WlanLib *wlanapi = m_system->m_wlanapi;
    int status;

    DWORD err;
    HANDLE wlanHandle = NULL;
    PWLAN_INTERFACE_INFO_LIST intfList = NULL;
    PWLAN_CONNECTION_ATTRIBUTES connInfo = NULL;

    do
    {
        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD ver;
        err = wlanapi->WlanOpenHandle(1, NULL, &ver, &wlanHandle);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        LOG_INFO(QString::fromUtf8("Wlan version %1").arg(ver));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        err = wlanapi->WlanEnumInterfaces(wlanHandle, NULL, &intfList);
        if (err != ERROR_SUCCESS)
        {
            status = translateWin32Error(err);
            break;
        }

        if (intfList->dwNumberOfItems == 0)
        {
            status = WlanNoDeviceError;
            break;
        }

        DWORD index = m_system->selectBestInterface(intfList);
        const GUID *intfGuid = &intfList->InterfaceInfo[index].InterfaceGuid;
        LOG_INFO(QString::fromUtf8("Use interface %1 %2").arg(QUuid(*intfGuid).toString()).arg(QString::fromWCharArray(intfList->InterfaceInfo[index].strInterfaceDescription)));

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

        DWORD cb;
        WLAN_OPCODE_VALUE_TYPE valueType;
        if (!m_reconnect)
        {
            err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
            if (err == ERROR_SUCCESS)
            {
                if (connInfo->isState == wlan_interface_state_connected && connInfo->wlanConnectionMode == wlan_connection_mode_profile)
                {
                    if (m_profile.compare(QString::fromWCharArray(connInfo->strProfileName)) == 0)
                    {
                        LOG_DEBUG(QString::fromUtf8("already connected to %1, nothing to do").arg(m_profile));
                        QMetaObject::invokeMethod(m_system->q_ptr(), "systemwakeup", Qt::QueuedConnection);//sleepp
                        status = NoError;
                        break;
                    }
                }
            }
        }

        LPWSTR profileXml;
        err = wlanapi->WlanGetProfile(wlanHandle, intfGuid, reinterpret_cast<LPCWSTR>(m_profile.data()), NULL, &profileXml, NULL, NULL);
        if (err == ERROR_SUCCESS)
        {
            wlanapi->WlanFreeMemory(profileXml);
        }
        else if (err == ERROR_NOT_FOUND)
        {
            LOG_DEBUG(QString::fromUtf8("WlanGetProfile failed, err=ERROR_NOT_FOUND"));
            status = WlanProfileNotFound;
            break;
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("WlanGetProfile failed, err=%1").arg(err));
        }

        err = wlanapi->WlanRegisterNotification(wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL, FALSE, &ConnectProfileOp::wlanNotify, this, NULL, NULL);
        if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanRegisterNotification failed, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }

        bool fLoop = true;
        int retryCount = 0;
        int maxRetryCount = 0;
        while (fLoop)
        {
            WLAN_CONNECTION_PARAMETERS conn;
            conn.wlanConnectionMode = wlan_connection_mode_profile;
            conn.strProfile = reinterpret_cast<LPCWSTR>(m_profile.data());
            conn.pDot11Ssid = NULL;
            conn.pDesiredBssidList = NULL;
            conn.dot11BssType = dot11_BSS_type_infrastructure;
            conn.dwFlags = 0;

            m_wlanReasonCode = -1;
            LOG_DEBUG(QString::fromUtf8("now WlanConnect %1 [%2]").arg(m_profile).arg(retryCount));
            err = wlanapi->WlanConnect(wlanHandle, intfGuid, &conn, NULL);

            if (err == ERROR_INVALID_PARAMETER)
            {
                // TODO:
                LOG_DEBUG(QString::fromUtf8("WlanConnect failed, err=%1, Perhaps profile disappeared?").arg(err));
                //status = translateWin32Error(err);
                status = WlanProfileNotFound;
                break;
            }
            else if (err != ERROR_SUCCESS)
            {
                LOG_DEBUG(QString::fromUtf8("WlanConnect failed, err=%1").arg(err));
                status = translateWin32Error(err);
                break;
            }

            HANDLE handles[] = { m_hAbortEvent, m_hEvent };
            DWORD dwWait = WaitForMultipleObjects(2, handles, FALSE, 10000);
            LOG_DEBUG(QString::fromUtf8("WaitForMultipleObjects result %1").arg(dwWait));
            if (dwWait == WAIT_OBJECT_0)
            {
                LOG_DEBUG(QString::fromUtf8("aborted!"));
                break;
            }
            else if (dwWait == WAIT_OBJECT_0 + 1)
            {
                if (m_wlanReasonCode == 0)
                {
                    LOG_DEBUG(QString::fromUtf8("seems WlanConnect completed successfully!"));
                    break;
                }
                else
                {
                    WCHAR reason[1024];
                    memset(reason, 0, sizeof(reason));
                    DWORD dd = wlanapi->WlanReasonCodeToString(m_wlanReasonCode, 1023, reason, NULL);
                    LOG_DEBUG(QString::fromUtf8("connectProfile failed reason: %1 %2 [%3]").arg(m_wlanReasonCode).arg(QString::fromWCharArray(reason)).arg(dd));
                    if (++retryCount > maxRetryCount)
                    {
                        LOG_DEBUG(QString::fromUtf8("WlanConnect failed too many times, bad!"));
                        break;
                    }
                }
            }
            else if (dwWait == WAIT_TIMEOUT)
            {
                if (++retryCount > maxRetryCount)
                {
                    LOG_DEBUG(QString::fromUtf8("WlanConnect failed too many times, bad!"));
                    break;
                }
            }
            else
            {
                LOG_DEBUG(QString::fromUtf8("WaitForMultipleObjects unexpected result %1").arg(dwWait));
                break;
            }
        }

        if (isAborted())
        {
            status = AbortedError;
            break;
        }

#if 0
        int retryCount = 0;
        LOG_DEBUG(QString::fromUtf8("now WlanQueryInterface %1 loop for %2").arg(m_profile).arg(retryCount));
        for (;;)
        {
            if (isAborted())
            {
                err = ERROR_INVALID_STATE;
                break;
            }
            err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
            if (err == ERROR_INVALID_STATE)
            {
                LOG_DEBUG(QString::fromUtf8("this WlanQueryInterface result ERROR_INVALID_STATE"));
                PWLAN_AVAILABLE_NETWORK_LIST p1;
                DWORD err2 = wlanapi->WlanGetAvailableNetworkList(wlanHandle, intfGuid, 0, NULL, &p1);
                if (err2 == 0)
                {
                    wlanapi->WlanFreeMemory(p1);
                }
                LOG_DEBUG(QString::fromUtf8("this WlanQueryInterface err2=%1").arg(err2));
                if (err2 == 0x80342002L/*ERROR_NDIS_DOT11_POWER_STATE_INVALID*/)
                {
                    err = err2;
                    break;
                }
                else
                {
                    if (++retryCount < 6)
                    {
                        LOG_DEBUG(QString::fromUtf8("wait more %1").arg(retryCount));
                        Sleep(1000);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                LOG_DEBUG(QString::fromUtf8("this WlanQueryInterface result %1").arg(err));
                if (err = ERROR_SUCCESS)
                {
                    if (connInfo->isState != wlan_interface_state_connected)
                    {
                        if (++retryCount < 6)
                        {
                            LOG_DEBUG(QString::fromUtf8("wait more connection %1").arg(retryCount));
                            Sleep(1000);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                break;
            }
        }

        if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanQueryInterface failed 2, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }
#else

        err = wlanapi->WlanQueryInterface(wlanHandle, intfGuid, wlan_intf_opcode_current_connection, NULL, &cb, reinterpret_cast<PVOID*>(&connInfo), &valueType);
        if (err == ERROR_INVALID_STATE)
        {
            PWLAN_AVAILABLE_NETWORK_LIST p1;
            DWORD err2 = wlanapi->WlanGetAvailableNetworkList(wlanHandle, intfGuid, 0, NULL, &p1);
            if (err2 == 0)
            {
                wlanapi->WlanFreeMemory(p1);
            }
            LOG_DEBUG(QString::fromUtf8("WlanQueryInterface failed, err=%1").arg(err));
            if (err2 == 0x80342002L/*ERROR_NDIS_DOT11_POWER_STATE_INVALID*/)
            {
                status = translateWin32Error(err2);
            }
            else
            {
                status = translateWin32Error(err);
            }
            break;
        }
        else if (err != ERROR_SUCCESS)
        {
            LOG_DEBUG(QString::fromUtf8("WlanQueryInterface failed, err=%1").arg(err));
            status = translateWin32Error(err);
            break;
        }
#endif

        LOG_DEBUG(QString::fromUtf8("conn state: %1").arg(connInfo->isState));
        if (connInfo->isState != wlan_interface_state_connected)
        {
            // TODO:
            LOG_WARNING(QString::fromUtf8("state not match"));
            status = UnknownError;
            break;
        }

        setValue("profileName", QString::fromWCharArray(connInfo->strProfileName));
        m_system->m_wlanSsid=QString::fromWCharArray(connInfo->strProfileName);
        setValue("ssid", QString::fromUtf8(reinterpret_cast<const char*>(connInfo->wlanAssociationAttributes.dot11Ssid.ucSSID), connInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength));
        setValue("mac", QByteArray(reinterpret_cast<const char*>(connInfo->wlanAssociationAttributes.dot11Bssid), 6));
        setValue("securityEnabled", connInfo->wlanSecurityAttributes.bSecurityEnabled ? true : false);
        status = NoError;
    }
    while (false);

    if (connInfo)
    {
        wlanapi->WlanFreeMemory(connInfo);
    }

    if (intfList)
    {
        wlanapi->WlanFreeMemory(intfList);
    }

    if (wlanHandle)
    {
        wlanapi->WlanCloseHandle(wlanHandle, NULL);
    }

    return status;
}

void ConnectProfileOp::wlanNotify(PWLAN_NOTIFICATION_DATA data, PVOID context)
{
    ConnectProfileOp *that = static_cast<ConnectProfileOp*>(context);
    /*	if (data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM && data->NotificationCode == wlan_notification_acm_connection_complete) {
    		PWLAN_CONNECTION_NOTIFICATION_DATA connData = static_cast<PWLAN_CONNECTION_NOTIFICATION_DATA>(data->pData);
    		SetEvent(that->m_hEvent);
    	}*/
    if (data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM)
    {
        if (data->NotificationCode == wlan_notification_acm_connection_complete ||
                data->NotificationCode == wlan_notification_acm_disconnected)
        {
            PWLAN_CONNECTION_NOTIFICATION_DATA connData = static_cast<PWLAN_CONNECTION_NOTIFICATION_DATA>(data->pData);
            LOG_DEBUG(QString::fromUtf8("wlanNotify A %1 %2").arg(data->NotificationCode).arg(connData->wlanReasonCode));
            if (data->NotificationCode == wlan_notification_acm_connection_complete)
            {
                that->m_wlanReasonCode = connData->wlanReasonCode;
                LOG_DEBUG(QString::fromUtf8("signal connection complete OK!"));
                SetEvent(that->m_hEvent);
            }
        }
        else
        {
            LOG_DEBUG(QString::fromUtf8("wlanNotify B %1").arg(data->NotificationCode));
        }
    }
}
