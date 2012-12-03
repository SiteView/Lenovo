#include "LoggerImpl.h"
#include <QtCore/QDateTime>

LoggerImpl::LoggerImpl(const QString& name)
	: m_name(name)
{
}

LoggerImpl::~LoggerImpl()
{
}

LoggerSystemImpl *LoggerSystemImpl::s_instance = NULL;

LoggerSystemImpl::LoggerSystemImpl()
	: m_hLogFile(INVALID_HANDLE_VALUE)
{
	s_instance = this;
}

LoggerSystemImpl::~LoggerSystemImpl()
{
	if (m_hLogFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hLogFile);
	}
	s_instance = NULL;
}

void LoggerSystemImpl::init(const QString& logFileName)
{
	WCHAR fname[MAX_PATH];
	int len = logFileName.toWCharArray(fname);
	fname[len] = 0;

	m_hLogFile = CreateFile(fname, FILE_READ_DATA | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void LoggerSystemImpl::output(Level level, LoggerImpl *logger, const QString& text)
{
	QString line;
	QDateTime t = QDateTime::currentDateTime();

	const int catSize = 20;
	QString cat = logger->m_name;
	if (cat.length() < catSize) {
		cat = QString(catSize - cat.length(), 32).append(cat);
	} else {
		cat = cat.right(catSize);
	}

	const char *catName[] = {
		"DBG", "INF", "WRN", "ERR"
	};

	line = QString::fromUtf8("%1[%5][%6][%2][%3] %4").arg(t.toString(Qt::ISODate), QLatin1String(catName[level]),cat, text).arg(GetCurrentProcessId(), 4).arg(GetCurrentThreadId(), 4);

#ifndef NDEBUG
	const int buffSize = 2048;
	WCHAR lineBuff[buffSize + 3];
	int len = line.toWCharArray(lineBuff);
	lineBuff[len] = '\r';
	lineBuff[len+1] = '\n';
	lineBuff[len+2] = 0;
	OutputDebugString(lineBuff);
#endif

	if (m_hLogFile != INVALID_HANDLE_VALUE) {
		QByteArray text = line.toUtf8();
		text.append("\r\n");
		char *ptr = text.data();
		DWORD cbRemain = text.length();
		DWORD cb;
		while (cbRemain > 0) {
			if (!WriteFile(m_hLogFile, ptr, cbRemain, &cb, NULL)) {
				break;
			}
			cbRemain -= cb;
		}
		FlushFileBuffers(m_hLogFile);
	}
}
