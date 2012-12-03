#ifndef __WlanCore_h__
#define __WlanCore_h__

#include <LenovoCore/Base.h>
#include <QtCore/QList>
#include <QtCore/QUuid>
#include <QtGui/QWidget>

class WlanCoreImpl;

class LENOVOCORE_API WlanCore
	: public QObject
{
	Q_OBJECT

public:
	WlanCore(QObject *parent = NULL);
	virtual ~WlanCore();
	WlanCoreImpl *d_ptr() const;
	bool init();

	bool softwareEnabled() const;
	bool hardwareEnabled() const;
	bool enabled() const;
	QString ssid() const;
	QByteArray macAddress() const;
	int signalQuality() const;
	QString profileName() const;
	bool connected() const;

	struct Network
	{
		QString ssid;
		int signalQuality;
		int bssType;
		bool securityEnabled;
		bool connected;
		QByteArray macAddress;
		QString profileName;
	};

	QList<Network> snapshotNetworkList() const;
	QString getProfileContent(const QString& profileName) const;
	void connectWithProfile(const QString& profileName, int bssType);
	void connectWithSsid(const QString& ssid, bool securityEnabled);
	void connectForce(const QString& ssid, const QString& profileName, bool securityEnabled);

Q_SIGNALS:
	void softwareEnabledChanged();
	void hardwareEnabledChanged();
	void enabledChanged();
	void ssidChanged();
	void macAddressChanged();
	void signalQualityChanged();
	void profileNameChanged();
	void connectedChanged();

private:
	friend class WlanCoreImpl;
	WlanCoreImpl *m_impl;
};

#endif // __WlanCore_h__
