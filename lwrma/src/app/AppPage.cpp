#include "AppPage.h"
#include "MiniApp.h"
#include <QtGui/QAbstractButton>
#include <QtGui/QLabel>

AppPage::AppPage(QWidget *parent)
	: QFrame(parent)
{
}

AppPage::~AppPage()
{
}

AppService *AppPage::app() const
{
	return m_app->service();
}

void AppPage::init(MiniApp *app, const QVariantMap& params)
{
	m_app = app;
	m_params = params;
	onLoad(params);
	if (m_jumpPageName.isEmpty()) {
		translate();
	}
}

void AppPage::term()
{
	onUnload();
}

QString AppPage::translateUIText(QWidget *widget)
{
	QString text;
	QVariant varText = widget->property("UIText");
	if (varText.isValid()) {
		bool ok;
		int textId = varText.toInt(&ok);
		if (ok) {
			text = m_app->translateUIText(textId, widget);
		}
	}
	return text;
}

void AppPage::translate()
{
	QList<QWidget*> widgetList = findChildren<QWidget*>();
	Q_FOREACH(QWidget *w, widgetList) {
		if (QLabel *label = qobject_cast<QLabel*>(w)) {
			QString text = translateUIText(w);
			if (!text.isNull()) {
				label->setText(text);
			}
		} else if (QAbstractButton *button = qobject_cast<QAbstractButton*>(w)) {
			QString text = translateUIText(w);
			if (!text.isNull()) {
				button->setText(text);
			}
		}
	}

	onTranslate();
}

void AppPage::jumpTo(const QString& pageName, const QVariantMap& params)
{
	m_jumpPageName = pageName;
	m_jumpParams = params;
}

const QVariantMap& AppPage::params() const
{
	return m_params;
}

void AppPage::onLoad(const QVariantMap& params)
{
}

void AppPage::onUnload()
{
}

void AppPage::onTranslate()
{
}

void AppPage::onMoviePlayButtonClicked()
{
	if (m_movie && m_moviePlayButton) {
		m_movie->start();
		m_moviePlayButton->hide();
	}
}

void AppPage::onMovieFinished()
{
	if (m_moviePlayButton) {
		m_moviePlayButton->show();
	}
}
