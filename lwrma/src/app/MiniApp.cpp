#include "AppPage.h"
#include "AppUILoader.h"
#include "MiniApp.h"
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QLocale>
#include <QtCore/QTextStream>
#include <QtCore/QVariantMap>
#include <QtGui/QAction>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QMovie>
#include <QtGui/QStyle>
#include <QtGui/QWindowStateChangeEvent>
#include <QDebug>
#include <time.h>
#include "DetailUiHelper.h"

DEFINE_LOGGER(App);

class PageInit
{
public:
	struct Entry
	{
		const char *pageName;
		AppPage *(*creator)();
	};

	static PageInit *instance()
	{
		static PageInit s_instance;
		return &s_instance;
	}

	void registerPageCreator(const char *pageName, AppPage *(*creator)())
	{
		Entry e;
		e.pageName = pageName;
		e.creator = creator;
		m_list.append(e);
	}

	void unregisterPageCreator(AppPage *(*creator)())
	{
	}

	QList<Entry> m_list;
};

void appRegisterPageCreator(const char *pageName, AppPage *(*creator)())
{
	PageInit::instance()->registerPageCreator(pageName, creator);
}

void appUnregisterPageCreator(AppPage *(*creator)())
{
	PageInit::instance()->unregisterPageCreator(creator);
}

static void readStringFile(QMap<int, QString>& dict, const QString& path)
{
	QFile f(path);
	if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream strm(&f);
		while (!strm.atEnd()) {
			QString line = strm.readLine();
			int sep = line.indexOf(QLatin1String("="));
			if (sep > 0) {
				bool ok;
				int key = line.left(sep).toInt(&ok);
				if (ok) {
					dict.insert(key, line.mid(sep + 1));
				}
			}
		}
	}
}

static QString guardPassword(const QString& input, bool encode)
{
	QString output;
	char xx = 73;
	if (encode) {
		QByteArray uu = input.toUtf8();
		for (int i = 0; i < uu.size(); i++) {
			uu[i] = uu[i] ^ xx;
			uu[i] = 255 - uu[i];
			xx = uu[i];
		}
		output = QString::fromUtf8(uu.toBase64());
	} else {
		QByteArray uu = QByteArray::fromBase64(input.toUtf8());
		for (int i = uu.count() - 1; i >= 0; i--) {
			if (i > 0) {
				uu[i] = uu[i - 1] ^ uu[i];
			} else {
				uu[i] = uu[i] ^ xx;
			}
			uu[i] = 255 - uu[i];
		}
		output = QString::fromUtf8(uu);
	}
	return output;
}

static const QString UPDATE_URL_TEMPLATE = QLatin1String("http://updates1.netgear.com/r3220/cn/%1");

MiniApp::MiniApp(int& argc, char **argv)
	: QApplication(argc, argv), m_incomingBPS(0), m_outgoingBPS(0), m_uptimeMinutes(0), m_wifiSecurity(false)
{
	QString u2 = guardPassword(QLatin1String("siteviewtest"), true);
	QString u3 = guardPassword(u2, false);
    //setOrganizationName(QLatin1String("Lenovo"));
	setOrganizationDomain(QLatin1String("lenovo.com"));
    //setApplicationName(QLatin1String("Wireless Router Manage Assistant"));
    setApplicationVersion(QLatin1String("1.0.169.16"));
   //m_settings = new QSettings(this);
   //m_settings = new QSettings(QSettings::SystemScope,QLatin1String("Lenovo"),QLatin1String("Wireless Router Manage Assistant"));
    QString ss1 = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
    QString iniFilePath= ss1.left(3);
//    QString iniFilePath = applicationDirPath();
    iniFilePath.append(QLatin1String("Users/Public/LenovoConfig.ini"));
    m_settings = new QSettings(iniFilePath,QSettings::IniFormat);
    DetailUiHelper::instance()->loadQSS();
	connect(&m_autoUpdateCheckTimer, SIGNAL(timeout()), SLOT(checkForUpdate()));
	m_autoUpdateCheckTimer.setInterval(60 * 60 * 1000);
	m_autoUpdateCheckTimer.setSingleShot(false);
	m_updateNam = new QNetworkAccessManager(this);
	m_updateReply = NULL;
	m_baseUptimeMinutes = 0;
	m_disconnectedPrompt = false;
}

MiniApp *MiniApp::instance()
{
	return static_cast<MiniApp*>(qApp);
}

int MiniApp::main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(app);
#if LOGGER_ENABLED
	LoggerSystem loggerSystem;
	loggerSystem.init(QString::fromUtf8("%1\\lwrma.log").arg(QDesktopServices::storageLocation(QDesktopServices::TempLocation)));
#endif
	MiniApp app(argc, argv);
	return app.run();
}

