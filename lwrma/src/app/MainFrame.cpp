#include "MainFrame.h"
#include <QtGui>
#include "DetailUiHelper.h"
#include <LenovoCore/Bean.h>
//UiMainFrame::UiMainFrame()
//{
//}

enum DETAIL_UI_FLAGS
{
    DUF_KEPRESSED=(1<<0)
};
enum LABLE_TYPE
{
    LT_OLD_ADMIN_PWD,LT_NEW_ADMIN_PWD,LT_NEW_ADMIN_PWD_CONFIRM
    ,LT_NEW_WIFI_PWD,LT_NEW_WIFI_PWD_CONFIRM,LT_NUM
};
static quint32 sFlags=0;

void UIMainFrame::onTranslate()
{
    //QLabel *labelLinkTime=findChild<QLabel *>(QString::fromUtf8("labelLinkTime"));
    //QLabel *labelRouter=findChild<QLabel *>(QString::fromUtf8("labelRouter"));
    QLabel *labelVersion=findChild<QLabel *>(QString::fromUtf8("labelVersion"));
    //QString linktime=app()->translateUIText(120011).arg(11).arg(22).arg(33).arg(44);
    QString version=app()->translateUIText(120019).arg(app()->versionString());
    //QString router=app()->translateUIText(120010).arg(QString::fromUtf8("lenovo5678"));

 /*   labelLinkTime->setText(linktime);
    labelRouter->setText(router);*/
    labelVersion->setText(version);
    DetailUiHelper::instance()->mSwitchObject->updateLan();
    update();

}


void UIMainFrame::onLoad(const QVariantMap &params)
{
    QList<QPushButton *> bts=qFindChildren<QPushButton *>(this);
    int c=bts.count();
    for(int i=0;i<c;i++)
    {
        connect(bts[i],SIGNAL(clicked()),this,SLOT(slot_butClicked()));
    }
    mStack=qFindChild<QStackedWidget *>(this,QString::fromUtf8("stackedWidget"));
    mStack->setCurrentIndex(0);
    QObject *hbo=DetailUiHelper::instance()->mHeadButtonObject;
    const char *cParas[]={"buttonRouterState","buttonPwdSeting","buttonSoftWareSetting"};
    QButtonGroup *&group=DetailUiHelper::instance()->mHeadButtonObject->mButGroup;
    group=new QButtonGroup(this);
    for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
    {
        QPushButton *w=qFindChild<QPushButton *>(this,QString::fromUtf8(cParas[i]));
        Q_ASSERT(w);
        w->installEventFilter(hbo);
        group->addButton(w);
        group->setId(w,i);
    }

	QStackedWidget *stack=qFindChild<QStackedWidget *>(this,QString::fromUtf8("stackedWidget"));
    QObject *co=DetailUiHelper::instance()->mContaintObject;
    connect(group,SIGNAL(buttonClicked(int)),stack,SLOT(update()));
    stack->installEventFilter(co);

    QLabel *lab_data=qFindChild<QLabel *>(this,QString::fromUtf8("lab_data"));
    Q_ASSERT(lab_data);
    QObject *lo=DetailUiHelper::instance()->mLabelObject;
    lab_data->installEventFilter(lo);

    const char *cBRButtons[]={"buttonInstallNew","buttonGotoAdvance","buttonYes"
                              ,"buttonNo","buttonYes3","buttonNo3"
                              ,"buttonExit","buttonAttachDevice","buttonQuestion"};
    QObject *bro=DetailUiHelper::instance()->mBottomRightButtonObject;
    for(int i=0;i<sizeof(cBRButtons)/sizeof(cBRButtons[0]);i++)
    {
        QPushButton *but=qFindChild<QPushButton *>(this,QString::fromUtf8(cBRButtons[i]));
        Q_ASSERT(but);
        but->installEventFilter(bro);
    }

    const char *cSwitchs[]={"switch1","switch2","switch3"/*,"switch4"*/};
    QObject *swo=DetailUiHelper::instance()->mSwitchObject;
    for(int i=0;i<sizeof(cSwitchs)/sizeof(cSwitchs[0]);i++)
    {
        QPushButton *but=qFindChild<QPushButton*>(this,QString::fromUtf8(cSwitchs[i]));
        Q_ASSERT(but);
        but->installEventFilter(swo);

    }

    DetailUiHelper::instance()->mSwitchObject->updateLan();

    this->setStyleSheet(QString::fromUtf8(".UIMainFrame{background-image: url(:/images/bg.png);}"));

    connect(app(), SIGNAL(incomingBPSChanged()), SLOT(updateData()));
    connect(app(), SIGNAL(outgoingBPSChanged()), SLOT(updateData()));
    connect(app(), SIGNAL(uptimeMinutesChanged()), SLOT(updateData()));
    connect(app(), SIGNAL(wifiNameChanged()), SLOT(updateData()));
    connect(app(), SIGNAL(wifiSecurityChanged()), SLOT(updateData()));

    initSettingPage();
}

