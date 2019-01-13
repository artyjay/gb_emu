#include "timer.h"
#include "cpu.h"

namespace gbhw
{
	namespace
	{
		static const uint32_t kTimaPeriods[] = { 1024, 16, 64, 256 };
	}

	Timer::Timer()
	{
		reset();
	}

	void Timer::initialise(CPU* cpu, MMU* mmu)
	{
		m_cpu = cpu;
		m_mmu = mmu;
	}

	void Timer::update(uint32_t cycles)
	{
		const uint32_t timaPeriod = kTimaPeriods[m_mmu->read_io(HWRegs::TAC) & 0x03];

		if(m_mmu->read_io(HWRegs::TAC) & (0x2))	// Bit 2 is timer enabled.
		{
			m_tima += cycles;

			while(m_tima >= timaPeriod)
			{
				// Increment tima once for the period.
				Byte newTima = m_mmu->read_io(HWRegs::TIMA) + 1;
				m_mmu->write_io(HWRegs::TIMA, newTima);

				// Wrapped around/overflowed.
				if(newTima == 0)
				{
					m_mmu->write_io(HWRegs::TIMA, m_mmu->read_io(HWRegs::TMA));	// Reset tima.
					m_cpu->generate_interrupt(HWInterrupts::Timer);				// Generate an interrupt.
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
			m_mmu->write_io(HWRegs::DIV, m_mmu->read_io(HWRegs::DIV) + 1);
			m_divt -= 64;
		}
	}

	void Timer::reset()
	{
		m_tima = 0;
		m_divt = 0;
	}
}