int MiniApp::run()
{
	LOG_DEBUG(QString::fromUtf8("version: %1").arg(applicationVersion()));
	LOG_DEBUG(QString::fromUtf8("exec: %1").arg(QDir::toNativeSeparators(applicationFilePath())));
	qsrand(time(NULL));

	int retval;
	if (System::runCommand(&retval)) {
		return retval;
	}

	m_bean = new Bean(QLatin1String("MiniAppInstance_4673"));
	if (!m_bean->init()) {
		// TODO:
		//QMessageBox::critical(NULL, applicationName(), tr("System::init failed!"));
		cleanup();
		return -1;
	}

	m_appIcon = m_bean->system()->loadAppIcon(101, 0, 0);
	setWindowIcon(m_appIcon);

	m_mainWnd = new MainWindow();
	m_mainWnd->setWindowIcon(m_appIcon);

	if (!system()->checkInstance(m_mainWnd)) {
		cleanup();
		return 0;
	}
/*
	if (!m_system->init2()) {
		cleanup();
		return -2;
	}
*/

	serializeConfig(false);

	bool autoStartMode = false;
	QStringList args = arguments();
	for (int i = 0; i < args.count(); i++) {
		if (args.at(i).compare(QLatin1String("-autostart"), Qt::CaseInsensitive) == 0) {
			autoStartMode = true;
			break;
		}
	}

	if (autoStartMode && !m_autoStart) {
		return 0;
	}

	m_svc = new AppService(this);
	soapCore()->setSessionId(QString::fromUtf8("AD28AE69687E58D9K77"));

	m_uiLoader = new AppUILoader();
	Q_FOREACH(const PageInit::Entry& e, PageInit::instance()->m_list) {
		m_uiLoader->registerPageCreator(QString::fromUtf8(e.pageName), e.creator);
	}

	//const char *srcRoot = DEV_SRC_ROOT;
	const char *srcRoot = ":";
	readStringFile(m_lang_en, QString::fromUtf8("%1/lang/en.txt").arg(QString::fromUtf8(srcRoot)));
	readStringFile(m_lang_zh, QString::fromUtf8("%1/lang/zh.txt").arg(QString::fromUtf8(srcRoot)));
/*
	if (QLocale::Chinese == QLocale::system().language()) {
		m_activeLangDict = &m_lang_zh;
	} else {
		m_activeLangDict = &m_lang_en;
	}
/*/
	m_activeLangDict = &m_lang_zh;
//*/
	QMenu *jumpMenu = m_mainWnd->menuBar()->addMenu(tr("Jump"));
	Q_FOREACH(const PageInit::Entry& e, PageInit::instance()->m_list) {
		jumpMenu->addAction(QString::fromUtf8(e.pageName).mid(2), this, SLOT(onJumpMenuAction()));
	}

	const char *langNames[] = {
		"zh", "en",
	};

	QMenu *langMenu = m_mainWnd->menuBar()->addMenu(tr("Language"));
	for (size_t i = 0; i < sizeof(langNames) / sizeof(langNames[0]); i++) {
		langMenu->addAction(QString::fromUtf8(langNames[i]), this, SLOT(onLangMenuAction()));
	}

	QMenu *debugMenu = m_mainWnd->menuBar()->addMenu(tr("Debug"));
	debugMenu->addAction(tr("Broadcast Quit Event"), this, SLOT(onBroadcastQuitEvent()));
	debugMenu->addAction(tr("Run As..."), this, SLOT(onRunAs()));
	debugMenu->addAction(tr("Start Wlan service"), this, SLOT(startWlanService()));
	debugMenu->addAction(tr("Wlan Diagnose"), this, SLOT(onWlanDiagnose()));
    debugMenu->addAction(tr("Reload Qss"),this,SLOT(onReloadQss()));
	debugMenu->addAction(tr("Toggle frame"), this, SLOT(toggleDebugFrame()));
    debugMenu->addAction(tr("Check Network"), this, SLOT(onNetworkConnectionChanged()));

	m_trayMenu.setObjectName(QLatin1String("trayMenu"));
	m_quitAction = m_trayMenu.addAction(QString(), this, SLOT(onQuitNow()));

	m_trayIcon.setIcon(m_appIcon);
	m_trayIcon.setContextMenu(&m_trayMenu);
	connect(&m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));

	QAction *action = new QAction(this);
	m_mainWnd->addAction(action);
	action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_D, Qt::CTRL + Qt::ALT + Qt::Key_E, Qt::CTRL + Qt::ALT + Qt::Key_V, Qt::CTRL + Qt::ALT + Qt::Key_E));
	connect(action, SIGNAL(triggered()), SLOT(toggleDebugFrame()));

	bool develMode = m_settings->value(QLatin1String("Config/DevelMode"), false).toBool();
	QString mainPageName = QLatin1String("MainFrame");
	bool debugFrame = false;
	m_bypassNetworkChange = false;

	if (develMode) {
		mainPageName = m_settings->value(QLatin1String("Config/MainPageName"), mainPageName).toString();
		debugFrame = m_settings->value(QLatin1String("Config/MainWindowFrame"), debugFrame).toBool();
		m_bypassNetworkChange = m_settings->value(QLatin1String("Config/BypassNetworkChange"), m_bypassNetworkChange).toBool();
	}

    m_mainPage = loadPage(mainPageName);
    connect(m_mainPage, SIGNAL(closeButtonClicked()), SLOT(onMainPageCloseButtonClicked()));
    connect(m_mainPage, SIGNAL(questionButtonClicked()), SLOT(onQuestionButtonClicked()));
	m_mainWnd->clientArea()->setMainPage(m_mainPage);

	if (m_routerMac.isEmpty()) {
		navigateTo(QString::fromUtf8("NewRouterConfirm"), QVariantMap());
	} else {
		QVariantMap params;
		//params.insert(QString::fromUtf8("profile"), m_wlanProfileName);
		params.insert(QString::fromUtf8("mac"), m_routerMac);
		params.insert(QString::fromUtf8("mode"), 1);
		navigateTo(QString::fromUtf8("RouterDetect"), params);
	}

	if (!debugFrame) {
		m_mainWnd->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
		m_mainWnd->menuBar()->hide();
		system()->markWindow(m_mainWnd);
	}

	QSize appSize(960, 700);

	QRect rc = desktop()->availableGeometry();
	if (rc.width() >= appSize.width() && rc.height() >= appSize.height()) {
		QPoint pt(rc.left() + (rc.width() - appSize.width()) / 2, rc.top() + (rc.height() - appSize.height()) / 2);
		m_mainWnd->move(pt);
	}

	translateUI();

	m_mainWnd->show();
	connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));

	collectNetworkPerf();
	connect(&m_perfTimer, SIGNAL(timeout()), SLOT(calcNetworkPerf()));
	m_perfTimer.start(1000);

	connect(system(), SIGNAL(quitNow()), SLOT(onQuitNow()));
	system()->startQuitDog();

	if (m_autoUpdate) {
		m_autoUpdateCheckTimer.start();
		checkForUpdate();
	}

	connect(system(), SIGNAL(networkConnectionChanged()), SLOT(onNetworkConnectionChanged()));
    connect(system(), SIGNAL(systemResumed()), SLOT(onNetworkConnectionChanged()));//sleep
    connect(system(), SIGNAL(systemwakeup()), SLOT(onNetworkConnectionChanged()));//sleepp

	retval = exec();
