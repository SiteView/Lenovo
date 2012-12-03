#ifndef __Logger_h__
#define __Logger_h__

#include <LenovoCore/Base.h>

#ifndef LOGGER_ENABLED
#define LOGGER_ENABLED 1
#endif // LOGGER_ENABLED

class LoggerImpl;

class LENOVOCORE_API Logger
{
public:
	Logger(const char *name);
	~Logger();

	void debug(const QString& text);
	void info(const QString& text);
	void warning(const QString& text);
	void error(const QString& text);

private:
	LoggerImpl *m_impl;
};

class LoggerSystemImpl;

class LENOVOCORE_API LoggerSystem
{
public:
	LoggerSystem();
	~LoggerSystem();

	void init(const QString& logFileName);

private:
	LoggerSystemImpl *m_impl;
};

#if LOGGER_ENABLED

#define DEFINE_LOGGER(name) static Logger g_defaultLogger( #name )
#define LOG_DEBUG(x) g_defaultLogger.debug(x)
#define LOG_INFO(x) g_defaultLogger.info(x)
#define LOG_WARNING(x) g_defaultLogger.warning(x)
#define LOG_ERROR(x) g_defaultLogger.error(x)

#else

#define DEFINE_LOGGER(name)
#define LOG_DEBUG(x)
#define LOG_INFO(x)
#define LOG_WARNING(x)
#define LOG_ERROR(x)

#endif

#endif // __Logger_h__
