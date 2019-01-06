#pragma once

#include "gbhw.h"
#include "types.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	class Log
	{
	public:
		Log();
		~Log();

		void initialise(gbhw_log_level level, gbhw_log_callback_t callback, void* userdata);

		template<typename... Args>
		inline void output(gbhw_log_level_t level, const char* type, const char* format, Args... parameters);

		static Log& instance();

	private:
		static const uint32_t	kMaxFormattedLength = 4096;
		char					m_buffer[2][kMaxFormattedLength];
		gbhw_log_level_t		m_level;

		struct
		{
			gbhw_log_callback_t cb;
			void*				userdata;

			inline void trigger(gbhw_log_level level, const char* msg)
			{
				if(cb)
				{
					cb(userdata, level, msg);
				}
			}
		} m_callback;
	};

	//--------------------------------------------------------------------------

	template<typename... Args>
	inline void Log::output(gbhw_log_level_t level, const char* type, const char* format, Args... parameters)
	{
		if(level < m_level)
			return;

#ifdef MSVC
		sprintf_s(m_buffer[0], format, parameters...);
		sprintf_s(m_buffer[1], "%s | %s", type, m_buffer[0]);
#else
		sprintf(m_buffer[0], format, parameters...);
		sprintf(m_buffer[1], "%s | %s", type, m_buffer[0]);
#endif

		m_callback.trigger(level, m_buffer[1]);
	}

	template<typename... Args>
	inline void log_debug(const char* format, Args... parameters)
	{
		Log::instance().output(l_debug, "dbg  ", format, std::forward<Args>(parameters)...);
	}

	template<typename... Args>
	inline void log_warning(const char* format, Args... parameters)
	{
		Log::instance().output(l_warning, "warn ", format, std::forward<Args>(parameters)...);
	}

	template<typename... Args>
	inline void log_error(const char* format, Args... parameters)
	{
		Log::instance().output(l_error, "error", format, std::forward<Args>(parameters)...);
	}

	//--------------------------------------------------------------------------
}