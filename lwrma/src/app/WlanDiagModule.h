#ifndef __WlanDiagModule_h__
#define __WlanDiagModule_h__

#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

class UIWlanDiagEnableWifi
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
	void onButtonNextClicked();
};

class UIWlanDiagEnableService
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onButtonNextClicked();

private:
	int retryCount() const;

private:
	QPointer<QLabel> m_labelPrompt;
};

class UIWlanDiagLanSetup
	: public AppPage
{
	Q_OBJECT

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onTranslate();

private Q_SLOTS:
	void onButtonNextClicked();

private:

private:
	QPointer<QLabel> m_labelPrompt;
};

#endif // __WlanDiagModule_h__
