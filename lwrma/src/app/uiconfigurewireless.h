#ifndef UICONFIGUREWIRELESS_H
#define UICONFIGUREWIRELESS_H
#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

class UIConfigureWireless
    :public AppPage
{
    Q_OBJECT

protected:
        virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
        void onConfigBtClicked();
        void onLaterBtClicked();
};


#endif // UICONFIGUREWIRELESS_H