//    serializeConfig(true);
	return retval;
}

void MiniApp::serializeConfig(bool save)
{
	static const QString CFG_MINIMIZE_ON_CLOSE = QLatin1String("Config/MinimizeOnClose");
	static const QString CFG_WLAN_PROFILE_NAME = QLatin1String("Config/WlanProfileName");
	static const QString CFG_ROUTER_MAC = QLatin1String("Config/RouterMac");
	static const QString CFG_AUTO_START = QLatin1String("Config/AutoStart");
	static const QString CFG_AUTO_UPDATE = QLatin1String("Config/AutoUpdate");
	static const QString CFG_ENABLE_POPUP = QLatin1String("Config/EnablePopup");
	static const QString CFG_WIFI_PASSWORD = QLatin1String("Config/WifiPassword");

    if (save) {
        m_settings->setValue(CFG_MINIMIZE_ON_CLOSE, m_minimizeOnClose);
        m_settings->setValue(CFG_WLAN_PROFILE_NAME, m_wlanProfileName);
        m_settings->setValue(CFG_ROUTER_MAC, m_routerMac);
        m_settings->setValue(CFG_AUTO_START, m_autoStart);
        m_settings->setValue(CFG_AUTO_UPDATE, m_autoUpdate);
        m_settings->setValue(CFG_ENABLE_POPUP, m_enablePopup);
        m_settings->setValue(CFG_WIFI_PASSWORD, guardPassword(m_safeWifiPassword, true));
    } else {
        m_minimizeOnClose = m_settings->value(CFG_MINIMIZE_ON_CLOSE, true).toBool();
        m_wlanProfileName = m_settings->value(CFG_WLAN_PROFILE_NAME).toString();
        m_routerMac = m_settings->value(CFG_ROUTER_MAC).toString();
        m_autoStart = m_settings->value(CFG_AUTO_START, false).toBool();
        m_autoUpdate = m_settings->value(CFG_AUTO_UPDATE, true).toBool();
        m_enablePopup = m_settings->value(CFG_ENABLE_POPUP, false).toBool();
        m_safeWifiPassword = guardPassword(m_settings->value(CFG_WIFI_PASSWORD).toString(), false);
	}
}

void MiniApp::checkForUpdate()
{
	if (m_updateReply) {
		return;
	}

	LOG_DEBUG(QString::fromUtf8("checking for update now..."));

	QUrl url(UPDATE_URL_TEMPLATE.arg(QLatin1String("update.txt")));
	QNetworkRequest req(url);
	m_updateReply = m_updateNam->get(req);
	connect(m_updateReply, SIGNAL(finished()), SLOT(onUpdateCheckFinished()));
}

static int compareAppVersion(const QString& ver1, const QString& ver2)
{
	if (ver1.compare(ver2) == 0) {
		return 0;
	}

	QStringList p1 = ver1.split(QLatin1String("."));
	QStringList p2 = ver2.split(QLatin1String("."));
	if (p1.count() != 4) {
		return -1;
	}

	if (p1.count() != p2.count()) {
		return -1;
	}

	int dd = p1[0].toInt() - p2[0].toInt();
	if (dd != 0) {
		return dd;
	}

	dd = p1[1].toInt() - p2[1].toInt();
	if (dd != 0) {
		return dd;
	}

	dd = p1[3].toInt() - p2[3].toInt();
	if (dd != 0) {
		return dd;
	}

	dd = p1[2].toInt() - p2[2].toInt();
	if (dd != 0) {
		return dd;
	}

	return 0;
}

// update.txt
// 1.0.102.7,lwrma_setup_v1.0.102.7.exe,d6c2b677110f391d59dec51feabe64c6

void MiniApp::onUpdateCheckFinished()
{
	QNetworkReply *reply = m_updateReply;
	m_updateReply->deleteLater();
	m_updateReply = NULL;

	if (reply->error() != QNetworkReply::NoError) {
		LOG_DEBUG(QString::fromUtf8("checking update.txt failed, NetworkError=%1 [%2]").arg(reply->error()).arg(reply->errorString()));
		return;
	}

	LOG_DEBUG(QString::fromUtf8("checking update.txt, file downloaded successfully"));

	QTextStream ts(reply->readAll());
	QString line = ts.readLine();
	LOG_DEBUG(QString::fromUtf8("update.txt line: %1").arg(line));
	QStringList parts = line.split(QLatin1String(","));
	if (parts.length() != 3) {
		LOG_DEBUG(QString::fromUtf8("update.txt line invalid format %1").arg(parts.length()));
		return;
	}

	m_updateVer = parts[0];
	int cmp1 = compareAppVersion(applicationVersion(), m_updateVer);
	LOG_DEBUG(QString::fromUtf8("compare %1 vs %2 [%3]").arg(applicationVersion()).arg(m_updateVer).arg(cmp1));
	if (cmp1 >= 0) {
		return;
	}

	m_updateMd5 = parts[2].toUtf8();

	LOG_DEBUG(QString::fromUtf8("downloading file: %1").arg(parts[1]));
	QUrl url(UPDATE_URL_TEMPLATE.arg(parts[1]));
	QNetworkRequest req(url);
	m_updateReply = m_updateNam->get(req);
	connect(m_updateReply, SIGNAL(finished()), SLOT(onUpdateDownloadFinished()));
	connect(m_updateReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(onUpdateDownloadProgress(qint64,qint64)));
}

