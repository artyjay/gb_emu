#pragma once

#include "gbhw_log.h"
#include "gbhw_mmu.h"
#include "gbhw_registers.h"

#include <assert.h>
#include <vector>

namespace gbhw
{
	class CPU;
	typedef bool(CPU::*InstructionFunction)();

	class Instruction
	{
	public:
		Instruction();

		void Set(Byte opcode, Byte extended, Byte byteSize, Byte cycles0, Byte cycles1, RFB::Type behaviour0, RFB::Type behaviour1, RFB::Type behaviour2, RFB::Type behaviour3, RTD::Type args0, RTD::Type args1, const char* assembly);
		void Set(InstructionFunction fn);
		void Set(Address address);

		inline Byte						GetOpcode() const;
		inline Byte						GetExtended() const;
		inline Byte						GetByteSize() const;
		inline Byte						GetCycles(bool bActionPerformed) const;
		inline RFB::Type				GetFlagBehaviour(RF::Type flag) const;
		inline RTD::Type				GetArgType(uint32_t argIndex) const;
		inline const char*				GetAssembly() const;
		inline InstructionFunction&		GetFunction();
		inline Address					GetAddress() const;

	private:
		InstructionFunction		m_function;
		Byte					m_opcode;
		Byte					m_extended;
		Byte					m_byteSize;
		Byte					m_cycles[2];	// 0 for normal behaviour, 1 for failed execution (i.e. branch failed, etc...)
		RFB::Type				m_flagBehaviour[RF::kCount];
		RTD::Type				m_args[2];
		const char*				m_assembly;
		Address					m_address;
	};

	typedef std::vector<Instruction> InstructionList;

	// CPU implements the processors instruction-set.
	class CPU
	{
		friend class Instruction;

	public:
		CPU();
		virtual ~CPU();

		void Initialise(MMU* mmu);

		uint16_t Update(uint16_t maxcycles);
		void UpdateStalled();
		bool IsStalled() const;

		const Registers& GetRegisters() const;

		// Instructions
		inline const Instruction& GetInstruction(Byte opcode) const;
		inline const Instruction& GetInstructionExtended(Byte opcode) const;

		// I/O - external interface for interrupt handling and such.
		Byte ReadIO(HWRegs::Type reg);
		void WriteIO(HWRegs::Type reg, Byte val);
		void GenerateInterrupt(HWInterrupts::Type interrupt);

		// Breakpoints
		bool IsBreakpoint() const;
		void BreakpointSet(Address address);
		void BreakpointRemove(Address address);
		void BreakpointSkip(); // Set a latch to skip the break-point we're currently on.

	protected:
		void HandleBreakpoints();
		void HandleInterrupts();
		void HandleInterrupt(HWInterrupts::Type interrupt, HWInterruptRoutines::Type routine, Byte regif, Byte regie);

		void InitialiseInstructions();
		bool InstructionNotImplemented();
		bool InstructionNotImplementedExt();

		// Helpers
		inline Byte ImmediateByte(bool steppc = true);
		inline Word ImmediateWord(bool steppc = true);

		// Stack management
		inline void StackPushWord(Word word);
		inline Word StackPopWord();

		// Misc.
		inline bool Instruction_NOP();															// NOP
		inline bool Instruction_DAA();															// DAA
		inline bool Instruction_CPL();															// CPL
		inline bool Instruction_CCF();															// CCF
		inline bool Instruction_SCF();															// SCF
		// scf
		inline bool Instruction_HALT();															// HALT
		inline bool Instruction_STOP();															// STOP
		inline bool Instruction_DI();															// DI
		inline bool Instruction_EI();															// EI

		// Jumps
		inline bool Instruction_JP();															// JP nn
		template<RF::Type FlagType, bool IsSet> bool Instruction_JP_CC();						// JP cc,nn
		inline bool Instruction_JP_HL();														// JP (HL)
		inline bool Instruction_JR();															// JR n
		template<RF::Type FlagType, bool IsSet> bool Instruction_JR_CC();						// JR cc,n

		// Calls
		inline bool Instruction_CALL();															// CALL nn
		template<RF::Type FlagType, bool IsSet> bool Instruction_CALL_CC();						// CALL cc,nn

		// Restarts
		template<Byte Offset> bool Instruction_RST();											// RST n

		// Returns
		inline bool Instruction_RET();															// RET
		template<RF::Type FlagType, bool IsSet> bool Instruction_RET_CC();						// RET cc
		inline bool Instruction_RETI();															// RETI