void UIMainFrame::onUnload()
{
    qDebug()<<"UIMainFrame::onUnload";
}

void UIMainFrame::getLables(QList<QLineEdit *> &edt_lst,QStringList * text_lst)
{
    edt_lst.clear();
    const char *cParas[LT_NUM]={"edt_1","edt_2","edt_3","edt_4","edt_5"
                                /*,"edt_6","edt_7"*/
                               };
    QWidget *w=mStack->widget(1);
    for(int i=0;i<LT_NUM;i++)
    {
        QLineEdit *edt=qFindChild<QLineEdit*>(w,QString::fromUtf8(cParas[i]));
        Q_ASSERT(edt);
        edt_lst<<edt;
        if(text_lst)
        {
            (*text_lst)<<(edt->text().isEmpty()?QString():edt->text());
        }
    }
}

void UIMainFrame::getSettingButtons(QList<QPushButton *> &but_lst)
{
    but_lst.clear();
    const char *cParas[]={"switch1","switch2","switch3"/*,"switch4"*/};
    QWidget *w=mStack->widget(2);
    for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
    {
        QPushButton *but=qFindChild<QPushButton*>(w,QString::fromUtf8(cParas[i]));
        Q_ASSERT(but);
        but_lst<<but;
    }
}

const char *cSettingParas[]={"MinimizeOnClose","AutoStart","AutoUpdate"/*,"EnablePopup"*/};
void UIMainFrame::initSettingPage()
{
    QList<QPushButton*> but_lst;
    getSettingButtons(but_lst);
     int c=but_lst.count();
     Q_ASSERT(c==sizeof(cSettingParas)/sizeof(cSettingParas[0]));
    for(int i=0;i<c;i++)
    {
        bool b=app()->configGet(QString::fromUtf8(cSettingParas[i])).toBool();
        but_lst[i]->setChecked(b);
    }
}

void UIMainFrame::setSettings()
{
    QList<QPushButton*> but_lst;
    getSettingButtons(but_lst);
    int c=but_lst.count();
    Q_ASSERT(c==sizeof(cSettingParas)/sizeof(cSettingParas[0]));
    for(int i=0;i<c;i++)
    {
        bool b=but_lst[i]->isChecked();
        app()->configSet(QString::fromUtf8(cSettingParas[i]),b);
    }
}

