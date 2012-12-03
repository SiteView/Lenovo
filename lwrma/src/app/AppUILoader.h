#ifndef __AppUILoader_h__
#define __AppUILoader_h__

#include <QtUiTools/QUiLoader>
#include <QtCore/QMap>
#include <QtGui/QLabel>

class AppPage;

class AppUILoader
	: public QUiLoader
{
	Q_OBJECT

public:
	typedef AppPage *(*PageCreator)();

	AppUILoader(QObject *parent = NULL);
	virtual ~AppUILoader();
	virtual QWidget *createWidget(const QString& className, QWidget *parent, const QString& name);
	void registerPageCreator(const QString& pageName, PageCreator creator);
	AppPage *loadAppPage(const QString& className, QIODevice *device, QWidget *parent);

private:
	typedef QMap<QString, PageCreator> PageCreatorMap;
	PageCreatorMap m_pageCreatorMap;
	QString m_pageClassName;
	bool m_loadingAppPage;
	bool m_firstObject;
};

class AppLabel
	: public QLabel
{
	Q_OBJECT

public:
	AppLabel(QWidget *parent);

protected:
	virtual void paintEvent(QPaintEvent *e);
};

#endif // __AppUILoader_h__
