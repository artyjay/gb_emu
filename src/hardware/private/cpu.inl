#include "mmu.h"

namespace gbhw
{
	//--------------------------------------------------------------------------
	// Instruction
	//--------------------------------------------------------------------------

	inline Byte Instruction::opcode() const
	{
		return m_opcode;
	}

	inline Byte Instruction::is_extended() const
	{
		return m_extended;
	}

	inline Byte Instruction::byte_size() const
	{
		return m_byteSize;
	}

	inline Byte Instruction::cycles(InstructionResult::Enum result) const
	{
		return m_cycles[result];
	}

	inline RFB::Enum Instruction::flag_behaviour(RF::Enum flag) const
	{
		return m_flagBehaviour[flag];
	}

	inline RTD::Enum Instruction::arg_type(uint32_t argIndex) const
	{
		if(argIndex < 2)
		{
			return m_args[argIndex];
		}

		return RTD::None;
	}

	inline const char* Instruction::assembly() const
	{
		return m_assembly;
	}

	inline InstructionFunction& Instruction::function()
	{
		return m_function;
	}

	inline Address Instruction::address() const
	{
		return m_address;
	}

	//--------------------------------------------------------------------------
	// Helpers
	//--------------------------------------------------------------------------

	inline Registers* CPU::get_registers()
	{
		return &m_registers;
	}

	inline Instruction CPU::get_instruction(Byte opcode, bool bExtended) const
	{
		return bExtended ? m_instructionsExt[opcode] : m_instructions[opcode];
	}

	inline Byte CPU::immediate_byte(bool steppc)
	{
		Byte res = m_mmu->read_byte(m_registers.pc);

		if(steppc)
		{
			m_registers.pc++;
		}

		return res;
	}

	inline Word CPU::immediate_word(bool steppc)
	{
		Word res = m_mmu->read_word(m_registers.pc);

		if (steppc)
		{
			m_registers.pc += 2;
		}

		return res;
	}

	//--------------------------------------------------------------------------
	// Stack management
	//--------------------------------------------------------------------------

	inline void CPU::stack_push(Word word)
	{
		m_registers.sp -= 2;
		m_mmu->write_word(m_registers.sp, word);
	}

	inline Word CPU::stack_pop()
	{
		m_registers.sp += 2;
		return m_mmu->read_word(m_registers.sp - 2);
	}

