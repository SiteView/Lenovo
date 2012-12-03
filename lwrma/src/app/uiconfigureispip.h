#ifndef UICONFIGUREISPIP_H
#define UICONFIGUREISPIP_H

#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

class UIConfigureISPIP
        : public AppPage
{
        Q_OBJECT

protected:
        virtual void onLoad(const QVariantMap& params);

private Q_SLOTS:
        void onNextBtClicked();
};


#endif // UICONFIGUREISPIP_H
