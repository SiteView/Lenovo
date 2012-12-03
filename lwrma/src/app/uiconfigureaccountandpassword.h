#ifndef UICONFIGUREACCOUNTANDPASSWORD_H
#define UICONFIGUREACCOUNTANDPASSWORD_H

#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>


class UIConfigureAccountAndPassword
        : public AppPage
{
        Q_OBJECT

protected:
        virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
        void onOkBtClicked();
};


#endif // UICONFIGUREACCOUNTANDPASSWORD_H
