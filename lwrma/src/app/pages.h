#ifndef __pages_h__
#define __pages_h__

#include "AppPage.h"
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QComboBox>

class UIAppWait
	: public AppPage
{
	Q_OBJECT
};

class UIAppMessage
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onOkButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
};

class UITestMain
	: public AppPage
{
	Q_OBJECT

Q_SIGNALS:
	void closeButtonClicked();

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();
	virtual void onUnload();

private Q_SLOTS:
	void updateSpeed();
	void updateTime();
	void updateName();
	void updateSecurity();
	void onTestButton1Clicked();
	void onTestButton2Clicked();
	void onTestButton3Clicked();
	void onChangePasswordFinished();

private:
	QPointer<QLabel> m_speedLabel;
	QPointer<QLabel> m_wifiNameLabel;
	QPointer<QLabel> m_wifiSecurityLabel;
	QPointer<QLabel> m_uptimeLabel;
	QPointer<AsyncOp> m_op;
};

class UINewRouterConfirm
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onYesButtonClicked();
	void onNoButtonClicked();
};

class UIInstallConfirm
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onYesButtonClicked();
	void onNoButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
};

class UIPurchaseInfo
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
};

class UIRouterSetupWelcome
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void keyPressEvent(QKeyEvent *e);

private Q_SLOTS:
	void onNextButtonClicked();
	void onBackButtonClicked();

private:
	QPointer<QAbstractButton> m_backButton;
};

class UIRouterSetupDetect
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();

private Q_SLOTS:
	void onSearchSsidFinished();
	void onConnectSsidFinished();
	void onDiscoveryFinished();
	void onDiscoveryLanFinished();
	void onChangeWifiNameFinished();
	void onCreateProfileFinished();
	void onReconnectFinished();

private:
	void searchSsid();
	void connectSsid();
	void discoverSoap();
	void connectLan();
	void changeWifiName();
	void createProfile();
	void lockDownRouter(bool lan, const QString& mac, const QString& host, const QVariantMap& info);

private:
	QPointer<AsyncOp> m_op;
	int m_radioRetryCount;
	int m_retryCount;
	int m_powerRetryCount;
	QVariantMap m_ssid;
	bool m_lanMode;
	QString m_targetMac;
	QString m_targetMac1;
};

class UILanSetupConnectPC
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UILanSetupConnectRouter
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UILanSetupPowerOn
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UIWlanServiceGuide
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();
	virtual void onTranslate();

private Q_SLOTS:
	void onNextButtonClicked();
	void onStartWlanServiceFinished();

private:
	QPointer<QLabel> m_promptLabel;
	QPointer<QAbstractButton> m_nextButton;
	QPointer<AsyncOp> m_op;
};

class UIWlanRadioGuide
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onNextButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
	int m_retryCount;
};

class UIWlanMultipleRouterSelect
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UIRouterPowerGuide1
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UIRouterPowerGuide2
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UISetSSID : public AppPage
{
	Q_OBJECT
protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();
private Q_SLOTS:
		void onComboxChanged(QString &name);
		void onNextButtonClicked();
private:
	QPointer<QAbstractButton> m_quitButton;
	QPointer<QLabel> m_promptLabel;
	QPointer<QLabel> m_modLabel;
//	QPointer<QLineEdit> m_SSIDEdit;
	QPointer<QLineEdit> m_SSIDNewName;
//	QPointer<QComboBox> m_SSIDList;

};

class UIReConnectWifi: public AppPage
{
	Q_OBJECT
protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();
	private Q_SLOTS:
		void onComboxChanged(QString &name);
		void onNextButtonClicked();
private:
	QPointer<QAbstractButton> m_quitButton;
	QPointer<QLabel> m_promptLabel;
	QPointer<QLabel> m_SSIDLabel;
	QPointer<QLabel> m_PWDLabel;
	QPointer<QLineEdit> m_SSIDEdit;
	QPointer<QComboBox> m_SSIDList;
	QPointer<QLineEdit> m_PasswordEdit;


};

