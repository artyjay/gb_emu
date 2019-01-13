#include "cpu.h"
#include "instructions.h"
#include "instructions_extended.h"
#include "log.h"

namespace gbhw
{
	//--------------------------------------------------------------------------
	// Instruction implementation
	//--------------------------------------------------------------------------

	Instruction::Instruction()
		: m_function(nullptr)
		, m_address(0)
	{
	}

	void Instruction::set(Byte opcode, Byte extended, Byte byteSize, Byte cycles0, Byte cycles1, RFB::Enum behaviour0, RFB::Enum behaviour1, RFB::Enum behaviour2, RFB::Enum behaviour3, RTD::Enum args0, RTD::Enum args1, const char* assembly)
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

	void Instruction::set(InstructionFunction fn)
	{
		if(m_function == nullptr || m_function == &CPU::instruction_not_implemented || m_function == &CPU::instruction_not_implemented_ext)
		{
			// Only allow setting if it was previously one of the above.
			m_function = fn;
		}
		else
		{
			log_error("Attempting to set an instructions function when it has already been set\n");
		}
	}

	void Instruction::set(Address address)
	{
		m_address = address;
	}

	//--------------------------------------------------------------------------
	// CPU implementation
	//--------------------------------------------------------------------------

	CPU::CPU()
		: m_bStopped(false)
		, m_bHalted(false)
		, m_speed(0)
		, m_currentOpcode(0)
		, m_currentOpcodeExt(0)
		, m_instructionCycles(0)
	{
	}

	CPU::~CPU()
	{
	}

	void CPU::initialise(MMU* mmu)
	{
		m_mmu = mmu;

		load_instructions();
	}

	uint16_t CPU::update(uint16_t maxcycles)
	{
		uint16_t cycles = 0;

		while(cycles < maxcycles)
		{
			// Check for interrupts before executing an instruction.
			handle_interrupts();

			// Run the next instruction.
			m_currentOpcode = immediate_byte();

			Instruction& instruction = m_instructions[m_currentOpcode];
			InstructionFunction& func = instruction.function();

			Byte instCycles = instruction.cycles((this->*func)());
			if(instCycles == 0)
				log_error("Instruction executes zero cycles, this is impossible, indicates unimplemented instruction\n");

			m_instructionCycles += instCycles;

			// Update cycles
			cycles += m_instructionCycles;
			m_instructionCycles = 0;
		}

		return cycles >> m_speed;
	}

	void CPU::update_stalled()
	{
		handle_interrupts();
	}

	bool CPU::is_stalled() const
	{
		return m_bStopped || m_bHalted;
	}

	void CPU::generate_interrupt(HWInterrupts::Type interrupt)
	{
		m_mmu->write_byte(HWRegs::IF, m_mmu->read_byte(HWRegs::IF) | static_cast<Byte>(interrupt));
	}

	void CPU::handle_interrupts()
	{
		Byte regif = m_mmu->read_byte(HWRegs::IF);
		Byte regie = m_mmu->read_byte(HWRegs::IE);

		if(regif == 0)
		{
			return;
		}

		handle_interrupt(HWInterrupts::VBlank, HWInterruptRoutines::VBlank, regif, regie);
		handle_interrupt(HWInterrupts::Stat,   HWInterruptRoutines::Stat,   regif, regie);
		handle_interrupt(HWInterrupts::Timer,  HWInterruptRoutines::Timer,  regif, regie);
		handle_interrupt(HWInterrupts::Serial, HWInterruptRoutines::Serial, regif, regie);
		handle_interrupt(HWInterrupts::Button, HWInterruptRoutines::Button, regif, regie);
	}

	void CPU::handle_interrupt(HWInterrupts::Type interrupt, HWInterruptRoutines::Type routine, Byte regif, Byte regie)
	{
		Byte inter = static_cast<Byte>(interrupt);

		if((regif & inter) && (regie & inter))
		{
			if(m_registers.ime)
			{
				if (HWInterrupts::Timer == interrupt)
				{
					log_debug("Handling timer interrupt\n");
				}

				m_mmu->write_byte(HWRegs::IF, regif & ~(inter));		// Remove flag, indicating handled.
				m_registers.ime = false;								// Disable interrupts.
				stack_push(m_registers.pc);								// Push current instruction onto the stack.
				m_registers.pc = static_cast<Word>(routine);			// Jump to interrupt routine.
			}

			if ((interrupt == HWInterrupts::Button) && m_bStopped)
			{
				log_debug("Button interrupt handled whilst stopped, resuming\n");
				m_bStopped = false;
			}

			// Resume hardware execution regardless of ime.
			if (m_bHalted)
			{
				log_debug("Interrupt has resumed the hardware\n");
				m_bHalted = false;
			}
		}
	}