void MiniApp::onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (bytesTotal > 0) {
		LOG_DEBUG(QString::fromUtf8("update download progress: %1 %2 %3%").arg(bytesReceived).arg(bytesTotal).arg(100 * static_cast<qreal>(bytesReceived) / static_cast<qreal>(bytesTotal), 0, '4', 2));
	} else {
		LOG_DEBUG(QString::fromUtf8("update download progress: %1").arg(bytesReceived));
	}
}

void MiniApp::onUpdateDownloadFinished()
{
	QNetworkReply *reply = m_updateReply;
	m_updateReply->deleteLater();
	m_updateReply = NULL;

	if (reply->error() != QNetworkReply::NoError) {
		LOG_DEBUG(QString::fromUtf8("update file downloading failed, NetworkError=%1 [%2]").arg(reply->error()).arg(reply->errorString()));
		return;
	}

	LOG_DEBUG(QString::fromUtf8("update file downloaded successfully"));

	QByteArray data = reply->readAll();
	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(data);
	QByteArray hash1 = hash.result().toHex();
	LOG_DEBUG(QString::fromUtf8("compare hash: %1 vs %2").arg(QLatin1String(hash1)).arg(QLatin1String(m_updateMd5)));
	if (hash1 != m_updateMd5) {
		LOG_DEBUG(QString::fromUtf8("hash does not match!!"));
		return;
	}

	QString title = applicationName();
	QString prompt = translateUIText(110050).arg(m_updateVer);
	QMessageBox mb(m_mainWnd);
	mb.setWindowTitle(title);
	mb.setWindowModality(Qt::WindowModal);
	mb.setText(prompt);
	QAbstractButton *btnOk = mb.addButton(translateUIText(110002), QMessageBox::AcceptRole);
	QAbstractButton *btnLater = mb.addButton(translateUIText(110001), QMessageBox::RejectRole);
	mb.setEscapeButton(btnLater);
	mb.exec();
	if (mb.clickedButton() == btnLater) {
		return;
	}

	QString s1 = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
	s1.append(QLatin1String("/lwrma_update_setup.exe"));
	QFile f(s1);
	if (f.open(QIODevice::WriteOnly)) {
		f.write(data);
		f.close();
	}

	system()->runUpdateSetup(m_mainWnd, s1);
}

void MiniApp::translateUI()
{
	m_quitAction->setText(translateUIText(110012));
}

void MiniApp::onJumpMenuAction()
{
	QAction *act = qobject_cast<QAction*>(sender());
	navigateTo(act->text(), QVariantMap());
}

void MiniApp::onLangMenuAction()
{
	QAction *act = qobject_cast<QAction*>(sender());
	setLanguage(act->text());
}

void MiniApp::cleanup()
{
	if (m_updateTimeOp) {
		m_updateTimeOp->abort();
		delete m_updateTimeOp;
	}

	unloadPage();

	if (m_mainPage) {
		m_mainPage->term();
		delete m_mainPage;
	}

	m_perfTimer.stop();

	if (m_mainWnd) {
		delete m_mainWnd;
	}

	if (m_uiLoader) {
		delete m_uiLoader;
	}

	if (m_svc) {
		delete m_svc;
	}

	if (m_bean) {
		delete m_bean;
	}
}

AppService *MiniApp::service() const
{
	return m_svc;
}

Bean *MiniApp::bean() const
{
	return m_bean;
}

SoapCore *MiniApp::soapCore() const
{
	return m_bean->soapCore();
}

System *MiniApp::system() const
{
	return m_bean->system();
}

QString MiniApp::translateUIText(int stringId, QWidget *widget)
{
	QString text;
	LangDict::const_iterator it = m_activeLangDict->find(stringId);
	if (it != m_activeLangDict->end()) {
		text = it.value();
	}
	return text;
}

void MiniApp::setLanguage(const QString& lang)
{
	if (lang.compare(QLatin1String("zh"), Qt::CaseInsensitive) == 0) {
		m_activeLangDict = &m_lang_zh;
	} else if (lang.compare(QLatin1String("en"), Qt::CaseInsensitive) == 0) {
		m_activeLangDict = &m_lang_en;
	}

	if (m_activePage) {
		m_activePage->translate();
	}

	if (m_mainPage) {
		m_mainPage->translate();
	}

	translateUI();
}

void MiniApp::closePage()
{
	unloadPage();
	QMetaObject::invokeMethod(this, "onNetworkConnectionChanged", Qt::QueuedConnection);
}

void MiniApp::navigateTo(const QString& pageName, const QVariantMap& params)
{
	if (m_activePage.isNull()) {
		navigateToSlot(pageName, params);
	} else {
		QMetaObject::invokeMethod(this, "navigateToSlot", Qt::QueuedConnection, Q_ARG(QString, pageName), Q_ARG(QVariantMap, params));
	}
}

