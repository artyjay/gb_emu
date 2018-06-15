#include "gbe_log.h"

#include <iostream>
#include <stdarg.h>
#include <stdio.h>

namespace gbe
{
	Log::Log()
	{
	}

	Log::~Log()
	{
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

} // gbe