		// 8-bit Loads
		template<RT::Type DstRegister, RT::Type SrcRegister> bool Instruction_LD_N_N();			// LD r1,r2
		template<RT::Type DstRegister, RT::Type SrcRegisterPtr> bool Instruction_LD_N_N_PTR();	// LD r1,(NN)
		template<RT::Type DstRegisterPtr, RT::Type SrcRegister> bool Instruction_LD_N_PTR_N();	// LD (NN),r2
		template<RT::Type DstRegister> bool Instruction_LD_N_IMM();								// LD nn,n
		inline bool Instruction_LD_A_IMM_PTR();													// LD A, (nn)
		inline bool Instruction_LD_IMM_PTR_A();													// LD (nn), A
		inline bool Instruction_LD_HL_PTR_IMM();												// LD (HL), n
		inline bool Instruction_LDD_A_HL_PTR();													// LDD A, (HL)
		inline bool Instruction_LDD_HL_PTR_A();													// LDD (HL), A
		inline bool Instruction_LDI_A_HL_PTR();													// LDI A, (HL)
		inline bool Instruction_LDI_HL_PTR_A();													// LDI (HL), A
		inline bool Instruction_LDH_IMM_PTR_A();												// LDH (n), A
		inline bool Instruction_LDH_A_IMM_PTR();												// LDH A, (n)
		inline bool Instruction_LD_C_PTR_A();													// LD (C), A or LDH (C), A
		inline bool Instruction_LD_A_C_PTR();													// LD A, (C) or LDH A, (C)

		// 16-bit Loads
		template<RT::Type DstRegister> bool Instruction_LD_NN_IMM();							// LD n,nn
		inline bool Instruction_LD_SP_HL();														// LD SP, HL
		inline bool Instruction_LD_HL_SP_IMM();													// LD HL, SP+n
		inline bool Instruction_IMM_PTR_SP();													// LD (nn), SP
		template<RT::Type SrcRegister> bool Instruction_PUSH_NN();								// PUSH NN
		template<RT::Type DstRegister> bool Instruction_POP_NN();								// POP NN
		inline bool Instruction_POP_AF();														// POP AF

		// 8-bit ALU - ADD
		inline bool Instruction_ADD(Byte val);
		template<RT::Type SrcRegister> bool Instruction_ADD_N();								// ADD A, N
		inline bool Instruction_ADD_HL_PTR();													// ADD A, (HL)
		inline bool Instruction_ADD_IMM();														// ADD A, n

		// 8-bit ALU - ADC
		inline bool Instruction_ADC(Byte val);
		template<RT::Type SrcRegister> bool Instruction_ADC_N();								// ADC A, N
		inline bool Instruction_ADC_HL_PTR();													// ADC A, (HL)
		inline bool Instruction_ADC_IMM();														// ADC A, n

		// 8-bit ALU - SUB
		inline bool Instruction_SUB(Byte val);
		template<RT::Type SrcRegister> bool Instruction_SUB_N();								// SUB A, N
		inline bool Instruction_SUB_HL_PTR();													// SUB A, (HL)
		inline bool Instruction_SUB_IMM();														// SUB A, n

		// 8-bit ALU - SBC
		inline bool Instruction_SBC(Byte val);
		template<RT::Type SrcRegister> bool Instruction_SBC_N();								// SBC A, N
		inline bool Instruction_SBC_HL_PTR();													// SBC A, (HL)
		inline bool Instruction_SBC_IMM();														// SBC A, n

		// 8-bit ALU - AND
		inline bool Instruction_AND(Byte val);
		template<RT::Type SrcRegister> bool Instruction_AND_N();								// AND A, N
		inline bool Instruction_AND_HL_PTR();													// AND A, (HL)
		inline bool Instruction_AND_IMM();														// AND A, n

		// 8-bit ALU - OR
		inline bool Instruction_OR(Byte val);
		template<RT::Type SrcRegister> bool Instruction_OR_N();									// OR A, N
		inline bool Instruction_OR_HL_PTR();													// OR A, (HL)
		inline bool Instruction_OR_IMM();														// OR A, n

		// 8-bit ALU - XOR
		inline bool Instruction_XOR(Byte val);
		template<RT::Type SrcRegister> bool Instruction_XOR_N();								// XOR A, N
		inline bool Instruction_XOR_HL_PTR();													// XOR A, (HL)
		inline bool Instruction_XOR_IMM();														// XOR A, n

		// 8-bit ALU - CP
		inline bool Instruction_CP(Byte val);
		template<RT::Type SrcRegister> bool Instruction_CP_N();									// CP N
		inline bool Instruction_CP_HL_PTR();													// CP (HL)
		inline bool Instruction_CP_IMM();														// CP n

		// 8-bit ALU - INC
		inline Byte Instruction_INC(Byte val);
		template<RT::Type DstRegister> bool Instruction_INC_N();								// INC N
		inline bool Instruction_INC_HL_PTR();													// INC (HL)

		// 8-bit ALU - DEC
		inline Byte Instruction_DEC(Byte val);
		template<RT::Type DstRegister> bool Instruction_DEC_N();								// DEC N
		inline bool Instruction_DEC_HL_PTR();													// DEC (HL)

