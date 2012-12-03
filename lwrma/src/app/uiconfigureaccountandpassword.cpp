#include "uiconfigureaccountandpassword.h"


void UIConfigureAccountAndPassword::onLoad(const QVariantMap& params)
{
        QPushButton *okbtn = findChild<QPushButton*>(QString::fromUtf8("OkButton"));
        connect(okbtn, SIGNAL(clicked()), SLOT(onOkBtClicked()));
}

void UIConfigureAccountAndPassword::onOkBtClicked()
{
    app()->navigateTo(QString::fromUtf8("ConfigureWireless"));
}

APP_REGISTER_PAGE(UIConfigureAccountAndPassword)
