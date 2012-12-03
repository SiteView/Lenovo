#include "uiconfigurewirelessfinish.h"

void UIConfigureWirelessFinish::onLoad(const QVariantMap& params)
{

        label = findChild<QLabel*>(QString::fromUtf8("label"));
        QPushButton *finishbtn = findChild<QPushButton*>(QString::fromUtf8("FinishButton"));
        connect(finishbtn, SIGNAL(clicked()), SLOT(onFinishButtonClicked()));
}

void UIConfigureWirelessFinish::onFinishButtonClicked()
{

}

void UIConfigureWirelessFinish::onTranslate()
{
    label->setText(app()->translateUIText(130012).arg(QString::fromUtf8("Lenovo5678")));
}

APP_REGISTER_PAGE(UIConfigureWirelessFinish)
