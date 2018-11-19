#pragma once

#include "context.h"
#include "log.h"
#include "registers.h"

namespace gbhw
{
	//--------------------------------------------------------------------------
	// Instruction
	//--------------------------------------------------------------------------

	struct InstructionResult
	{
		enum Enum
		{
			Passed = 0,
			Failed
		};
	};

	class CPU;
	using InstructionFunction = InstructionResult::Enum (CPU::*)();

	class Instruction
	{
	public:
		Instruction();

		void set(Byte opcode, Byte extended, Byte byteSize, Byte cycles0, Byte cycles1, RFB::Enum behaviour0, RFB::Enum behaviour1, RFB::Enum behaviour2, RFB::Enum behaviour3, RTD::Enum args0, RTD::Enum args1, const char* assembly);
		void set(InstructionFunction fn);
		void set(Address address);

		inline Byte						opcode() const;
		inline Byte						is_extended() const;
		inline Byte						byte_size() const;
		inline Byte						cycles(InstructionResult::Enum result) const;
		inline RFB::Enum				flag_behaviour(RF::Enum flag) const;
		inline RTD::Enum				arg_type(uint32_t argIndex) const;
		inline const char*				assembly() const;
		inline InstructionFunction&		function();
		inline Address					address() const;

	private:
		InstructionFunction		m_function;
		Byte					m_opcode;
		Byte					m_extended;
		Byte					m_byteSize;
		Byte					m_cycles[2];	// 0 for normal behaviour, 1 for failed execution (i.e. branch failed, etc...)
		RFB::Enum				m_flagBehaviour[RF::kCount];
		RTD::Enum				m_args[2];
		const char*				m_assembly;
		Address					m_address;
	};

	typedef std::vector<Instruction> InstructionList;

	//--------------------------------------------------------------------------
	// CPU
	//--------------------------------------------------------------------------

	class CPU
	{
		friend class Instruction;

	public:
		CPU();
		virtual ~CPU();

		void initialise(MMU_ptr mmu);
		void release();

		uint16_t update(uint16_t maxcycles);
		void update_stalled();
		bool is_stalled() const;

		// I/O - external interface for interrupt handling and i/o.
		Byte read_io(HWRegs::Type reg);
		void write_io(HWRegs::Type reg, Byte val);
		void generate_interrupt(HWInterrupts::Type interrupt);

	protected:
		void handle_interrupts();
		void handle_interrupt(HWInterrupts::Type interrupt, HWInterruptRoutines::Type routine, Byte regif, Byte regie);

		void load_instructions();
		InstructionResult::Enum instruction_not_implemented();
		InstructionResult::Enum instruction_not_implemented_ext();

		// Helpers
		inline Byte immediate_byte(bool steppc = true);
		inline Word immediate_word(bool steppc = true);

		// Stack management
		inline void stack_push(Word word);
		inline Word stack_pop();

		// Misc.
		inline InstructionResult::Enum inst_nop();																	// NOP
		inline InstructionResult::Enum inst_daa();																	// DAA
		inline InstructionResult::Enum inst_cpl();																	// CPL
		inline InstructionResult::Enum inst_ccf();																	// CCF
		inline InstructionResult::Enum inst_scf();																	// SCF

		// scf
		inline InstructionResult::Enum inst_halt();																	// HALT
		inline InstructionResult::Enum inst_stop();																	// STOP
		inline InstructionResult::Enum inst_di();																	// DI
		inline InstructionResult::Enum inst_ei();																	// EI

		// Jumps
		inline InstructionResult::Enum inst_jp();																	// JP nn
		template<RF::Enum FlagType, bool IsSet> inline InstructionResult::Enum inst_jp_cc();						// JP cc,nn
		inline InstructionResult::Enum inst_jp_hl();																// JP (HL)
		inline InstructionResult::Enum inst_jr();																	// JR n
		template<RF::Enum FlagType, bool IsSet> inline InstructionResult::Enum inst_jr_cc();						// JR cc,n

		// Calls
		inline InstructionResult::Enum inst_call();																	// CALL nn
		template<RF::Enum FlagType, bool IsSet> inline InstructionResult::Enum inst_call_cc();						// CALL cc,nn

		// Restarts
		template<Byte Offset> InstructionResult::Enum inst_rst();													// RST n

		// Returns
		inline InstructionResult::Enum inst_ret();																	// RET
		template<RF::Enum FlagType, bool IsSet> inline InstructionResult::Enum inst_ret_cc();						// RET cc
		inline InstructionResult::Enum inst_reti();																	// RETI