AppPage *MiniApp::loadPage(const QString& pageName, const QVariantMap& params)
{
	//const char *srcRoot = DEV_SRC_ROOT;
	const char *srcRoot = ":";
	QString uiFileName = QString::fromUtf8("%1/ui/%2.ui").arg(QString::fromUtf8(srcRoot), pageName);
	QFile uiFile(uiFileName);
	if (!uiFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return NULL;
	}

	QString className = QString::fromUtf8("UI%1").arg(pageName);
	AppPage *newPage = m_uiLoader->loadAppPage(className, &uiFile, NULL);
	if (!newPage) {
		return NULL;
	}

	newPage->init(this, params);

	QList<QWidget*> widgetList = newPage->findChildren<QWidget*>();
	Q_FOREACH(QWidget *w, widgetList) {
		QVariant varUIMovie = w->property("UIMovie");
		if (varUIMovie.isValid()) {
			if (QLabel *label = qobject_cast<QLabel*>(w)) {
				QMovie *movie = new QMovie(QString::fromUtf8(":/images/%1.gif").arg(varUIMovie.toString()), "gif", newPage);
				qDebug("movie loopCount: %d", movie->loopCount());
				label->setMovie(movie);
				QPushButton *btn1 = new QPushButton();
				btn1->setProperty("UIClass", QLatin1String("moviePlayButton"));
				btn1->setVisible(false);
				btn1->setParent(label);
				btn1->resize(46, 46);
				btn1->move((label->width() - btn1->width()) / 2, (label->height() - btn1->height()) / 2);
				connect(btn1, SIGNAL(clicked()), newPage, SLOT(onMoviePlayButtonClicked()));
				connect(movie, SIGNAL(finished()), newPage, SLOT(onMovieFinished()));
				newPage->m_movie = movie;
				newPage->m_moviePlayButton = btn1;
				movie->start();
			}
		}

		QVariant varUIHide = w->property("UIHide");
		if (varUIHide.isValid() && varUIHide.toBool()) {
			w->setVisible(false);
		}
	}

	if (newPage->property("UIWaiting").toBool()) {
		QLabel *circleLabel = new QLabel();
		QMovie *circleMovie = new QMovie(QString::fromUtf8(":/images/loading.gif"), "gif", circleLabel);
		circleLabel->setMovie(circleMovie);
		circleLabel->setParent(newPage);
		circleLabel->setGeometry(264, 164, 72, 72);
		circleLabel->setVisible(true);
		circleMovie->start();
	}

	return newPage;
}

void MiniApp::setWifiName(const QString& wifiName)
{
	if (m_wifiName != wifiName) {
		m_wifiName = wifiName;
		emit wifiNameChanged();
	}
}

void MiniApp::setWifiSecurity(bool wifiSecurity)
{
	if (m_wifiSecurity != wifiSecurity) {
		m_wifiSecurity = wifiSecurity;
		emit wifiSecurityChanged();
	}
}

void MiniApp::setUptimeMinutes(int uptimeMinutes)
{
	if (m_uptimeMinutes != uptimeMinutes) {
		m_uptimeMinutes = uptimeMinutes;
		emit uptimeMinutesChanged();
	}
}

void MiniApp::generateWifiName()
{
	QString s;
	s.sprintf("lenovo%04d", qrand() % 9999);
	setWifiName(s);
}

void MiniApp::confirmWifiName()
{
	setWifiName(m_wlanProfileName);
}

void MiniApp::updateUptime()
{
	if (!m_updateTimeOp) {
		m_updateTimeOp = soapCore()->invoke(QString::fromUtf8("DeviceInfo"), QString::fromUtf8("GetSysUpTime"));
		connect(m_updateTimeOp, SIGNAL(finished()), SLOT(onUpdateUptimeFinished()));
	}
}

static bool parseUptimeString(const QString& s, int& seconds)
{
	QString t = s.trimmed();
	int len = t.length();
	if (len < 8) {
		return false;
	}

	QString p1 = t.left(len - 8);
	QString p2 = t.right(8);

	QStringList parts = p2.split(QLatin1String(":"));
	if (parts.count() != 3) {
		return false;
	}

	seconds = (parts[0].toInt() * 60 + parts[1].toInt()) * 60;

	int sep1 = p1.indexOf(QLatin1String("day"), 0, Qt::CaseInsensitive);
	if (sep1 > 0) {
		seconds += p1.left(sep1).toInt() * 24 * 60 * 60;
	}

	return true;
}

void MiniApp::onUpdateUptimeFinished()
{
	AsyncOp *op = m_updateTimeOp;
	m_updateTimeOp->deleteLater();
	m_updateTimeOp = NULL;

	if (op->result() == AsyncOp::NoError) {
		bool ok;
		if (op->value("responseCode").toInt(&ok) == 0 && ok) {
			QVariantMap resp = qvariant_cast<QVariantMap>(op->value("response"));
			QString uptimeStr = resp.value(QString::fromUtf8("SysUpTime")).toString();
			int uptimeSeconds;
			if (parseUptimeString(uptimeStr, uptimeSeconds)) {
				m_baseUptimeMinutes = uptimeSeconds / 60;
				setUptimeMinutes(m_baseUptimeMinutes);
			}
		}
	}
}

void MiniApp::toggleDebugFrame()
{
	if (m_mainWnd) {
		if (m_mainWnd->menuBar()->isVisible()) {
			m_mainWnd->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
			m_mainWnd->menuBar()->hide();
		} else {
			m_mainWnd->setWindowFlags(Qt::Window);
			m_mainWnd->menuBar()->show();
		}
		system()->markWindow(m_mainWnd);
		m_mainWnd->show();
	}
}

void MiniApp::navigateToSlot(const QString& pageName, const QVariantMap& params)
{
	AppPage *newPage = loadPage(pageName, params);
	if (!newPage->m_jumpPageName.isEmpty() && newPage->m_jumpPageName.compare(pageName) != 0) {
		QMetaObject::invokeMethod(this, "navigateToSlot", Qt::QueuedConnection, Q_ARG(QString, newPage->m_jumpPageName), Q_ARG(QVariantMap, newPage->m_jumpParams));
		delete newPage;
		return;
	}

	unloadPage();

	m_mainWnd->clientArea()->setNavPage(newPage);
	m_activePage = newPage;
}

