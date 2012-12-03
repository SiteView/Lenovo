#include "AppPage.h"
#include "AppUILoader.h"

AppUILoader::AppUILoader(QObject *parent)
	: QUiLoader(parent), m_loadingAppPage(false)
{
}

AppUILoader::~AppUILoader()
{
}

void AppUILoader::registerPageCreator(const QString& pageName, PageCreator creator)
{
	m_pageCreatorMap.insert(pageName, creator);
}

QWidget *AppUILoader::createWidget(const QString& className, QWidget *parent, const QString& name)
{
	if (m_loadingAppPage) {
		if (m_firstObject) {
			m_firstObject = false;
			if (parent == NULL) {
				PageCreatorMap::const_iterator it = m_pageCreatorMap.find(m_pageClassName);
				if (it != m_pageCreatorMap.end()) {
					PageCreator creator = it.value();
					AppPage *page = (*creator)();
					page->setObjectName(name);
					return page;
				}
			}
		}
	}

	if (className.compare(QLatin1String("QLabel")) == 0) {
		QWidget *w = new AppLabel(parent);
		w->setObjectName(name);
		return w;
	}

	return QUiLoader::createWidget(className, parent, name);
}

AppPage *AppUILoader::loadAppPage(const QString& className, QIODevice *device, QWidget *parent)
{
	m_loadingAppPage = true;
	m_firstObject = true;
	m_pageClassName = className;
	QWidget *w = QUiLoader::load(device, NULL);
	m_loadingAppPage = false;
	if (!w) {
		return NULL;
	}

	AppPage *page = qobject_cast<AppPage*>(w);
	if (!page) {
		delete w;
		return NULL;
	}

	page->setParent(parent);
	return page;
}

AppLabel::AppLabel(QWidget *parent)
	: QLabel(parent)
{
}

void AppLabel::paintEvent(QPaintEvent *e)
{
	QLabel::paintEvent(e);
}