	//--------------------------------------------------------------------------
	// Misc.
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_nop()
	{
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_daa()
	{
		Word val = m_registers.a;

		if(!m_registers.is_flag_set(RF::Negative))
		{
			if(m_registers.is_flag_set(RF::HalfCarry) || ((val & 0xF) > 9))
			{
				val += 0x06;
			}

			if(m_registers.is_flag_set(RF::Carry) || (val > 0x9F))
			{
				val += 0x60;
			}
		}
		else
		{
			if(m_registers.is_flag_set(RF::HalfCarry))
			{
				val = (val - 6) & 0xFF;
			}

			if(m_registers.is_flag_set(RF::Carry))
			{
				val -= 0x60;
			}
		}

		m_registers.clear_flags(RF::HalfCarry | RF::Zero);

		if ((val & 0x100) == 0x100)
		{
			m_registers.set_flag(RF::Carry);
		}

		val &= 0xFF;

		if(val == 0)
		{
			m_registers.set_flag(RF::Zero);
		}

		m_registers.a = static_cast<Byte>(val);

		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_cpl()
	{
		m_registers.a = ~m_registers.a;
		m_registers.set_flag(RF::Negative);
		m_registers.set_flag(RF::HalfCarry);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ccf()
	{
		m_registers.set_flag_if<RF::Carry>(!m_registers.is_flag_set(RF::Carry));
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_scf()
	{
		m_registers.set_flag(RF::Carry);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_halt()
	{
		//log_debug("Halting CPU until interrupt is generated\n");
		//m_bHalted = true;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_stop()
	{
		Byte empty = immediate_byte();
		log_debug("Stopping CPU until input is detected, following byte = 0x%02x\n", empty);
		m_bStopped = true;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_di()
	{
		m_registers.ime = false;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ei()
	{
		m_registers.ime = true;
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Jumps
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_jp()
	{
		m_registers.pc = immediate_word(false);
		return InstructionResult::Passed;
	}

	template<RF::Enum FlagType, bool IsSet>
	inline InstructionResult::Enum CPU::inst_jp_cc()
	{
		// Read new possible address
		Word address = immediate_word();

		// Jump only if condition is met.
		if (m_registers.is_flag_set(FlagType) == IsSet)
		{
			m_registers.pc = address;
			return InstructionResult::Passed;
		}

		return InstructionResult::Failed;
	}

	inline InstructionResult::Enum CPU::inst_jp_hl()
	{
		m_registers.pc = m_registers.hl;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_jr()
	{
		Byte amount = immediate_byte();

		if((amount & 0x80) == 0x80)
		{
			// Remove two's complement.
			amount = ~(amount - 1);
			m_registers.pc -= amount;
		}
		else
		{
			m_registers.pc += amount;
		}

		return InstructionResult::Passed;
	}

	template<RF::Enum FlagType, bool IsSet>
	inline InstructionResult::Enum CPU::inst_jr_cc()
	{
		// Read new possible address
		Byte amount = immediate_byte();

		// Jump only if condition is met.
		if (m_registers.is_flag_set(FlagType) == IsSet)
		{
			if ((amount & 0x80) == 0x80)
			{
				// Remove two's complement.
				amount = ~(amount - 1);
				m_registers.pc -= amount;
			}
			else
			{
				m_registers.pc += amount;
			}

			return InstructionResult::Passed;
		}

		return InstructionResult::Failed;
	}

	//--------------------------------------------------------------------------
	// Calls
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_call()
	{
		stack_push(m_registers.pc + 2);		// Push next instruction address onto the stack.
		m_registers.pc = immediate_word(false);	// Jump to new address.
		return InstructionResult::Passed;
	}

	template<RF::Enum FlagType, bool IsSet>
	inline InstructionResult::Enum CPU::inst_call_cc()
	{
		Word address = immediate_word(); // Read new address always, this will move PC along.

		if (m_registers.is_flag_set(FlagType) == IsSet)
		{
			stack_push(m_registers.pc);	// Push next instruction address onto the stack.
			m_registers.pc = address;		// Jump to new address.
			return InstructionResult::Passed;
		}

		return InstructionResult::Failed;
	}

	//--------------------------------------------------------------------------
	// Restarts
	//--------------------------------------------------------------------------

	template<Byte Offset>
	inline InstructionResult::Enum CPU::inst_rst()
	{
		stack_push(m_registers.pc);
		m_registers.pc = (0x0000 + Offset);
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Returns
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_ret()
	{
		m_registers.pc = stack_pop();
		return InstructionResult::Passed;
	}

	template<RF::Enum FlagType, bool IsSet>
	inline InstructionResult::Enum CPU::inst_ret_cc()
	{
		if (m_registers.is_flag_set(FlagType) == IsSet)
		{
			m_registers.pc = stack_pop();
			return InstructionResult::Passed;
		}

		return InstructionResult::Failed;
	}

	inline InstructionResult::Enum CPU::inst_reti()
	{
		m_registers.pc = stack_pop();
		m_registers.ime = true;
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 8-bit Loads
	//--------------------------------------------------------------------------

	template<RT::Enum DstRegister, RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_ld_n_n()
	{
		m_registers.set_register<DstRegister>(m_registers.get_register<SrcRegister>());
		return InstructionResult::Passed;
	}

	template<RT::Enum DstRegister, RT::Enum SrcRegisterPtr>
	inline InstructionResult::Enum CPU::inst_ld_n_n_ptr()
	{
		Byte val = m_mmu->read_byte(m_registers.get_register_word<SrcRegisterPtr>());
		m_registers.set_register<DstRegister>(val);
		return InstructionResult::Passed;
	}

	template<RT::Enum DstRegisterPtr, RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_ld_n_ptr_n()
	{
		m_mmu->write_byte(m_registers.get_register_word<DstRegisterPtr>(),
						 m_registers.get_register<SrcRegister>());
		return InstructionResult::Passed;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_ld_n_imm()
	{
		m_registers.set_register<DstRegister>(immediate_byte());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_a_imm_ptr()
	{
		m_registers.a = m_mmu->read_byte(immediate_word());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_imm_ptr_a()
	{
		m_mmu->write_byte(immediate_word(), m_registers.a);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_hl_ptr_imm()
	{
		m_mmu->write_byte(m_registers.hl, immediate_byte());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldd_a_hl_ptr()
	{
		m_registers.a = m_mmu->read_byte(m_registers.hl);
		m_registers.hl = inst_dec_w(m_registers.hl);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldd_hl_ptr_a()
	{
		m_mmu->write_byte(m_registers.hl, m_registers.a);
		m_registers.hl = inst_dec_w(m_registers.hl);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldi_a_hl_ptr()
	{
		m_registers.a = m_mmu->read_byte(m_registers.hl);
		m_registers.hl = inst_inc_w(m_registers.hl);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldi_hl_ptr_a()
	{
		m_mmu->write_byte(m_registers.hl, m_registers.a);
		m_registers.hl = inst_inc_w(m_registers.hl);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldh_imm_ptr_a()
	{
		m_mmu->write_byte(0xFF00 + immediate_byte(), m_registers.a);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ldh_a_imm_ptr()
	{
		m_registers.a = m_mmu->read_byte(0xFF00 + immediate_byte());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_c_ptr_a()
	{
		m_mmu->write_byte(0xFF00 + m_registers.c, m_registers.a);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_a_c_ptr()
	{
		m_registers.a = m_mmu->read_byte(0xFF00 + m_registers.c);
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 16-bit Loads
	//--------------------------------------------------------------------------

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_ld_nn_imm()
	{
		m_registers.set_register_word<DstRegister>(immediate_word());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_sp_hl()
	{
		m_registers.sp = m_registers.hl;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ld_hl_sp_imm()
	{
		const SByte amount = static_cast<SByte>(immediate_byte());
		const int32_t res  = m_registers.sp + amount;
		const int32_t temp = m_registers.sp ^ amount ^ res;

		m_registers.set_flag_if<RF::Carry>((temp & 0x100) == 0x100);
		m_registers.set_flag_if<RF::HalfCarry>((temp & 0x10) == 0x10);
		m_registers.clear_flags(RF::Zero | RF::Negative);
		m_registers.hl = res;

		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_imm_ptr_sp()
	{
		m_mmu->write_word(immediate_word(), m_registers.sp);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_push_nn()
	{
		stack_push(m_registers.get_register_word<SrcRegister>());
		return InstructionResult::Passed;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_pop_nn()
	{
		m_registers.set_register_word<DstRegister>(stack_pop());
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_pop_af()
	{
		// Lower nibble of F register is always 0.
		m_registers.af = (stack_pop() & 0xFFF0);
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - ADD
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_add(Byte val)
	{
		Byte calc = m_registers.a + val;
		m_registers.set_flag_if<RF::Zero>((calc & 0xFF) == 0);
		m_registers.clear_flag(RF::Negative);
		m_registers.set_flag_if<RF::Carry>((0xFF - m_registers.a) < val);
		m_registers.set_flag_if<RF::HalfCarry>(0x0F - (m_registers.a & 0x0F) < (val & 0x0F));
		m_registers.a = calc;

		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_add_n()
	{
		return inst_add(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_add_hl_ptr()
	{
		return inst_add(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_add_imm()
	{
		return inst_add(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - ADC
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_adc(Byte val)
	{
		Byte carryval = m_registers.is_flag_set(RF::Carry) ? 1 : 0;
		Word calc = static_cast<Word>(m_registers.a) + static_cast<Word>(val) + carryval;

		m_registers.clear_flag(RF::Negative);
		m_registers.set_flag_if<RF::HalfCarry>(((m_registers.a & 0x0F) + (val & 0x0F) + carryval) > 0x0F);
		m_registers.set_flag_if<RF::Carry>(calc > 0xFF);
		m_registers.a = static_cast<Byte>(calc & 0x00FF);
		m_registers.set_flag_if<RF::Zero>(m_registers.a == 0);

		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_adc_n()
	{
		return inst_adc(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_adc_hl_ptr()
	{
		return inst_adc(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_adc_imm()
	{
		return inst_adc(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - SUB
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_sub(Byte val)
	{
		m_registers.set_flag(RF::Negative);
		m_registers.set_flag_if<RF::Carry>(m_registers.a < val);
		m_registers.set_flag_if<RF::HalfCarry>((m_registers.a & 0x0F) < (val & 0x0F));
		m_registers.a -= val;
		m_registers.set_flag_if<RF::Zero>(m_registers.a  == 0);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_sub_n()
	{
		return inst_sub(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_sub_hl_ptr()
	{
		return inst_sub(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_sub_imm()
	{
		return inst_sub(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - SBC
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_sbc(Byte val)
	{
		const Word carryval		= m_registers.is_flag_set(RF::Carry) ? 1 : 0;
		const Word subamount	= (val + carryval);
		const Byte calc			= m_registers.a - subamount;

		m_registers.set_flag_if<RF::Zero>(calc == 0);
		m_registers.set_flag(RF::Negative);
		m_registers.set_flag_if<RF::Carry>(m_registers.a < subamount);

		// @todo Understand this better, not convinced...
		if(((m_registers.a & 0x0F) < (val & 0x0F)) ||
		   ((m_registers.a & 0x0F) < (subamount & 0x0F)) ||
		   (((m_registers.a & 0x0F) == (val & 0x0F)) && ((val & 0x0F) == 0x0F) && (carryval == 1))
		   )
		{
			m_registers.set_flag(RF::HalfCarry);
		}
		else
		{
			m_registers.clear_flag(RF::HalfCarry);
		}

		m_registers.a = calc;

		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_sbc_n()
	{
		return inst_sbc(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_sbc_hl_ptr()
	{
		return inst_sbc(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_sbc_imm()
	{
		return inst_sbc(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - AND
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_and(Byte val)
	{
		m_registers.a &= val;
		m_registers.clear_flags(RF::Negative | RF::Carry);
		m_registers.set_flag(RF::HalfCarry);
		m_registers.set_flag_if<RF::Zero>(m_registers.a == 0);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_and_n()
	{
		return inst_and(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_and_hl_ptr()
	{
		return inst_and(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_and_imm()
	{
		return inst_and(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - OR
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_or(Byte val)
	{
		m_registers.a |= val;
		m_registers.clear_flags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.set_flag_if<RF::Zero>(m_registers.a == 0);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_or_n()
	{
		return inst_or(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_or_hl_ptr()
	{
		return inst_or(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_or_imm()
	{
		return inst_or(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - XOR
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_xor(Byte val)
	{
		m_registers.a = m_registers.a ^ val;
		m_registers.clear_flags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.set_flag_if<RF::Zero>(m_registers.a == 0);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_xor_n()
	{
		return inst_xor(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_xor_hl_ptr()
	{
		return inst_xor(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_xor_imm()
	{
		return inst_xor(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - INC
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_cp(Byte val)
	{
		m_registers.set_flag_if<RF::Zero>(m_registers.a == val);
		m_registers.set_flag_if<RF::Carry>(val > m_registers.a);
		m_registers.set_flag_if<RF::HalfCarry>((val & 0x0F) > (m_registers.a & 0x0F));
		m_registers.set_flag(RF::Negative);
		return InstructionResult::Passed;
	}

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_cp_n()
	{
		return inst_cp(m_registers.get_register<SrcRegister>());
	}

	inline InstructionResult::Enum CPU::inst_cp_hl_ptr()
	{
		return inst_cp(m_mmu->read_byte(m_registers.hl));
	}

	inline InstructionResult::Enum CPU::inst_cp_imm()
	{
		return inst_cp(immediate_byte());
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - INC
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_inc(Byte val)
	{
		val++;
		m_registers.set_flag_if<RegisterFlag::HalfCarry>((val & 0x0F) == 0x00);
		m_registers.set_flag_if<RegisterFlag::Zero>(val == 0);
		m_registers.clear_flag(RegisterFlag::Negative);
		return val;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_inc_n()
	{
		Byte& dst = m_registers.get_register<DstRegister>();
		dst = inst_inc(dst);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_inc_hl_ptr()
	{
		m_mmu->write_byte(m_registers.hl, inst_inc(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 8-bit ALU - DEC
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_dec(Byte val)
	{
		val--;
		m_registers.set_flag_if<RegisterFlag::HalfCarry>((val & 0x0F) == 0x0F);
		m_registers.set_flag_if<RegisterFlag::Zero>(val == 0);
		m_registers.set_flag(RegisterFlag::Negative);
		return val;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_dec_n()
	{
		Byte& dst = m_registers.get_register<DstRegister>();
		dst = inst_dec(dst);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_dec_hl_ptr()
	{
		m_mmu->write_byte(m_registers.hl, inst_dec(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 16-bit ALU - ADD
	//--------------------------------------------------------------------------

	template<RT::Enum SrcRegister>
	inline InstructionResult::Enum CPU::inst_add_hl_nn()
	{
		Word src = m_registers.get_register_word<SrcRegister>();

		m_registers.set_flag_if<RF::Carry>((0xFFFF - m_registers.hl) < src);
		m_registers.set_flag_if<RF::HalfCarry>((0x0FFF - (m_registers.hl & 0x0FFF)) < (src & 0x0FFF));
		m_registers.clear_flag(RF::Negative);
		m_registers.hl += src;

		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_add_sp_imm()
	{
		// Sign extend.
		Byte immb = immediate_byte();
		SWord imm = static_cast<SWord>(static_cast<SByte>(immb));

		m_registers.set_flag_if<RF::Carry>((0xFF - (m_registers.sp & 0x00FF)) < immb);
		m_registers.set_flag_if<RF::HalfCarry>((0x0F - (m_registers.sp & 0x0F)) < (immb & 0x0F));
		m_registers.clear_flags(RF::Zero | RF::Negative);

		m_registers.sp += static_cast<Word>(imm);

		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 16-bit ALU - INC
	//--------------------------------------------------------------------------

	inline Word CPU::inst_inc_w(Word val)
	{
		// No flags modified in 16-bit mode
		return val + 1;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_inc_nn()
	{
		Word& dst = m_registers.get_register_word<DstRegister>();
		dst = inst_inc_w(dst);
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// 16-bit ALU - DEC
	//--------------------------------------------------------------------------

	inline Word CPU::inst_dec_w(Word val)
	{
		return val - 1;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_dec_nn()
	{
		Word& dst = m_registers.get_register_word<DstRegister>();
		dst = inst_dec_w(dst);
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Rotates & Shifts
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_rlca()
	{
		Byte& dst = m_registers.a;
		Byte carry = (dst & 0x80) >> 7;
		m_registers.set_flag_if<RF::Carry>(carry == 1);
		m_registers.clear_flags(RF::Negative | RF::Zero | RF::HalfCarry);

		dst <<= 1;
		dst |= carry;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_rla()
	{
		Byte& dst = m_registers.a;
		Byte carry = m_registers.is_flag_set(RF::Carry) ? 1 : 0;

		m_registers.set_flag_if<RF::Carry>((dst & 0x80) != 0);
		m_registers.clear_flags(RF::Negative | RF::Zero | RF::HalfCarry);
		dst <<= 1;
		dst |= carry;
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_rrca()
	{
		Byte& dst = m_registers.a;
		Byte carry = (dst & 0x01);
		m_registers.set_flag_if<RF::Carry>(carry != 0);
		m_registers.clear_flags(RF::Negative | RF::Zero | RF::HalfCarry);

		dst >>= 1;

		if(carry)
			dst |= 0x80;

		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_rra()
	{
		Byte& dst = m_registers.a;
		Byte carry = m_registers.is_flag_set(RF::Carry) ? 0x80 : 0;

		m_registers.set_flag_if<RF::Carry>((dst & 0x01) != 0);
		m_registers.clear_flags(RF::Negative | RF::Zero | RF::HalfCarry);
		dst >>= 1;
		dst |= carry;
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended
	//--------------------------------------------------------------------------

	inline InstructionResult::Enum CPU::inst_ext()
	{
		m_currentOpcodeExt = immediate_byte();

		Instruction& extendedInstruction = m_instructionsExt[m_currentOpcodeExt];
		InstructionFunction& func = extendedInstruction.function();
		Byte cycles = extendedInstruction.cycles((this->*func)());

		if(cycles == 0)
			log_error("Extended instruction executes zero cycles, this is impossible, indicates unimplemented instruction\n");

		m_instructionCycles += cycles;

		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - RLC
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_rlc(Byte val)
	{
		if((val & 0x80) != 0)
		{
			m_registers.set_flag(RF::Carry);
			val <<= 1;
			val += 1;
		}
		else
		{
			m_registers.clear_flag(RF::Carry);
			val <<= 1;
		}

		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::HalfCarry | RF::Negative);

		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_rlc_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_rlc(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_rlc_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_rlc(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - RRC
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_rrc(Byte val)
	{
		Byte carry = val & 0x01;

		val >>= 1;
		val |= ((carry != 0) ? 0x80 : 0);

		m_registers.set_flag_if<RF::Carry>(carry == 1);
		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);

		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_rrc_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_rrc(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_rrc_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_rrc(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - RL
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_rl(Byte val)
	{
		Byte carry = m_registers.is_flag_set(RF::Carry) ? 1 : 0;
		m_registers.set_flag_if<RF::Carry>((val & 0x80) != 0);

		val <<= 1;
		val += carry;

		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);

		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_rl_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_rl(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_rl_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_rl(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - RR
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_rr(Byte val)
	{
		bool bSetCarry = (val & 0x01) != 0;

		val >>= 1;

		if (m_registers.is_flag_set(RF::Carry))
		{
			val |= 0x80;
		}

		m_registers.set_flag_if<RF::Carry>(bSetCarry);
		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);

		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_rr_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_rr(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_rr_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_rr(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - SLA
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_sla(Byte val)
	{
		m_registers.set_flag_if<RF::Carry>((val & 0x80) != 0);
		val <<= 1;
		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);
		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_sla_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_sla(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_sla_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_sla(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - SRA
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_sra(Byte val)
	{
		m_registers.set_flag_if<RF::Carry>((val & 0x01) != 0);

		val = (val & 0x80) | (val >> 1);

		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);

		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_sra_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_sra(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_sra_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_sra(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - SWAP
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_swap(Byte val)
	{
		// Swap top 4 bits with bottom 4 bits.
		Byte res = (((val & 0xF0) >> 4) | ((val & 0x0F) << 4));
		m_registers.clear_flags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.set_flag_if<RF::Zero>(res == 0);
		return res;
	}

	template<RT::Enum DstRegister>
	inline InstructionResult::Enum CPU::inst_ext_swap_n()
	{
		Byte& dst = m_registers.get_register<DstRegister>();
		dst = inst_ext_swap(dst);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_swap_hl_ptr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_swap(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - SRL
	//--------------------------------------------------------------------------

	inline Byte CPU::inst_ext_srl(Byte val)
	{
		m_registers.set_flag_if<RF::Carry>((val & 0x01) != 0);
		val >>= 1;
		m_registers.set_flag_if<RF::Zero>(val == 0);
		m_registers.clear_flags(RF::Negative | RF::HalfCarry);
		return val;
	}

	template<RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_srl_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_srl(reg);
		return InstructionResult::Passed;
	}

	inline InstructionResult::Enum CPU::inst_ext_srl_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_srl(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - BIT
	//--------------------------------------------------------------------------

	template<Byte Bit>
	void CPU::inst_ext_bit(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = 1 << Bit;

		m_registers.set_flag_if<RF::Zero>((val & kBitMask) == 0); // Set if Bit is set on value, otherwise reset.
		m_registers.clear_flag(RF::Negative);
		m_registers.set_flag(RF::HalfCarry);
	}

	template<Byte Bit, RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_bit_b_n()
	{
		inst_ext_bit<Bit>(m_registers.get_register<Register>());
		return InstructionResult::Passed;
	}

	template<Byte Bit>
	inline InstructionResult::Enum CPU::inst_ext_bit_b_hl_addr()
	{
		inst_ext_bit<Bit>(m_mmu->read_byte(m_registers.hl));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - RESET
	//--------------------------------------------------------------------------

	template<Byte Bit>
	Byte CPU::inst_ext_reset(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = ~(1 << Bit);
		return (val & kBitMask);
	}

	template<Byte Bit, RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_reset_b_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_reset<Bit>(reg);
		return InstructionResult::Passed;
	}

	template<Byte Bit>
	inline InstructionResult::Enum CPU::inst_ext_reset_b_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_reset<Bit>(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
	// Extended - SET
	//--------------------------------------------------------------------------

	template<Byte Bit>
	Byte CPU::inst_ext_set(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = 1 << Bit;
		return (val | kBitMask);
	}

	template<Byte Bit, RT::Enum Register>
	inline InstructionResult::Enum CPU::inst_ext_set_b_n()
	{
		Byte& reg = m_registers.get_register<Register>();
		reg = inst_ext_set<Bit>(reg);
		return InstructionResult::Passed;
	}

	template<Byte Bit>
	inline InstructionResult::Enum CPU::inst_ext_set_b_hl_addr()
	{
		m_mmu->write_byte(m_registers.hl, inst_ext_set<Bit>(m_mmu->read_byte(m_registers.hl)));
		return InstructionResult::Passed;
	}

	//--------------------------------------------------------------------------
}