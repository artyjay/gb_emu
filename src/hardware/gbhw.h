#pragma once

#include "gbhw_types.h"

namespace gbhw
{
	struct ExecuteMode
	{
		enum Type
		{
			SingleInstruction,
			SingleVSync
		};
	};

	class CPU;
	class GPU;
	class MMU;
	class Rom;
	class Timer;

	class Hardware
	{
	public:
		Hardware();
		~Hardware();

		void Initialise();
		void Release();

		bool LoadROM(const char* path);
		void Execute();

		ExecuteMode::Type GetExecuteMode() const;
		void SetExecuteMode(ExecuteMode::Type mode);
		void RegisterLogCallback(LogCallback callback);

		void PressButton(HWButton::Type button);
		void ReleaseButton(HWButton::Type button);

		const Byte* GetScreenData() const;

	private:
		ExecuteMode::Type	m_executeMode;
		CPU*				m_cpu;
		GPU*				m_gpu;
		MMU*				m_mmu;
		Rom*				m_rom;
		Timer*				m_timer;
	};
}