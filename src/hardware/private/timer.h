#pragma once

#include "types.h"

namespace gbhw
{
	class CPU;
	class MMU;

	class Timer
	{
	public:
		Timer();

		void initialise(CPU* cpu, MMU* mmu);
		void update(uint32_t cycles);

	private:
		void reset();

		CPU*		m_cpu;
		MMU*		m_mmu;
		uint32_t	m_tima;
		uint32_t	m_divt;
	};
}