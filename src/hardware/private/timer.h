#pragma once

#include "context.h"

namespace gbhw
{
	class Timer
	{
	public:
		Timer();

		void initialise(CPU_ptr cpu, MMU_ptr mmu);
		void release();

		void update(uint32_t cycles);
		void reset();

	private:
		CPU_ptr		m_cpu;
		MMU_ptr		m_mmu;
		uint32_t	m_tima;
		uint32_t	m_divt;
	};
}