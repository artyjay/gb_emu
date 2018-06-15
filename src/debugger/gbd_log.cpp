#include "gbd_log.h"
#include "gbd_platform.h"

#include <iostream>
#include <stdarg.h>
#include <stdio.h>

namespace gbd
{
	namespace
	{
		const char* kLogTypeStrs[] =
		{
			"Message",
			"Warning",
			"Error  "
		};
	}

	const char* LogType::ToStr(LogType::Type type)
	{
		return kLogTypeStrs[type];
	}

	Log::Log()
	{
	}

	Log::~Log()
	{
	}

	void Log::Message(LogType::Type type, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vsprintf_s(m_buffer[0], format, args);
		va_end(args);

		MessageL(type, m_buffer[0]);
	}

	void Log::MessageL(LogType::Type type, const char* message)
	{
		sprintf_s(m_buffer[1], "gbd  - [%s] | %s", LogType::ToStr(type), message);
		MessageRaw(m_buffer[1]);
	}

	void Log::MessageRaw(const char* message)
	{
		std::cout << message;

#ifdef WIN32
		OutputDebugString(message);
#endif	
	}

	Log& Log::GetLog()
	{
		static Log log;
		return log;
	}

} // gbd