void MiniApp::unloadPage()
{
	if (m_activePage) {
		m_activePage->term();
		m_mainWnd->clientArea()->setNavPage(NULL);
		delete m_activePage;
	}
}

void MiniApp::calcNetworkPerf()
{
	System::NetworkPerf prevPerf = m_perf;
	double prevPerfIn = m_perfIn;
	double prevPerfOut = m_perfOut;
	collectNetworkPerf();
	double dtIn = 0;
	double dtOut = 0;
	if (m_perfIn > prevPerfIn) {
		dtIn = m_perfIn - prevPerfIn;
	}
	if (m_perfOut > prevPerfOut) {
		dtOut = m_perfOut - prevPerfOut;
	}

	double dt = 1;
	qint64 dc = m_perf.perfCounter - prevPerf.perfCounter;
	if (dc > 0) {
		dt = static_cast<double>(dc) / static_cast<double>(m_perf.perfFreq);
	}

	dtIn /= dt;
	dtOut /= dt;

	if (m_incomingBPS != dtIn) {
		m_incomingBPS = dtIn;
		emit incomingBPSChanged();
	}

	if (m_outgoingBPS != dtOut) {
		m_outgoingBPS = dtOut;
		emit outgoingBPSChanged();
	}

	//qDebug("%f %f", dtIn, dtOut);
}

void MiniApp::onMainWindowMinimized()
{
//    serializeConfig(true);
	m_trayIcon.show();
	//m_mainWnd->hide();
	QTimer *t = new QTimer(this);
	t->setSingleShot(true);
	connect(t, SIGNAL(timeout()), SLOT(deferredHideWindow()));
	t->start(120);
}

void MiniApp::deferredHideWindow()
{
    //serializeConfig(true);
	m_mainWnd->hide();
	sender()->deleteLater();
}

void MiniApp::onAppInstanceActivated()
{
	if (!m_mainWnd->isVisible()) {
		restoreFromTray();
	}
}

void MiniApp::onQuitNow()
{
    serializeConfig(true);
	m_mainWnd->close();
}

void MiniApp::onBroadcastQuitEvent()
{
	system()->broadcastQuitEvent();
}

void MiniApp::onRunAs()
{
	system()->executeAs(m_mainWnd, applicationFilePath());
}

void MiniApp::startWlanService()
{
	AsyncOp *op = system()->startWlanService();
	if (op) {
		wait(op, 110001);
		delete op;
	}
}

void MiniApp::onWlanDiagnose()
{
}

void MiniApp::onReloadQss()
{
    DetailUiHelper::instance()->loadQSS();
}


void MiniApp::onMainPageCloseButtonClicked()
{
	if (m_minimizeOnClose) {
		m_mainWnd->setWindowState(Qt::WindowMinimized);

	} else {
		onQuitNow();
	}
}

void MiniApp::onQuestionButtonClicked()
{
	QString helpFilePath = applicationDirPath();
	helpFilePath.append(QLatin1String("/help.pdf"));
	QUrl url = QUrl::fromLocalFile(helpFilePath);
	QDesktopServices::openUrl(url);
}

void MiniApp::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger) {
		restoreFromTray();
	}
}

void MiniApp::restoreFromTray()
{
	m_mainWnd->showNormal();
	m_mainWnd->activateWindow();
	m_trayIcon.hide();
}

void MiniApp::collectNetworkPerf()
{
	system()->queryNetworkPerf(m_perf);
	m_perfIn = 0;
	m_perfOut = 0;
	for (int i = 0; i < m_perf.instances.count(); i++) {
		const System::NetworkPerfInstance& inst = m_perf.instances.at(i);
		m_perfIn += inst.inBytes;
		m_perfOut += inst.outBytes;
	}
}

double MiniApp::incomingBPS() const
{
	return m_incomingBPS;
}

double MiniApp::outgoingBPS() const
{
	return m_outgoingBPS;
}

int MiniApp::uptimeMinutes() const
{
	return m_uptimeMinutes;
}

QString MiniApp::wifiName() const
{
	return m_wifiName;
}

bool MiniApp::wifiSecurity() const
{
	return m_wifiSecurity;
}

AsyncOp *MiniApp::testRouter(int timeout)
{
	AsyncOp *op1 = soapCore()->invoke(QLatin1String("DeviceInfo"), QLatin1String("GetInfo"));
	return new TimeLimitedOp(op1, timeout);
}

void MiniApp::setWifiPassword(const QString& wifiPassword)
{
	m_wifiPassword = wifiPassword;
}

QString MiniApp::wifiPassword() const
{
	return m_wifiPassword;
}

QString MiniApp::versionString() const
{
	return applicationVersion();
}

void MiniApp::wait(AsyncOp *op, QObject *obj, const char *member)
{
	m_waitObj = obj;
	m_waitMethod = member;
	connect(op, SIGNAL(finished()), SLOT(onWaitOpFinished()));
	navigateToSlot(QLatin1String("AppWait"), QVariantMap());
}

void MiniApp::onWaitOpFinished()
{
	closePage();
	QMetaObject::invokeMethod(m_waitObj, m_waitMethod);
}

bool MiniApp::wait(AsyncOp *op, int stringId)
{
	AppWaiter waiter(this, stringId);
	return waiter.wait(op);
}

void MiniApp::enterWaitMode(AppWaiter *waiter)
{
	m_mainWnd->enterWaitMode(waiter);
}

void MiniApp::leaveWaitMode()
{
	m_mainWnd->leaveWaitMode();
}

