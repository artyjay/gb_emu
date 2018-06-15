#include <type_traits>

namespace gbhw
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Instruction
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte Instruction::GetOpcode() const
	{
		return m_opcode;
	}

	inline Byte Instruction::GetExtended() const
	{
		return m_extended;
	}

	inline Byte Instruction::GetByteSize() const
	{
		return m_byteSize;
	}

	inline Byte Instruction::GetCycles(bool bActionPerformed) const
	{
		return m_cycles[bActionPerformed ? 0 : 1];
	}

	inline RFB::Type Instruction::GetFlagBehaviour(RF::Type flag) const
	{
		return m_flagBehaviour[flag];
	}

	inline RTD::Type Instruction::GetArgType(uint32_t argIndex) const
	{
		if(argIndex < 2)
		{
			return m_args[argIndex];
		}

		return RTD::None;
	}

	inline const char* Instruction::GetAssembly() const
	{
		return m_assembly;
	}

	inline InstructionFunction& Instruction::GetFunction()
	{
		return m_function;
	}

	inline Address Instruction::GetAddress() const
	{
		return m_address;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Helpers
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline const Instruction& CPU::GetInstruction(Byte opcode) const
	{
		return m_instructions[opcode];
	}

	inline const Instruction& CPU::GetInstructionExtended(Byte opcode) const
	{
		return m_instructionsExtended[opcode];
	}

	inline Byte CPU::ImmediateByte(bool steppc)
	{
		Byte res = m_mmu->ReadByte(m_registers.pc);

		if(steppc)
		{
			m_registers.pc++;
		}

		return res;
	}

	inline Word CPU::ImmediateWord(bool steppc)
	{
		Word res = m_mmu->ReadWord(m_registers.pc);

		if (steppc)
		{
			m_registers.pc += 2;
		}

		return res;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Stack management
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline void CPU::StackPushWord(Word word)
	{
		m_registers.sp -= 2;
		m_mmu->WriteWord(m_registers.sp, word);
	}

	inline Word CPU::StackPopWord()
	{
		m_registers.sp += 2;
		return m_mmu->ReadWord(m_registers.sp - 2);
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Misc.
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_NOP()
	{
		// Empty.
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_DAA()
	{
		Word val = m_registers.a;

		if(!m_registers.IsFlagSet(RF::Negative))
		{
			if(m_registers.IsFlagSet(RF::HalfCarry) || ((val & 0xF) > 9))
			{
				val += 0x06;
			}

			if(m_registers.IsFlagSet(RF::Carry) || (val > 0x9F))
			{
				val += 0x60;
			}
		}
		else
		{
			if(m_registers.IsFlagSet(RF::HalfCarry))
			{
				val = (val - 6) & 0xFF;
			}

			if(m_registers.IsFlagSet(RF::Carry))
			{
				val -= 0x60;
			}
		}

		m_registers.ClearFlags(RF::HalfCarry | RF::Zero);

		if ((val & 0x100) == 0x100)
		{
			m_registers.SetFlag(RF::Carry);
		}

		val &= 0xFF;

		if(val == 0)
		{
			m_registers.SetFlag(RF::Zero);
		}

		m_registers.a = static_cast<Byte>(val);

		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_CPL()
	{
		m_registers.a = ~m_registers.a;
		m_registers.SetFlag(RF::Negative);
		m_registers.SetFlag(RF::HalfCarry);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_CCF()
	{
		m_registers.SetFlagIf<RF::Carry>(!m_registers.IsFlagSet(RF::Carry));
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_SCF()
	{
		m_registers.SetFlag(RF::Carry);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_HALT()
	{
		//Message("Halting CPU until interrupt is generated\n");
		//m_bHalted = true;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_STOP()
	{
		Byte empty = ImmediateByte();
		Message("Stopping CPU until input is detected, following byte = 0x%02x\n", empty);
		m_bStopped = true;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_DI()
	{
		m_registers.ime = false;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_EI()
	{
		m_registers.ime = true;
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Jumps
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_JP()
	{
		m_registers.pc = ImmediateWord(false);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RF::Type FlagType, bool IsSet>
	bool CPU::Instruction_JP_CC()
	{
		// Read new possible address
		Word address = ImmediateWord();

		// Jump only if condition is met.
		if (m_registers.IsFlagSet(FlagType) == IsSet)
		{
			m_registers.pc = address;
			GBHW_CPU_ACTION_PERFORMED;
		}

		GBHW_CPU_ACTION_FAILED;
	}

	inline bool CPU::Instruction_JP_HL()
	{
		m_registers.pc = m_registers.hl;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_JR()
	{
		Byte amount = ImmediateByte();

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

		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RF::Type FlagType, bool IsSet>
	bool CPU::Instruction_JR_CC()
	{
		// Read new possible address
		Byte amount = ImmediateByte();

		// Jump only if condition is met.
		if (m_registers.IsFlagSet(FlagType) == IsSet)
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

			GBHW_CPU_ACTION_PERFORMED;
		}

		GBHW_CPU_ACTION_FAILED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Calls
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_CALL()
	{
		StackPushWord(m_registers.pc + 2);		// Push next instruction address onto the stack.
		m_registers.pc = ImmediateWord(false);	// Jump to new address.
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RF::Type FlagType, bool IsSet>
	bool CPU::Instruction_CALL_CC()
	{
		Word address = ImmediateWord(); // Read new address always, this will move PC along.

		if (m_registers.IsFlagSet(FlagType) == IsSet)
		{
			StackPushWord(m_registers.pc);	// Push next instruction address onto the stack.
			m_registers.pc = address;		// Jump to new address.
			GBHW_CPU_ACTION_PERFORMED;
		}

		GBHW_CPU_ACTION_FAILED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Restarts
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<Byte Offset>
	bool CPU::Instruction_RST()
	{
		StackPushWord(m_registers.pc);
		m_registers.pc = (0x0000 + Offset);
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Returns
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_RET()
	{
		m_registers.pc = StackPopWord();
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RF::Type FlagType, bool IsSet>
	bool CPU::Instruction_RET_CC()
	{
		if (m_registers.IsFlagSet(FlagType) == IsSet)
		{
			m_registers.pc = StackPopWord();
			GBHW_CPU_ACTION_PERFORMED;
		}

		GBHW_CPU_ACTION_FAILED;
	}

	inline bool CPU::Instruction_RETI()
	{
		m_registers.pc = StackPopWord();
		m_registers.ime = true;
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit Loads
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<RT::Type DstRegister, RT::Type SrcRegister>
	bool CPU::Instruction_LD_N_N()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		Byte& src = m_registers.GetRegisterByte<SrcRegister>();
		dst = src;
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type DstRegister, RT::Type SrcRegisterPtr>
	bool CPU::Instruction_LD_N_N_PTR()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		Byte src = m_mmu->ReadByte(m_registers.GetRegisterWord<SrcRegisterPtr>());
		dst = src;
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type DstRegisterPtr, RT::Type SrcRegister>
	bool CPU::Instruction_LD_N_PTR_N()
	{
		Byte& src = m_registers.GetRegisterByte<SrcRegister>();
		m_mmu->WriteByte(m_registers.GetRegisterWord<DstRegisterPtr>(), src);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_LD_N_IMM()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		dst = ImmediateByte();
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LD_A_IMM_PTR()
	{
		m_registers.a = m_mmu->ReadByte(ImmediateWord());
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_LD_IMM_PTR_A()
	{
		m_mmu->WriteByte(ImmediateWord(), m_registers.a);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_LD_HL_PTR_IMM()
	{
		m_mmu->WriteByte(m_registers.hl, ImmediateByte());
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LDD_A_HL_PTR()
	{
		m_registers.a = m_mmu->ReadByte(m_registers.hl);
		m_registers.hl = Instruction_DEC_W(m_registers.hl);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LDD_HL_PTR_A()
	{
		m_mmu->WriteByte(m_registers.hl, m_registers.a);
		m_registers.hl = Instruction_DEC_W(m_registers.hl);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LDI_A_HL_PTR()
	{
		m_registers.a = m_mmu->ReadByte(m_registers.hl);
		m_registers.hl = Instruction_INC_W(m_registers.hl);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_LDI_HL_PTR_A()
	{
		m_mmu->WriteByte(m_registers.hl, m_registers.a);
		m_registers.hl = Instruction_INC_W(m_registers.hl);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LDH_IMM_PTR_A()
	{
		m_mmu->WriteByte(0xFF00 + ImmediateByte(), m_registers.a);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LDH_A_IMM_PTR()
	{
		m_registers.a = m_mmu->ReadByte(0xFF00 + ImmediateByte());
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LD_C_PTR_A()
	{
		m_mmu->WriteByte(0xFF00 + m_registers.c, m_registers.a);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LD_A_C_PTR()
	{
		m_registers.a = m_mmu->ReadByte(0xFF00 + m_registers.c);
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 16-bit Loads
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<RT::Type DstRegister>
	bool CPU::Instruction_LD_NN_IMM()
	{
		Word& dst = m_registers.GetRegisterWord<DstRegister>();
		dst = ImmediateWord();
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LD_SP_HL()
	{
		m_registers.sp = m_registers.hl;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_LD_HL_SP_IMM()
	{
		SByte amount = static_cast<SByte>(ImmediateByte());
		int32_t res = m_registers.sp + amount;
		int32_t temp = m_registers.sp ^ amount ^ res;

		m_registers.SetFlagIf<RF::Carry>((temp & 0x100) == 0x100);
		m_registers.SetFlagIf<RF::HalfCarry>((temp & 0x10) == 0x10);
		m_registers.ClearFlags(RF::Zero | RF::Negative);
		m_registers.hl = res;

		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_IMM_PTR_SP()
	{
		m_mmu->WriteWord(ImmediateWord(), m_registers.sp);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type SrcRegister>
	bool CPU::Instruction_PUSH_NN()
	{
		StackPushWord(m_registers.GetRegisterWord<SrcRegister>());
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_POP_NN()
	{
		Word& dst = m_registers.GetRegisterWord<DstRegister>();
		dst = StackPopWord();
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_POP_AF()
	{
		Word& dst = m_registers.GetRegisterWord<RT::AF>();
		dst = (StackPopWord() & 0xFFF0);	// Lower nibble of F register is always 0.
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - ADD
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_ADD(Byte val)
	{
		Byte calc = m_registers.a + val; 
		m_registers.SetFlagIf<RF::Zero>((calc & 0xFF) == 0);
		m_registers.ClearFlag(RF::Negative);
		m_registers.SetFlagIf<RF::Carry>((0xFF - m_registers.a) < val);
		m_registers.SetFlagIf<RF::HalfCarry>(0x0F - (m_registers.a & 0x0F) < (val & 0x0F));
		m_registers.a = calc;
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type SrcRegister>
	bool CPU::Instruction_ADD_N()
	{
		return Instruction_ADD(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_ADD_HL_PTR()
	{
		return Instruction_ADD(m_mmu->ReadByte(m_registers.hl));
	}

	inline bool CPU::Instruction_ADD_IMM()
	{
		return Instruction_ADD(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - ADC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_ADC(Byte val)
	{
		Byte carryval = m_registers.IsFlagSet(RF::Carry) ? 1 : 0;
		Word calc = static_cast<Word>(m_registers.a) + static_cast<Word>(val) + carryval;

		m_registers.ClearFlag(RF::Negative);
		m_registers.SetFlagIf<RF::HalfCarry>(((m_registers.a & 0x0F) + (val & 0x0F) + carryval) > 0x0F);
		m_registers.SetFlagIf<RF::Carry>(calc > 0xFF);
		m_registers.a = static_cast<Byte>(calc & 0x00FF);
		m_registers.SetFlagIf<RF::Zero>(m_registers.a == 0);

		GBHW_CPU_ACTION_PERFORMED;

#if 0

		let cy = if use_carry && self.regs.f.contains(CARRY) { 1 }
		else { 0 };
		let result = self.regs.a.wrapping_add(value).wrapping_add(cy);
		self.regs.f = ZERO.test(result == 0) |
			CARRY.test(self.regs.a as u16 + value as u16 + cy as u16 > 0xff) |
			HALF_CARRY.test((self.regs.a & 0xf) + (value & 0xf) + cy > 0xf);
		self.regs.a = result;

#endif
	}
	
	template<RT::Type SrcRegister>
	bool CPU::Instruction_ADC_N()
	{
		return Instruction_ADC(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_ADC_HL_PTR()
	{
		return Instruction_ADC(m_mmu->ReadByte(m_registers.hl));
	}
	
	inline bool CPU::Instruction_ADC_IMM()
	{
		return Instruction_ADC(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - SUB
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_SUB(Byte val)
	{
		m_registers.SetFlag(RF::Negative);
		m_registers.SetFlagIf<RF::Carry>(m_registers.a < val);
		m_registers.SetFlagIf<RF::HalfCarry>((m_registers.a & 0x0F) < (val & 0x0F));
		m_registers.a -= val;
		m_registers.SetFlagIf<RF::Zero>(m_registers.a  == 0);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	template<RT::Type SrcRegister> bool CPU::Instruction_SUB_N()
	{
		return Instruction_SUB(m_registers.GetRegisterByte<SrcRegister>());
	}

	inline bool CPU::Instruction_SUB_HL_PTR()
	{
		return Instruction_SUB(m_mmu->ReadByte(m_registers.hl));
	}
	
	inline bool CPU::Instruction_SUB_IMM()
	{
		return Instruction_SUB(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - SBC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_SBC(Byte val)
	{
		Word carryval	= m_registers.IsFlagSet(RF::Carry) ? 1 : 0;
		Word subamount	= (val + carryval);
		Byte calc		= m_registers.a - subamount;

		m_registers.SetFlagIf<RF::Zero>(calc == 0);
		m_registers.SetFlag(RF::Negative);
		m_registers.SetFlagIf<RF::Carry>(m_registers.a < subamount);

		// @todo Understand this better, not convinced...
		if(((m_registers.a & 0x0F) < (val & 0x0F)) ||
		   ((m_registers.a & 0x0F) < (subamount & 0x0F)) ||
		   (((m_registers.a & 0x0F) == (val & 0x0F)) && ((val & 0x0F) == 0x0F) && (carryval == 1))
		   )
		{
			m_registers.SetFlag(RF::HalfCarry);
		}
		else
		{
			m_registers.ClearFlag(RF::HalfCarry);
		}

		m_registers.a = calc;

		GBHW_CPU_ACTION_PERFORMED;
	}
	
	template<RT::Type SrcRegister>
	bool CPU::Instruction_SBC_N()
	{
		return Instruction_SBC(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_SBC_HL_PTR()
	{
		return Instruction_SBC(m_mmu->ReadByte(m_registers.hl));
	}
	
	inline bool CPU::Instruction_SBC_IMM()
	{
		return Instruction_SBC(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - AND
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_AND(Byte val)
	{
		m_registers.a &= val;
		m_registers.ClearFlags(RF::Negative | RF::Carry);
		m_registers.SetFlag(RF::HalfCarry);
		m_registers.SetFlagIf<RF::Zero>(m_registers.a == 0);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type SrcRegister>
	bool CPU::Instruction_AND_N()
	{
		return Instruction_AND(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_AND_HL_PTR()
	{
		return Instruction_AND(m_mmu->ReadByte(m_registers.hl));
	}
	
	inline bool CPU::Instruction_AND_IMM()
	{
		return Instruction_AND(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - OR
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_OR(Byte val)
	{
		m_registers.a |= val;
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.SetFlagIf<RF::Zero>(m_registers.a == 0);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	template<RT::Type SrcRegister>
	bool CPU::Instruction_OR_N()
	{
		return Instruction_OR(m_registers.GetRegisterByte<SrcRegister>());
	}

	inline bool CPU::Instruction_OR_HL_PTR()
	{
		return Instruction_OR(m_mmu->ReadByte(m_registers.hl));
	}
	
	inline bool CPU::Instruction_OR_IMM()
	{
		return Instruction_OR(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - XOR
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_XOR(Byte val)
	{
		m_registers.a = m_registers.a ^ val;
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.SetFlagIf<RF::Zero>(m_registers.a == 0);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<RT::Type SrcRegister>
	bool CPU::Instruction_XOR_N()
	{
		return Instruction_XOR(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_XOR_HL_PTR()
	{
		return Instruction_XOR(m_mmu->ReadByte(m_registers.hl));
	}

	inline bool CPU::Instruction_XOR_IMM()
	{
		return Instruction_XOR(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - INC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_CP(Byte val)
	{
		m_registers.SetFlagIf<RF::Zero>(m_registers.a == val);
		m_registers.SetFlagIf<RF::Carry>(val > m_registers.a);
		m_registers.SetFlagIf<RF::HalfCarry>((val & 0x0F) > (m_registers.a & 0x0F));
		m_registers.SetFlag(RF::Negative);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	template<RT::Type SrcRegister>
	bool CPU::Instruction_CP_N()
	{
		return Instruction_CP(m_registers.GetRegisterByte<SrcRegister>());
	}
	
	inline bool CPU::Instruction_CP_HL_PTR()
	{
		return Instruction_CP(m_mmu->ReadByte(m_registers.hl));
	}

	inline bool CPU::Instruction_CP_IMM()
	{
		return Instruction_CP(ImmediateByte());
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - INC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_INC(Byte val)
	{
		val++;
		m_registers.SetFlagIf<Registers::Flags::HalfCarry>((val & 0x0F) == 0x00);
		m_registers.SetFlagIf<Registers::Flags::Zero>(val == 0);
		m_registers.ClearFlag(Registers::Flags::Negative);
		return val;
	}
	
	template<RT::Type DstRegister>
	bool CPU::Instruction_INC_N()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		dst = Instruction_INC(dst);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_INC_HL_PTR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_INC(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 8-bit ALU - DEC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_DEC(Byte val)
	{
		val--;
		m_registers.SetFlagIf<Registers::Flags::HalfCarry>((val & 0x0F) == 0x0F);
		m_registers.SetFlagIf<Registers::Flags::Zero>(val == 0);
		m_registers.SetFlag(Registers::Flags::Negative);
		return val;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_DEC_N()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		dst = Instruction_DEC(dst);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_DEC_HL_PTR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_DEC(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 16-bit ALU - ADD
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<RT::Type SrcRegister>
	bool CPU::Instruction_ADD_HL_NN()
	{
		Word& src = m_registers.GetRegisterWord<SrcRegister>();

		m_registers.SetFlagIf<RF::Carry>((0xFFFF - m_registers.hl) < src);
		m_registers.SetFlagIf<RF::HalfCarry>((0x0FFF - (m_registers.hl & 0x0FFF)) < (src & 0x0FFF));
		m_registers.ClearFlag(RF::Negative);

		m_registers.hl += src;

		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_ADD_SP_IMM()
	{
		// Sign extend.
		Byte immb = ImmediateByte();
		SWord imm = static_cast<SWord>(static_cast<SByte>(immb));

		m_registers.SetFlagIf<RF::Carry>((0xFF - (m_registers.sp & 0x00FF)) < immb);
		m_registers.SetFlagIf<RF::HalfCarry>((0x0F - (m_registers.sp & 0x0F)) < (immb & 0x0F));
		m_registers.ClearFlags(RF::Zero | RF::Negative);

		m_registers.sp += static_cast<Word>(imm);

		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 16-bit ALU - INC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Word CPU::Instruction_INC_W(Word val)
	{
		// No flags modifiied in 16-bit mode
		return val + 1;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_INC_NN()
	{
		Word& dst = m_registers.GetRegisterWord<DstRegister>();
		dst = Instruction_INC_W(dst);
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 16-bit ALU - DEC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Word CPU::Instruction_DEC_W(Word val)
	{
		return val - 1;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_DEC_NN()
	{
		Word& dst = m_registers.GetRegisterWord<DstRegister>();
		dst = Instruction_DEC_W(dst);
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Rotates & Shifts
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_RLCA()
	{
		Byte& dst = m_registers.GetRegisterByte<RT::A>();
		Byte carry = (dst & 0x80) >> 7;
		m_registers.SetFlagIf<RF::Carry>(carry == 1);
		m_registers.ClearFlags(RF::Negative | RF::Zero | RF::HalfCarry);

		dst <<= 1;
		dst |= carry;
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_RLA()
	{
		Byte& dst = m_registers.GetRegisterByte<RT::A>();
		Byte carry = m_registers.IsFlagSet(RF::Carry) ? 1 : 0;

		m_registers.SetFlagIf<RF::Carry>((dst & 0x80) != 0);
		m_registers.ClearFlags(RF::Negative | RF::Zero | RF::HalfCarry);
		dst <<= 1;
		dst |= carry;
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_RRCA()
	{
		Byte& dst = m_registers.GetRegisterByte<RT::A>();
		Byte carry = (dst & 0x01);
		m_registers.SetFlagIf<RF::Carry>(carry != 0);
		m_registers.ClearFlags(RF::Negative | RF::Zero | RF::HalfCarry);

		dst >>= 1;

		if(carry)
			dst |= 0x80;

		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_RRA()
	{
		Byte& dst = m_registers.GetRegisterByte<RT::A>();
		Byte carry = m_registers.IsFlagSet(RF::Carry) ? 0x80 : 0;

		m_registers.SetFlagIf<RF::Carry>((dst & 0x01) != 0);
		m_registers.ClearFlags(RF::Negative | RF::Zero | RF::HalfCarry);
		dst >>= 1;
		dst |= carry;
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline bool CPU::Instruction_EXT()
	{
		// do the extended instruction...
		// unbounded cycling
		m_currentExtendedInstruction = ImmediateByte();

		Instruction& extendedInstruction = m_instructionsExtended[m_currentExtendedInstruction];
		InstructionFunction& func = extendedInstruction.GetFunction();
		bool bActionPerformed = (this->*func)();
		Byte cycles = extendedInstruction.GetCycles(bActionPerformed);
		assert(cycles != 0);
		m_currentInstructionCycles += cycles;

		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - RLC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	inline Byte CPU::Instruction_EXT_RLC(Byte val)
	{
		if((val & 0x80) != 0)
		{
			m_registers.SetFlag(RF::Carry);
			val <<= 1;
			val += 1;
		}
		else
		{
			m_registers.ClearFlag(RF::Carry);
			val <<= 1;
		}

		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::HalfCarry | RF::Negative);

		return val;
	}
	
	template<RT::Type Register>
	bool CPU::Instruction_EXT_RLC_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_RLC(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_EXT_RLC_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_RLC(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - RRC
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_RRC(Byte val)
	{
		Byte carry = val & 0x01;

		val >>= 1;
		val |= ((carry != 0) ? 0x80 : 0);

		m_registers.SetFlagIf<RF::Carry>(carry == 1);
		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);

		return val;
	}
	
	template<RT::Type Register>
	bool CPU::Instruction_EXT_RRC_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_RRC(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_EXT_RRC_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_RRC(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - RL
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_RL(Byte val)
	{
		Byte carry = m_registers.IsFlagSet(RF::Carry) ? 1 : 0;
		m_registers.SetFlagIf<RF::Carry>((val & 0x80) != 0);

		val <<= 1;
		val += carry;

		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);

		return val;
	}
	
	template<RT::Type Register>
	bool CPU::Instruction_EXT_RL_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_RL(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_EXT_RL_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_RL(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - RR
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_RR(Byte val)
	{
		bool bSetCarry = (val & 0x01) != 0;

		val >>= 1;

		if (m_registers.IsFlagSet(RF::Carry))
		{
			val |= 0x80;
		}

		m_registers.SetFlagIf<RF::Carry>(bSetCarry);
		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);

		return val;
	}

	template<RT::Type Register>
	bool CPU::Instruction_EXT_RR_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_RR(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_EXT_RR_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_RR(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - SLA
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_SLA(Byte val)
	{
		m_registers.SetFlagIf<RF::Carry>((val & 0x80) != 0);
		val <<= 1;
		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);
		return val;
	}

	template<RT::Type Register>
	bool CPU::Instruction_EXT_SLA_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_SLA(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_EXT_SLA_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_SLA(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - SRA
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_SRA(Byte val)
	{
		m_registers.SetFlagIf<RF::Carry>((val & 0x01) != 0);

		val = (val & 0x80) | (val >> 1);

		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);

		return val;
	}
	
	template<RT::Type Register> bool CPU::Instruction_EXT_SRA_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_SRA(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}
	
	inline bool CPU::Instruction_EXT_SRA_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_SRA(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - SWAP
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_SWAP(Byte val)
	{
		// Swap top 4 bits with bottom 4 bits.
		Byte res = (((val & 0xF0) >> 4) | ((val & 0x0F) << 4));
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry | RF::Carry);
		m_registers.SetFlagIf<RF::Zero>(res == 0);
		return res;
	}

	template<RT::Type DstRegister>
	bool CPU::Instruction_EXT_SWAP_N()
	{
		Byte& dst = m_registers.GetRegisterByte<DstRegister>();
		dst = Instruction_EXT_SWAP(dst);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_EXT_SWAP_HL_PTR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_SWAP(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - SRL
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	inline Byte CPU::Instruction_EXT_SRL(Byte val)
	{
		m_registers.SetFlagIf<RF::Carry>((val & 0x01) != 0);
		val >>= 1;
		m_registers.SetFlagIf<RF::Zero>(val == 0);
		m_registers.ClearFlags(RF::Negative | RF::HalfCarry);
		return val;
	}

	template<RT::Type Register>
	bool CPU::Instruction_EXT_SRL_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_SRL(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}

	inline bool CPU::Instruction_EXT_SRL_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_SRL(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - BIT
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<Byte Bit>
	void CPU::Instruction_EXT_BIT(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = 1 << Bit;

		m_registers.SetFlagIf<RF::Zero>((val & kBitMask) == 0); // Set if Bit is set on value, otherwise reset.
		m_registers.ClearFlag(RF::Negative);
		m_registers.SetFlag(RF::HalfCarry);
	}

	template<Byte Bit, RT::Type Register>
	bool CPU::Instruction_EXT_BIT_B_N()
	{
		Instruction_EXT_BIT<Bit>(m_registers.GetRegisterByte<Register>());
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<Byte Bit>
	bool CPU::Instruction_EXT_BIT_B_HL_ADDR()
	{
		Instruction_EXT_BIT<Bit>(m_mmu->ReadByte(m_registers.hl));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - RESET
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<Byte Bit>
	Byte CPU::Instruction_EXT_RESET(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = ~(1 << Bit);
		return (val & kBitMask);
	}

	template<Byte Bit, RT::Type Register>
	bool CPU::Instruction_EXT_RESET_B_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_RESET<Bit>(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<Byte Bit>
	bool CPU::Instruction_EXT_RESET_B_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_RESET<Bit>(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Extended - SET
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<Byte Bit>
	Byte CPU::Instruction_EXT_SET(Byte val)
	{
		static_assert(Bit >= 0 && Bit <= 7, "Bit is invalid, must be between 0 and 7 inclusive");
		static const Byte kBitMask = 1 << Bit;
		return (val | kBitMask);
	}

	template<Byte Bit, RT::Type Register>
	bool CPU::Instruction_EXT_SET_B_N()
	{
		Byte& reg = m_registers.GetRegisterByte<Register>();
		reg = Instruction_EXT_SET<Bit>(reg);
		GBHW_CPU_ACTION_PERFORMED;
	}

	template<Byte Bit>
	bool CPU::Instruction_EXT_SET_B_HL_ADDR()
	{
		m_mmu->WriteByte(m_registers.hl, Instruction_EXT_SET<Bit>(m_mmu->ReadByte(m_registers.hl)));
		GBHW_CPU_ACTION_PERFORMED;
	}

} // gbhw