#include "LoggerImpl.h"

Logger::Logger(const char *name)
{
	m_impl = new LoggerImpl(QString::fromUtf8(name));
}

Logger::~Logger()
{
	delete m_impl;
}

void Logger::debug(const QString& text)
{
	if (LoggerSystemImpl::s_instance) {
		LoggerSystemImpl::s_instance->output(LoggerSystemImpl::Debug, m_impl, text);
	}
}

void Logger::info(const QString& text)
{
	if (LoggerSystemImpl::s_instance) {
		LoggerSystemImpl::s_instance->output(LoggerSystemImpl::Info, m_impl, text);
	}
}

void Logger::warning(const QString& text)
{
	if (LoggerSystemImpl::s_instance) {
		LoggerSystemImpl::s_instance->output(LoggerSystemImpl::Warning, m_impl, text);
	}
}

void Logger::error(const QString& text)
{
	if (LoggerSystemImpl::s_instance) {
		LoggerSystemImpl::s_instance->output(LoggerSystemImpl::Error, m_impl, text);
	}
}

LoggerSystem::LoggerSystem()
{
	m_impl = new LoggerSystemImpl();
}

LoggerSystem::~LoggerSystem()
{
	delete m_impl;
}

void LoggerSystem::init(const QString& logFileName)
{
	m_impl->init(logFileName);
}
