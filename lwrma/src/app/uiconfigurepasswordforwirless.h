#ifndef UICONFIGUREPASSWORDFORWIRLESS_H
#define UICONFIGUREPASSWORDFORWIRLESS_H

#include "AppPage.h"
#include <QtGui/QLabel>
#include <QtGui/QPushButton>


class UIConfigurePasswordForWirless
    :public AppPage
{
    Q_OBJECT

protected:
        virtual void onLoad(const QVariantMap& params);
        virtual void onTranslate();

private:
        QPointer<QLabel> label;

private Q_SLOTS:
        void onConfigButtonClicked();
        void onCancelButtonClicked();
};


#endif // UICONFIGUREPASSWORDFORWIRLESS_H
