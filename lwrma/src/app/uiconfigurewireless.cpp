#include "uiconfigurewireless.h"
void UIConfigureWireless::onLoad(const QVariantMap& params)
{
        QPushButton *configbtn = findChild<QPushButton*>(QString::fromUtf8("ConfigButton"));
        QPushButton *laterbtn = findChild<QPushButton*>(QString::fromUtf8("LaterButton"));
        connect(configbtn, SIGNAL(clicked()), SLOT(onConfigBtClicked()));
        connect(configbtn, SIGNAL(clicked()), SLOT(onLaterBtClicked()));
}

void UIConfigureWireless::onConfigBtClicked()
{
    app()->navigateTo(QString::fromUtf8("ConfigurePasswordForWirless"));
}

void UIConfigureWireless::onLaterBtClicked()
{

}

APP_REGISTER_PAGE(UIConfigureWireless)