void MiniApp::configSet(const QString& name, const QVariant& value)
{
	if (name.compare(QLatin1String("WlanProfileName"), Qt::CaseInsensitive) == 0) {
		m_wlanProfileName = value.toString();
	} else if (name.compare(QLatin1String("RouterMac"), Qt::CaseInsensitive) == 0) {
		m_routerMac = value.toString();
         serializeConfig(true);
	} else if (name.compare(QLatin1String("MinimizeOnClose"), Qt::CaseInsensitive) == 0) {
		m_minimizeOnClose = value.toBool();
         serializeConfig(true);
	} else if (name.compare(QLatin1String("AutoStart"), Qt::CaseInsensitive) == 0) {
        m_autoStart = value.toBool();
         serializeConfig(true);
	} else if (name.compare(QLatin1String("AutoUpdate"), Qt::CaseInsensitive) == 0) {

        bool oldAutoUpdate = m_autoUpdate;
		m_autoUpdate = value.toBool();
        serializeConfig(true);
		if (oldAutoUpdate != m_autoUpdate) {
			if (m_autoUpdate) {
				m_autoUpdateCheckTimer.start();
				checkForUpdate();
			} else {
				m_autoUpdateCheckTimer.stop();
			}
		}
	} else if (name.compare(QLatin1String("EnablePopup"), Qt::CaseInsensitive) == 0) {
        serializeConfig(true);
        m_enablePopup = value.toBool();
	}
}

QVariant MiniApp::configGet(const QString& name) const
{
	QVariant value;
	if (name.compare(QLatin1String("WlanProfileName"), Qt::CaseInsensitive) == 0) {
		value = m_wlanProfileName;
	} else if (name.compare(QLatin1String("RouterMac"), Qt::CaseInsensitive) == 0) {
		value = m_routerMac;
	} else if (name.compare(QLatin1String("MinimizeOnClose"), Qt::CaseInsensitive) == 0) {
		value = m_minimizeOnClose;
	} else if (name.compare(QLatin1String("AutoStart"), Qt::CaseInsensitive) == 0) {
		value = m_autoStart;
	} else if (name.compare(QLatin1String("AutoUpdate"), Qt::CaseInsensitive) == 0) {
		value = m_autoUpdate;
	} else if (name.compare(QLatin1String("EnablePopup"), Qt::CaseInsensitive) == 0) {
		value = m_enablePopup;
	}
	return value;
}

void MiniApp::showMessage(int stringId)
{
	QVariantMap params1;
	params1.insert(QString::fromUtf8("stringId"), stringId);
	navigateToSlot(QString::fromUtf8("AppMessage"), params1);
}

void MiniApp::beginUserMode()
{
	//updateUptime();
	connect(&m_userTimer1, SIGNAL(timeout()), SLOT(changeUptimeTimer()));
	m_userTimer2.start();
	m_userTimer1.start(1000);
	checkForUpdate();
}

bool MiniApp::validateWifiPassword(const QString& password) const
{
	int count = password.count();
	if (count < 8 || count > 64) {
		return false;
	}

	static const QString hexChars = QLatin1String("0123456789ABCDEFabcdef");

	if (count == 64) {
		for (int i = 0; i < count; i++) {
			QChar c = password.at(i);
			if (hexChars.indexOf(c) < 0) {
				return false;
			}
		}
	}

	return true;
}

void MiniApp::confirmNewWifiPassword(const QString& password)
{
	m_safeWifiPassword = password.isNull() ? QLatin1String("") : password;
}

QString MiniApp::savedWifiPassword() const
{
	return m_safeWifiPassword;
}

void MiniApp::changeUptimeTimer()
{
	setUptimeMinutes(m_baseUptimeMinutes + m_userTimer2.elapsed() / 60000);
}

void MiniApp::onNetworkConnectionChanged()
{
	if (m_bypassNetworkChange) {
        LOG_DEBUG(QString::fromUtf8("onNetworkConnectionChanged bypassed"));
		return;
	}
	if (!m_routerMac.isEmpty()) {
		if (m_disconnectedPrompt || !m_activePage) {
			bool checkResult = system()->checkRouter(m_routerMac);
			if (checkResult && m_disconnectedPrompt) {
                LOG_DEBUG(QString::fromUtf8("onNetworkConnectionChanged close prompt page"));
                m_userTimer2.restart();
                changeUptimeTimer();
				closePage();
				m_disconnectedPrompt = false;
			} else if (!checkResult && !m_disconnectedPrompt) {
                LOG_DEBUG(QString::fromUtf8("onNetworkConnectionChanged popup prompt page"));
                m_disconnectedPrompt = true;
				navigateTo(QString::fromUtf8("DisconnectPrompt"), QVariantMap());
            }else if(!checkResult && m_disconnectedPrompt)
            {
                LOG_DEBUG(QString::fromUtf8("checkResult false disconnectedPrompt true"));
            }else if(checkResult && !m_disconnectedPrompt)
            {
                LOG_DEBUG(QString::fromUtf8("checkResult true disconnectedPrompt false"));
            }
        } else {
            serializeConfig(true);
            LOG_DEBUG(QString::fromUtf8("onNetworkConnectionChanged blocked or already shown"));
        }
    } else {
        LOG_DEBUG(QString::fromUtf8("onNetworkConnectionChanged but m_routerMac.isEmpty()"));
    }
}

