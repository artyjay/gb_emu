namespace gbd
{

	inline void MessageRaw(const char* message)
	{
		Log::GetLog().MessageRaw(message);
	}

	template<typename... Args>
	inline void Message(const char* format, Args... parameters)
	{
		Log::GetLog().Message(LogType::Msg, format, std::forward<Args>(parameters)...);
	}

	inline void MessageL(const char* message)
	{
		Log::GetLog().MessageL(LogType::Msg, message);
	}

	template<typename... Args>
	inline void Warning(const char* format, Args... parameters)
	{
		Log::GetLog().Message(LogType::Warn, format, std::forward<Args>(parameters)...);
	}

	inline void WarningL(const char* message)
	{
		Log::GetLog().MessageL(LogType::Warn, message);
	}

	template<typename... Args>
	inline void Error(const char* format, Args... parameters)
	{
		Log::GetLog().Message(LogType::Err, format, std::forward<Args>(parameters)...);
	}

	inline void ErrorL(const char* message)
	{
		Log::GetLog().MessageL(LogType::Err, message);
	}

} // gbd
