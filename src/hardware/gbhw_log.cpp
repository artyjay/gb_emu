#include "gbhw_log.h"

namespace gbhw
{
	Log::Log()
		: m_callback(nullptr)
	{
	}

	Log::~Log()
	{
	}

	void Log::RegisterCallback(LogCallback callback)
	{
		m_callback = callback;
	}

	Log& Log::GetLog()
	{
		static Log log;
		return log;
	}
}