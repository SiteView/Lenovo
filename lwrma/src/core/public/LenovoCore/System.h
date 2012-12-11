#ifndef __System_h__
#define __System_h__

#include <LenovoCore/Base.h>
#include <QtCore/QList>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>
#include <QtGui/QIcon>
#include <QtGui/QWidget>

class AsyncOp;
class SystemImpl;

class LENOVOCORE_API System
	: public QObject
{
	Q_OBJECT

public:
	System(const QString& ident, QObject *parent = NULL);
	virtual ~System();
	SystemImpl *d_ptr() const;

	static bool runCommand(int *retval);

	bool init();

	bool checkInstance(QWidget *mainWindow);
	bool checkMessage(MSG *msg, long *result);
	void executeAs(QWidget *mainWindow, const QString& path);
	void broadcastQuitEvent();
	void startQuitDog();
	void stopQuitDog();

	void markWindow(QWidget *mainWindow);

	AsyncOp *startWlanService();
	AsyncOp *searchSsid(const QString& ssid);
	AsyncOp *connectSsid(const QVariantMap& ssid);
	AsyncOp *httpGet(const QString& url);
	AsyncOp *createWlanProfile(const QString& ssid, const QString& password);
	AsyncOp *connectWlanProfile(const QString& profile, bool reconnect);

	bool needsElevation() const;
	bool wlanServiceActive() const;
	QUuid wlanInterfaceUuid() const;
	QString wlanInterfaceDescription() const;

	QStringList queryGatewayList() const;

	void updateWlanState();

	struct NetworkPerfInstance
	{
		QString name;
		quint64 inBytes;
		quint64 outBytes;
	};

	struct NetworkPerf
	{
		qint64 perfCounter;
		qint64 perfFreq;
		QList<NetworkPerfInstance> instances;
	};

	void queryNetworkPerf(NetworkPerf& perf);

	QIcon loadAppIcon(int name, int cx, int cy);
	void runUpdateSetup(QWidget *mainWindow, const QString& path);

	bool checkRouter(const QString& mac);
    bool checkRouter2(const QString& ssidName);

Q_SIGNALS:
	void quitNow();
	void wlanServiceActiveChanged();
	void wlanInterfaceChanged();
	void networkConnectionChanged();
    void systemResumed();
    void systemSleep();
    void systemwakeup();

private:
	friend class SystemImpl;
	SystemImpl *m_impl;
};

Q_DECLARE_METATYPE(System*)

#endif // __System_h__
