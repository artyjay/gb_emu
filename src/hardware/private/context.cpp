#include "context.h"

#include "cpu.h"
#include "gpu.h"
#include "mmu.h"
#include "rom.h"
#include "timer.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	Context::Context()
	{
	}

	Context::~Context()
	{
		cpu->release();
		gpu->release();
		mmu->release();
	}

	bool Context::initialise()
	{
		cpu	= std::make_shared<CPU>();
		gpu	= std::make_shared<GPU>();
		mmu	= std::make_shared<MMU>();
		rom	= std::make_shared<Rom>();
		timer = std::make_shared<Timer>();

		cpu->initialise(mmu);
		gpu->initialise(cpu, mmu);
		mmu->initialise(cpu, gpu, rom);
		timer->initialise(cpu);

		return true;
	}

	//--------------------------------------------------------------------------
}