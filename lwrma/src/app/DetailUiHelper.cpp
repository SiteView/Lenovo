#include "DetailUiHelper.h"
#include <QtCore>
#include <QtGui>
#include <LenovoCore/LenovoCore.h>
#include "AppPage.h"
#include "MiniApp.h"

enum DETAIL_UI_FLAGS
{
    DUF_KEPRESSED=(1<<0)
};
static quint32 sFlags=0;
static QWidget *sCurW=NULL;

static void drawshadowText(QPainter &p,int x,int y,const QString &text,bool shadow=false)
{
    p.save();
    p.setBackgroundMode(Qt::TransparentMode);
    if(shadow)
    {
        p.setPen(qRgba(100,100,100,30));
        p.drawText(x+2,y+2,text);

        p.setPen(qRgb(0,0,0));
        p.drawText(x+1,y+1,text);

    }
    p.setPen(qRgb(255,255,255));
    p.drawText(x,y,text);
    p.restore();
}

static void drawshadowText(QPainter &p, const QPoint&point , const QString &text,bool shadow=false)
{
    drawshadowText(p,point.x(),point.y(),text,shadow);
}

static void drawshadowText(QPainter &p, const QRect &rect, const QString &text,int flags,bool shadow=false)
{
    p.save();
    p.setBackgroundMode(Qt::TransparentMode);
    if(shadow)
    {
        p.setPen(qRgba(100,100,100,30));
        p.drawText(rect.adjusted(2,2,2,2),flags,text);

        p.setPen(qRgb(0,0,0));
        p.drawText(rect.adjusted(1,1,1,1),flags,text);
    }

    p.setPen(qRgb(255,255,255));
    p.drawText(rect,flags,text);
    p.restore();
}

DetailUiHelper *DetailUiHelper::sInstance=NULL;
DetailUiHelper::DetailUiHelper(QObject *parent) :
    QObject(parent)
  ,mHeadButtonObject(new HeadButtonObject(this))
  ,mContaintObject(new ContaintObject(this))
  ,mLabelObject(new LabelObject(this))
  ,mBottomRightButtonObject(new BottomRightButtonObject(this))
  ,mSwitchObject(new SwitchObject(this))
{

}

DetailUiHelper *DetailUiHelper::instance()
{
    if(!sInstance)
    {
        sInstance=new DetailUiHelper();
    }
    return sInstance;
}

void DetailUiHelper::loadQSS()
{
    QFile f(QString::fromUtf8(":/qss/app.qss"));
//    QFile f(QString::fromUtf8("../src/app/qss/app.qss"));
    bool b=f.open(QIODevice::ReadOnly);
    Q_ASSERT(b);
    QString qss=QString::fromUtf8(f.readAll());
    MiniApp::instance()->setStyleSheet(qss);

}

HeadButtonObject::HeadButtonObject(QObject *parent)
    :QObject(parent)
    ,mButGroup(NULL)
{

}

bool HeadButtonObject::eventFilter(QObject *obj, QEvent *env)
{
    QEvent::Type type=env->type();
    if(type==QEvent::Destroy)
    {
        return false;
    }
    QPushButton *w=qobject_cast<QPushButton *>(obj);
    if(!w)
    {
        return false;
    }
    if(QEvent::Enter==type)
    {
        w->setCursor(Qt::PointingHandCursor);
        sCurW=w;
        w->update();
    }
    else if(QEvent::Leave==type)
    {
        w->setCursor(Qt::ArrowCursor);
        sCurW=NULL;
        w->update();
    }
    else if(QEvent::Paint==type)
    {
        QString bg=w->property("bg").toString();
        QPainter p(w);
        p.drawPixmap(0,0,QPixmap(bg));
        if(w->isChecked())
        {
            p.drawPixmap(0,0,QPixmap(QString::fromUtf8(":/images/selected.png")));
        }
        else if(w==sCurW )
        {
            p.drawPixmap(0,0,QPixmap(QString::fromUtf8(":/images/selected2.png")));
        }


        QString text=w->text();
        QFont font;
        font.setPixelSize(22);
        p.setFont(font);

        QPoint point(37,50);
        drawshadowText(p,point,text,true);

//        p.setPen(qRgba(100,100,100,100));
//        p.drawText(point+QPoint(2,2),text);

//        p.setPen(qRgb(0,0,0));
//        p.drawText(point+QPoint(1,1),text);


//        p.setPen(qRgb(255,255,255));
//        p.drawText(point,text);
        return true;
    }
	return false;

}
////////////////////////////////////////////////////////////
ContaintObject::ContaintObject(QObject *parent)
    :QObject(parent)
{

}



