#include "gbhw_timer.h"

namespace gbhw
{
	namespace
	{
		static const uint32_t kTimaPeriods[] = { 1024, 16, 64, 256 };
	}

	Timer::Timer()
	{
		Reset();
	}

	void Timer::Initialise(CPU* cpu)
	{
		m_cpu = cpu;
	}

	void Timer::Update(uint32_t cycles)
	{
		uint32_t timaPeriod = kTimaPeriods[m_cpu->ReadIO(HWRegs::TAC) & 0x03];

		if(m_cpu->ReadIO(HWRegs::TAC) & (0x2))	// Bit 2 is timer enabled.
		{
			m_tima += cycles;

			while(m_tima >= timaPeriod)
			{
				// Increment tima once for the period.
				Byte newTima = m_cpu->ReadIO(HWRegs::TIMA) + 1;
				m_cpu->WriteIO(HWRegs::TIMA, newTima);

				// Wrapped around/overflowed.
				if(newTima == 0)
				{
					m_cpu->WriteIO(HWRegs::TIMA, m_cpu->ReadIO(HWRegs::TMA));	// Reset tima.
					m_cpu->GenerateInterrupt(HWInterrupts::Timer);			// Generate an interrupt.
				}

				m_tima -= timaPeriod;
			}
		}
		else
		{
			m_tima = 0;
		}

		m_divt += cycles;

		while(m_divt >= 64)
		{
			m_cpu->WriteIO(HWRegs::DIV, m_cpu->ReadIO(HWRegs::DIV) + 1);
			m_divt -= 64;
		}
	}

	void Timer::Reset()
	{
		m_tima = 0;
		m_divt = 0;
	}
} // gbhw