void UIMainFrame::slot_butClicked()
{
    QPushButton *but=qobject_cast<QPushButton *>(sender());
    Q_ASSERT(but);
    QString obj_name=but->objectName();

    const struct _{const char *but;int page;} cParas[]={
        {"buttonRouterState",0}
        ,{"buttonPwdSeting",1}
        ,{"buttonSoftWareSetting",2}
        ,{"buttonBlackList",3}
    };
    for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
    {
        if(obj_name==QString::fromUtf8(cParas[i].but))
        {
            mStack->setCurrentIndex(cParas[i].page);
            return;
        }
    }
    if(obj_name==QString::fromUtf8("buttonExit"))
    {
        emit closeButtonClicked();
    }
    else if(obj_name==QString::fromUtf8("buttonQuestion"))
    {
        emit questionButtonClicked();
    }
    else if(obj_name==QString::fromUtf8("buttonYes"))
    {
        QList<QLineEdit *> edt_lst;
        QStringList text_lst;
        getLables(edt_lst,&text_lst);

        bool haveText=false;
        for(int i=0;i<LT_NUM;i++)
        {
            if(!text_lst[i].isEmpty())
            {
                haveText=true;
                break;
            }
        }

        if(!haveText)
        {
            //todo message "No Data To Set"
            app()->showMessage(120031);
            return;
        }

        if((text_lst[LT_NEW_WIFI_PWD]!=text_lst[LT_NEW_WIFI_PWD_CONFIRM])
                ||(text_lst[LT_NEW_ADMIN_PWD]!=text_lst[LT_NEW_ADMIN_PWD_CONFIRM])
                )
        {
            //todo message "pwd not the same"
            app()->showMessage(120032);
            return;
        }
        if(!text_lst[LT_NEW_WIFI_PWD].isEmpty() && !app()->validateWifiPassword(text_lst[LT_NEW_WIFI_PWD]))
        {
            app()->showMessage(110503);
            return;
        }


        {
			m_activeWifiPassword = text_lst[LT_NEW_WIFI_PWD];
            m_op = app()->bean()->changeRouterPassword
                    (text_lst[LT_NEW_WIFI_PWD],QString()
                     , text_lst[LT_NEW_ADMIN_PWD], text_lst[LT_OLD_ADMIN_PWD]);
            app()->wait(m_op, this, "slot_OpReturned");
        }
    }
    else if(obj_name==QString::fromUtf8("buttonNo"))
    {
        QList<QLineEdit *> edt_lst;
        getLables(edt_lst);
        foreach(QLineEdit *edt,edt_lst)
        {
            edt->clear();
        }
    }
    else if(obj_name==QString::fromUtf8("buttonNo3"))
    {
        initSettingPage();
		
    }
    else if(obj_name==QString::fromUtf8("buttonYes3"))
    {
        setSettings();
		mStack->setCurrentIndex(0);
		/*this->update();*/
		QPushButton *but=qFindChild<QPushButton *>(this,QString::fromUtf8("buttonRouterState"));
		but->setChecked(true);
		this->update();

    }
    else if(obj_name==QString::fromUtf8("buttonResolve"))
    {
        QVariantMap params1;
        params1.insert(QString::fromUtf8("popupMode"), QString::fromUtf8("1"));
        app()->navigateTo(QString::fromUtf8("RouterWlanSecurity"), params1);
    }
	else if (obj_name==QString::fromUtf8("buttonInstallNew"))
	{
		app()->navigateTo(QString::fromUtf8("InstallConfirm"));
	}
    else if (obj_name==QString::fromUtf8("buttonGotoAdvance"))
    {
        QDesktopServices::openUrl(QString::fromUtf8("http://routerlogin.net"));
    }
	else if(obj_name==QString::fromUtf8("buttonAttachDevice"))
    {
		QDesktopServices::openUrl(QString::fromUtf8("http://routerlogin.net/DEV_device.htm"));
	}
}

void UIMainFrame::slot_OpReturned()
{
    qDebug()<<"UIMainFrame::slot_OpReturned";
    AsyncOp *op = m_op;
    m_op->deleteLater();
    m_op = NULL;
    if (op->result() == AsyncOp::NoError) {
        app()->showMessage(120037);
		app()->setWifiSecurity(true);
		if (!m_activeWifiPassword.isEmpty()) {
			app()->confirmNewWifiPassword(m_activeWifiPassword);
		}
        //app()->showMessage(130004);
    } else if(op->result() == AsyncOp::InvalidWifiPasswordError){
		app()->showMessage(120033);
       //
    } else if(op->result() == AsyncOp::InvalidAdminPasswordError){
		app()->showMessage(120034);
	}
    else {
        app()->showMessage(110214);
    }

    QList<QLineEdit *> edt_lst;
    getLables(edt_lst);
    foreach(QLineEdit *edt,edt_lst)
    {
        edt->clear();
    }


}

void UIMainFrame::updateData()
{
    QLabel *lab_data=qFindChild<QLabel *>(this,QString::fromUtf8("lab_data"));
    Q_ASSERT(lab_data);
    lab_data->update();
    QPushButton *buttonResolve=qFindChild<QPushButton *>(this,QString::fromUtf8("buttonResolve"));
    Q_ASSERT(buttonResolve);
    bool bSecure=app()->wifiSecurity();
    buttonResolve->setVisible(!bSecure);

 }

//void UIMainFrame::updateSecurity()
//{

//}


APP_REGISTER_PAGE(UIMainFrame)