	void CPU::load_instructions()
	{
		// Initialise with instruction meta-data.
		initialise_instructions(m_instructions);
		initialise_instructions_ext(m_instructionsExt);

		// Bind everything as unimplemented initially.
		for(uint32_t i = 0; i < kInstructionCount; ++i)
		{
			m_instructions[i].set(&CPU::instruction_not_implemented);
			m_instructionsExt[i].set(&CPU::instruction_not_implemented_ext);
		}

		// Bind implemented instructions

		// Misc.
		m_instructions[0x00].set(&CPU::inst_nop);
		m_instructions[0x27].set(&CPU::inst_daa);
		m_instructions[0x2F].set(&CPU::inst_cpl);
		m_instructions[0x3F].set(&CPU::inst_ccf);
		m_instructions[0x37].set(&CPU::inst_scf);
		m_instructions[0x76].set(&CPU::inst_halt);
		m_instructions[0x10].set(&CPU::inst_stop);
		m_instructions[0xF3].set(&CPU::inst_di);
		m_instructions[0xFB].set(&CPU::inst_ei);

		// Jumps
		m_instructions[0xC3].set(&CPU::inst_jp);
		m_instructions[0xC2].set(&CPU::inst_jp_cc<RF::Zero,  false>);
		m_instructions[0xCA].set(&CPU::inst_jp_cc<RF::Zero,  true>);
		m_instructions[0xD2].set(&CPU::inst_jp_cc<RF::Carry, false>);
		m_instructions[0xDA].set(&CPU::inst_jp_cc<RF::Carry, true>);
		m_instructions[0xE9].set(&CPU::inst_jp_hl);
		m_instructions[0x18].set(&CPU::inst_jr);
		m_instructions[0x20].set(&CPU::inst_jr_cc<RF::Zero, false>);
		m_instructions[0x28].set(&CPU::inst_jr_cc<RF::Zero, true>);
		m_instructions[0x30].set(&CPU::inst_jr_cc<RF::Carry, false>);
		m_instructions[0x38].set(&CPU::inst_jr_cc<RF::Carry, true>);

		// Calls
		m_instructions[0xCD].set(&CPU::inst_call);
		m_instructions[0xC4].set(&CPU::inst_call_cc<RF::Zero, false>);
		m_instructions[0xCC].set(&CPU::inst_call_cc<RF::Zero, true>);
		m_instructions[0xD4].set(&CPU::inst_call_cc<RF::Carry, false>);
		m_instructions[0xDC].set(&CPU::inst_call_cc<RF::Carry, true>);

		// Restarts
		m_instructions[0xC7].set(&CPU::inst_rst<0x00>);
		m_instructions[0xCF].set(&CPU::inst_rst<0x08>);
		m_instructions[0xD7].set(&CPU::inst_rst<0x10>);
		m_instructions[0xDF].set(&CPU::inst_rst<0x18>);
		m_instructions[0xE7].set(&CPU::inst_rst<0x20>);
		m_instructions[0xEF].set(&CPU::inst_rst<0x28>);
		m_instructions[0xF7].set(&CPU::inst_rst<0x30>);
		m_instructions[0xFF].set(&CPU::inst_rst<0x38>);

		// Returns
		m_instructions[0xC9].set(&CPU::inst_ret);
		m_instructions[0xC0].set(&CPU::inst_ret_cc<RF::Zero, false>);
		m_instructions[0xC8].set(&CPU::inst_ret_cc<RF::Zero, true>);
		m_instructions[0xD0].set(&CPU::inst_ret_cc<RF::Carry, false>);
		m_instructions[0xD8].set(&CPU::inst_ret_cc<RF::Carry, true>);
		m_instructions[0xD9].set(&CPU::inst_reti);

		// 8-bit Loads
		m_instructions[0x7F].set(&CPU::inst_ld_n_n<RT::A, RT::A>);
		m_instructions[0x78].set(&CPU::inst_ld_n_n<RT::A, RT::B>);
		m_instructions[0x79].set(&CPU::inst_ld_n_n<RT::A, RT::C>);
		m_instructions[0x7A].set(&CPU::inst_ld_n_n<RT::A, RT::D>);
		m_instructions[0x7B].set(&CPU::inst_ld_n_n<RT::A, RT::E>);
		m_instructions[0x7C].set(&CPU::inst_ld_n_n<RT::A, RT::H>);
		m_instructions[0x7D].set(&CPU::inst_ld_n_n<RT::A, RT::L>);
		m_instructions[0x0A].set(&CPU::inst_ld_n_n_ptr<RT::A, RT::BC>);
		m_instructions[0x1A].set(&CPU::inst_ld_n_n_ptr<RT::A, RT::DE>);
		m_instructions[0x7E].set(&CPU::inst_ld_n_n_ptr<RT::A, RT::HL>);

		m_instructions[0x47].set(&CPU::inst_ld_n_n<RT::B, RT::A>);
		m_instructions[0x40].set(&CPU::inst_ld_n_n<RT::B, RT::B>);
		m_instructions[0x41].set(&CPU::inst_ld_n_n<RT::B, RT::C>);
		m_instructions[0x42].set(&CPU::inst_ld_n_n<RT::B, RT::D>);
		m_instructions[0x43].set(&CPU::inst_ld_n_n<RT::B, RT::E>);
		m_instructions[0x44].set(&CPU::inst_ld_n_n<RT::B, RT::H>);
		m_instructions[0x45].set(&CPU::inst_ld_n_n<RT::B, RT::L>);
		m_instructions[0x46].set(&CPU::inst_ld_n_n_ptr<RT::B, RT::HL>);

		m_instructions[0x4F].set(&CPU::inst_ld_n_n<RT::C, RT::A>);
		m_instructions[0x48].set(&CPU::inst_ld_n_n<RT::C, RT::B>);
		m_instructions[0x49].set(&CPU::inst_ld_n_n<RT::C, RT::C>);
		m_instructions[0x4A].set(&CPU::inst_ld_n_n<RT::C, RT::D>);
		m_instructions[0x4B].set(&CPU::inst_ld_n_n<RT::C, RT::E>);
		m_instructions[0x4C].set(&CPU::inst_ld_n_n<RT::C, RT::H>);
		m_instructions[0x4D].set(&CPU::inst_ld_n_n<RT::C, RT::L>);
		m_instructions[0x4E].set(&CPU::inst_ld_n_n_ptr<RT::C, RT::HL>);

		m_instructions[0x57].set(&CPU::inst_ld_n_n<RT::D, RT::A>);
		m_instructions[0x50].set(&CPU::inst_ld_n_n<RT::D, RT::B>);
		m_instructions[0x51].set(&CPU::inst_ld_n_n<RT::D, RT::C>);
		m_instructions[0x52].set(&CPU::inst_ld_n_n<RT::D, RT::D>);
		m_instructions[0x53].set(&CPU::inst_ld_n_n<RT::D, RT::E>);
		m_instructions[0x54].set(&CPU::inst_ld_n_n<RT::D, RT::H>);
		m_instructions[0x55].set(&CPU::inst_ld_n_n<RT::D, RT::L>);
		m_instructions[0x56].set(&CPU::inst_ld_n_n_ptr<RT::D, RT::HL>);

		m_instructions[0x5F].set(&CPU::inst_ld_n_n<RT::E, RT::A>);
		m_instructions[0x58].set(&CPU::inst_ld_n_n<RT::E, RT::B>);
		m_instructions[0x59].set(&CPU::inst_ld_n_n<RT::E, RT::C>);
		m_instructions[0x5A].set(&CPU::inst_ld_n_n<RT::E, RT::D>);
		m_instructions[0x5B].set(&CPU::inst_ld_n_n<RT::E, RT::E>);
		m_instructions[0x5C].set(&CPU::inst_ld_n_n<RT::E, RT::H>);
		m_instructions[0x5D].set(&CPU::inst_ld_n_n<RT::E, RT::L>);
		m_instructions[0x5E].set(&CPU::inst_ld_n_n_ptr<RT::E, RT::HL>);

		m_instructions[0x67].set(&CPU::inst_ld_n_n<RT::H, RT::A>);
		m_instructions[0x60].set(&CPU::inst_ld_n_n<RT::H, RT::B>);
		m_instructions[0x61].set(&CPU::inst_ld_n_n<RT::H, RT::C>);
		m_instructions[0x62].set(&CPU::inst_ld_n_n<RT::H, RT::D>);
		m_instructions[0x63].set(&CPU::inst_ld_n_n<RT::H, RT::E>);
		m_instructions[0x64].set(&CPU::inst_ld_n_n<RT::H, RT::H>);
		m_instructions[0x65].set(&CPU::inst_ld_n_n<RT::H, RT::L>);
		m_instructions[0x66].set(&CPU::inst_ld_n_n_ptr<RT::H, RT::HL>);

		m_instructions[0x6F].set(&CPU::inst_ld_n_n<RT::L, RT::A>);
		m_instructions[0x68].set(&CPU::inst_ld_n_n<RT::L, RT::B>);
		m_instructions[0x69].set(&CPU::inst_ld_n_n<RT::L, RT::C>);
		m_instructions[0x6A].set(&CPU::inst_ld_n_n<RT::L, RT::D>);
		m_instructions[0x6B].set(&CPU::inst_ld_n_n<RT::L, RT::E>);
		m_instructions[0x6C].set(&CPU::inst_ld_n_n<RT::L, RT::H>);
		m_instructions[0x6D].set(&CPU::inst_ld_n_n<RT::L, RT::L>);
		m_instructions[0x6E].set(&CPU::inst_ld_n_n_ptr<RT::L, RT::HL>);

		m_instructions[0x02].set(&CPU::inst_ld_n_ptr_n<RT::BC, RT::A>);
		m_instructions[0x12].set(&CPU::inst_ld_n_ptr_n<RT::DE, RT::A>);
		m_instructions[0x77].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::A>);
		m_instructions[0x70].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::B>);
		m_instructions[0x71].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::C>);
		m_instructions[0x72].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::D>);
		m_instructions[0x73].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::E>);
		m_instructions[0x74].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::H>);
		m_instructions[0x75].set(&CPU::inst_ld_n_ptr_n<RT::HL, RT::L>);

		m_instructions[0x3E].set(&CPU::inst_ld_n_imm<RT::A>);
		m_instructions[0x06].set(&CPU::inst_ld_n_imm<RT::B>);
		m_instructions[0x0E].set(&CPU::inst_ld_n_imm<RT::C>);
		m_instructions[0x16].set(&CPU::inst_ld_n_imm<RT::D>);
		m_instructions[0x1E].set(&CPU::inst_ld_n_imm<RT::E>);
		m_instructions[0x26].set(&CPU::inst_ld_n_imm<RT::H>);
		m_instructions[0x2E].set(&CPU::inst_ld_n_imm<RT::L>);

		m_instructions[0xFA].set(&CPU::inst_ld_a_imm_ptr);
		m_instructions[0xEA].set(&CPU::inst_ld_imm_ptr_a);
		m_instructions[0x36].set(&CPU::inst_ld_hl_ptr_imm);
		m_instructions[0x32].set(&CPU::inst_ldd_hl_ptr_a);
		m_instructions[0x3A].set(&CPU::inst_ldd_a_hl_ptr);
		m_instructions[0x22].set(&CPU::inst_ldi_hl_ptr_a);
		m_instructions[0x2A].set(&CPU::inst_ldi_a_hl_ptr);
		m_instructions[0xE0].set(&CPU::inst_ldh_imm_ptr_a);
		m_instructions[0xF0].set(&CPU::inst_ldh_a_imm_ptr);
		m_instructions[0xE2].set(&CPU::inst_ld_c_ptr_a);
		m_instructions[0xF2].set(&CPU::inst_ld_a_c_ptr);

		// 16-bit Loads
		m_instructions[0x01].set(&CPU::inst_ld_nn_imm<RT::BC>);
		m_instructions[0x11].set(&CPU::inst_ld_nn_imm<RT::DE>);
		m_instructions[0x21].set(&CPU::inst_ld_nn_imm<RT::HL>);
		m_instructions[0x31].set(&CPU::inst_ld_nn_imm<RT::StackPointer>);

		m_instructions[0xF9].set(&CPU::inst_ld_sp_hl);
		m_instructions[0xF8].set(&CPU::inst_ld_hl_sp_imm);
		m_instructions[0x08].set(&CPU::inst_imm_ptr_sp);

		m_instructions[0xF5].set(&CPU::inst_push_nn<RT::AF>);
		m_instructions[0xC5].set(&CPU::inst_push_nn<RT::BC>);
		m_instructions[0xD5].set(&CPU::inst_push_nn<RT::DE>);
		m_instructions[0xE5].set(&CPU::inst_push_nn<RT::HL>);

		m_instructions[0xF1].set(&CPU::inst_pop_af);
		m_instructions[0xC1].set(&CPU::inst_pop_nn<RT::BC>);
		m_instructions[0xD1].set(&CPU::inst_pop_nn<RT::DE>);
		m_instructions[0xE1].set(&CPU::inst_pop_nn<RT::HL>);

		// 8-bit ALU - ADD
		m_instructions[0x87].set(&CPU::inst_add_n<RT::A>);
		m_instructions[0x80].set(&CPU::inst_add_n<RT::B>);
		m_instructions[0x81].set(&CPU::inst_add_n<RT::C>);
		m_instructions[0x82].set(&CPU::inst_add_n<RT::D>);
		m_instructions[0x83].set(&CPU::inst_add_n<RT::E>);
		m_instructions[0x84].set(&CPU::inst_add_n<RT::H>);
		m_instructions[0x85].set(&CPU::inst_add_n<RT::L>);
		m_instructions[0x86].set(&CPU::inst_add_hl_ptr);
		m_instructions[0xC6].set(&CPU::inst_add_imm);

		// 8-bit ALU - ADC
		m_instructions[0x8F].set(&CPU::inst_adc_n<RT::A>);
		m_instructions[0x88].set(&CPU::inst_adc_n<RT::B>);
		m_instructions[0x89].set(&CPU::inst_adc_n<RT::C>);
		m_instructions[0x8A].set(&CPU::inst_adc_n<RT::D>);
		m_instructions[0x8B].set(&CPU::inst_adc_n<RT::E>);
		m_instructions[0x8C].set(&CPU::inst_adc_n<RT::H>);
		m_instructions[0x8D].set(&CPU::inst_adc_n<RT::L>);
		m_instructions[0x8E].set(&CPU::inst_adc_hl_ptr);
		m_instructions[0xCE].set(&CPU::inst_adc_imm);

		// 8-bit ALU - SUB
		m_instructions[0x97].set(&CPU::inst_sub_n<RT::A>);
		m_instructions[0x90].set(&CPU::inst_sub_n<RT::B>);
		m_instructions[0x91].set(&CPU::inst_sub_n<RT::C>);
		m_instructions[0x92].set(&CPU::inst_sub_n<RT::D>);
		m_instructions[0x93].set(&CPU::inst_sub_n<RT::E>);
		m_instructions[0x94].set(&CPU::inst_sub_n<RT::H>);
		m_instructions[0x95].set(&CPU::inst_sub_n<RT::L>);
		m_instructions[0x96].set(&CPU::inst_sub_hl_ptr);
		m_instructions[0xD6].set(&CPU::inst_sub_imm);

		// 8-bit ALU - SBC
		m_instructions[0x9F].set(&CPU::inst_sbc_n<RT::A>);
		m_instructions[0x98].set(&CPU::inst_sbc_n<RT::B>);
		m_instructions[0x99].set(&CPU::inst_sbc_n<RT::C>);
		m_instructions[0x9A].set(&CPU::inst_sbc_n<RT::D>);
		m_instructions[0x9B].set(&CPU::inst_sbc_n<RT::E>);
		m_instructions[0x9C].set(&CPU::inst_sbc_n<RT::H>);
		m_instructions[0x9D].set(&CPU::inst_sbc_n<RT::L>);
		m_instructions[0x9E].set(&CPU::inst_sbc_hl_ptr);
		m_instructions[0xDE].set(&CPU::inst_sbc_imm);

		// 8-bit ALU - AND
		m_instructions[0xA7].set(&CPU::inst_and_n<RT::A>);
		m_instructions[0xA0].set(&CPU::inst_and_n<RT::B>);
		m_instructions[0xA1].set(&CPU::inst_and_n<RT::C>);
		m_instructions[0xA2].set(&CPU::inst_and_n<RT::D>);
		m_instructions[0xA3].set(&CPU::inst_and_n<RT::E>);
		m_instructions[0xA4].set(&CPU::inst_and_n<RT::H>);
		m_instructions[0xA5].set(&CPU::inst_and_n<RT::L>);
		m_instructions[0xA6].set(&CPU::inst_and_hl_ptr);
		m_instructions[0xE6].set(&CPU::inst_and_imm);

		// 8-bit ALU - OR
		m_instructions[0xB7].set(&CPU::inst_or_n<RT::A>);
		m_instructions[0xB0].set(&CPU::inst_or_n<RT::B>);
		m_instructions[0xB1].set(&CPU::inst_or_n<RT::C>);
		m_instructions[0xB2].set(&CPU::inst_or_n<RT::D>);
		m_instructions[0xB3].set(&CPU::inst_or_n<RT::E>);
		m_instructions[0xB4].set(&CPU::inst_or_n<RT::H>);
		m_instructions[0xB5].set(&CPU::inst_or_n<RT::L>);
		m_instructions[0xB6].set(&CPU::inst_or_hl_ptr);
		m_instructions[0xF6].set(&CPU::inst_or_imm);

		// 8-bit ALU - XOR
		m_instructions[0xAF].set(&CPU::inst_xor_n<RT::A>);
		m_instructions[0xA8].set(&CPU::inst_xor_n<RT::B>);
		m_instructions[0xA9].set(&CPU::inst_xor_n<RT::C>);
		m_instructions[0xAA].set(&CPU::inst_xor_n<RT::D>);
		m_instructions[0xAB].set(&CPU::inst_xor_n<RT::E>);
		m_instructions[0xAC].set(&CPU::inst_xor_n<RT::H>);
		m_instructions[0xAD].set(&CPU::inst_xor_n<RT::L>);
		m_instructions[0xAE].set(&CPU::inst_xor_hl_ptr);
		m_instructions[0xEE].set(&CPU::inst_xor_imm);

		// 8-bit ALU - CP
		m_instructions[0xBF].set(&CPU::inst_cp_n<RT::A>);
		m_instructions[0xB8].set(&CPU::inst_cp_n<RT::B>);
		m_instructions[0xB9].set(&CPU::inst_cp_n<RT::C>);
		m_instructions[0xBA].set(&CPU::inst_cp_n<RT::D>);
		m_instructions[0xBB].set(&CPU::inst_cp_n<RT::E>);
		m_instructions[0xBC].set(&CPU::inst_cp_n<RT::H>);
		m_instructions[0xBD].set(&CPU::inst_cp_n<RT::L>);
		m_instructions[0xBE].set(&CPU::inst_cp_hl_ptr);
		m_instructions[0xFE].set(&CPU::inst_cp_imm);

		// 8-bit ALU - INC
		m_instructions[0x3C].set(&CPU::inst_inc_n<RT::A>);
		m_instructions[0x04].set(&CPU::inst_inc_n<RT::B>);
		m_instructions[0x0C].set(&CPU::inst_inc_n<RT::C>);
		m_instructions[0x14].set(&CPU::inst_inc_n<RT::D>);
		m_instructions[0x1C].set(&CPU::inst_inc_n<RT::E>);
		m_instructions[0x24].set(&CPU::inst_inc_n<RT::H>);
		m_instructions[0x2C].set(&CPU::inst_inc_n<RT::L>);
		m_instructions[0x34].set(&CPU::inst_inc_hl_ptr);

		// 8-bit ALU - DEC
		m_instructions[0x3D].set(&CPU::inst_dec_n<RT::A>);
		m_instructions[0x05].set(&CPU::inst_dec_n<RT::B>);
		m_instructions[0x0D].set(&CPU::inst_dec_n<RT::C>);
		m_instructions[0x15].set(&CPU::inst_dec_n<RT::D>);
		m_instructions[0x1D].set(&CPU::inst_dec_n<RT::E>);
		m_instructions[0x25].set(&CPU::inst_dec_n<RT::H>);
		m_instructions[0x2D].set(&CPU::inst_dec_n<RT::L>);
		m_instructions[0x35].set(&CPU::inst_dec_hl_ptr);

		// 16-bit ALU - ADD
		m_instructions[0x09].set(&CPU::inst_add_hl_nn<RT::BC>);
		m_instructions[0x19].set(&CPU::inst_add_hl_nn<RT::DE>);
		m_instructions[0x29].set(&CPU::inst_add_hl_nn<RT::HL>);
		m_instructions[0x39].set(&CPU::inst_add_hl_nn<RT::StackPointer>);
		m_instructions[0xE8].set(&CPU::inst_add_sp_imm);

		// 16-bit ALU - INC
		m_instructions[0x03].set(&CPU::inst_inc_nn<RT::BC>);
		m_instructions[0x13].set(&CPU::inst_inc_nn<RT::DE>);
		m_instructions[0x23].set(&CPU::inst_inc_nn<RT::HL>);
		m_instructions[0x33].set(&CPU::inst_inc_nn<RT::StackPointer>);

		// 16-bit ALU - DEC
		m_instructions[0x0B].set(&CPU::inst_dec_nn<RT::BC>);
		m_instructions[0x1B].set(&CPU::inst_dec_nn<RT::DE>);
		m_instructions[0x2B].set(&CPU::inst_dec_nn<RT::HL>);
		m_instructions[0x3B].set(&CPU::inst_dec_nn<RT::StackPointer>);

		// Rotates & Shifts
		m_instructions[0x07].set(&CPU::inst_rlca);
		m_instructions[0x0f].set(&CPU::inst_rrca);
		m_instructions[0x17].set(&CPU::inst_rla);
		m_instructions[0x1f].set(&CPU::inst_rra);

		// Extended instructions
		m_instructions[0xCB].set(&CPU::inst_ext);

		// Extended - RCL
		m_instructionsExt[0x00].set(&CPU::inst_ext_rlc_n<RT::B>);
		m_instructionsExt[0x01].set(&CPU::inst_ext_rlc_n<RT::C>);
		m_instructionsExt[0x02].set(&CPU::inst_ext_rlc_n<RT::D>);
		m_instructionsExt[0x03].set(&CPU::inst_ext_rlc_n<RT::E>);
		m_instructionsExt[0x04].set(&CPU::inst_ext_rlc_n<RT::H>);
		m_instructionsExt[0x05].set(&CPU::inst_ext_rlc_n<RT::L>);
		m_instructionsExt[0x06].set(&CPU::inst_ext_rlc_hl_addr);
		m_instructionsExt[0x07].set(&CPU::inst_ext_rlc_n<RT::A>);

		// Extended - RRC
		m_instructionsExt[0x08].set(&CPU::inst_ext_rrc_n<RT::B>);
		m_instructionsExt[0x09].set(&CPU::inst_ext_rrc_n<RT::C>);
		m_instructionsExt[0x0A].set(&CPU::inst_ext_rrc_n<RT::D>);
		m_instructionsExt[0x0B].set(&CPU::inst_ext_rrc_n<RT::E>);
		m_instructionsExt[0x0C].set(&CPU::inst_ext_rrc_n<RT::H>);
		m_instructionsExt[0x0D].set(&CPU::inst_ext_rrc_n<RT::L>);
		m_instructionsExt[0x0E].set(&CPU::inst_ext_rrc_hl_addr);
		m_instructionsExt[0x0F].set(&CPU::inst_ext_rrc_n<RT::A>);

		// Extended - RL
		m_instructionsExt[0x10].set(&CPU::inst_ext_rl_n<RT::B>);
		m_instructionsExt[0x11].set(&CPU::inst_ext_rl_n<RT::C>);
		m_instructionsExt[0x12].set(&CPU::inst_ext_rl_n<RT::D>);
		m_instructionsExt[0x13].set(&CPU::inst_ext_rl_n<RT::E>);
		m_instructionsExt[0x14].set(&CPU::inst_ext_rl_n<RT::H>);
		m_instructionsExt[0x15].set(&CPU::inst_ext_rl_n<RT::L>);
		m_instructionsExt[0x16].set(&CPU::inst_ext_rl_hl_addr);
		m_instructionsExt[0x17].set(&CPU::inst_ext_rl_n<RT::A>);

		// Extended - RR
		m_instructionsExt[0x18].set(&CPU::inst_ext_rr_n<RT::B>);
		m_instructionsExt[0x19].set(&CPU::inst_ext_rr_n<RT::C>);
		m_instructionsExt[0x1A].set(&CPU::inst_ext_rr_n<RT::D>);
		m_instructionsExt[0x1B].set(&CPU::inst_ext_rr_n<RT::E>);
		m_instructionsExt[0x1C].set(&CPU::inst_ext_rr_n<RT::H>);
		m_instructionsExt[0x1D].set(&CPU::inst_ext_rr_n<RT::L>);
		m_instructionsExt[0x1E].set(&CPU::inst_ext_rr_hl_addr);
		m_instructionsExt[0x1F].set(&CPU::inst_ext_rr_n<RT::A>);

		// Extended - SLA
		m_instructionsExt[0x20].set(&CPU::inst_ext_sla_n<RT::B>);
		m_instructionsExt[0x21].set(&CPU::inst_ext_sla_n<RT::C>);
		m_instructionsExt[0x22].set(&CPU::inst_ext_sla_n<RT::D>);
		m_instructionsExt[0x23].set(&CPU::inst_ext_sla_n<RT::E>);
		m_instructionsExt[0x24].set(&CPU::inst_ext_sla_n<RT::H>);
		m_instructionsExt[0x25].set(&CPU::inst_ext_sla_n<RT::L>);
		m_instructionsExt[0x26].set(&CPU::inst_ext_sla_hl_addr);
		m_instructionsExt[0x27].set(&CPU::inst_ext_sla_n<RT::A>);

		// Extended - SRA
		m_instructionsExt[0x28].set(&CPU::inst_ext_sra_n<RT::B>);
		m_instructionsExt[0x29].set(&CPU::inst_ext_sra_n<RT::C>);
		m_instructionsExt[0x2A].set(&CPU::inst_ext_sra_n<RT::D>);
		m_instructionsExt[0x2B].set(&CPU::inst_ext_sra_n<RT::E>);
		m_instructionsExt[0x2C].set(&CPU::inst_ext_sra_n<RT::H>);
		m_instructionsExt[0x2D].set(&CPU::inst_ext_sra_n<RT::L>);
		m_instructionsExt[0x2E].set(&CPU::inst_ext_sra_hl_addr);
		m_instructionsExt[0x2F].set(&CPU::inst_ext_sra_n<RT::A>);

		// Extended - SWAP
		m_instructionsExt[0x30].set(&CPU::inst_ext_swap_n<RT::B>);
		m_instructionsExt[0x31].set(&CPU::inst_ext_swap_n<RT::C>);
		m_instructionsExt[0x32].set(&CPU::inst_ext_swap_n<RT::D>);
		m_instructionsExt[0x33].set(&CPU::inst_ext_swap_n<RT::E>);
		m_instructionsExt[0x34].set(&CPU::inst_ext_swap_n<RT::H>);
		m_instructionsExt[0x35].set(&CPU::inst_ext_swap_n<RT::L>);
		m_instructionsExt[0x36].set(&CPU::inst_ext_swap_hl_ptr);
		m_instructionsExt[0x37].set(&CPU::inst_ext_swap_n<RT::A>);

		// Extended - SRL
		m_instructionsExt[0x38].set(&CPU::inst_ext_srl_n<RT::B>);
		m_instructionsExt[0x39].set(&CPU::inst_ext_srl_n<RT::C>);
		m_instructionsExt[0x3A].set(&CPU::inst_ext_srl_n<RT::D>);
		m_instructionsExt[0x3B].set(&CPU::inst_ext_srl_n<RT::E>);
		m_instructionsExt[0x3C].set(&CPU::inst_ext_srl_n<RT::H>);
		m_instructionsExt[0x3D].set(&CPU::inst_ext_srl_n<RT::L>);
		m_instructionsExt[0x3E].set(&CPU::inst_ext_srl_hl_addr);
		m_instructionsExt[0x3F].set(&CPU::inst_ext_srl_n<RT::A>);

		// Extended - BIT
		m_instructionsExt[0x40].set(&CPU::inst_ext_bit_b_n<0, RT::B>);
		m_instructionsExt[0x41].set(&CPU::inst_ext_bit_b_n<0, RT::C>);
		m_instructionsExt[0x42].set(&CPU::inst_ext_bit_b_n<0, RT::D>);
		m_instructionsExt[0x43].set(&CPU::inst_ext_bit_b_n<0, RT::E>);
		m_instructionsExt[0x44].set(&CPU::inst_ext_bit_b_n<0, RT::H>);
		m_instructionsExt[0x45].set(&CPU::inst_ext_bit_b_n<0, RT::L>);
		m_instructionsExt[0x46].set(&CPU::inst_ext_bit_b_hl_addr<0>);
		m_instructionsExt[0x47].set(&CPU::inst_ext_bit_b_n<0, RT::A>);
		m_instructionsExt[0x48].set(&CPU::inst_ext_bit_b_n<1, RT::B>);
		m_instructionsExt[0x49].set(&CPU::inst_ext_bit_b_n<1, RT::C>);
		m_instructionsExt[0x4A].set(&CPU::inst_ext_bit_b_n<1, RT::D>);
		m_instructionsExt[0x4B].set(&CPU::inst_ext_bit_b_n<1, RT::E>);
		m_instructionsExt[0x4C].set(&CPU::inst_ext_bit_b_n<1, RT::H>);
		m_instructionsExt[0x4D].set(&CPU::inst_ext_bit_b_n<1, RT::L>);
		m_instructionsExt[0x4E].set(&CPU::inst_ext_bit_b_hl_addr<1>);
		m_instructionsExt[0x4F].set(&CPU::inst_ext_bit_b_n<1, RT::A>);
		m_instructionsExt[0x50].set(&CPU::inst_ext_bit_b_n<2, RT::B>);
		m_instructionsExt[0x51].set(&CPU::inst_ext_bit_b_n<2, RT::C>);
		m_instructionsExt[0x52].set(&CPU::inst_ext_bit_b_n<2, RT::D>);
		m_instructionsExt[0x53].set(&CPU::inst_ext_bit_b_n<2, RT::E>);
		m_instructionsExt[0x54].set(&CPU::inst_ext_bit_b_n<2, RT::H>);
		m_instructionsExt[0x55].set(&CPU::inst_ext_bit_b_n<2, RT::L>);
		m_instructionsExt[0x56].set(&CPU::inst_ext_bit_b_hl_addr<2>);
		m_instructionsExt[0x57].set(&CPU::inst_ext_bit_b_n<2, RT::A>);
		m_instructionsExt[0x58].set(&CPU::inst_ext_bit_b_n<3, RT::B>);
		m_instructionsExt[0x59].set(&CPU::inst_ext_bit_b_n<3, RT::C>);
		m_instructionsExt[0x5A].set(&CPU::inst_ext_bit_b_n<3, RT::D>);
		m_instructionsExt[0x5B].set(&CPU::inst_ext_bit_b_n<3, RT::E>);
		m_instructionsExt[0x5C].set(&CPU::inst_ext_bit_b_n<3, RT::H>);
		m_instructionsExt[0x5D].set(&CPU::inst_ext_bit_b_n<3, RT::L>);
		m_instructionsExt[0x5E].set(&CPU::inst_ext_bit_b_hl_addr<3>);
		m_instructionsExt[0x5F].set(&CPU::inst_ext_bit_b_n<3, RT::A>);
		m_instructionsExt[0x60].set(&CPU::inst_ext_bit_b_n<4, RT::B>);
		m_instructionsExt[0x61].set(&CPU::inst_ext_bit_b_n<4, RT::C>);
		m_instructionsExt[0x62].set(&CPU::inst_ext_bit_b_n<4, RT::D>);
		m_instructionsExt[0x63].set(&CPU::inst_ext_bit_b_n<4, RT::E>);
		m_instructionsExt[0x64].set(&CPU::inst_ext_bit_b_n<4, RT::H>);
		m_instructionsExt[0x65].set(&CPU::inst_ext_bit_b_n<4, RT::L>);
		m_instructionsExt[0x66].set(&CPU::inst_ext_bit_b_hl_addr<4>);
		m_instructionsExt[0x67].set(&CPU::inst_ext_bit_b_n<4, RT::A>);
		m_instructionsExt[0x68].set(&CPU::inst_ext_bit_b_n<5, RT::B>);
		m_instructionsExt[0x69].set(&CPU::inst_ext_bit_b_n<5, RT::C>);
		m_instructionsExt[0x6A].set(&CPU::inst_ext_bit_b_n<5, RT::D>);
		m_instructionsExt[0x6B].set(&CPU::inst_ext_bit_b_n<5, RT::E>);
		m_instructionsExt[0x6C].set(&CPU::inst_ext_bit_b_n<5, RT::H>);
		m_instructionsExt[0x6D].set(&CPU::inst_ext_bit_b_n<5, RT::L>);
		m_instructionsExt[0x6E].set(&CPU::inst_ext_bit_b_hl_addr<5>);
		m_instructionsExt[0x6F].set(&CPU::inst_ext_bit_b_n<5, RT::A>);
		m_instructionsExt[0x70].set(&CPU::inst_ext_bit_b_n<6, RT::B>);
		m_instructionsExt[0x71].set(&CPU::inst_ext_bit_b_n<6, RT::C>);
		m_instructionsExt[0x72].set(&CPU::inst_ext_bit_b_n<6, RT::D>);
		m_instructionsExt[0x73].set(&CPU::inst_ext_bit_b_n<6, RT::E>);
		m_instructionsExt[0x74].set(&CPU::inst_ext_bit_b_n<6, RT::H>);
		m_instructionsExt[0x75].set(&CPU::inst_ext_bit_b_n<6, RT::L>);
		m_instructionsExt[0x76].set(&CPU::inst_ext_bit_b_hl_addr<6>);
		m_instructionsExt[0x77].set(&CPU::inst_ext_bit_b_n<6, RT::A>);
		m_instructionsExt[0x78].set(&CPU::inst_ext_bit_b_n<7, RT::B>);
		m_instructionsExt[0x79].set(&CPU::inst_ext_bit_b_n<7, RT::C>);
		m_instructionsExt[0x7A].set(&CPU::inst_ext_bit_b_n<7, RT::D>);
		m_instructionsExt[0x7B].set(&CPU::inst_ext_bit_b_n<7, RT::E>);
		m_instructionsExt[0x7C].set(&CPU::inst_ext_bit_b_n<7, RT::H>);
		m_instructionsExt[0x7D].set(&CPU::inst_ext_bit_b_n<7, RT::L>);
		m_instructionsExt[0x7E].set(&CPU::inst_ext_bit_b_hl_addr<7>);
		m_instructionsExt[0x7F].set(&CPU::inst_ext_bit_b_n<7, RT::A>);

		// Extended - RES
		m_instructionsExt[0x80].set(&CPU::inst_ext_reset_b_n<0, RT::B>);
		m_instructionsExt[0x81].set(&CPU::inst_ext_reset_b_n<0, RT::C>);
		m_instructionsExt[0x82].set(&CPU::inst_ext_reset_b_n<0, RT::D>);
		m_instructionsExt[0x83].set(&CPU::inst_ext_reset_b_n<0, RT::E>);
		m_instructionsExt[0x84].set(&CPU::inst_ext_reset_b_n<0, RT::H>);
		m_instructionsExt[0x85].set(&CPU::inst_ext_reset_b_n<0, RT::L>);
		m_instructionsExt[0x86].set(&CPU::inst_ext_reset_b_hl_addr<0>);
		m_instructionsExt[0x87].set(&CPU::inst_ext_reset_b_n<0, RT::A>);
		m_instructionsExt[0x88].set(&CPU::inst_ext_reset_b_n<1, RT::B>);
		m_instructionsExt[0x89].set(&CPU::inst_ext_reset_b_n<1, RT::C>);
		m_instructionsExt[0x8A].set(&CPU::inst_ext_reset_b_n<1, RT::D>);
		m_instructionsExt[0x8B].set(&CPU::inst_ext_reset_b_n<1, RT::E>);
		m_instructionsExt[0x8C].set(&CPU::inst_ext_reset_b_n<1, RT::H>);
		m_instructionsExt[0x8D].set(&CPU::inst_ext_reset_b_n<1, RT::L>);
		m_instructionsExt[0x8E].set(&CPU::inst_ext_reset_b_hl_addr<1>);
		m_instructionsExt[0x8F].set(&CPU::inst_ext_reset_b_n<1, RT::A>);
		m_instructionsExt[0x90].set(&CPU::inst_ext_reset_b_n<2, RT::B>);
		m_instructionsExt[0x91].set(&CPU::inst_ext_reset_b_n<2, RT::C>);
		m_instructionsExt[0x92].set(&CPU::inst_ext_reset_b_n<2, RT::D>);
		m_instructionsExt[0x93].set(&CPU::inst_ext_reset_b_n<2, RT::E>);
		m_instructionsExt[0x94].set(&CPU::inst_ext_reset_b_n<2, RT::H>);
		m_instructionsExt[0x95].set(&CPU::inst_ext_reset_b_n<2, RT::L>);
		m_instructionsExt[0x96].set(&CPU::inst_ext_reset_b_hl_addr<2>);
		m_instructionsExt[0x97].set(&CPU::inst_ext_reset_b_n<2, RT::A>);
		m_instructionsExt[0x98].set(&CPU::inst_ext_reset_b_n<3, RT::B>);
		m_instructionsExt[0x99].set(&CPU::inst_ext_reset_b_n<3, RT::C>);
		m_instructionsExt[0x9A].set(&CPU::inst_ext_reset_b_n<3, RT::D>);
		m_instructionsExt[0x9B].set(&CPU::inst_ext_reset_b_n<3, RT::E>);
		m_instructionsExt[0x9C].set(&CPU::inst_ext_reset_b_n<3, RT::H>);
		m_instructionsExt[0x9D].set(&CPU::inst_ext_reset_b_n<3, RT::L>);
		m_instructionsExt[0x9E].set(&CPU::inst_ext_reset_b_hl_addr<3>);
		m_instructionsExt[0x9F].set(&CPU::inst_ext_reset_b_n<3, RT::A>);
		m_instructionsExt[0xA0].set(&CPU::inst_ext_reset_b_n<4, RT::B>);
		m_instructionsExt[0xA1].set(&CPU::inst_ext_reset_b_n<4, RT::C>);
		m_instructionsExt[0xA2].set(&CPU::inst_ext_reset_b_n<4, RT::D>);
		m_instructionsExt[0xA3].set(&CPU::inst_ext_reset_b_n<4, RT::E>);
		m_instructionsExt[0xA4].set(&CPU::inst_ext_reset_b_n<4, RT::H>);
		m_instructionsExt[0xA5].set(&CPU::inst_ext_reset_b_n<4, RT::L>);
		m_instructionsExt[0xA6].set(&CPU::inst_ext_reset_b_hl_addr<4>);
		m_instructionsExt[0xA7].set(&CPU::inst_ext_reset_b_n<4, RT::A>);
		m_instructionsExt[0xA8].set(&CPU::inst_ext_reset_b_n<5, RT::B>);
		m_instructionsExt[0xA9].set(&CPU::inst_ext_reset_b_n<5, RT::C>);
		m_instructionsExt[0xAA].set(&CPU::inst_ext_reset_b_n<5, RT::D>);
		m_instructionsExt[0xAB].set(&CPU::inst_ext_reset_b_n<5, RT::E>);
		m_instructionsExt[0xAC].set(&CPU::inst_ext_reset_b_n<5, RT::H>);
		m_instructionsExt[0xAD].set(&CPU::inst_ext_reset_b_n<5, RT::L>);
		m_instructionsExt[0xAE].set(&CPU::inst_ext_reset_b_hl_addr<5>);
		m_instructionsExt[0xAF].set(&CPU::inst_ext_reset_b_n<5, RT::A>);
		m_instructionsExt[0xB0].set(&CPU::inst_ext_reset_b_n<6, RT::B>);
		m_instructionsExt[0xB1].set(&CPU::inst_ext_reset_b_n<6, RT::C>);
		m_instructionsExt[0xB2].set(&CPU::inst_ext_reset_b_n<6, RT::D>);
		m_instructionsExt[0xB3].set(&CPU::inst_ext_reset_b_n<6, RT::E>);
		m_instructionsExt[0xB4].set(&CPU::inst_ext_reset_b_n<6, RT::H>);
		m_instructionsExt[0xB5].set(&CPU::inst_ext_reset_b_n<6, RT::L>);
		m_instructionsExt[0xB6].set(&CPU::inst_ext_reset_b_hl_addr<6>);
		m_instructionsExt[0xB7].set(&CPU::inst_ext_reset_b_n<6, RT::A>);
		m_instructionsExt[0xB8].set(&CPU::inst_ext_reset_b_n<7, RT::B>);
		m_instructionsExt[0xB9].set(&CPU::inst_ext_reset_b_n<7, RT::C>);
		m_instructionsExt[0xBA].set(&CPU::inst_ext_reset_b_n<7, RT::D>);
		m_instructionsExt[0xBB].set(&CPU::inst_ext_reset_b_n<7, RT::E>);
		m_instructionsExt[0xBC].set(&CPU::inst_ext_reset_b_n<7, RT::H>);
		m_instructionsExt[0xBD].set(&CPU::inst_ext_reset_b_n<7, RT::L>);
		m_instructionsExt[0xBE].set(&CPU::inst_ext_reset_b_hl_addr<7>);
		m_instructionsExt[0xBF].set(&CPU::inst_ext_reset_b_n<7, RT::A>);

		// Extended - SET
		m_instructionsExt[0xC0].set(&CPU::inst_ext_set_b_n<0, RT::B>);
		m_instructionsExt[0xC1].set(&CPU::inst_ext_set_b_n<0, RT::C>);
		m_instructionsExt[0xC2].set(&CPU::inst_ext_set_b_n<0, RT::D>);
		m_instructionsExt[0xC3].set(&CPU::inst_ext_set_b_n<0, RT::E>);
		m_instructionsExt[0xC4].set(&CPU::inst_ext_set_b_n<0, RT::H>);
		m_instructionsExt[0xC5].set(&CPU::inst_ext_set_b_n<0, RT::L>);
		m_instructionsExt[0xC6].set(&CPU::inst_ext_set_b_hl_addr<0>);
		m_instructionsExt[0xC7].set(&CPU::inst_ext_set_b_n<0, RT::A>);
		m_instructionsExt[0xC8].set(&CPU::inst_ext_set_b_n<1, RT::B>);
		m_instructionsExt[0xC9].set(&CPU::inst_ext_set_b_n<1, RT::C>);
		m_instructionsExt[0xCA].set(&CPU::inst_ext_set_b_n<1, RT::D>);
		m_instructionsExt[0xCB].set(&CPU::inst_ext_set_b_n<1, RT::E>);
		m_instructionsExt[0xCC].set(&CPU::inst_ext_set_b_n<1, RT::H>);
		m_instructionsExt[0xCD].set(&CPU::inst_ext_set_b_n<1, RT::L>);
		m_instructionsExt[0xCE].set(&CPU::inst_ext_set_b_hl_addr<1>);
		m_instructionsExt[0xCF].set(&CPU::inst_ext_set_b_n<1, RT::A>);
		m_instructionsExt[0xD0].set(&CPU::inst_ext_set_b_n<2, RT::B>);
		m_instructionsExt[0xD1].set(&CPU::inst_ext_set_b_n<2, RT::C>);
		m_instructionsExt[0xD2].set(&CPU::inst_ext_set_b_n<2, RT::D>);
		m_instructionsExt[0xD3].set(&CPU::inst_ext_set_b_n<2, RT::E>);
		m_instructionsExt[0xD4].set(&CPU::inst_ext_set_b_n<2, RT::H>);
		m_instructionsExt[0xD5].set(&CPU::inst_ext_set_b_n<2, RT::L>);
		m_instructionsExt[0xD6].set(&CPU::inst_ext_set_b_hl_addr<2>);
		m_instructionsExt[0xD7].set(&CPU::inst_ext_set_b_n<2, RT::A>);
		m_instructionsExt[0xD8].set(&CPU::inst_ext_set_b_n<3, RT::B>);
		m_instructionsExt[0xD9].set(&CPU::inst_ext_set_b_n<3, RT::C>);
		m_instructionsExt[0xDA].set(&CPU::inst_ext_set_b_n<3, RT::D>);
		m_instructionsExt[0xDB].set(&CPU::inst_ext_set_b_n<3, RT::E>);
		m_instructionsExt[0xDC].set(&CPU::inst_ext_set_b_n<3, RT::H>);
		m_instructionsExt[0xDD].set(&CPU::inst_ext_set_b_n<3, RT::L>);
		m_instructionsExt[0xDE].set(&CPU::inst_ext_set_b_hl_addr<3>);
		m_instructionsExt[0xDF].set(&CPU::inst_ext_set_b_n<3, RT::A>);
		m_instructionsExt[0xE0].set(&CPU::inst_ext_set_b_n<4, RT::B>);
		m_instructionsExt[0xE1].set(&CPU::inst_ext_set_b_n<4, RT::C>);
		m_instructionsExt[0xE2].set(&CPU::inst_ext_set_b_n<4, RT::D>);
		m_instructionsExt[0xE3].set(&CPU::inst_ext_set_b_n<4, RT::E>);
		m_instructionsExt[0xE4].set(&CPU::inst_ext_set_b_n<4, RT::H>);
		m_instructionsExt[0xE5].set(&CPU::inst_ext_set_b_n<4, RT::L>);
		m_instructionsExt[0xE6].set(&CPU::inst_ext_set_b_hl_addr<4>);
		m_instructionsExt[0xE7].set(&CPU::inst_ext_set_b_n<4, RT::A>);
		m_instructionsExt[0xE8].set(&CPU::inst_ext_set_b_n<5, RT::B>);
		m_instructionsExt[0xE9].set(&CPU::inst_ext_set_b_n<5, RT::C>);
		m_instructionsExt[0xEA].set(&CPU::inst_ext_set_b_n<5, RT::D>);
		m_instructionsExt[0xEB].set(&CPU::inst_ext_set_b_n<5, RT::E>);
		m_instructionsExt[0xEC].set(&CPU::inst_ext_set_b_n<5, RT::H>);
		m_instructionsExt[0xED].set(&CPU::inst_ext_set_b_n<5, RT::L>);
		m_instructionsExt[0xEE].set(&CPU::inst_ext_set_b_hl_addr<5>);
		m_instructionsExt[0xEF].set(&CPU::inst_ext_set_b_n<5, RT::A>);
		m_instructionsExt[0xF0].set(&CPU::inst_ext_set_b_n<6, RT::B>);
		m_instructionsExt[0xF1].set(&CPU::inst_ext_set_b_n<6, RT::C>);
		m_instructionsExt[0xF2].set(&CPU::inst_ext_set_b_n<6, RT::D>);
		m_instructionsExt[0xF3].set(&CPU::inst_ext_set_b_n<6, RT::E>);
		m_instructionsExt[0xF4].set(&CPU::inst_ext_set_b_n<6, RT::H>);
		m_instructionsExt[0xF5].set(&CPU::inst_ext_set_b_n<6, RT::L>);
		m_instructionsExt[0xF6].set(&CPU::inst_ext_set_b_hl_addr<6>);
		m_instructionsExt[0xF7].set(&CPU::inst_ext_set_b_n<6, RT::A>);
		m_instructionsExt[0xF8].set(&CPU::inst_ext_set_b_n<7, RT::B>);
		m_instructionsExt[0xF9].set(&CPU::inst_ext_set_b_n<7, RT::C>);
		m_instructionsExt[0xFA].set(&CPU::inst_ext_set_b_n<7, RT::D>);
		m_instructionsExt[0xFB].set(&CPU::inst_ext_set_b_n<7, RT::E>);
		m_instructionsExt[0xFC].set(&CPU::inst_ext_set_b_n<7, RT::H>);
		m_instructionsExt[0xFD].set(&CPU::inst_ext_set_b_n<7, RT::L>);
		m_instructionsExt[0xFE].set(&CPU::inst_ext_set_b_hl_addr<7>);
		m_instructionsExt[0xFF].set(&CPU::inst_ext_set_b_n<7, RT::A>);
	}

	InstructionResult::Enum CPU::instruction_not_implemented()
	{
		Instruction& inst = m_instructions[m_currentOpcode];
		log_error("Instruction not implemented [Opcode: 0x%02x, Assembly: %s]\n", inst.opcode(), inst.assembly());
		throw std::runtime_error("Instruction not implemented");
		return InstructionResult::Failed;
	}

	InstructionResult::Enum CPU::instruction_not_implemented_ext()
	{
		Instruction& inst = m_instructionsExt[m_currentOpcodeExt];
		log_error("Extended instruction not implemented [Opcode: 0x%02x, Assembly: %s]\n", inst.is_extended(), inst.assembly());
		throw std::runtime_error("Extended instruction not implemented");
		return InstructionResult::Failed;
	}

} // gbhw