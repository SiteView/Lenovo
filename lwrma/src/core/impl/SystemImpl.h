#ifndef __SystemImpl_h__
#define __SystemImpl_h__

#include <LenovoCore/AsyncOp.h>
#include <LenovoCore/System.h>
#include "wlanlib.h"
#include "iphlpapi.h"
#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <winsock.h>
#include <windows.h>

class QuitDog;

class SystemImpl
	: public QObject
{
	Q_OBJECT

public:
	SystemImpl(System *intf, const QString& ident);
	virtual ~SystemImpl();
	System *q_ptr() const;

	bool init();

	bool checkInstance(QWidget *mainWindow);
	bool checkMessage(MSG *msg, long *result);
	void executeAs(QWidget *mainWindow, const QString& path);
	void broadcastQuitEvent();
	void startQuitDog();
	void stopQuitDog();
	void runUpdateSetup(QWidget *mainWindow, const QString& path);

	QStringList queryGatewayList();

	void markWindow(QWidget *mainWindow);

	AsyncOp *startWlanService();
	AsyncOp *searchSsid(const QString& ssid);
	AsyncOp *connectSsid(const QVariantMap& ssid);
	AsyncOp *httpGet(const QString& url);
	AsyncOp *createWlanProfile(const QString& ssid, const QString& password);
	AsyncOp *connectWlanProfile(const QString& profile, bool reconnect);

	void queryNetworkPerf(System::NetworkPerf& perf);
	void updatwWlanInterface(PWLAN_INTERFACE_INFO_LIST intfList);
	DWORD selectBestInterface(PWLAN_INTERFACE_INFO_LIST intfList);
	static void WINAPI __wlanNotificationCallback(PWLAN_NOTIFICATION_DATA data, PVOID ctx);

	void updateWlanState();
	void updateWlanRadioState();

	bool checkRouter(const QString& mac);
    bool checkRouter2(const QString& ssidName);

public Q_SLOTS:
	void process();

public:
	System *m_intf;
	QString m_ident;
	QuitDog *m_quitDog;
	WCHAR m_globalEventName[256];
	WCHAR m_instanceName[256];
	OSVERSIONINFOEX m_osver;
	WlanLib *m_wlanapi;
	QTimer m_processTimer;
	HANDLE m_wlanHandle;
	bool m_needsElevation;
	bool m_wlanInit;
	bool m_wlanServiceActive;
	QUuid m_wlanInterfaceUuid;
	GUID m_wlanInterfaceGuid;
	QString m_wlanInterfaceDescription;
    QString m_wlanSsid;
	QNetworkAccessManager *m_nam;
};

class QuitDog
	: public QObject
{
	Q_OBJECT

public:
	QuitDog(SystemImpl *system, const WCHAR *eventName);
    bool wlanConnectedStatus();
	~QuitDog();
	void dogRun();

	class DogThread
		: public QThread
	{
	public:
		DogThread(QuitDog *owner);
		virtual void run();
		QuitDog *m_owner;
	};

private:
	void setupNetworkChangeNotify();

private Q_SLOTS:
	void onAddrChanged();

private:
	SystemImpl *m_system;
	HANDLE m_hQuitEvent;
	HANDLE m_hGlobalQuitEvent;
	HANDLE m_hIpChanged;
	DogThread m_thread;
	OVERLAPPED m_overlapped;
	HANDLE m_handle;

	typedef DWORD (WINAPI *PFN_NotifyAddrChange)(PHANDLE, LPOVERLAPPED);
	typedef BOOL (WINAPI *PFN_CancelIPChangeNotify)(LPOVERLAPPED);
	PFN_NotifyAddrChange m_pfnNotifyAddrChange;
	PFN_CancelIPChangeNotify m_pfnCancelIPChangeNotify;
};

class ThreadedAsyncOp
	: public AsyncOp
{
	Q_OBJECT

public:
	ThreadedAsyncOp(QObject *parent = NULL);
	virtual ~ThreadedAsyncOp();
	void start();

protected:
	virtual int process(QVariantMap& result);

private Q_SLOTS:
	void onWorkerThreadFinished(int status, const QVariantMap& result);

private:
	class WorkerThread
		: public QThread
	{
	public:
		WorkerThread(ThreadedAsyncOp *owner);

	protected:
		virtual void run();

	private:
		ThreadedAsyncOp *m_owner;
	};

	WorkerThread *m_workerThread;
};

class StartWlanServiceOp
	: public AsyncOp
{
	Q_OBJECT

public:
	StartWlanServiceOp(const QString& appFilePath, const QString& appDirPath, bool needsElevation);
	virtual ~StartWlanServiceOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void workerThreadFinished();

private:
	class WorkerThread
		: public QThread
	{
	public:
		WorkerThread(StartWlanServiceOp *owner);

	protected:
		virtual void run();

	private:
		StartWlanServiceOp *m_owner;
	};

	void workerThreadProc();

private:
	QString m_appFilePath;
	QString m_appDirPath;
	QPointer<WorkerThread> m_workerThread;
	HANDLE m_quitEvent;
	bool m_workSucceeded;
	bool m_needsElevation;
};

class SearchSsidOp
	: public ThreadedAsyncOp
{
	Q_OBJECT

public:
	SearchSsidOp(SystemImpl *system, const QString& ssid);

protected:
	virtual void onAbort();
	virtual int process(QVariantMap& result);

private:
	SystemImpl *m_system;
	QString m_ssid;
	int m_tryCount;
};

class ConnectSsidOp
	: public ThreadedAsyncOp
{
	Q_OBJECT

public:
	ConnectSsidOp(SystemImpl *system, const QVariantMap& ssid);
	virtual ~ConnectSsidOp();

protected:
	virtual void onAbort();
	virtual int process(QVariantMap& result);

private:
	static void WINAPI wlanNotify(PWLAN_NOTIFICATION_DATA data, PVOID context);

private:
	SystemImpl *m_system;
	QVariantMap m_ssid;
	HANDLE m_hEvent;
	HANDLE m_hAbortEvent;
	WLAN_REASON_CODE m_wlanReasonCode;
};

class HttpGetOp
	: public AsyncOp
{
	Q_OBJECT

public:
	HttpGetOp(SystemImpl *system, const QString& url);
	virtual ~HttpGetOp();
	void start();

protected:
	virtual void onAbort();

private Q_SLOTS:
	void onReplyFinished();

private:
	SystemImpl *m_system;
	QString m_url;
	QNetworkReply *m_reply;
};

class CreateWlanProfileOp
	: public ThreadedAsyncOp
{
	Q_OBJECT

public:
	CreateWlanProfileOp(SystemImpl *system, const QString& ssid, const QString& password);
	virtual ~CreateWlanProfileOp();

protected:
	virtual void onAbort();
	virtual int process(QVariantMap& result);

private:
	SystemImpl *m_system;
	QString m_ssid;
	QString m_password;
};

class ConnectProfileOp
	: public ThreadedAsyncOp
{
	Q_OBJECT

public:
	ConnectProfileOp(SystemImpl *system, const QString& profile, bool reconnect = false);
	virtual ~ConnectProfileOp();

protected:
	virtual void onAbort();
	virtual int process(QVariantMap& result);

private:
	static void WINAPI wlanNotify(PWLAN_NOTIFICATION_DATA data, PVOID context);

private:
	SystemImpl *m_system;
	QString m_profile;
	bool m_reconnect;
	HANDLE m_hEvent;
	HANDLE m_hAbortEvent;
	WLAN_REASON_CODE m_wlanReasonCode;
};

#endif // __SystemImpl_h__
