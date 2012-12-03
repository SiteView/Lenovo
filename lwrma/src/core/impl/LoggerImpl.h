#ifndef __LoggerImpl_h__
#define __LoggerImpl_h__

#include <LenovoCore/Logger.h>
#include <Windows.h>

class LoggerImpl
{
public:
	LoggerImpl(const QString& name);
	~LoggerImpl();

	QString m_name;
};

class LoggerSystemImpl
{
public:
	LoggerSystemImpl();
	~LoggerSystemImpl();

	void init(const QString& logFileName);

	enum Level
	{
		Debug,
		Info,
		Warning,
		Error,
	};

	void output(Level level, LoggerImpl *logger, const QString& text);

	static LoggerSystemImpl *s_instance;
	HANDLE m_hLogFile;
};

#endif // __LoggerImpl_h__
