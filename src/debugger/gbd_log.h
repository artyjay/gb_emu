#pragma once

#include "gbd_types.h"

namespace gbd
{
	struct LogType
	{
		enum Type
		{
			Msg,
			Warn,
			Err
		};

		static const char* ToStr(Type type);
	};

	class Log
	{
	public:
		Log();
		~Log();

		void Message(LogType::Type type, const char* format, ...);
		void MessageL(LogType::Type type, const char* message);
		void MessageRaw(const char* message);

		static Log& GetLog();

	private:
		static const uint32_t kMaxFormattedLength = 4096;
		char m_buffer[2][kMaxFormattedLength];
	};

	inline void MessageRaw(const char* message);

	template<typename... Args>
	inline void Message(const char* format, Args... parameters);
	inline void MessageL(const char* message);

	template<typename... Args>
	inline void Warning(const char* format, Args... parameters);
	inline void WarningL(const char* message);

	template<typename... Args>
	inline void Error(const char* format, Args... parameters);
	inline void ErrorL(const char* message);

} // gbd

#include "gbd_log.inl"