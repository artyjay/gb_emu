#pragma once

#include "gbhw_types.h"

#include <memory>

namespace gbhw
{
	//--------------------------------------------------------------------------

	class CPU;
	class GPU;
	class MMU;
	class Rom;
	class Timer;

	using CPU_ptr	= std::shared_ptr<CPU>;
	using GPU_ptr	= std::shared_ptr<GPU>;
	using MMU_ptr	= std::shared_ptr<MMU>;
	using Rom_ptr	= std::shared_ptr<Rom>;
	using Timer_ptr = std::shared_ptr<Timer>;

	//--------------------------------------------------------------------------

	class Context
	{
	public:
		Context();
		~Context();

		bool initialise();

	private:
		CPU_ptr		m_cpu;
		GPU_ptr		m_gpu;
		MMU_ptr		m_mmu;
		Rom_ptr		m_rom;
		Timer_ptr	m_timer;
	};

	//--------------------------------------------------------------------------
}