bool ContaintObject::eventFilter(QObject *obj, QEvent *env)
{
    QEvent::Type type=env->type();
    if(type==QEvent::Destroy)
    {
        return false;
    }
    QWidget *w=qobject_cast<QWidget *>(obj);
    if(!w)
    {
        return false;
    }

    if(type==QEvent::Paint)
    {
        QButtonGroup *group=DetailUiHelper::instance()->mHeadButtonObject->mButGroup;
        Q_ASSERT(group);
        int idx=group->checkedId();
        QPainter p(w);
        paintBGText(p,idx);
    }
    return false;
}

void ContaintObject::paintSaveCancel(QPainter &p)
{
    static struct _{int lan;int x;int y;} cParas1[]={
        {120038,675,304}
        ,{120039,738,304}
    };
    for(int i=0;i<sizeof(cParas1)/sizeof(cParas1[0]);i++)
    {
        QString text=MiniApp::instance()->translateUIText(cParas1[i].lan,NULL);
        QFont f;
        f.setPixelSize(18);
        p.setFont(f);
        drawshadowText(p,cParas1[i].x,cParas1[i].y,text);
    }
}

void ContaintObject::paintBGText(QPainter &p, int idx)
{
    const char *cParas[]={
        ":/images/r_state_c.png"
        ,":/images/p_setting_c.png"
        ,":/images/s_setting_c.png"
    };
    if(idx >=0 &&idx<sizeof(cParas)/sizeof(cParas[0]))
    {
        p.drawPixmap(0,0,QPixmap(QString::fromUtf8(cParas[idx])));
    }
    if(idx==0)
    {
        QFont f;
        f.setPixelSize(22);
        f.setWeight(QFont::Light);
        p.setFont(f);

        const int cX=38;
        const int cY=61;
        const int cH=48;
        int cParas[]={120010,120011,120029,120012};
        for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
        {
            p.setPen(qRgb(255,255,255));
           // p.drawText(cX,cY+cH*i,MiniApp::instance()->translateUIText(cParas[i],NULL));
			QString text=MiniApp::instance()->translateUIText(cParas[i],NULL);
			drawshadowText(p,cX,cY+cH*i,text);
        }
       // p.drawRect(21+149+91,300-112,200,20);
    }
    else if(idx==1)
    { 
        const struct _{int lan;int x;int y;int w;int h;int flags;int fontSize;} cParas[]={
            {120003,20+18,146,400,30,Qt::AlignTop|Qt::AlignLeft,22}//设置无线路由管理密码
            ,{120004,20+18,27/*130+32+16*/,400,30,Qt::AlignTop|Qt::AlignLeft,22}//设置无线密码
            //,{120005,20,70,100,20,Qt::AlignVCenter|Qt::AlignRight,15}//old user
            ,{120007,20,146+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15}//old password 
			,{120036,470,146+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15}//default password is xxx
          //  ,{120006,20,102,100,20,Qt::AlignVCenter|Qt::AlignRight,15}//new user
            ,{120008,20,146+32+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15} //new password
            ,{120009,20,146+32+32+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15} //confirm new password
            ,{120008,20,27+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15} 
            ,{120009,20,27+32+32+16,200,20,Qt::AlignVCenter|Qt::AlignRight,15} 

        };
        int curFSize=-1;
        for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
        {
            if(curFSize!=cParas[i].fontSize)
            {
                curFSize=cParas[i].fontSize;
                QFont f;
                f.setPixelSize(curFSize);
                p.setFont(f);
            }
            QString text=MiniApp::instance()->translateUIText(cParas[i].lan,NULL);
            drawshadowText(p
                           ,QRect(cParas[i].x,cParas[i].y,cParas[i].w,cParas[i].h)
                           ,text,cParas[i].flags);
		//	p.drawRect(cParas[i].x,cParas[i].y,cParas[i].w,cParas[i].h);

        }
        paintSaveCancel(p);
    }
    else if(idx == 2)
    {
        const int cParas[]={120022,120025,120026/*,120027*/};
        const int cX=20+16;
        const int cY=30+27+1+20;
        const int cH=58;
        for(int i=0;i<sizeof(cParas)/sizeof(cParas[0]);i++)
        {
            QString text=MiniApp::instance()->translateUIText(cParas[i],NULL);
            QFont f;
            f.setPixelSize(22);
            p.setFont(f);
            drawshadowText(p,cX,cY+i*cH,text);
        }

        paintSaveCancel(p);

    }
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

LabelObject::LabelObject(QObject *parent)
    :QObject(parent)
{

}

bool LabelObject::eventFilter(QObject *obj, QEvent *env)
{
    QEvent::Type type=env->type();
    if(type==QEvent::Destroy)
    {
        return false;
    }
    QWidget *w=qobject_cast<QWidget *>(obj);
    if(!w)
    {
        return false;
    }

    if(type==QEvent::Paint)
    {
        QPainter p(w);
        QFont f;
        f.setPixelSize(22);
        p.setFont(f);
        const int cX=0;
        const int cY=61;
        const int cH=48;
        QStringList lst;
        div_t d = div(MiniApp::instance()->uptimeMinutes(), 60);
//        m_uptimeLabel->setText(app()->translateUIText(120028).arg(d.quot).arg(d.rem));


        lst << MiniApp::instance()->wifiName();  
        lst << MiniApp::instance()->translateUIText(120028,NULL).arg(d.quot).arg(d.rem);
        lst << MiniApp::instance()->translateUIText(120030,NULL)
               .arg(MiniApp::instance()->incomingBPS() / 1024, 0, 'f', 2)
               .arg(MiniApp::instance()->outgoingBPS() / 1024, 0, 'f', 2);

        int c=lst.count();


        for(int i=0;i<c;i++)
        {
            drawshadowText(p,cX,cY+cH*i,lst[i]);
        }
		p.setPen(Qt::white);
		p.drawRect(0,190,200,20);

		bool bSecure=MiniApp::instance()->wifiSecurity();
		//p.setBrush();
        p.fillRect(1,190+1,bSecure?(200-2):((200-2)>>1)-45,18,bSecure?qRgb(74,143,0):qRgb(255,0,0));

		QString text=(MiniApp::instance()->translateUIText(bSecure ? 120013 : 120014,NULL));
		drawshadowText(p,210,206,text);

    }
    return false;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
BottomRightButtonObject::BottomRightButtonObject(QObject *parent)
    :QObject(parent)
{

}

bool BottomRightButtonObject::eventFilter(QObject *obj, QEvent *env)
{
    QEvent::Type type=env->type();
    if(type==QEvent::Destroy)
    {
        return false;
    }
    QPushButton *w=qobject_cast<QPushButton *>(obj);
    if(!w)
    {
        return false;
    }
    if(QEvent::Enter==type)
    {
        w->setCursor(Qt::PointingHandCursor);
        sCurW=w;
        w->update();
    }
    else if(QEvent::Leave==type)
    {
        w->setCursor(Qt::ArrowCursor);
        sCurW=NULL;
        w->update();
    }
    else if(QEvent::MouseButtonPress==type)
    {
        sFlags |=DUF_KEPRESSED;
        w->update();
    }
    else if(QEvent::MouseButtonRelease==type)
    {
        sFlags &=~DUF_KEPRESSED;
        w->update();
    }
    else if(QEvent::Paint==type)
    {
        int lanidx=w->property("UIText").toInt();
        QString bg=w->property("bg").toString();
        QString state=QString::fromUtf8("normal");
        if(w==sCurW)
        {
            state=(sFlags&DUF_KEPRESSED)?QString::fromUtf8("selected")
                                       :QString::fromUtf8("hover");
        }

        bg=QString::fromUtf8(":/images/%1_%2.png").arg(bg,state);
        QPainter p(w);
        QFont f;
        f.setPixelSize(14);
        p.setFont(f);
        QPixmap pixmap(bg);
        int x=((w->width()-pixmap.width())>>1);
        p.drawPixmap(x,0,pixmap);

        if(lanidx >=0)
        {
            QRect rect(0,57,w->width(),20);
            QString text=MiniApp::instance()->translateUIText(lanidx,NULL);
            drawshadowText(p,rect,text,Qt::AlignHCenter);
        }

        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

SwitchObject::SwitchObject(QObject *parent)
    :QObject(parent)
{
  //  updateLan();
}

bool SwitchObject::eventFilter(QObject *obj, QEvent *env)
{
    QEvent::Type type=env->type();
    if(type==QEvent::Destroy)
    {
        return false;
    }
    QPushButton *w=qobject_cast<QPushButton *>(obj);
    if(!w)
    {
        return false;
    }
    if(type==QEvent::Paint)
    {
        QPainter p(w);
        QFont f;
        f.setPixelSize(18);
        bool checked=w->isChecked();
        p.drawText(0,4+14-3+2,checked?mLanYes:mLanNo);

        QRgb rgb=checked?qRgb(163,214,2):qRgb(211,139,112);
        p.fillRect(37+0,0,56,2,rgb);
        p.fillRect(37+0,20,56,2,rgb);
        p.fillRect(37+0,2,2,18,rgb);
        p.fillRect(37+54,2,2,18,rgb);
		p.fillRect(37+4,4,48,14,rgb);
		p.fillRect(37+(checked?36:0),0,20,22,Qt::white);
        return true;
    }
    return false;
}

void SwitchObject::updateLan()
{
    mLanYes=MiniApp::instance()->translateUIText(110005,NULL);
    mLanNo=MiniApp::instance()->translateUIText(110006,NULL);


}