ClientArea::ClientArea()
{
	QWidget *layer0 = new QWidget(this);
	QHBoxLayout *layoutV0 = new QHBoxLayout();
	layoutV0->setMargin(0);
	layer0->setLayout(layoutV0);
	m_layout0 = new QVBoxLayout();
	m_layout0->setMargin(0);
	//layoutV0->addStretch();
	layoutV0->addLayout(m_layout0);
	//layoutV0->addStretch();

	QWidget *layer1 = new QWidget(this);
	QHBoxLayout *layoutV1 = new QHBoxLayout();
	layoutV1->setMargin(0);
	layer1->setLayout(layoutV1);
	QVBoxLayout *layoutA = new QVBoxLayout();
	layoutA->setMargin(0);
	layoutA->addStretch();
	m_layout1 = new QStackedLayout();
	layoutA->addLayout(m_layout1);
	layoutA->addStretch();
	layoutV1->addStretch();
	layoutV1->addLayout(layoutA);
	layoutV1->addStretch();

	m_rootLayout = new QStackedLayout();
	m_rootLayout->setStackingMode(QStackedLayout::StackAll);
	m_rootLayout->addWidget(layer0);
	m_rootLayout->addWidget(layer1);
	setLayout(m_rootLayout);
	setMinimumSize(960, 700);

	//m_rootLayout->setCurrentIndex(1);
}

#include <qpushbutton.h>

void ClientArea::setMainPage(AppPage *page)
{
	if (m_mainPage) {
		m_layout0->removeWidget(m_mainPage);
		delete m_mainPage;
	}
	m_mainPage = page;
	m_layout0->addWidget(m_mainPage);
}

void ClientArea::setNavPage(AppPage *page)
{
	AppPage *oldPage = m_navPage;
	m_navPage = page;
	if (m_navPage) {
		m_navPage->setStyleSheet(QLatin1String("AppPage { border-image:url(:/images/nav_bg.png) }"));
		m_navPage->setMinimumSize(600, 300);
		m_layout1->addWidget(m_navPage);
		m_rootLayout->setCurrentIndex(1);
	} else {
		m_rootLayout->setCurrentIndex(0);
	}

	if (oldPage) {
		m_layout1->removeWidget(oldPage);
		delete oldPage;
	}
}

MainWindow::MainWindow()
	: m_tracking(false)
{
	m_clientArea = new ClientArea();
	setCentralWidget(m_clientArea);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (m_waiter) {
		m_waiter->abort();
		e->ignore();
	}
}

void MainWindow::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::WindowStateChange) {
		QWindowStateChangeEvent *ev = static_cast<QWindowStateChangeEvent*>(e);
		if (!ev->oldState().testFlag(Qt::WindowMinimized) && windowState().testFlag(Qt::WindowMinimized)) {
			QMetaObject::invokeMethod(qApp, "onMainWindowMinimized", Qt::QueuedConnection);
		}
	}
	return QMainWindow::changeEvent(e);
}

bool MainWindow::winEvent(MSG *msg, long *result)
{
	if (MiniApp::instance()->system()->checkMessage(msg, result)) {
		QMetaObject::invokeMethod(qApp, "onAppInstanceActivated", Qt::QueuedConnection);
		return true;
	}
	return QMainWindow::winEvent(msg, result);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
	QMainWindow::mousePressEvent(e);
	if (!e->isAccepted()) {
		if (e->button() == Qt::LeftButton) {
			m_trackPos = e->globalPos() - pos();
			m_tracking = true;
			e->accept();
		}
	}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
	QMainWindow::mouseReleaseEvent(e);
	m_tracking = false;
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
	QMainWindow::mouseMoveEvent(e);
	if (!e->isAccepted()) {
		if (m_tracking) {
			move(e->globalPos() - m_trackPos);
			e->accept();
		}
	}
}

void MainWindow::enterWaitMode(AppWaiter *waiter)
{
	m_waiter = waiter;
	setEnabled(false);
}

void MainWindow::leaveWaitMode()
{
	m_waiter = NULL;
	setEnabled(true);
}

ClientArea *MainWindow::clientArea() const
{
	return m_clientArea;
}

AppWaiter::AppWaiter(MiniApp *app, int stringId)
	: m_app(app), m_stringId(stringId)
{
}

bool AppWaiter::wait(AsyncOp *op)
{
	if (op->isFinished()) {
		return true;
	}
	if (op->isAborted()) {
		return false;
	}

	m_aborted = false;
	connect(op, SIGNAL(finished()), SLOT(onOpFinished()));

	connect(&m_timer, SIGNAL(timeout()), &m_eventLoop1, SLOT(quit()));
	m_timer.start(200);
	m_eventLoop1.exec(QEventLoop::ExcludeUserInputEvents);
	if (op->isFinished()) {
		return true;
	}

	m_app->enterWaitMode(this);
	m_eventLoop2.exec();
	m_app->leaveWaitMode();
	if (m_aborted) {
		op->abort();
		return false;
	}
	return true;
}

void AppWaiter::onOpFinished()
{
	m_eventLoop1.quit();
	m_eventLoop2.quit();
}

void AppWaiter::abort()
{
	m_aborted = true;
	m_eventLoop1.quit();
	m_eventLoop2.quit();
}

TimeLimitedOp::TimeLimitedOp(AsyncOp *op, int timeout)
	: m_op(op)
{
	connect(&m_timer, SIGNAL(timeout()), SLOT(onTimeout()));
	m_timer.setSingleShot(true);
	m_timer.start(timeout);
	connect(m_op, SIGNAL(finished()), SLOT(onOpFinished()));
}

TimeLimitedOp::~TimeLimitedOp()
{
	if (m_op) {
		m_op->abort();
		delete m_op;
	}
}

void TimeLimitedOp::onTimeout()
{
	if (m_op) {
		m_op->abort();
	}
	setValue("timeout", true);
	notifyFinished(NoError);
}

void TimeLimitedOp::onOpFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;
	setValue("timeout", false);
	copyValues(op);
	notifyFinished(op->result());
}