class UICallHelp
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onQuitButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
	QPointer<QAbstractButton> m_quitButton;
	bool m_stage2;
};

class UIDisconnectPrompt
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();
	virtual void onTranslate();

private Q_SLOTS:
	void onReconnectButtonClicked();
	void onConnectFinished();

private:
	QPointer<AsyncOp> m_op;
	QPointer<QAbstractButton> m_quitButton;
	QPointer<QAbstractButton> m_reconnectButton;
	QPointer<QLabel> m_loadingLabel;
	QPointer<QLabel> m_promptLabel;
	QPointer<QLabel> m_SSIDLabel;
	QPointer<QLabel> m_PWDLabel;
	QPointer<QLineEdit> m_SSIDEdit;
	QPointer<QLineEdit> m_PasswordEdit;
	int m_promptId;
};

class UIRouterWanDetect
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();

private Q_SLOTS:
	void onConfigStarted();
	void onSmartDetectionFinished();
    void onSmartDetectionTimeout();

private:
    void startSmartDetection();

private:
	QPointer<AsyncOp> m_op;
	int m_routerWanGuide;
    QTimer m_smartDetectionTimer;
    int m_smartDetectionRetryCount;
};

class UIWanSave
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();

private Q_SLOTS:
	void onSaveFinished();
	void onConfigFinished();
	void onReconnectFinished();
	void onCheckInternetFinished();
	void onTimeout1();
	void onRestartRouterFinished();
	void onTimeout2();

private:
	QPointer<AsyncOp> m_op;
	QTimer m_timer1;
};

class UIWanSelect
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onDhcpButtonClicked();
	void onStaticButtonClicked();
	void onDialButtonClicked();
};

class UIWanStatic
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();

private:
	QPointer<QLineEdit> m_ipEdit;
	QPointer<QLineEdit> m_maskEdit;
	QPointer<QLineEdit> m_gatewayEdit;
	QPointer<QLineEdit> m_dns1Edit;
	QPointer<QLineEdit> m_dns2Edit;
};

class UIWanDial
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onNextButtonClicked();
    void onEditButtonClicked();
	void updateUIState();

private:
	QPointer<QLineEdit> m_accountEdit;
	QPointer<QLineEdit> m_passwordEdit;
	QPointer<QAbstractButton> m_nextButton;
    QPointer<QAbstractButton> m_editButton;
    QPointer<QLabel> m_promptLabel;
	int m_textId;
};

class UIRouterWanGuide1
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UIRouterWanGuide2
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNextButtonClicked();
};

class UIWlanSetupGuide
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onNowButtonClicked();
	void onLaterButtonClicked();
};

class UIRouterSetupComplete
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onOkButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
};

class UIRouterWlanSecurity
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();
	virtual void keyPressEvent(QKeyEvent *e);

private Q_SLOTS:
	void onOkButtonClicked();
	void onBackButtonClicked();
	void updateUIState();

private:
	QPointer<QAbstractButton> m_okButton;
	QPointer<QAbstractButton> m_backButton;
	QPointer<QLineEdit> m_passwordEdit1;
	QPointer<QLineEdit> m_passwordEdit2;
	QPointer<AsyncOp> m_op;
};

class UIRouterWlanSave
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();

private Q_SLOTS:
	void onChangeWifiFinished();
	void onSaveProfileFinished();
	void onReconnectFinished();

private:
	QPointer<AsyncOp> m_op;
};

class UIRouterDetect
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();
	virtual void onUnload();

private Q_SLOTS:
	void onDiscoveryFinished();
	void onReconnectFinished();
	void onCreateProfileFinished();

private:
	QPointer<AsyncOp> m_op;
	QPointer<QLabel> m_promptLabel;
};

class UIRouterDetectResult
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onYesButtonClicked();
	void onNoButtonClicked();

private:
	QPointer<QLabel> m_promptLabel;
};

class UIWlanInvalidPassword
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void keyPressEvent(QKeyEvent *e);

private Q_SLOTS:
	void onOkButtonClicked();

private:
	QPointer<QAbstractButton> m_okButton;
};

#endif // __pages_h__
