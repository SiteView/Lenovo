#include "uiconfigureispip.h"
void UIConfigureISPIP::onLoad(const QVariantMap& params)
{
        QPushButton *nextbtn = findChild<QPushButton*>(QString::fromUtf8("NextButton"));
        connect(nextbtn, SIGNAL(clicked()), SLOT(onNextBtClicked()));
}

void UIConfigureISPIP::onNextBtClicked()
{
    app()->navigateTo(QString::fromUtf8("ConfigureWireless"));
}

APP_REGISTER_PAGE(UIConfigureISPIP)
