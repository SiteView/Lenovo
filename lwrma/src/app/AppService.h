#ifndef __AppService_h__
#define __AppService_h__

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

class AsyncOp;
class Bean;
class MiniApp;
class QWidget;
class SoapCore;
class System;
class WlanCore;

class AppService
	: public QObject
{
	Q_OBJECT

public:
	AppService(MiniApp *app);
	virtual ~AppService();
	SoapCore *soapCore() const;
	System *system() const;
	Bean *bean() const;
	QString translateUIText(int stringId);
	QString translateUIText(int stringId, QWidget *widget);
	bool wait(AsyncOp *op);
	bool wait(AsyncOp *op, int stringId);
	AsyncOp *testRouter(int timeout);
	void setWifiPassword(const QString& wifiPassword);
	QString wifiPassword() const;
	void generateWifiName();
	void SetWifiName(QString WifiName);

	void confirmWifiName();
	void setWifiSecurity(bool wifiSecurity);
	void confirmNewWifiPassword(const QString& password);
	QString savedWifiPassword() const;

	double incomingBPS() const;
	double outgoingBPS() const;
	int uptimeMinutes() const;
	QString wifiName() const;
	bool wifiSecurity() const;

	bool validateWifiPassword(const QString& password) const;

	QString versionString() const;
	void wait(AsyncOp *op, QObject *obj, const char *member);

	void configSet(const QString& name, const QVariant& value);
	QVariant configGet(const QString& name) const;

	void showMessage(int stringId);

Q_SIGNALS:
	void incomingBPSChanged();
	void outgoingBPSChanged();
	void uptimeMinutesChanged();
	void wifiNameChanged();
	void wifiSecurityChanged();

public Q_SLOTS:
	void navigateTo(const QString& pageName);
	void navigateTo(const QString& pageName, const QVariantMap& params);
	void setLanguage(const QString& lang);
	void closePage();
	void quit();
	void beginUserMode();

private:
	MiniApp *m_app;
};

#endif // __AppService_h__
