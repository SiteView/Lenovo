#include "uiconfigurepasswordforwirless.h"

void UIConfigurePasswordForWirless::onLoad(const QVariantMap& params)
{
        label = findChild<QLabel*>(QString::fromUtf8("label"));
        QPushButton *configbtn = findChild<QPushButton*>(QString::fromUtf8("OKButton"));
        QPushButton *cancelbtn = findChild<QPushButton*>(QString::fromUtf8("CancelButton"));
        connect(configbtn, SIGNAL(clicked()), SLOT(onConfigButtonClicked()));
        connect(cancelbtn, SIGNAL(clicked()), SLOT(onCancelButtonClicked()));
}

void UIConfigurePasswordForWirless::onConfigButtonClicked()
{
    app()->navigateTo(QString::fromUtf8("ConfigureWirelessFinish"));
}

void UIConfigurePasswordForWirless::onCancelButtonClicked()
{
    app()->navigateTo(QString::fromUtf8("ConfigureWireless"));
}

void UIConfigurePasswordForWirless::onTranslate()
{
    label->setText(app()->translateUIText(130008).arg(QString::fromUtf8("Lenovo5678")));
}
APP_REGISTER_PAGE(UIConfigurePasswordForWirless)
