#pragma once

#include "types.h"
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

		CPU_ptr		cpu;
		GPU_ptr		gpu;
		MMU_ptr		mmu;
		Rom_ptr		rom;
		Timer_ptr	timer;
	};

	//--------------------------------------------------------------------------
}
