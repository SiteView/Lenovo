#ifndef __AppPage_h__
#define __AppPage_h__

#include "AppService.h"
#include <QtCore/QPointer>
#include <QtCore/QVariantMap>
#include <QtGui/QFrame>
#include <QtGui/QMovie>
#include <QtGui/QPushButton>

class MiniApp;

class AppPage
	: public QFrame
{
	Q_OBJECT

public:
	AppPage(QWidget *parent = NULL);
	virtual ~AppPage();
	AppService *app() const;

protected:
	virtual void onLoad(const QVariantMap& params);
	virtual void onUnload();
	virtual void onTranslate();

	void jumpTo(const QString& pageName, const QVariantMap& params = QVariantMap());
	const QVariantMap& params() const;

private:
	void init(MiniApp *app, const QVariantMap& params);
	void term();
	void translate();
	QString translateUIText(QWidget *widget);

private Q_SLOTS:
	void onMoviePlayButtonClicked();
	void onMovieFinished();

public:
	QPointer<QMovie> m_movie;
	QPointer<QPushButton> m_moviePlayButton;

private:
	friend class MiniApp;
	QPointer<MiniApp> m_app;
	QString m_jumpPageName;
	QVariantMap m_jumpParams;
	QVariantMap m_params;
};

void appRegisterPageCreator(const char *pageName, AppPage *(*creator)());
void appUnregisterPageCreator(AppPage *(*creator)());

template <class T>
class AppPageAutoInitializer
{
public:
	AppPageAutoInitializer(const char *pageName)
	{
		appRegisterPageCreator(pageName, &AppPageAutoInitializer<T>::creator);
	}

	~AppPageAutoInitializer()
	{
		appUnregisterPageCreator(&AppPageAutoInitializer<T>::creator);
	}

	static AppPage *creator()
	{
		return new T();
	}
};

#define APP_REGISTER_PAGE(page) static AppPageAutoInitializer<page> g_staticInitializer_ ## page ( #page );

#endif // __AppPage_h__
