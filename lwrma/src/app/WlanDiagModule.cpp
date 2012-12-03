#include "WlanDiagModule.h"
#include <LenovoCore/LenovoCore.h>

/*
** UIWlanDiagEnableWifi
*/

void UIWlanDiagEnableWifi::onLoad(const QVariantMap& params)
{
	System *system = app()->system();
	system->updateWlanState();
	if (!system->wlanServiceActive()) {
		QVariantMap newParams;
		newParams[QString::fromUtf8("nextPage")] = QString::fromUtf8("WlanDiagEnableWifi");
		return jumpTo(QString::fromUtf8("WlanDiagEnableService"), newParams);
	}

	QPushButton *buttonNext = findChild<QPushButton*>(QString::fromUtf8("buttonNext"));
	connect(buttonNext, SIGNAL(clicked()), SLOT(onButtonNextClicked()));
}

void UIWlanDiagEnableWifi::onButtonNextClicked()
{
//	app()->system()->updateWlanRadioState();
}

/*
** UIWlanDiagEnableService
*/

void UIWlanDiagEnableService::onLoad(const QVariantMap& params)
{
	m_labelPrompt = findChild<QLabel*>(QString::fromUtf8("labelPrompt"));
	QPushButton *buttonNext = findChild<QPushButton*>(QString::fromUtf8("buttonNext"));
	connect(buttonNext, SIGNAL(clicked()), SLOT(onButtonNextClicked()));
}

void UIWlanDiagEnableService::onTranslate()
{
	m_labelPrompt->setText(app()->translateUIText(retryCount() == 0 ? 110102 : 110103));
}

void UIWlanDiagEnableService::onButtonNextClicked()
{
	System *system = app()->system();
	AsyncOp *op = system->startWlanService();
	if (op) {
		app()->wait(op);
		delete op;
	}

	system->updateWlanState();
	if (system->wlanServiceActive()) {
		QVariant varNextPage = params().value(QString::fromUtf8("nextPage"));
		if (varNextPage.isValid()) {
			app()->navigateTo(varNextPage.toString(), qvariant_cast<QVariantMap>(params().value(QString::fromUtf8("nextPageParams"))));
		}
	} else {
		int rc = retryCount();
		if (rc > 0) {
			app()->navigateTo(QString::fromUtf8("WlanDiagLanSetup"));
		} else {
			QVariantMap newParams = params();
			newParams[QString::fromUtf8("retryCount")] = rc + 1;
			app()->navigateTo(QString::fromUtf8("WlanDiagEnableService"), newParams);
		}
	}
}

int UIWlanDiagEnableService::retryCount() const
{
	return params().value(QString::fromUtf8("retryCount"), 0).toInt();
}

/*
** UIWlanDiagLanSetup
*/

void UIWlanDiagLanSetup::onLoad(const QVariantMap& params)
{
	m_labelPrompt = findChild<QLabel*>(QString::fromUtf8("labelPrompt"));
	QPushButton *buttonNext = findChild<QPushButton*>(QString::fromUtf8("buttonNext"));
	connect(buttonNext, SIGNAL(clicked()), SLOT(onButtonNextClicked()));
}

void UIWlanDiagLanSetup::onTranslate()
{
	//m_labelPrompt->setText(app()->translateUIText(retryCount() == 0 ? 110102 : 110103));
}

void UIWlanDiagLanSetup::onButtonNextClicked()
{
/*	System *system = app()->system();
	AsyncOp *op = system->startWlanService();
	if (op) {
		app()->wait(op);
		delete op;
	}

	system->updateWlanState();
	if (system->wlanServiceActive()) {
		QVariant varNextPage = params().value(QString::fromUtf8("nextPage"));
		if (varNextPage.isValid()) {
			app()->navigateTo(varNextPage.toString(), qvariant_cast<QVariantMap>(params().value(QString::fromUtf8("nextPageParams"))));
		}
	} else {
		QVariantMap newParams = params();
		newParams[QString::fromUtf8("retryCount")] = retryCount() + 1;
		app()->navigateTo(QString::fromUtf8("WlanDiagEnableService"), newParams);
	}*/
}

APP_REGISTER_PAGE(UIWlanDiagEnableWifi)
APP_REGISTER_PAGE(UIWlanDiagEnableService)
APP_REGISTER_PAGE(UIWlanDiagLanSetup)
