#pragma once

#include "context.h"

namespace gbhw
{
	class Timer
	{
	public:
		Timer();

		void initialise(CPU_ptr cpu);
		void release();

		void update(uint32_t cycles);
		void reset();

	private:
		CPU_ptr		m_cpu;
		uint32_t	m_tima;
		uint32_t	m_divt;
	};
}