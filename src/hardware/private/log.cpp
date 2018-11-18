#include "log.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	Log::Log()
	{
		m_callback.cb		= nullptr;
		m_callback.userdata = nullptr;
	}

	Log::~Log()
	{
	}

	void Log::initialise(gbhw_log_level level, gbhw_log_callback_t callback, void* userdata)
	{
		m_level				= level;
		m_callback.cb		= callback;
		m_callback.userdata = userdata;
	}

	Log& Log::instance()
	{
		static Log log;
		return log;
	}

	//--------------------------------------------------------------------------
}