		// 16-bit ALU - ADD
		template<RT::Type SrcRegister> bool Instruction_ADD_HL_NN();							// ADD HL, NN
		inline bool Instruction_ADD_SP_IMM();													// ADD SP, n

		// 16-bit ALU - INC
		inline Word Instruction_INC_W(Word val);
		template<RT::Type DstRegister> bool Instruction_INC_NN();								// INC NN

		// 16-bit ALU - DEC
		inline Word Instruction_DEC_W(Word val);
		template<RT::Type DstRegister> bool Instruction_DEC_NN();								// DEC NN

		// Rotates & Shifts
		inline bool Instruction_RLCA();
		inline bool Instruction_RLA();
		inline bool Instruction_RRCA();
		inline bool Instruction_RRA();

		// Extended Instructions
		inline bool Instruction_EXT();

		// Extended - RLC
		inline Byte Instruction_EXT_RLC(Byte val);
		template<RT::Type Register> bool Instruction_EXT_RLC_N();								// RLC N
		inline bool Instruction_EXT_RLC_HL_ADDR();												// RLC (HL)

		// Extended - RRC
		inline Byte Instruction_EXT_RRC(Byte val);
		template<RT::Type Register> bool Instruction_EXT_RRC_N();								// RRC N
		inline bool Instruction_EXT_RRC_HL_ADDR();												// RRC (HL)

		// Extended - RL
		inline Byte Instruction_EXT_RL(Byte val);
		template<RT::Type Register> bool Instruction_EXT_RL_N();								// RL N
		inline bool Instruction_EXT_RL_HL_ADDR();												// RL (HL)

		// Extended - RR
		inline Byte Instruction_EXT_RR(Byte val);
		template<RT::Type Register> bool Instruction_EXT_RR_N();								// RR N
		inline bool Instruction_EXT_RR_HL_ADDR();												// RR (HL)

		// Extended - SLA
		inline Byte Instruction_EXT_SLA(Byte val);
		template<RT::Type Register> bool Instruction_EXT_SLA_N();								// SLA N
		inline bool Instruction_EXT_SLA_HL_ADDR();												// SLA (HL)

		// Extended - SRA
		inline Byte Instruction_EXT_SRA(Byte val);
		template<RT::Type Register> bool Instruction_EXT_SRA_N();								// SRA N
		inline bool Instruction_EXT_SRA_HL_ADDR();												// SRA (HL)

		// Extended - SWAP
		inline Byte Instruction_EXT_SWAP(Byte val);
		template<RT::Type DstRegister> bool Instruction_EXT_SWAP_N();							// SWAP N
		inline bool Instruction_EXT_SWAP_HL_PTR();												// SWAP (HL)

		// Extended - SRL
		inline Byte Instruction_EXT_SRL(Byte val);
		template<RT::Type Register> bool Instruction_EXT_SRL_N();								// SRL N
		inline bool Instruction_EXT_SRL_HL_ADDR();												// SRL (HL)

		// Extended - BIT
		template<Byte Bit> void Instruction_EXT_BIT(Byte val);
		template<Byte Bit, RT::Type Register> bool Instruction_EXT_BIT_B_N();					// BIT B,N
		template<Byte Bit> bool Instruction_EXT_BIT_B_HL_ADDR();								// BIT B,(HL)

		// Extended - RES
		template<Byte Bit> Byte Instruction_EXT_RESET(Byte val);
		template<Byte Bit, RT::Type Register> bool Instruction_EXT_RESET_B_N();					// RES B,N
		template<Byte Bit> bool Instruction_EXT_RESET_B_HL_ADDR();								// RES B,N

		// Extended - SET
		template<Byte Bit> Byte Instruction_EXT_SET(Byte val);
		template<Byte Bit, RT::Type Register> bool Instruction_EXT_SET_B_N();					// SET B,N
		template<Byte Bit> bool Instruction_EXT_SET_B_HL_ADDR();								// SET B,(HL)

		static const uint32_t kInstructionCount = 256;

		MMU*					m_mmu;
		Registers				m_registers;

		bool					m_bStopped;
		bool					m_bHalted;
		bool					m_bBreakpoint;
		bool					m_bBreakpointSkip;

		Byte					m_currentInstruction;
		Byte					m_currentExtendedInstruction;
		uint16_t				m_currentInstructionCycles;	// Normal or extended.

		Instruction				m_instructions[kInstructionCount];
		Instruction				m_instructionsExtended[kInstructionCount];
		std::vector<Address>	m_breakpoints;

		// @todo timers
	};

	#define GBHW_CPU_ACTION_PERFORMED	return true
	#define GBHW_CPU_ACTION_FAILED		return false

} // gbhw

#include "gbhw_cpu.inl"
