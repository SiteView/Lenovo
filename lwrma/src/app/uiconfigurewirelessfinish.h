#ifndef UICONFIGUREWIRELESSFINISH_H
#define UICONFIGUREWIRELESSFINISH_H

#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

class UIConfigureWirelessFinish
    :public AppPage
{
    Q_OBJECT

protected:
        virtual void onLoad(const QVariantMap& params);
        virtual void onTranslate();
private:
        QPointer<QLabel> label;

private Q_SLOTS:
        void onFinishButtonClicked();
};

#endif // UICONFIGUREWIRELESSFINISH_H
