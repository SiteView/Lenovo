#ifndef __AsyncOp_h__
#define __AsyncOp_h__

#include <LenovoCore/Base.h>
#include <QtCore/QVariantMap>

class AsyncOpImpl;

class LENOVOCORE_API AsyncOp
	: public QObject
{
	Q_OBJECT

public:
	enum Status
	{
		NoError = 0,
		UnknownError = -1,
		AbortedError,
		NetworkError = -4000,
		XmlReaderError,
		SoapConfigurationStartedError,
		SoapConfigurationFinishedError,
		WlanRadioOffError = -7000,
		WlanServiceDownError,
		WlanNoDeviceError,
		WlanProfileNotFound,
		InvalidWifiPasswordError,
		InvalidAdminPasswordError,
	};

	virtual ~AsyncOp();
	AsyncOpImpl *d_ptr() const;
	void setValue(const char *name, const QVariant& value);
	QVariant value(const char *name) const;
	void copyValues(const AsyncOp *other);
	void setValues(const QVariantMap& values);
	QVariantMap values() const;
	void abort();
	bool isFinished() const;
	bool isAborted() const;
	int result() const;

Q_SIGNALS:
	void finished();

protected:
	AsyncOp(QObject *parent = NULL);
	virtual void onAbort();
	void notifyFinished(int result);

private:
	friend class AsyncOpImpl;
	AsyncOpImpl *m_impl;
};

Q_DECLARE_METATYPE(AsyncOp*)

#endif // __AsyncOp_h__
