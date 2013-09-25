#include "SoapCoreImpl.h"
#include <QtCore/QXmlStreamReader>
#include <LenovoCore/Logger.h>

DEFINE_LOGGER(SoapCore);

SoapCoreImpl::SoapCoreImpl(SoapCore *intf)
	: m_intf(intf), m_wrappedMode(false)
{
	m_nam = new QNetworkAccessManager();
	m_sessionId = QLatin1String("AD28AE69687E58D9K77");
    //m_portList.append(80);
	m_portList.append(5000);
	m_wrappedStatus = QLatin1String("ChangesApplied");
}

SoapCoreImpl::~SoapCoreImpl()
{
	delete m_nam;
}

SoapCore *SoapCoreImpl::q_ptr() const
{
	return m_intf;
}

AsyncOp *SoapCoreImpl::invoke(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList)
{
	if (m_wrappedMode) {
		return invokeWrapped(ns, action, nameList, valueList, m_routerAddress);
	}

	return invokeFallback(ns, action, nameList, valueList, m_routerAddress);
}

AsyncOp *SoapCoreImpl::invokeBasic(int port, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
{
	QString objectName;
	QString fullNS;
	QStringList nsParts = ns.split(QLatin1String(":"));
	if (nsParts.count() == 5) {
		objectName = nsParts.at(3);
		fullNS = ns;
	} else if (nsParts.count() == 1) {
		objectName = ns;
		fullNS = QString::fromUtf8("urn:NETGEAR-ROUTER:service:%1:1").arg(ns);
	} else {
		// TODO:
		objectName = ns;
		fullNS = ns;
	}

	if (fullNS.compare(QLatin1String("urn:NETGEAR-ROUTER:service:DeviceConfig:1")) == 0 && action.compare(QLatin1String("ConfigurationStarted")) == 0 && valueList.count() > 0) {
		m_sessionId = valueList.at(0);
	}

	BasicSoapOp *op = new BasicSoapOp(this, port, fullNS, action, nameList, valueList, host);
	op->start();
	return op;
}

AsyncOp *SoapCoreImpl::invokeFallback(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
{
	FallbackSoapOp *op = new FallbackSoapOp(this, ns, action, nameList, valueList, host);
	op->start();
	return op;
}

AsyncOp *SoapCoreImpl::invokeWrapped(const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
{
	WrappedSoapOp *op = new WrappedSoapOp(this, ns, action, nameList, valueList, host);
	op->start();
	return op;
}

QNetworkAccessManager *SoapCoreImpl::networkAccessManager() const
{
	return m_nam;
}

QString SoapCoreImpl::sessionId() const
{
	return m_sessionId;
}

QList<int> SoapCoreImpl::portList() const
{
	return m_portList;
}

void SoapCoreImpl::setPreferredPort(int port)
{
	m_portList.removeAll(port);
	m_portList.push_front(port);
}

QString SoapCoreImpl::wrappedStatus() const
{
	return m_wrappedStatus;
}

BasicSoapOp::BasicSoapOp(SoapCoreImpl *core, int port, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
	: AsyncOp(core), m_core(core), m_port(port), m_ns(ns), m_action(action), m_nameList(nameList), m_valueList(valueList), m_host(host)
{
}

BasicSoapOp::~BasicSoapOp()
{
	if (m_reply) {
		delete m_reply;
	}
}

void BasicSoapOp::start()
{
	QString host = m_host;
	if (host.isNull() || host.isEmpty()) {
		host = QLatin1String("routerlogin.net");
	}
	QString soapUrl;
	if (m_port == 80) {
		soapUrl = QString::fromUtf8("http://%1/soap/server_sa/").arg(m_host);
	} else {
		soapUrl = QString::fromUtf8("http://%1:%2/soap/server_sa/").arg(m_host).arg(m_port);
	}
	QUrl url(soapUrl);
	QNetworkRequest req(url);
	QString soapXml = buildSoapXml(m_core->sessionId(), m_ns, m_action, m_nameList, m_valueList);
	QByteArray soapXmlData = soapXml.toUtf8();

	QString soapAction = QString::fromLatin1("\"%1#%2\"").arg(m_ns, m_action);

	req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("text/xml; charset=utf-8"));
	req.setHeader(QNetworkRequest::ContentLengthHeader, soapXmlData.length());
	req.setRawHeader("accept", "text/xml");
	req.setRawHeader("soapaction", soapAction.toUtf8());
	req.setRawHeader("connection", "close");

	LOG_DEBUG(QString::fromUtf8("post: %1").arg(url.toString()));
	m_reply = m_core->networkAccessManager()->post(req, soapXmlData);
	connect(m_reply, SIGNAL(finished()), SLOT(onReplyFinished()));
}

QString BasicSoapOp::buildSoapXml(const QString& sessionId, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList)
{
	const char *tmpl =
		"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\r\n"
		"<SOAP-ENV:Header>\r\n"
		"<SessionID>%1</SessionID>\r\n"
		"</SOAP-ENV:Header>\r\n"
		"<SOAP-ENV:Body>\r\n"
		"%2\r\n"
		"</SOAP-ENV:Body>\r\n"
		"</SOAP-ENV:Envelope>";

	const char *body1 =
		"<M1:%1 xmlns:M1=\"%2\">\r\n"
		"%3"
		"</M1:%4>";

	const char *body2 =
		"<%1>\r\n"
		"%2"
		"</%3>";

	bool actionNS = true;

	if (ns.compare(QLatin1String("urn:NETGEAR-ROUTER:service:ParentalControl:1")) == 0/* && action.compare(QLatin1String("Authenticate")) == 0*/) {
		actionNS = false;
	}

	QString paramXml;
	for (int i = 0; i < nameList.count(); i++) {
		const QString& name = nameList.at(i);
		const QString& value = valueList.at(i);
		paramXml.append(QString::fromLatin1("  <%1>%2</%1>\r\n").arg(name, value));
	}

	QString bodyXml;
	if (actionNS) {
		bodyXml = QString::fromLatin1(body1).arg(action, ns, paramXml, action);
	} else {
		bodyXml = QString::fromLatin1(body2).arg(action, paramXml, action);
	}

	return QString::fromLatin1(tmpl).arg(sessionId, bodyXml);
}

void BasicSoapOp::onAbort()
{
	if (m_reply) {
		m_reply->abort();
	}
}

void BasicSoapOp::onReplyFinished()
{
	QNetworkReply *reply = m_reply;
	m_reply->deleteLater();
	m_reply = NULL;

	if (isAborted()) {
		return;
	}

	QByteArray data = reply->readAll();
	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError && data.isEmpty()) {
		LOG_DEBUG(QString::fromUtf8("BasicSoapOp::onReplyFinished post networkError: %1 [%2]").arg(err).arg(reply->errorString()));
		return notifyFinished(NetworkError);
	}

	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (!statusCode.isValid() || statusCode.toInt() != 200) {
		LOG_DEBUG(QString::fromUtf8("BasicSoapOp::onReplyFinished post invalid statusCode %1 [%2]").arg(statusCode.toInt()).arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
		return notifyFinished(NetworkError);
	}

	LOG_DEBUG(QString::fromUtf8("BasicSoapOp::onReplyFinished post ok"));

	//QVariant contentType = reply->header(QNetworkRequest::ContentTypeHeader);

	QVariantMap responseMap;
	QString responseCodeText;
	int level = 0;
	bool inBody = false;
	QString text;
	QXmlStreamReader reader(data);
	while (!reader.atEnd()) {
		switch (reader.readNext()) {
		case QXmlStreamReader::StartElement:
			{
				++level;
				if (level == 1) {
					if (reader.name().compare(QLatin1String("Envelope"), Qt::CaseInsensitive) != 0) {
						return notifyFinished(XmlReaderError);
					}
				} else if (level == 2) {
					if (reader.name().compare(QLatin1String("Body"), Qt::CaseInsensitive) == 0) {
						inBody = true;
					}
				}
			}
			break;
		case QXmlStreamReader::EndElement:
			{
				if (level == 2) {
					if (inBody) {
						inBody = false;
					}
				} else if (level == 3) {
					if (inBody) {
						QString key = reader.name().toString();
						if (key.endsWith(QLatin1String("ResponseCode"), Qt::CaseInsensitive)) {
							responseCodeText = text;
						}
					}
				} else if (level == 4) {
					if (inBody) {
						responseMap.insert(reader.name().toString(), text);
					}
				}
				--level;
			}
			break;
		case QXmlStreamReader::Characters:
			text = reader.text().toString();
			break;
		default:
			break;
		}
	}
	if (reader.hasError()) {
		return notifyFinished(XmlReaderError);
	}

	setValue("responseCode", responseCodeText);
	setValue("response", responseMap);

	notifyFinished(NoError);
}

FallbackSoapOp::FallbackSoapOp(SoapCoreImpl *core, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
	: AsyncOp(core), m_core(core), m_ns(ns), m_action(action), m_nameList(nameList), m_valueList(valueList), m_host(host)
{
	m_portList = core->portList();
}

FallbackSoapOp::~FallbackSoapOp()
{
	if (m_op) {
		delete m_op;
	}
}

void FallbackSoapOp::start()
{
	m_activePort = m_portList.front();
	m_portList.pop_front();
	m_op = m_core->invokeBasic(m_activePort, m_ns, m_action, m_nameList, m_valueList, m_host);
	connect(m_op, SIGNAL(finished()), SLOT(onSoapFinished()));
}

void FallbackSoapOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void FallbackSoapOp::onSoapFinished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	if (result != NoError) {
		if (!m_portList.empty()) {
			start();
		} else {
			notifyFinished(result);
		}
		return;
	}

	m_core->setPreferredPort(m_activePort);
	copyValues(op);
	notifyFinished(NoError);
}

WrappedSoapOp::WrappedSoapOp(SoapCoreImpl *core, const QString& ns, const QString& action, const QStringList& nameList, const QStringList& valueList, const QString& host)
	: AsyncOp(core), m_core(core), m_ns(ns), m_action(action), m_nameList(nameList), m_valueList(valueList), m_host(host)
{
}

WrappedSoapOp::~WrappedSoapOp()
{
	if (m_op) {
		delete m_op;
	}
}

void WrappedSoapOp::start()
{
	m_op = m_core->invokeFallback(QLatin1String("urn:NETGEAR-ROUTER:service:DeviceConfig:1"), QLatin1String("ConfigurationStarted"), QStringList(QLatin1String("NewSessionID")), QStringList(m_core->sessionId()), m_host);
	LOG_DEBUG(QString::fromUtf8("ConfigurationStarted ssid: %1").arg(m_core->sessionId()));
	connect(m_op, SIGNAL(finished()), SLOT(onSoap1Finished()));
}

void WrappedSoapOp::onAbort()
{
	if (m_op) {
		m_op->abort();
	}
}

void WrappedSoapOp::onSoap1Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant responseCode = op->value("responseCode");
	if (responseCode.toInt() != 0) {
		setValue("responseCode", responseCode);
		return notifyFinished(SoapConfigurationStartedError);
	}

	m_op = m_core->invokeFallback(m_ns, m_action, m_nameList, m_valueList, m_host);
	connect(m_op, SIGNAL(finished()), SLOT(onSoap2Finished()));
}

void WrappedSoapOp::onSoap2Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	copyValues(op);
	m_op = m_core->invokeFallback(QLatin1String("urn:NETGEAR-ROUTER:service:DeviceConfig:1"), QLatin1String("ConfigurationFinished"), QStringList(QLatin1String("NewStatus")), QStringList(m_core->wrappedStatus()), m_host);
	connect(m_op, SIGNAL(finished()), SLOT(onSoap3Finished()));
}

void WrappedSoapOp::onSoap3Finished()
{
	AsyncOp *op = m_op;
	m_op->deleteLater();
	m_op = NULL;

	int result = op->result();
	if (result != NoError) {
		return notifyFinished(result);
	}

	QVariant responseCode = op->value("responseCode");
	if (responseCode.toInt() != 0) {
		setValue("responseCode", responseCode);
		return notifyFinished(SoapConfigurationFinishedError);
	}

	notifyFinished(NoError);
}
