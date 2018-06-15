#include "gbhw_cpu.h"
#include "gbhw_instructions.h"
#include "gbhw_instructions_extended.h"

#include <algorithm>
#include <iostream>

namespace gbhw
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Instruction implementation
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Instruction::Instruction()
		: m_function(nullptr)
		, m_address(0)
	{
	}

	void Instruction::Set(Byte opcode, Byte extended, Byte byteSize, Byte cycles0, Byte cycles1, RFB::Type behaviour0, RFB::Type behaviour1, RFB::Type behaviour2, RFB::Type behaviour3, RTD::Type args0, RTD::Type args1, const char* assembly)
	{
		m_opcode = opcode;
		m_extended = extended;

		m_byteSize = byteSize;

		m_cycles[0] = cycles0;
		m_cycles[1] = cycles1;

		m_flagBehaviour[0] = behaviour0;
		m_flagBehaviour[1] = behaviour1;
		m_flagBehaviour[2] = behaviour2;
		m_flagBehaviour[3] = behaviour3;

		m_args[0] = args0;
		m_args[1] = args1;

		m_assembly = assembly;
	}

	void Instruction::Set(InstructionFunction fn)
	{
		assert(m_function == nullptr || m_function == &CPU::InstructionNotImplemented || m_function == &CPU::InstructionNotImplementedExt);
		m_function = fn;
	}

	void Instruction::Set(Address address)
	{
		m_address = address;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// CPU implementation
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	CPU::CPU()
		: m_mmu(nullptr)
		, m_bStopped(false)
		, m_bHalted(false)
		, m_bBreakpoint(false)
		, m_bBreakpointSkip(false)
		, m_currentInstruction(0)
		, m_currentExtendedInstruction(0)
		, m_currentInstructionCycles(0)
	{
		InitialiseInstructions();
	}

	CPU::~CPU()
	{
	}

	void CPU::Initialise(MMU* mmu)
	{
		m_mmu = mmu;
	}

	uint16_t CPU::Update(uint16_t maxcycles)
	{
		uint16_t cycles = 0;

		while(cycles < maxcycles)
		{
			// Check for breakpoints.
			HandleBreakpoints();

			if(m_bBreakpoint)
			{
				break;
			}

			// Check for interrupts before executing an instruction.
			HandleInterrupts();

			// Run the next instruction.
			m_currentInstruction = ImmediateByte();

			Instruction& instruction = m_instructions[m_currentInstruction];
			InstructionFunction& func = instruction.GetFunction();
			bool bActionPerformed = (this->*func)();
			Byte instCycles = instruction.GetCycles(bActionPerformed);
			assert(instCycles != 0);
			m_currentInstructionCycles += instCycles;

			// Update cycles
			cycles += m_currentInstructionCycles;
			m_currentInstructionCycles = 0;
		}

		return cycles;
	}

	void CPU::UpdateStalled()
	{
		HandleInterrupts();
	}

	bool CPU::IsStalled() const
	{
		return m_bStopped || m_bHalted;
	}

	const Registers& CPU::GetRegisters() const
	{
		return m_registers;
	}

	Byte CPU::ReadIO(HWRegs::Type reg)
	{
		return m_mmu->ReadByte(static_cast<Address>(reg));
	}

	void CPU::WriteIO(HWRegs::Type reg, Byte val)
	{
		m_mmu->WriteIO(reg, val);
	}

	void CPU::GenerateInterrupt(HWInterrupts::Type interrupt)
	{
		m_mmu->WriteByte(HWRegs::IF, m_mmu->ReadByte(HWRegs::IF) | static_cast<Byte>(interrupt));
	}

	bool CPU::IsBreakpoint() const
	{
		return m_bBreakpoint;
	}

	void CPU::BreakpointSet(Address address)
	{
		if(std::find(m_breakpoints.begin(), m_breakpoints.end(), address) == m_breakpoints.end())
		{
			m_breakpoints.push_back(address);
		}
	}

	void CPU::BreakpointRemove(Address address)
	{
		m_breakpoints.erase(std::remove(m_breakpoints.begin(), m_breakpoints.end(), address), m_breakpoints.end());
	}

	void CPU::BreakpointSkip()
	{
		m_bBreakpointSkip = true;
	}

	void CPU::HandleBreakpoints()
	{
		m_bBreakpoint = false;

		if (m_bBreakpointSkip)
		{
			m_bBreakpointSkip = false;
		}
		else
		{
			if (m_mmu->CheckResetBreakpoint())
			{
				m_bBreakpoint = true;
			}
			else
			{
				for (auto bp : m_breakpoints)
				{
					if (m_registers.pc == bp)
					{
						m_bBreakpoint = true;
						return;
					}
				}
			}
		}
	}

	void CPU::HandleInterrupts()
	{
		Byte regif = m_mmu->ReadByte(HWRegs::IF);
		Byte regie = m_mmu->ReadByte(HWRegs::IE);

		if(regif == 0)
		{
			return;
		}

		HandleInterrupt(HWInterrupts::VBlank, HWInterruptRoutines::VBlank, regif, regie);
		HandleInterrupt(HWInterrupts::Stat,   HWInterruptRoutines::Stat,   regif, regie);
		HandleInterrupt(HWInterrupts::Timer,  HWInterruptRoutines::Timer,  regif, regie);
		HandleInterrupt(HWInterrupts::Serial, HWInterruptRoutines::Serial, regif, regie);
		HandleInterrupt(HWInterrupts::Button, HWInterruptRoutines::Button, regif, regie);
	}

	void CPU::HandleInterrupt(HWInterrupts::Type interrupt, HWInterruptRoutines::Type routine, Byte regif, Byte regie)
	{
		Byte inter = static_cast<Byte>(interrupt);

		if((regif & inter) && (regie & inter))
		{
			if(m_registers.ime)
			{
				//gbhw::Message("Handling interrupt: %s\n", HWInterrupts::ToStr(interrupt));

				if (HWInterrupts::Timer == interrupt)
				{
					Message("Handling timer interrupt\n");
				}

				m_mmu->WriteByte(HWRegs::IF, regif & ~(inter));			// Remove flag, indicating handled.
				m_registers.ime = false;								// Disable interrupts.
				StackPushWord(m_registers.pc);							// Push current instruction onto the stack.
				m_registers.pc = static_cast<Word>(routine);			// Jump to interrupt routine.
			}

			if ((interrupt == HWInterrupts::Button) && m_bStopped)
			{
				Message("Button interrupt handled whilst stopped, resuming\n");
				m_bStopped = false;
			}

			// Resume hardware execution regardless of ime.
			if (m_bHalted)
			{
				Message("Interrupt has resumed the hardware\n");
				m_bHalted = false;
			}
		}
	}

	void CPU::InitialiseInstructions()
	{
		// Initialise with instruction meta-data.
		SetupInstructions(m_instructions);
		SetupInstructionsExtended(m_instructionsExtended);

		// Bind everything as unimplemented initially.
		for(uint32_t i = 0; i < kInstructionCount; ++i)
		{
			m_instructions[i].Set(&CPU::InstructionNotImplemented);
			m_instructionsExtended[i].Set(&CPU::InstructionNotImplementedExt);
		}

		// Bind implemented instructions

		// Misc.
		m_instructions[0x00].Set(&CPU::Instruction_NOP);
		m_instructions[0x27].Set(&CPU::Instruction_DAA);
		m_instructions[0x2F].Set(&CPU::Instruction_CPL);
		m_instructions[0x3F].Set(&CPU::Instruction_CCF);
		m_instructions[0x37].Set(&CPU::Instruction_SCF);
		m_instructions[0x76].Set(&CPU::Instruction_HALT);
		m_instructions[0x10].Set(&CPU::Instruction_STOP);
		m_instructions[0xF3].Set(&CPU::Instruction_DI);
		m_instructions[0xFB].Set(&CPU::Instruction_EI);

		// Jumps
		m_instructions[0xC3].Set(&CPU::Instruction_JP);
		m_instructions[0xC2].Set(&CPU::Instruction_JP_CC<RF::Zero,  false>);
		m_instructions[0xCA].Set(&CPU::Instruction_JP_CC<RF::Zero,  true>);
		m_instructions[0xD2].Set(&CPU::Instruction_JP_CC<RF::Carry, false>);
		m_instructions[0xDA].Set(&CPU::Instruction_JP_CC<RF::Carry, true>);
		m_instructions[0xE9].Set(&CPU::Instruction_JP_HL);
		m_instructions[0x18].Set(&CPU::Instruction_JR);
		m_instructions[0x20].Set(&CPU::Instruction_JR_CC<RF::Zero, false>);
		m_instructions[0x28].Set(&CPU::Instruction_JR_CC<RF::Zero, true>);
		m_instructions[0x30].Set(&CPU::Instruction_JR_CC<RF::Carry, false>);
		m_instructions[0x38].Set(&CPU::Instruction_JR_CC<RF::Carry, true>);

		// Calls
		m_instructions[0xCD].Set(&CPU::Instruction_CALL);
		m_instructions[0xC4].Set(&CPU::Instruction_CALL_CC<RF::Zero, false>);
		m_instructions[0xCC].Set(&CPU::Instruction_CALL_CC<RF::Zero, true>);
		m_instructions[0xD4].Set(&CPU::Instruction_CALL_CC<RF::Carry, false>);
		m_instructions[0xDC].Set(&CPU::Instruction_CALL_CC<RF::Carry, true>);

		// Restarts
		m_instructions[0xC7].Set(&CPU::Instruction_RST<0x00>);
		m_instructions[0xCF].Set(&CPU::Instruction_RST<0x08>);
		m_instructions[0xD7].Set(&CPU::Instruction_RST<0x10>);
		m_instructions[0xDF].Set(&CPU::Instruction_RST<0x18>);
		m_instructions[0xE7].Set(&CPU::Instruction_RST<0x20>);
		m_instructions[0xEF].Set(&CPU::Instruction_RST<0x28>);
		m_instructions[0xF7].Set(&CPU::Instruction_RST<0x30>);
		m_instructions[0xFF].Set(&CPU::Instruction_RST<0x38>);

		// Returns
		m_instructions[0xC9].Set(&CPU::Instruction_RET);
		m_instructions[0xC0].Set(&CPU::Instruction_RET_CC<RF::Zero, false>);
		m_instructions[0xC8].Set(&CPU::Instruction_RET_CC<RF::Zero, true>);
		m_instructions[0xD0].Set(&CPU::Instruction_RET_CC<RF::Carry, false>);
		m_instructions[0xD8].Set(&CPU::Instruction_RET_CC<RF::Carry, true>);
		m_instructions[0xD9].Set(&CPU::Instruction_RETI);

		// 8-bit Loads
		m_instructions[0x7F].Set(&CPU::Instruction_LD_N_N<RT::A, RT::A>);
		m_instructions[0x78].Set(&CPU::Instruction_LD_N_N<RT::A, RT::B>);
		m_instructions[0x79].Set(&CPU::Instruction_LD_N_N<RT::A, RT::C>);
		m_instructions[0x7A].Set(&CPU::Instruction_LD_N_N<RT::A, RT::D>);
		m_instructions[0x7B].Set(&CPU::Instruction_LD_N_N<RT::A, RT::E>);
		m_instructions[0x7C].Set(&CPU::Instruction_LD_N_N<RT::A, RT::H>);
		m_instructions[0x7D].Set(&CPU::Instruction_LD_N_N<RT::A, RT::L>);
		m_instructions[0x0A].Set(&CPU::Instruction_LD_N_N_PTR<RT::A, RT::BC>);
		m_instructions[0x1A].Set(&CPU::Instruction_LD_N_N_PTR<RT::A, RT::DE>);
		m_instructions[0x7E].Set(&CPU::Instruction_LD_N_N_PTR<RT::A, RT::HL>);

		m_instructions[0x47].Set(&CPU::Instruction_LD_N_N<RT::B, RT::A>);
		m_instructions[0x40].Set(&CPU::Instruction_LD_N_N<RT::B, RT::B>);
		m_instructions[0x41].Set(&CPU::Instruction_LD_N_N<RT::B, RT::C>);
		m_instructions[0x42].Set(&CPU::Instruction_LD_N_N<RT::B, RT::D>);
		m_instructions[0x43].Set(&CPU::Instruction_LD_N_N<RT::B, RT::E>);
		m_instructions[0x44].Set(&CPU::Instruction_LD_N_N<RT::B, RT::H>);
		m_instructions[0x45].Set(&CPU::Instruction_LD_N_N<RT::B, RT::L>);
		m_instructions[0x46].Set(&CPU::Instruction_LD_N_N_PTR<RT::B, RT::HL>);

		m_instructions[0x4F].Set(&CPU::Instruction_LD_N_N<RT::C, RT::A>);
		m_instructions[0x48].Set(&CPU::Instruction_LD_N_N<RT::C, RT::B>);
		m_instructions[0x49].Set(&CPU::Instruction_LD_N_N<RT::C, RT::C>);
		m_instructions[0x4A].Set(&CPU::Instruction_LD_N_N<RT::C, RT::D>);
		m_instructions[0x4B].Set(&CPU::Instruction_LD_N_N<RT::C, RT::E>);
		m_instructions[0x4C].Set(&CPU::Instruction_LD_N_N<RT::C, RT::H>);
		m_instructions[0x4D].Set(&CPU::Instruction_LD_N_N<RT::C, RT::L>);
		m_instructions[0x4E].Set(&CPU::Instruction_LD_N_N_PTR<RT::C, RT::HL>);

		m_instructions[0x57].Set(&CPU::Instruction_LD_N_N<RT::D, RT::A>);
		m_instructions[0x50].Set(&CPU::Instruction_LD_N_N<RT::D, RT::B>);
		m_instructions[0x51].Set(&CPU::Instruction_LD_N_N<RT::D, RT::C>);
		m_instructions[0x52].Set(&CPU::Instruction_LD_N_N<RT::D, RT::D>);
		m_instructions[0x53].Set(&CPU::Instruction_LD_N_N<RT::D, RT::E>);
		m_instructions[0x54].Set(&CPU::Instruction_LD_N_N<RT::D, RT::H>);
		m_instructions[0x55].Set(&CPU::Instruction_LD_N_N<RT::D, RT::L>);
		m_instructions[0x56].Set(&CPU::Instruction_LD_N_N_PTR<RT::D, RT::HL>);

		m_instructions[0x5F].Set(&CPU::Instruction_LD_N_N<RT::E, RT::A>);
		m_instructions[0x58].Set(&CPU::Instruction_LD_N_N<RT::E, RT::B>);
		m_instructions[0x59].Set(&CPU::Instruction_LD_N_N<RT::E, RT::C>);
		m_instructions[0x5A].Set(&CPU::Instruction_LD_N_N<RT::E, RT::D>);
		m_instructions[0x5B].Set(&CPU::Instruction_LD_N_N<RT::E, RT::E>);
		m_instructions[0x5C].Set(&CPU::Instruction_LD_N_N<RT::E, RT::H>);
		m_instructions[0x5D].Set(&CPU::Instruction_LD_N_N<RT::E, RT::L>);
		m_instructions[0x5E].Set(&CPU::Instruction_LD_N_N_PTR<RT::E, RT::HL>);

		m_instructions[0x67].Set(&CPU::Instruction_LD_N_N<RT::H, RT::A>);
		m_instructions[0x60].Set(&CPU::Instruction_LD_N_N<RT::H, RT::B>);
		m_instructions[0x61].Set(&CPU::Instruction_LD_N_N<RT::H, RT::C>);
		m_instructions[0x62].Set(&CPU::Instruction_LD_N_N<RT::H, RT::D>);
		m_instructions[0x63].Set(&CPU::Instruction_LD_N_N<RT::H, RT::E>);
		m_instructions[0x64].Set(&CPU::Instruction_LD_N_N<RT::H, RT::H>);
		m_instructions[0x65].Set(&CPU::Instruction_LD_N_N<RT::H, RT::L>);
		m_instructions[0x66].Set(&CPU::Instruction_LD_N_N_PTR<RT::H, RT::HL>);

		m_instructions[0x6F].Set(&CPU::Instruction_LD_N_N<RT::L, RT::A>);
		m_instructions[0x68].Set(&CPU::Instruction_LD_N_N<RT::L, RT::B>);
		m_instructions[0x69].Set(&CPU::Instruction_LD_N_N<RT::L, RT::C>);
		m_instructions[0x6A].Set(&CPU::Instruction_LD_N_N<RT::L, RT::D>);
		m_instructions[0x6B].Set(&CPU::Instruction_LD_N_N<RT::L, RT::E>);
		m_instructions[0x6C].Set(&CPU::Instruction_LD_N_N<RT::L, RT::H>);
		m_instructions[0x6D].Set(&CPU::Instruction_LD_N_N<RT::L, RT::L>);
		m_instructions[0x6E].Set(&CPU::Instruction_LD_N_N_PTR<RT::L, RT::HL>);

		m_instructions[0x02].Set(&CPU::Instruction_LD_N_PTR_N<RT::BC, RT::A>);
		m_instructions[0x12].Set(&CPU::Instruction_LD_N_PTR_N<RT::DE, RT::A>);
		m_instructions[0x77].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::A>);
		m_instructions[0x70].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::B>);
		m_instructions[0x71].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::C>);
		m_instructions[0x72].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::D>);
		m_instructions[0x73].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::E>);
		m_instructions[0x74].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::H>);
		m_instructions[0x75].Set(&CPU::Instruction_LD_N_PTR_N<RT::HL, RT::L>);

		m_instructions[0x3E].Set(&CPU::Instruction_LD_N_IMM<RT::A>);
		m_instructions[0x06].Set(&CPU::Instruction_LD_N_IMM<RT::B>);
		m_instructions[0x0E].Set(&CPU::Instruction_LD_N_IMM<RT::C>);
		m_instructions[0x16].Set(&CPU::Instruction_LD_N_IMM<RT::D>);
		m_instructions[0x1E].Set(&CPU::Instruction_LD_N_IMM<RT::E>);
		m_instructions[0x26].Set(&CPU::Instruction_LD_N_IMM<RT::H>);
		m_instructions[0x2E].Set(&CPU::Instruction_LD_N_IMM<RT::L>);

		m_instructions[0xFA].Set(&CPU::Instruction_LD_A_IMM_PTR);
		m_instructions[0xEA].Set(&CPU::Instruction_LD_IMM_PTR_A);
		m_instructions[0x36].Set(&CPU::Instruction_LD_HL_PTR_IMM);
		m_instructions[0x32].Set(&CPU::Instruction_LDD_HL_PTR_A);
		m_instructions[0x3A].Set(&CPU::Instruction_LDD_A_HL_PTR);
		m_instructions[0x22].Set(&CPU::Instruction_LDI_HL_PTR_A);
		m_instructions[0x2A].Set(&CPU::Instruction_LDI_A_HL_PTR);
		m_instructions[0xE0].Set(&CPU::Instruction_LDH_IMM_PTR_A);
		m_instructions[0xF0].Set(&CPU::Instruction_LDH_A_IMM_PTR);
		m_instructions[0xE2].Set(&CPU::Instruction_LD_C_PTR_A);
		m_instructions[0xF2].Set(&CPU::Instruction_LD_A_C_PTR);
		
		// 16-bit Loads
		m_instructions[0x01].Set(&CPU::Instruction_LD_NN_IMM<RT::BC>);
		m_instructions[0x11].Set(&CPU::Instruction_LD_NN_IMM<RT::DE>);
		m_instructions[0x21].Set(&CPU::Instruction_LD_NN_IMM<RT::HL>);
		m_instructions[0x31].Set(&CPU::Instruction_LD_NN_IMM<RT::StackPointer>);

		m_instructions[0xF9].Set(&CPU::Instruction_LD_SP_HL);
		m_instructions[0xF8].Set(&CPU::Instruction_LD_HL_SP_IMM);
		m_instructions[0x08].Set(&CPU::Instruction_IMM_PTR_SP);

		m_instructions[0xF5].Set(&CPU::Instruction_PUSH_NN<RT::AF>);
		m_instructions[0xC5].Set(&CPU::Instruction_PUSH_NN<RT::BC>);
		m_instructions[0xD5].Set(&CPU::Instruction_PUSH_NN<RT::DE>);
		m_instructions[0xE5].Set(&CPU::Instruction_PUSH_NN<RT::HL>);

		m_instructions[0xF1].Set(&CPU::Instruction_POP_AF);
		m_instructions[0xC1].Set(&CPU::Instruction_POP_NN<RT::BC>);
		m_instructions[0xD1].Set(&CPU::Instruction_POP_NN<RT::DE>);
		m_instructions[0xE1].Set(&CPU::Instruction_POP_NN<RT::HL>);

		// 8-bit ALU - ADD
		m_instructions[0x87].Set(&CPU::Instruction_ADD_N<RT::A>);
		m_instructions[0x80].Set(&CPU::Instruction_ADD_N<RT::B>);
		m_instructions[0x81].Set(&CPU::Instruction_ADD_N<RT::C>);
		m_instructions[0x82].Set(&CPU::Instruction_ADD_N<RT::D>);
		m_instructions[0x83].Set(&CPU::Instruction_ADD_N<RT::E>);
		m_instructions[0x84].Set(&CPU::Instruction_ADD_N<RT::H>);
		m_instructions[0x85].Set(&CPU::Instruction_ADD_N<RT::L>);
		m_instructions[0x86].Set(&CPU::Instruction_ADD_HL_PTR);
		m_instructions[0xC6].Set(&CPU::Instruction_ADD_IMM);

		// 8-bit ALU - ADC
		m_instructions[0x8F].Set(&CPU::Instruction_ADC_N<RT::A>);
		m_instructions[0x88].Set(&CPU::Instruction_ADC_N<RT::B>);
		m_instructions[0x89].Set(&CPU::Instruction_ADC_N<RT::C>);
		m_instructions[0x8A].Set(&CPU::Instruction_ADC_N<RT::D>);
		m_instructions[0x8B].Set(&CPU::Instruction_ADC_N<RT::E>);
		m_instructions[0x8C].Set(&CPU::Instruction_ADC_N<RT::H>);
		m_instructions[0x8D].Set(&CPU::Instruction_ADC_N<RT::L>);
		m_instructions[0x8E].Set(&CPU::Instruction_ADC_HL_PTR);
		m_instructions[0xCE].Set(&CPU::Instruction_ADC_IMM);

		// 8-bit ALU - SUB
		m_instructions[0x97].Set(&CPU::Instruction_SUB_N<RT::A>);
		m_instructions[0x90].Set(&CPU::Instruction_SUB_N<RT::B>);
		m_instructions[0x91].Set(&CPU::Instruction_SUB_N<RT::C>);
		m_instructions[0x92].Set(&CPU::Instruction_SUB_N<RT::D>);
		m_instructions[0x93].Set(&CPU::Instruction_SUB_N<RT::E>);
		m_instructions[0x94].Set(&CPU::Instruction_SUB_N<RT::H>);
		m_instructions[0x95].Set(&CPU::Instruction_SUB_N<RT::L>);
		m_instructions[0x96].Set(&CPU::Instruction_SUB_HL_PTR);
		m_instructions[0xD6].Set(&CPU::Instruction_SUB_IMM);

		// 8-bit ALU - SBC
		m_instructions[0x9F].Set(&CPU::Instruction_SBC_N<RT::A>);
		m_instructions[0x98].Set(&CPU::Instruction_SBC_N<RT::B>);
		m_instructions[0x99].Set(&CPU::Instruction_SBC_N<RT::C>);
		m_instructions[0x9A].Set(&CPU::Instruction_SBC_N<RT::D>);
		m_instructions[0x9B].Set(&CPU::Instruction_SBC_N<RT::E>);
		m_instructions[0x9C].Set(&CPU::Instruction_SBC_N<RT::H>);
		m_instructions[0x9D].Set(&CPU::Instruction_SBC_N<RT::L>);
		m_instructions[0x9E].Set(&CPU::Instruction_SBC_HL_PTR);
		m_instructions[0xDE].Set(&CPU::Instruction_SBC_IMM);

		// 8-bit ALU - AND
		m_instructions[0xA7].Set(&CPU::Instruction_AND_N<RT::A>);
		m_instructions[0xA0].Set(&CPU::Instruction_AND_N<RT::B>);
		m_instructions[0xA1].Set(&CPU::Instruction_AND_N<RT::C>);
		m_instructions[0xA2].Set(&CPU::Instruction_AND_N<RT::D>);
		m_instructions[0xA3].Set(&CPU::Instruction_AND_N<RT::E>);
		m_instructions[0xA4].Set(&CPU::Instruction_AND_N<RT::H>);
		m_instructions[0xA5].Set(&CPU::Instruction_AND_N<RT::L>);
		m_instructions[0xA6].Set(&CPU::Instruction_AND_HL_PTR);
		m_instructions[0xE6].Set(&CPU::Instruction_AND_IMM);

		// 8-bit ALU - OR
		m_instructions[0xB7].Set(&CPU::Instruction_OR_N<RT::A>);
		m_instructions[0xB0].Set(&CPU::Instruction_OR_N<RT::B>);
		m_instructions[0xB1].Set(&CPU::Instruction_OR_N<RT::C>);
		m_instructions[0xB2].Set(&CPU::Instruction_OR_N<RT::D>);
		m_instructions[0xB3].Set(&CPU::Instruction_OR_N<RT::E>);
		m_instructions[0xB4].Set(&CPU::Instruction_OR_N<RT::H>);
		m_instructions[0xB5].Set(&CPU::Instruction_OR_N<RT::L>);
		m_instructions[0xB6].Set(&CPU::Instruction_OR_HL_PTR);
		m_instructions[0xF6].Set(&CPU::Instruction_OR_IMM);

		// 8-bit ALU - XOR
		m_instructions[0xAF].Set(&CPU::Instruction_XOR_N<RT::A>);
		m_instructions[0xA8].Set(&CPU::Instruction_XOR_N<RT::B>);
		m_instructions[0xA9].Set(&CPU::Instruction_XOR_N<RT::C>);
		m_instructions[0xAA].Set(&CPU::Instruction_XOR_N<RT::D>);
		m_instructions[0xAB].Set(&CPU::Instruction_XOR_N<RT::E>);
		m_instructions[0xAC].Set(&CPU::Instruction_XOR_N<RT::H>);
		m_instructions[0xAD].Set(&CPU::Instruction_XOR_N<RT::L>);
		m_instructions[0xAE].Set(&CPU::Instruction_XOR_HL_PTR);
		m_instructions[0xEE].Set(&CPU::Instruction_XOR_IMM);

		// 8-bit ALU - CP
		m_instructions[0xBF].Set(&CPU::Instruction_CP_N<RT::A>);
		m_instructions[0xB8].Set(&CPU::Instruction_CP_N<RT::B>);
		m_instructions[0xB9].Set(&CPU::Instruction_CP_N<RT::C>);
		m_instructions[0xBA].Set(&CPU::Instruction_CP_N<RT::D>);
		m_instructions[0xBB].Set(&CPU::Instruction_CP_N<RT::E>);
		m_instructions[0xBC].Set(&CPU::Instruction_CP_N<RT::H>);
		m_instructions[0xBD].Set(&CPU::Instruction_CP_N<RT::L>);
		m_instructions[0xBE].Set(&CPU::Instruction_CP_HL_PTR);
		m_instructions[0xFE].Set(&CPU::Instruction_CP_IMM);

		// 8-bit ALU - INC
		m_instructions[0x3C].Set(&CPU::Instruction_INC_N<RT::A>);
		m_instructions[0x04].Set(&CPU::Instruction_INC_N<RT::B>);
		m_instructions[0x0C].Set(&CPU::Instruction_INC_N<RT::C>);
		m_instructions[0x14].Set(&CPU::Instruction_INC_N<RT::D>);
		m_instructions[0x1C].Set(&CPU::Instruction_INC_N<RT::E>);
		m_instructions[0x24].Set(&CPU::Instruction_INC_N<RT::H>);
		m_instructions[0x2C].Set(&CPU::Instruction_INC_N<RT::L>);
		m_instructions[0x34].Set(&CPU::Instruction_INC_HL_PTR);

		// 8-bit ALU - DEC
		m_instructions[0x3D].Set(&CPU::Instruction_DEC_N<RT::A>);
		m_instructions[0x05].Set(&CPU::Instruction_DEC_N<RT::B>);
		m_instructions[0x0D].Set(&CPU::Instruction_DEC_N<RT::C>);
		m_instructions[0x15].Set(&CPU::Instruction_DEC_N<RT::D>);
		m_instructions[0x1D].Set(&CPU::Instruction_DEC_N<RT::E>);
		m_instructions[0x25].Set(&CPU::Instruction_DEC_N<RT::H>);
		m_instructions[0x2D].Set(&CPU::Instruction_DEC_N<RT::L>);
		m_instructions[0x35].Set(&CPU::Instruction_DEC_HL_PTR);

		// 16-bit ALU - ADD
		m_instructions[0x09].Set(&CPU::Instruction_ADD_HL_NN<RT::BC>);
		m_instructions[0x19].Set(&CPU::Instruction_ADD_HL_NN<RT::DE>);
		m_instructions[0x29].Set(&CPU::Instruction_ADD_HL_NN<RT::HL>);
		m_instructions[0x39].Set(&CPU::Instruction_ADD_HL_NN<RT::StackPointer>);
		m_instructions[0xE8].Set(&CPU::Instruction_ADD_SP_IMM);

		// 16-bit ALU - INC
		m_instructions[0x03].Set(&CPU::Instruction_INC_NN<RT::BC>);
		m_instructions[0x13].Set(&CPU::Instruction_INC_NN<RT::DE>);
		m_instructions[0x23].Set(&CPU::Instruction_INC_NN<RT::HL>);
		m_instructions[0x33].Set(&CPU::Instruction_INC_NN<RT::StackPointer>);

		// 16-bit ALU - DEC
		m_instructions[0x0B].Set(&CPU::Instruction_DEC_NN<RT::BC>);
		m_instructions[0x1B].Set(&CPU::Instruction_DEC_NN<RT::DE>);
		m_instructions[0x2B].Set(&CPU::Instruction_DEC_NN<RT::HL>);
		m_instructions[0x3B].Set(&CPU::Instruction_DEC_NN<RT::StackPointer>);

		// Rotates & Shifts
		m_instructions[0x07].Set(&CPU::Instruction_RLCA);
		m_instructions[0x0f].Set(&CPU::Instruction_RRCA);
		m_instructions[0x17].Set(&CPU::Instruction_RLA);
		m_instructions[0x1f].Set(&CPU::Instruction_RRA);

		// Extended instructions
		m_instructions[0xCB].Set(&CPU::Instruction_EXT);

		// Extended - RCL
		m_instructionsExtended[0x00].Set(&CPU::Instruction_EXT_RLC_N<RT::B>);
		m_instructionsExtended[0x01].Set(&CPU::Instruction_EXT_RLC_N<RT::C>);
		m_instructionsExtended[0x02].Set(&CPU::Instruction_EXT_RLC_N<RT::D>);
		m_instructionsExtended[0x03].Set(&CPU::Instruction_EXT_RLC_N<RT::E>);
		m_instructionsExtended[0x04].Set(&CPU::Instruction_EXT_RLC_N<RT::H>);
		m_instructionsExtended[0x05].Set(&CPU::Instruction_EXT_RLC_N<RT::L>);
		m_instructionsExtended[0x06].Set(&CPU::Instruction_EXT_RLC_HL_ADDR);
		m_instructionsExtended[0x07].Set(&CPU::Instruction_EXT_RLC_N<RT::A>);

		// Extended - RRC
		m_instructionsExtended[0x08].Set(&CPU::Instruction_EXT_RRC_N<RT::B>);
		m_instructionsExtended[0x09].Set(&CPU::Instruction_EXT_RRC_N<RT::C>);
		m_instructionsExtended[0x0A].Set(&CPU::Instruction_EXT_RRC_N<RT::D>);
		m_instructionsExtended[0x0B].Set(&CPU::Instruction_EXT_RRC_N<RT::E>);
		m_instructionsExtended[0x0C].Set(&CPU::Instruction_EXT_RRC_N<RT::H>);
		m_instructionsExtended[0x0D].Set(&CPU::Instruction_EXT_RRC_N<RT::L>);
		m_instructionsExtended[0x0E].Set(&CPU::Instruction_EXT_RRC_HL_ADDR);
		m_instructionsExtended[0x0F].Set(&CPU::Instruction_EXT_RRC_N<RT::A>);

		// Extended - RL
		m_instructionsExtended[0x10].Set(&CPU::Instruction_EXT_RL_N<RT::B>);
		m_instructionsExtended[0x11].Set(&CPU::Instruction_EXT_RL_N<RT::C>);
		m_instructionsExtended[0x12].Set(&CPU::Instruction_EXT_RL_N<RT::D>);
		m_instructionsExtended[0x13].Set(&CPU::Instruction_EXT_RL_N<RT::E>);
		m_instructionsExtended[0x14].Set(&CPU::Instruction_EXT_RL_N<RT::H>);
		m_instructionsExtended[0x15].Set(&CPU::Instruction_EXT_RL_N<RT::L>);
		m_instructionsExtended[0x16].Set(&CPU::Instruction_EXT_RL_HL_ADDR);
		m_instructionsExtended[0x17].Set(&CPU::Instruction_EXT_RL_N<RT::A>);

		// Extended - RR
		m_instructionsExtended[0x18].Set(&CPU::Instruction_EXT_RR_N<RT::B>);
		m_instructionsExtended[0x19].Set(&CPU::Instruction_EXT_RR_N<RT::C>);
		m_instructionsExtended[0x1A].Set(&CPU::Instruction_EXT_RR_N<RT::D>);
		m_instructionsExtended[0x1B].Set(&CPU::Instruction_EXT_RR_N<RT::E>);
		m_instructionsExtended[0x1C].Set(&CPU::Instruction_EXT_RR_N<RT::H>);
		m_instructionsExtended[0x1D].Set(&CPU::Instruction_EXT_RR_N<RT::L>);
		m_instructionsExtended[0x1E].Set(&CPU::Instruction_EXT_RR_HL_ADDR);
		m_instructionsExtended[0x1F].Set(&CPU::Instruction_EXT_RR_N<RT::A>);

		// Extended - SLA
		m_instructionsExtended[0x20].Set(&CPU::Instruction_EXT_SLA_N<RT::B>);
		m_instructionsExtended[0x21].Set(&CPU::Instruction_EXT_SLA_N<RT::C>);
		m_instructionsExtended[0x22].Set(&CPU::Instruction_EXT_SLA_N<RT::D>);
		m_instructionsExtended[0x23].Set(&CPU::Instruction_EXT_SLA_N<RT::E>);
		m_instructionsExtended[0x24].Set(&CPU::Instruction_EXT_SLA_N<RT::H>);
		m_instructionsExtended[0x25].Set(&CPU::Instruction_EXT_SLA_N<RT::L>);
		m_instructionsExtended[0x26].Set(&CPU::Instruction_EXT_SLA_HL_ADDR);
		m_instructionsExtended[0x27].Set(&CPU::Instruction_EXT_SLA_N<RT::A>);

		// Extended - SRA
		m_instructionsExtended[0x28].Set(&CPU::Instruction_EXT_SRA_N<RT::B>);
		m_instructionsExtended[0x29].Set(&CPU::Instruction_EXT_SRA_N<RT::C>);
		m_instructionsExtended[0x2A].Set(&CPU::Instruction_EXT_SRA_N<RT::D>);
		m_instructionsExtended[0x2B].Set(&CPU::Instruction_EXT_SRA_N<RT::E>);
		m_instructionsExtended[0x2C].Set(&CPU::Instruction_EXT_SRA_N<RT::H>);
		m_instructionsExtended[0x2D].Set(&CPU::Instruction_EXT_SRA_N<RT::L>);
		m_instructionsExtended[0x2E].Set(&CPU::Instruction_EXT_SRA_HL_ADDR);
		m_instructionsExtended[0x2F].Set(&CPU::Instruction_EXT_SRA_N<RT::A>);

		// Extended - SWAP
		m_instructionsExtended[0x30].Set(&CPU::Instruction_EXT_SWAP_N<RT::B>);
		m_instructionsExtended[0x31].Set(&CPU::Instruction_EXT_SWAP_N<RT::C>);
		m_instructionsExtended[0x32].Set(&CPU::Instruction_EXT_SWAP_N<RT::D>);
		m_instructionsExtended[0x33].Set(&CPU::Instruction_EXT_SWAP_N<RT::E>);
		m_instructionsExtended[0x34].Set(&CPU::Instruction_EXT_SWAP_N<RT::H>);
		m_instructionsExtended[0x35].Set(&CPU::Instruction_EXT_SWAP_N<RT::L>);
		m_instructionsExtended[0x36].Set(&CPU::Instruction_EXT_SWAP_HL_PTR);
		m_instructionsExtended[0x37].Set(&CPU::Instruction_EXT_SWAP_N<RT::A>);

		// Extended - SRL
		m_instructionsExtended[0x38].Set(&CPU::Instruction_EXT_SRL_N<RT::B>);
		m_instructionsExtended[0x39].Set(&CPU::Instruction_EXT_SRL_N<RT::C>);
		m_instructionsExtended[0x3A].Set(&CPU::Instruction_EXT_SRL_N<RT::D>);
		m_instructionsExtended[0x3B].Set(&CPU::Instruction_EXT_SRL_N<RT::E>);
		m_instructionsExtended[0x3C].Set(&CPU::Instruction_EXT_SRL_N<RT::H>);
		m_instructionsExtended[0x3D].Set(&CPU::Instruction_EXT_SRL_N<RT::L>);
		m_instructionsExtended[0x3E].Set(&CPU::Instruction_EXT_SRL_HL_ADDR);
		m_instructionsExtended[0x3F].Set(&CPU::Instruction_EXT_SRL_N<RT::A>);

		// Extended - BIT
		m_instructionsExtended[0x40].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::B>);
		m_instructionsExtended[0x41].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::C>);
		m_instructionsExtended[0x42].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::D>);
		m_instructionsExtended[0x43].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::E>);
		m_instructionsExtended[0x44].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::H>);
		m_instructionsExtended[0x45].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::L>);
		m_instructionsExtended[0x46].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<0>);
		m_instructionsExtended[0x47].Set(&CPU::Instruction_EXT_BIT_B_N<0, RT::A>);
		m_instructionsExtended[0x48].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::B>);
		m_instructionsExtended[0x49].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::C>);
		m_instructionsExtended[0x4A].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::D>);
		m_instructionsExtended[0x4B].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::E>);
		m_instructionsExtended[0x4C].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::H>);
		m_instructionsExtended[0x4D].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::L>);
		m_instructionsExtended[0x4E].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<1>);
		m_instructionsExtended[0x4F].Set(&CPU::Instruction_EXT_BIT_B_N<1, RT::A>);
		m_instructionsExtended[0x50].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::B>);
		m_instructionsExtended[0x51].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::C>);
		m_instructionsExtended[0x52].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::D>);
		m_instructionsExtended[0x53].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::E>);
		m_instructionsExtended[0x54].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::H>);
		m_instructionsExtended[0x55].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::L>);
		m_instructionsExtended[0x56].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<2>);
		m_instructionsExtended[0x57].Set(&CPU::Instruction_EXT_BIT_B_N<2, RT::A>);
		m_instructionsExtended[0x58].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::B>);
		m_instructionsExtended[0x59].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::C>);
		m_instructionsExtended[0x5A].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::D>);
		m_instructionsExtended[0x5B].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::E>);
		m_instructionsExtended[0x5C].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::H>);
		m_instructionsExtended[0x5D].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::L>);
		m_instructionsExtended[0x5E].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<3>);
		m_instructionsExtended[0x5F].Set(&CPU::Instruction_EXT_BIT_B_N<3, RT::A>);
		m_instructionsExtended[0x60].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::B>);
		m_instructionsExtended[0x61].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::C>);
		m_instructionsExtended[0x62].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::D>);
		m_instructionsExtended[0x63].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::E>);
		m_instructionsExtended[0x64].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::H>);
		m_instructionsExtended[0x65].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::L>);
		m_instructionsExtended[0x66].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<4>);
		m_instructionsExtended[0x67].Set(&CPU::Instruction_EXT_BIT_B_N<4, RT::A>);
		m_instructionsExtended[0x68].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::B>);
		m_instructionsExtended[0x69].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::C>);
		m_instructionsExtended[0x6A].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::D>);
		m_instructionsExtended[0x6B].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::E>);
		m_instructionsExtended[0x6C].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::H>);
		m_instructionsExtended[0x6D].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::L>);
		m_instructionsExtended[0x6E].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<5>);
		m_instructionsExtended[0x6F].Set(&CPU::Instruction_EXT_BIT_B_N<5, RT::A>);
		m_instructionsExtended[0x70].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::B>);
		m_instructionsExtended[0x71].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::C>);
		m_instructionsExtended[0x72].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::D>);
		m_instructionsExtended[0x73].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::E>);
		m_instructionsExtended[0x74].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::H>);
		m_instructionsExtended[0x75].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::L>);
		m_instructionsExtended[0x76].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<6>);
		m_instructionsExtended[0x77].Set(&CPU::Instruction_EXT_BIT_B_N<6, RT::A>);
		m_instructionsExtended[0x78].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::B>);
		m_instructionsExtended[0x79].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::C>);
		m_instructionsExtended[0x7A].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::D>);
		m_instructionsExtended[0x7B].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::E>);
		m_instructionsExtended[0x7C].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::H>);
		m_instructionsExtended[0x7D].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::L>);
		m_instructionsExtended[0x7E].Set(&CPU::Instruction_EXT_BIT_B_HL_ADDR<7>);
		m_instructionsExtended[0x7F].Set(&CPU::Instruction_EXT_BIT_B_N<7, RT::A>);

		// Extended - RES
		m_instructionsExtended[0x80].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::B>);
		m_instructionsExtended[0x81].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::C>);
		m_instructionsExtended[0x82].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::D>);
		m_instructionsExtended[0x83].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::E>);
		m_instructionsExtended[0x84].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::H>);
		m_instructionsExtended[0x85].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::L>);
		m_instructionsExtended[0x86].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<0>);
		m_instructionsExtended[0x87].Set(&CPU::Instruction_EXT_RESET_B_N<0, RT::A>);
		m_instructionsExtended[0x88].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::B>);
		m_instructionsExtended[0x89].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::C>);
		m_instructionsExtended[0x8A].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::D>);
		m_instructionsExtended[0x8B].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::E>);
		m_instructionsExtended[0x8C].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::H>);
		m_instructionsExtended[0x8D].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::L>);
		m_instructionsExtended[0x8E].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<1>);
		m_instructionsExtended[0x8F].Set(&CPU::Instruction_EXT_RESET_B_N<1, RT::A>);
		m_instructionsExtended[0x90].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::B>);
		m_instructionsExtended[0x91].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::C>);
		m_instructionsExtended[0x92].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::D>);
		m_instructionsExtended[0x93].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::E>);
		m_instructionsExtended[0x94].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::H>);
		m_instructionsExtended[0x95].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::L>);
		m_instructionsExtended[0x96].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<2>);
		m_instructionsExtended[0x97].Set(&CPU::Instruction_EXT_RESET_B_N<2, RT::A>);
		m_instructionsExtended[0x98].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::B>);
		m_instructionsExtended[0x99].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::C>);
		m_instructionsExtended[0x9A].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::D>);
		m_instructionsExtended[0x9B].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::E>);
		m_instructionsExtended[0x9C].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::H>);
		m_instructionsExtended[0x9D].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::L>);
		m_instructionsExtended[0x9E].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<3>);
		m_instructionsExtended[0x9F].Set(&CPU::Instruction_EXT_RESET_B_N<3, RT::A>);
		m_instructionsExtended[0xA0].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::B>);
		m_instructionsExtended[0xA1].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::C>);
		m_instructionsExtended[0xA2].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::D>);
		m_instructionsExtended[0xA3].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::E>);
		m_instructionsExtended[0xA4].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::H>);
		m_instructionsExtended[0xA5].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::L>);
		m_instructionsExtended[0xA6].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<4>);
		m_instructionsExtended[0xA7].Set(&CPU::Instruction_EXT_RESET_B_N<4, RT::A>);
		m_instructionsExtended[0xA8].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::B>);
		m_instructionsExtended[0xA9].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::C>);
		m_instructionsExtended[0xAA].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::D>);
		m_instructionsExtended[0xAB].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::E>);
		m_instructionsExtended[0xAC].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::H>);
		m_instructionsExtended[0xAD].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::L>);
		m_instructionsExtended[0xAE].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<5>);
		m_instructionsExtended[0xAF].Set(&CPU::Instruction_EXT_RESET_B_N<5, RT::A>);
		m_instructionsExtended[0xB0].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::B>);
		m_instructionsExtended[0xB1].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::C>);
		m_instructionsExtended[0xB2].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::D>);
		m_instructionsExtended[0xB3].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::E>);
		m_instructionsExtended[0xB4].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::H>);
		m_instructionsExtended[0xB5].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::L>);
		m_instructionsExtended[0xB6].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<6>);
		m_instructionsExtended[0xB7].Set(&CPU::Instruction_EXT_RESET_B_N<6, RT::A>);
		m_instructionsExtended[0xB8].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::B>);
		m_instructionsExtended[0xB9].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::C>);
		m_instructionsExtended[0xBA].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::D>);
		m_instructionsExtended[0xBB].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::E>);
		m_instructionsExtended[0xBC].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::H>);
		m_instructionsExtended[0xBD].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::L>);
		m_instructionsExtended[0xBE].Set(&CPU::Instruction_EXT_RESET_B_HL_ADDR<7>);
		m_instructionsExtended[0xBF].Set(&CPU::Instruction_EXT_RESET_B_N<7, RT::A>);

		// Extended - SET
		m_instructionsExtended[0xC0].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::B>);
		m_instructionsExtended[0xC1].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::C>);
		m_instructionsExtended[0xC2].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::D>);
		m_instructionsExtended[0xC3].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::E>);
		m_instructionsExtended[0xC4].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::H>);
		m_instructionsExtended[0xC5].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::L>);
		m_instructionsExtended[0xC6].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<0>);
		m_instructionsExtended[0xC7].Set(&CPU::Instruction_EXT_SET_B_N<0, RT::A>);
		m_instructionsExtended[0xC8].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::B>);
		m_instructionsExtended[0xC9].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::C>);
		m_instructionsExtended[0xCA].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::D>);
		m_instructionsExtended[0xCB].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::E>);
		m_instructionsExtended[0xCC].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::H>);
		m_instructionsExtended[0xCD].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::L>);
		m_instructionsExtended[0xCE].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<1>);
		m_instructionsExtended[0xCF].Set(&CPU::Instruction_EXT_SET_B_N<1, RT::A>);
		m_instructionsExtended[0xD0].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::B>);
		m_instructionsExtended[0xD1].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::C>);
		m_instructionsExtended[0xD2].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::D>);
		m_instructionsExtended[0xD3].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::E>);
		m_instructionsExtended[0xD4].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::H>);
		m_instructionsExtended[0xD5].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::L>);
		m_instructionsExtended[0xD6].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<2>);
		m_instructionsExtended[0xD7].Set(&CPU::Instruction_EXT_SET_B_N<2, RT::A>);
		m_instructionsExtended[0xD8].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::B>);
		m_instructionsExtended[0xD9].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::C>);
		m_instructionsExtended[0xDA].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::D>);
		m_instructionsExtended[0xDB].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::E>);
		m_instructionsExtended[0xDC].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::H>);
		m_instructionsExtended[0xDD].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::L>);
		m_instructionsExtended[0xDE].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<3>);
		m_instructionsExtended[0xDF].Set(&CPU::Instruction_EXT_SET_B_N<3, RT::A>);
		m_instructionsExtended[0xE0].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::B>);
		m_instructionsExtended[0xE1].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::C>);
		m_instructionsExtended[0xE2].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::D>);
		m_instructionsExtended[0xE3].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::E>);
		m_instructionsExtended[0xE4].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::H>);
		m_instructionsExtended[0xE5].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::L>);
		m_instructionsExtended[0xE6].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<4>);
		m_instructionsExtended[0xE7].Set(&CPU::Instruction_EXT_SET_B_N<4, RT::A>);
		m_instructionsExtended[0xE8].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::B>);
		m_instructionsExtended[0xE9].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::C>);
		m_instructionsExtended[0xEA].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::D>);
		m_instructionsExtended[0xEB].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::E>);
		m_instructionsExtended[0xEC].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::H>);
		m_instructionsExtended[0xED].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::L>);
		m_instructionsExtended[0xEE].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<5>);
		m_instructionsExtended[0xEF].Set(&CPU::Instruction_EXT_SET_B_N<5, RT::A>);
		m_instructionsExtended[0xF0].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::B>);
		m_instructionsExtended[0xF1].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::C>);
		m_instructionsExtended[0xF2].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::D>);
		m_instructionsExtended[0xF3].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::E>);
		m_instructionsExtended[0xF4].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::H>);
		m_instructionsExtended[0xF5].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::L>);
		m_instructionsExtended[0xF6].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<6>);
		m_instructionsExtended[0xF7].Set(&CPU::Instruction_EXT_SET_B_N<6, RT::A>);
		m_instructionsExtended[0xF8].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::B>);
		m_instructionsExtended[0xF9].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::C>);
		m_instructionsExtended[0xFA].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::D>);
		m_instructionsExtended[0xFB].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::E>);
		m_instructionsExtended[0xFC].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::H>);
		m_instructionsExtended[0xFD].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::L>);
		m_instructionsExtended[0xFE].Set(&CPU::Instruction_EXT_SET_B_HL_ADDR<7>);
		m_instructionsExtended[0xFF].Set(&CPU::Instruction_EXT_SET_B_N<7, RT::A>);
	}

	bool CPU::InstructionNotImplemented()
	{
		Instruction& inst = m_instructions[m_currentInstruction];
		gbhw::Error("Instruction not implemented [Opcode: 0x%02x, Assembly: %s]\n", inst.GetOpcode(), inst.GetAssembly());
		throw std::runtime_error("Instruction not implemented");
	}

	bool CPU::InstructionNotImplementedExt()
	{
		Instruction& inst = m_instructionsExtended[m_currentExtendedInstruction];
		gbhw::Error("Extended instruction not implemented [Opcode: 0x%02x, Assembly: %s]\n", inst.GetExtended(), inst.GetAssembly());
		throw std::runtime_error("Extended instruction not implemented");
	}

} // gbhw