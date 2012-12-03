#ifndef DETAILUIHELPER_H
#define DETAILUIHELPER_H

#include <QPushButton>

class HeadButtonObject;
class ContaintObject;
class LabelObject;
class BottomRightButtonObject;
class SwitchObject;
class DetailUiHelper : public QObject
{
    Q_OBJECT
public:
    explicit DetailUiHelper(QObject *parent = 0);
    static DetailUiHelper *instance();
    HeadButtonObject *mHeadButtonObject;
    ContaintObject *mContaintObject;
    LabelObject *mLabelObject;
    BottomRightButtonObject *mBottomRightButtonObject;
    SwitchObject *mSwitchObject;
    void loadQSS();
signals:
    
public slots:
protected:
    static DetailUiHelper *sInstance;
    
};

class HeadButtonObject: public QObject
{
    Q_OBJECT
public:
    HeadButtonObject(QObject *parent);
    bool eventFilter(QObject *, QEvent *);
    QButtonGroup *mButGroup;
protected:
//    QWidget *sCurW;
};

class ContaintObject:public QObject
{
    Q_OBJECT
public :
    ContaintObject(QObject *parent);
    bool eventFilter(QObject *, QEvent *);
protected:
    void paintSaveCancel(QPainter &p);
    void paintBGText(QPainter &p,int idx);
};

class LabelObject:public QObject
{
    Q_OBJECT
public:
    LabelObject(QObject *parent);
    bool eventFilter(QObject *obj, QEvent *env);
};

class BottomRightButtonObject:public QObject
{
    Q_OBJECT
public:
    BottomRightButtonObject(QObject *parent);
    bool eventFilter(QObject *, QEvent *);
};

class SwitchObject:public QObject
{
    Q_OBJECT
public:
    SwitchObject(QObject *parent);
    bool eventFilter(QObject *obj, QEvent *env);
    void updateLan();
protected:
    QString mLanYes;
    QString mLanNo;
};

#endif // DETAILUIHELPER_H