		// 8-bit Loads
		template<RT::Enum DstRegister, RT::Enum SrcRegister> inline InstructionResult::Enum inst_ld_n_n();			// LD r1,r2
		template<RT::Enum DstRegister, RT::Enum SrcRegisterPtr> inline InstructionResult::Enum inst_ld_n_n_ptr();	// LD r1,(NN)
		template<RT::Enum DstRegisterPtr, RT::Enum SrcRegister> inline InstructionResult::Enum inst_ld_n_ptr_n();	// LD (NN),r2
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_ld_n_imm();								// LD nn,n
		inline InstructionResult::Enum inst_ld_a_imm_ptr();															// LD A, (nn)
		inline InstructionResult::Enum inst_ld_imm_ptr_a();															// LD (nn), A
		inline InstructionResult::Enum inst_ld_hl_ptr_imm();														// LD (HL), n
		inline InstructionResult::Enum inst_ldd_a_hl_ptr();															// LDD A, (HL)
		inline InstructionResult::Enum inst_ldd_hl_ptr_a();															// LDD (HL), A
		inline InstructionResult::Enum inst_ldi_a_hl_ptr();															// LDI A, (HL)
		inline InstructionResult::Enum inst_ldi_hl_ptr_a();															// LDI (HL), A
		inline InstructionResult::Enum inst_ldh_imm_ptr_a();														// LDH (n), A
		inline InstructionResult::Enum inst_ldh_a_imm_ptr();														// LDH A, (n)
		inline InstructionResult::Enum inst_ld_c_ptr_a();															// LD (C), A or LDH (C), A
		inline InstructionResult::Enum inst_ld_a_c_ptr();															// LD A, (C) or LDH A, (C)

		// 16-bit Loads
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_ld_nn_imm();								// LD n,nn
		inline InstructionResult::Enum inst_ld_sp_hl();																// LD SP, HL
		inline InstructionResult::Enum inst_ld_hl_sp_imm();															// LD HL, SP+n
		inline InstructionResult::Enum inst_imm_ptr_sp();															// LD (nn), SP
		template<RT::Enum SrcRegister> InstructionResult::Enum inst_push_nn();										// PUSH NN
		template<RT::Enum DstRegister> InstructionResult::Enum inst_pop_nn();										// POP NN
		inline InstructionResult::Enum inst_pop_af();																// POP AF

