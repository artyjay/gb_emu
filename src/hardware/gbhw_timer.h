#pragma once

#include "gbhw_cpu.h"

namespace gbhw
{
	class Timer
	{
	public:
		Timer();

		void Initialise(CPU* cpu);

		void Update(uint32_t cycles);
		void Reset();

	private:
		CPU*		m_cpu;
		uint32_t	m_tima;
		uint32_t	m_divt;
	};
}