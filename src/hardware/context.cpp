#include "context.h"

#include "cpu.h"
#include "gpu.h"
#include "mmu.h"


#include "gbhw_rom.h"
#include "gbhw_timer.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	Context::Context()
	{
	}

	Context::~Context()
	{
		m_cpu->release();
		m_gpu->release();
		m_mmu->release();
	}

	bool Context::initialise()
	{
		m_cpu	= std::make_shared<CPU>();
		m_gpu	= std::make_shared<GPU>();
		m_mmu	= std::make_shared<MMU>();
		m_rom	= std::make_shared<Rom>();
		m_timer = std::make_shared<Timer>();

		m_cpu->initialise(m_mmu);
		m_gpu->initialise(m_cpu, m_mmu);
		m_mmu->initialise(m_cpu, m_gpu, m_rom);
	}

	//--------------------------------------------------------------------------
}