		// 8-bit ALU - ADD
		inline InstructionResult::Enum inst_add(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_add_n();									// ADD A, N
		inline InstructionResult::Enum inst_add_hl_ptr();															// ADD A, (HL)
		inline InstructionResult::Enum inst_add_imm();																// ADD A, n

		// 8-bit ALU - ADC
		inline InstructionResult::Enum inst_adc(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_adc_n();									// ADC A, N
		inline InstructionResult::Enum inst_adc_hl_ptr();															// ADC A, (HL)
		inline InstructionResult::Enum inst_adc_imm();																// ADC A, n

		// 8-bit ALU - SUB
		inline InstructionResult::Enum inst_sub(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_sub_n();									// SUB A, N
		inline InstructionResult::Enum inst_sub_hl_ptr();															// SUB A, (HL)
		inline InstructionResult::Enum inst_sub_imm();																// SUB A, n

		// 8-bit ALU - SBC
		inline InstructionResult::Enum inst_sbc(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_sbc_n();									// SBC A, N
		inline InstructionResult::Enum inst_sbc_hl_ptr();															// SBC A, (HL)
		inline InstructionResult::Enum inst_sbc_imm();																// SBC A, n

		// 8-bit ALU - AND
		inline InstructionResult::Enum inst_and(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_and_n();									// AND A, N
		inline InstructionResult::Enum inst_and_hl_ptr();															// AND A, (HL)
		inline InstructionResult::Enum inst_and_imm();																// AND A, n

		// 8-bit ALU - OR
		inline InstructionResult::Enum inst_or(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_or_n();									// OR A, N
		inline InstructionResult::Enum inst_or_hl_ptr();															// OR A, (HL)
		inline InstructionResult::Enum inst_or_imm();																// OR A, n

		// 8-bit ALU - XOR
		inline InstructionResult::Enum inst_xor(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_xor_n();									// XOR A, N
		inline InstructionResult::Enum inst_xor_hl_ptr();															// XOR A, (HL)
		inline InstructionResult::Enum inst_xor_imm();																// XOR A, n

		// 8-bit ALU - CP
		inline InstructionResult::Enum inst_cp(Byte val);
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_cp_n();									// CP N
		inline InstructionResult::Enum inst_cp_hl_ptr();															// CP (HL)
		inline InstructionResult::Enum inst_cp_imm();																// CP n

		// 8-bit ALU - INC
		inline Byte inst_inc(Byte val);
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_inc_n();									// INC N
		inline InstructionResult::Enum inst_inc_hl_ptr();															// INC (HL)

		// 8-bit ALU - DEC
		inline Byte inst_dec(Byte val);
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_dec_n();									// DEC N
		inline InstructionResult::Enum inst_dec_hl_ptr();															// DEC (HL)

		// 16-bit ALU - ADD
		template<RT::Enum SrcRegister> inline InstructionResult::Enum inst_add_hl_nn();								// ADD HL, NN
		inline InstructionResult::Enum inst_add_sp_imm();															// ADD SP, n

		// 16-bit ALU - INC
		inline Word inst_inc_w(Word val);
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_inc_nn();								// INC NN

		// 16-bit ALU - DEC
		inline Word inst_dec_w(Word val);
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_dec_nn();								// DEC NN

		// Rotates & Shifts
		inline InstructionResult::Enum inst_rlca();
		inline InstructionResult::Enum inst_rla();
		inline InstructionResult::Enum inst_rrca();
		inline InstructionResult::Enum inst_rra();

		// Extended Instructions
		inline InstructionResult::Enum inst_ext();

		// Extended - RLC
		inline Byte inst_ext_rlc(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_rlc_n();								// RLC N
		inline InstructionResult::Enum inst_ext_rlc_hl_addr();														// RLC (HL)

		// Extended - RRC
		inline Byte inst_ext_rrc(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_rrc_n();								// RRC N
		inline InstructionResult::Enum inst_ext_rrc_hl_addr();														// RRC (HL)

		// Extended - RL
		inline Byte inst_ext_rl(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_rl_n();									// RL N
		inline InstructionResult::Enum inst_ext_rl_hl_addr();														// RL (HL)

		// Extended - RR
		inline Byte inst_ext_rr(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_rr_n();									// RR N
		inline InstructionResult::Enum inst_ext_rr_hl_addr();														// RR (HL)

		// Extended - SLA
		inline Byte inst_ext_sla(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_sla_n();								// SLA N
		inline InstructionResult::Enum inst_ext_sla_hl_addr();														// SLA (HL)

		// Extended - SRA
		inline Byte inst_ext_sra(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_sra_n();								// SRA N
		inline InstructionResult::Enum inst_ext_sra_hl_addr();														// SRA (HL)

		// Extended - SWAP
		inline Byte inst_ext_swap(Byte val);
		template<RT::Enum DstRegister> inline InstructionResult::Enum inst_ext_swap_n();							// SWAP N
		inline InstructionResult::Enum inst_ext_swap_hl_ptr();														// SWAP (HL)

		// Extended - SRL
		inline Byte inst_ext_srl(Byte val);
		template<RT::Enum Register> inline InstructionResult::Enum inst_ext_srl_n();								// SRL N
		inline InstructionResult::Enum inst_ext_srl_hl_addr();														// SRL (HL)

		// Extended - BIT
		template<Byte Bit> void inst_ext_bit(Byte val);
		template<Byte Bit, RT::Enum Register> inline InstructionResult::Enum inst_ext_bit_b_n();					// BIT B,N
		template<Byte Bit> InstructionResult::Enum inst_ext_bit_b_hl_addr();										// BIT B,(HL)

		// Extended - RES
		template<Byte Bit> Byte inst_ext_reset(Byte val);
		template<Byte Bit, RT::Enum Register> inline InstructionResult::Enum inst_ext_reset_b_n();					// RES B,N
		template<Byte Bit> InstructionResult::Enum inst_ext_reset_b_hl_addr();										// RES B,N

		// Extended - SET
		template<Byte Bit> Byte inst_ext_set(Byte val);
		template<Byte Bit, RT::Enum Register> inline InstructionResult::Enum inst_ext_set_b_n();					// SET B,N
		template<Byte Bit> InstructionResult::Enum inst_ext_set_b_hl_addr();										// SET B,(HL)

		static const uint32_t kInstructionCount = 256;

		MMU_ptr					m_mmu;
		Registers				m_registers;

		bool					m_bStopped;
		bool					m_bHalted;

		Byte					m_currentOpcode;
		Byte					m_currentOpcodeExt;
		uint16_t				m_instructionCycles;	// Normal or extended.

		Instruction				m_instructions[kInstructionCount];
		Instruction				m_instructionsExt[kInstructionCount];
	};
}

#include "cpu.inl"
