#pragma once

#include "gbhw_types.h"

namespace gbhw
{
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	class Log
	{
	public:
		Log();
		~Log();

		void RegisterCallback(LogCallback callback);

		template<typename... Args>
		GBInline void Message(const char* type, const char* format, Args... parameters);

		static Log& GetLog();

	private:
		static const uint32_t	kMaxFormattedLength = 4096;
		char					m_buffer[2][kMaxFormattedLength];
		LogCallback				m_callback;
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<typename... Args>
	GBInline void Log::Message(const char* type, const char* format, Args... parameters)
	{
		sprintf_s(m_buffer[0], format, parameters...);
		sprintf_s(m_buffer[1], "%s | %s", type, m_buffer[0]);

		if (m_callback)
		{
			m_callback(m_buffer[1]);
		}

	}

	template<typename... Args>
	GBInline void Message(const char* format, Args... parameters)
	{
		Log::GetLog().Message("msg  ", format, std::forward<Args>(parameters)...);
	}

	template<typename... Args>
	GBInline void Warning(const char* format, Args... parameters)
	{
		Log::GetLog().Message("warn ", format, std::forward<Args>(parameters)...);
	}

	template<typename... Args>
	GBInline void Error(const char* format, Args... parameters)
	{
		Log::GetLog().Message("error", format, std::forward<Args>(parameters)...);
	}
}