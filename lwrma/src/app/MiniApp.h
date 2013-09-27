#ifndef __MiniApp_h__
#define __MiniApp_h__

#include <LenovoCore/LenovoCore.h>
#include <QtCore/QEventLoop>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QStackedLayout>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QVBoxLayout>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QProcess>
#include <QNetworkInterface>

class AppPage;
class AppService;
class AppUILoader;
class AppWaiter;
class AsyncOp;
class Bean;
class MainWindow;
class SoapCore;

class MiniApp
	: public QApplication
{
	Q_OBJECT

public:
	static MiniApp *instance();
	static int main(int argc, char *argv[]);
	void navigateTo(const QString& pageName, const QVariantMap& params);
	AppService *service() const;
	SoapCore *soapCore() const;
	System *system() const;
	Bean *bean() const;
	QString translateUIText(int stringId, QWidget *widget = NULL);
	void setLanguage(const QString& lang);
	bool wait(AsyncOp *op, int stringId);
	void enterWaitMode(AppWaiter *waiter);
	void leaveWaitMode();
	void closePage();

	double incomingBPS() const;
	double outgoingBPS() const;
	int uptimeMinutes() const;
	QString wifiName() const;
	bool wifiSecurity() const;

	AsyncOp *testRouter(int timeout);
	void setWifiPassword(const QString& wifiPassword);
	QString wifiPassword() const;
	void generateWifiName();
	void confirmWifiName();
    bool checkRouter2();

	QString versionString() const;

	void wait(AsyncOp *op, QObject *obj, const char *member);

	void configSet(const QString& name, const QVariant& value);
	QVariant configGet(const QString& name) const;
	void setWifiSecurity(bool wifiSecurity);
    void showMessage(int stringId);

	void beginUserMode();

	bool validateWifiPassword(const QString& password) const;
	void confirmNewWifiPassword(const QString& password);
    QString savedWifiPassword() const;

	void setWifiName(const QString& wifiName);

Q_SIGNALS:
	void incomingBPSChanged();
	void outgoingBPSChanged();
	void uptimeMinutesChanged();
	void wifiNameChanged();
	void wifiSecurityChanged();

protected:
	MiniApp(int& argc, char **argv);
	int run();

private Q_SLOTS:
	void cleanup();
	void navigateToSlot(const QString& pageName, const QVariantMap& params);
	void onJumpMenuAction();
	void onLangMenuAction();
	void calcNetworkPerf();
	void onMainWindowMinimized();
	void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void deferredHideWindow();
	void onAppInstanceActivated();
	void onQuitNow();
	void onBroadcastQuitEvent();
	void onRunAs();
	void startWlanService();
	void onWlanDiagnose();
    void onReloadQss();
	void onMainPageCloseButtonClicked();
	void onQuestionButtonClicked();
	void onUpdateUptimeFinished();
	void toggleDebugFrame();
	void onWaitOpFinished();
	void checkForUpdate();
	void onUpdateCheckFinished();
	void onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onUpdateDownloadFinished();
	void changeUptimeTimer();
	void onNetworkConnectionChanged();
    void onWakeUpNetworkConnectionChanged();
    void onSleepNetworkConnectionChanged();

private:
	void unloadPage();
	void collectNetworkPerf();
	void restoreFromTray();
	AppPage *loadPage(const QString& pageName, const QVariantMap& params = QVariantMap());
	void setUptimeMinutes(int uptimeMinutes);
	void updateUptime();
	void serializeConfig(bool save);
	void translateUI();

private:
	QPointer<Bean> m_bean;
	QPointer<MainWindow> m_mainWnd;
	QPointer<AppService> m_svc;
	QPointer<AppUILoader> m_uiLoader;
	QPointer<AppPage> m_mainPage;
	QPointer<AppPage> m_activePage;
	QTimer m_perfTimer;
	System::NetworkPerf m_perf;
	double m_perfIn;
	double m_perfOut;
	double m_incomingBPS;
	double m_outgoingBPS;
	int m_uptimeMinutes;
	QString m_wifiName;
	bool m_wifiSecurity;
	QString m_wifiPassword;

	QSystemTrayIcon m_trayIcon;
	QMenu m_trayMenu;

	typedef QMap<int, QString> LangDict;
	LangDict m_lang_zh;
	LangDict m_lang_en;
	const LangDict *m_activeLangDict;

	QSettings *m_settings;
	QPointer<AsyncOp> m_updateTimeOp;

	QPointer<QObject> m_waitObj;
	QByteArray m_waitMethod;

	QString m_wlanProfileName;
	QString m_routerMac;
	bool m_minimizeOnClose;
	bool m_autoStart;
	bool m_autoUpdate;
	bool m_enablePopup;
	bool m_disconnectedPrompt;
	bool m_bypassNetworkChange;

	QAction *m_quitAction;
	QIcon m_appIcon;

	QTimer m_autoUpdateCheckTimer;
	QNetworkAccessManager *m_updateNam;
	QNetworkReply *m_updateReply;
	QByteArray m_updateMd5;
	QString m_updateVer;

    int m_baseUptimeMinutes;

    int m_baseConnectMinutes;
    bool m_AddrChangedSwitch;
	QTimer m_userTimer1;
	QElapsedTimer m_userTimer2;

    QElapsedTimer m_blockedTimer;
    bool m_blocked30s;


	QString m_safeWifiPassword;
};

class ClientArea
	: public QWidget
{
public:
	ClientArea();
	void setMainPage(AppPage *page);
	void setNavPage(AppPage *page);

private:
	QPointer<QStackedLayout> m_rootLayout;
	QPointer<QVBoxLayout> m_layout0;
	QPointer<QStackedLayout> m_layout1;
	QPointer<AppPage> m_mainPage;
	QPointer<AppPage> m_navPage;
};

class MainWindow
	: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	void enterWaitMode(AppWaiter *waiter);
	void leaveWaitMode();
	ClientArea *clientArea() const;

protected:
	virtual void closeEvent(QCloseEvent *e);
	virtual void changeEvent(QEvent *e);
	virtual bool winEvent(MSG *msg, long *result);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);

private:
	QPointer<AppWaiter> m_waiter;
	QPointer<ClientArea> m_clientArea;
	QPoint m_trackPos;
	bool m_tracking;
};

class AppWaiter
	: public QObject
{
	Q_OBJECT

public:
	AppWaiter(MiniApp *app, int stringId);
	bool wait(AsyncOp *op);
	void abort();

private Q_SLOTS:
	void onOpFinished();

private:
	MiniApp *m_app;
	QEventLoop m_eventLoop1;
	QEventLoop m_eventLoop2;
	QTimer m_timer;
	int m_stringId;
	bool m_aborted;
};

class TimeLimitedOp
	: public AsyncOp
{
	Q_OBJECT

public:
	TimeLimitedOp(AsyncOp *op, int timeout);
	virtual ~TimeLimitedOp();

private Q_SLOTS:
	void onTimeout();
	void onOpFinished();

private:
	QPointer<AsyncOp> m_op;
	QTimer m_timer;
};


#endif // __MiniApp_h__
