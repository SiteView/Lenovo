#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "AppPage.h"
#include <LenovoCore/AsyncOp.h>
#include <QtGui>

class UIMainFrame:public AppPage
{
    Q_OBJECT
Q_SIGNALS:
    void closeButtonClicked();
    void questionButtonClicked();
public:
//    UiMainFrame();
protected:
    virtual void onTranslate();
    virtual void onLoad(const QVariantMap& params);
    virtual void onUnload();
    void getLables(QList<QLineEdit *> &lab_lst, QStringList *text_lst=NULL);
    void getSettingButtons(QList<QPushButton *> &but_lst);
    void initSettingPage();
    void setSettings();
protected:
    QStackedWidget *mStack;
    QPointer<AsyncOp> m_op;
	QString m_activeWifiPassword;
protected slots:
    void slot_butClicked();
    void slot_OpReturned();
private Q_SLOTS:
    void updateData();

//    void updateTime();
//    void updateName();
//    void updateSecurity();
};

#endif // MAINFRAME_H
