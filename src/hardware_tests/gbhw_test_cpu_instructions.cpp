#include <gtest/gtest.h>

#include "gbe_test_cpu.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Misc
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TEST(CPU_MISC, NOP)
{
	const gbe::Byte kInstructions[] =
	{
		0x00	// NOP
	};

	// Load up registers starting values.
	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers copyregs = cpu.GetRegisters();
	gbe::core::Registers& regs = cpu.GetRegisters();
	copyregs.pc = copyregs.pc + sizeof(kInstructions);

	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(regs, regs);
}

TEST(CPU_MISC, DAA)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_MISC, CPL)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_MISC, CCF)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_MISC, SCF)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_MISC, DI)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_MISC, EI)
{
	std::cout << "TODO" << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Jumps
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_JUMPS, JP)
{
	// Jump to address nn
	const gbe::Byte kInstructions[] =
	{
		0xC3,	// JP $$
		0x10,
		0x20
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();

	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	gbe::Address kExpectedPC = 0x2010;
	EXPECT_EQ(kExpectedPC, regs.pc);
}

TEST(CPU_JUMPS, JP_CC)
{
	const gbe::Byte kInstructionsNZ[] =
	{
		0xC2,	// JP NZ $$
		0x10,
		0x20
	};

	const gbe::Byte kInstructionsZ[] =
	{
		0xCA,	// JP Z $$
		0x10,
		0x20
	};

	const gbe::Byte kInstructionsNC[] =
	{
		0xD2,	// JP NC $$
		0x10,
		0x20
	};

	const gbe::Byte kInstructionsC[] =
	{
		0xDA,	// JP C $$
		0x10,
		0x20
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Not Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_NZWhenZSet = regs.pc + sizeof(kInstructionsNZ);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NZWhenZSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_NZWhenZNotSet = 0x2010;
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NZWhenZNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_ZWhenZSet = 0x2010;
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_ZWhenZSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_ZWhenZNotSet = regs.pc + sizeof(kInstructionsZ);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_ZWhenZNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// No Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_NCWhenCSet = regs.pc + sizeof(kInstructionsNC);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NCWhenCSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_NCWhenCNotSet = 0x2010;
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NCWhenCNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_CWhenCSet = 0x2010;
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_CWhenCSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_CWhenCNotSet = regs.pc + sizeof(kInstructionsC);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_CWhenCNotSet, regs.pc);
}

TEST(CPU_JUMPS, JP_HL)
{
	// Jump to address HL
	const gbe::Byte kInstructions[] =
	{
		0xE9	// JP HL
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.hl = 0x2010;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	gbe::Address kExpectedPC = 0x2010;
	EXPECT_EQ(kExpectedPC, regs.pc);
}

TEST(CPU_JUMPS, JR)
{
	// Jump relative to address with signed imm.
	const gbe::Byte kInstructions[] =
	{
		0x18,	// JR $
		0x0F	// +15
	};

	const gbe::Byte kInstructions2[] =
	{
		0x18,	// JR $ (0x100)
		0x08,	// +8	(0x101)
		0x00,	//		(0x102)
		0x00,	//		(0x103)
		0x18,	// JR $	(0x104)
		0x7F,	// +127 (0x105)
		0x00,	//		(0x106)
		0x00,	//		(0x107)
		0x00,	//		(0x108)
		0x00,	//		(0x109)
		0x18,	// JR $	(0x10A)
		0xF8	// -8	(0x10B)
				//		(0x10C)
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();
	gbe::Address kExpectedPC = regs.pc + sizeof(kInstructions) + 15;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);

	// Reset for negative test...
	regs.pc = 0x100;
	gbe::Address kExpectedPC2 = 0x185;
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 3);
	EXPECT_EQ(kExpectedPC2, regs.pc);
}

TEST(CPU_JUMPS, JR_CC)
{
	const gbe::Byte kInstructionsNZ[] =
	{
		0x20,	// JR NZ $
		0x0F	// +15
	};

	const gbe::Byte kInstructionsZ[] =
	{
		0x28,	// JR Z $
		0x0F	// +15
	};

	const gbe::Byte kInstructionsNC[] =
	{
		0x30,	// JR NC $
		0x0F	// +15
	};

	const gbe::Byte kInstructionsC[] =
	{
		0x38,	// JR C $
		0x0F	// +15
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Not Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_NZWhenZSet = regs.pc + sizeof(kInstructionsNZ);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NZWhenZSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_NZWhenZNotSet = regs.pc + sizeof(kInstructionsNZ) + 15;
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NZWhenZNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_ZWhenZSet = regs.pc + sizeof(kInstructionsZ) + 15;
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_ZWhenZSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	gbe::Address kExpectedPC_ZWhenZNotSet = regs.pc + sizeof(kInstructionsZ);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_ZWhenZNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// No Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_NCWhenCSet = regs.pc + sizeof(kInstructionsNC);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NCWhenCSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_NCWhenCNotSet = regs.pc + sizeof(kInstructionsNC) + 15;
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NCWhenCNotSet, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_CWhenCSet = regs.pc + sizeof(kInstructionsC) + 15;
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_CWhenCSet, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	gbe::Address kExpectedPC_CWhenCNotSet = regs.pc + sizeof(kInstructionsC);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_CWhenCNotSet, regs.pc);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Calls
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_CALLS, CALL)
{
	// Store address of next instruction onto stack and then jump to address $$.
	const gbe::Byte kInstructions[] =
	{
		0xCD,	// Call to 0x104		(0x100)
		0x04,	//						(0x101)
		0x01	//						(0x102)
				//						(0x103)
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();
	gbe::Address kExpectedPC		= 0x104;
	gbe::Address kExpectedStack		= 0x103;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_EQ(kExpectedStack, cpu.StackPopWord_Mock());
}

TEST(CPU_CALLS, CALL_CC)
{
	// Store address of next instruction onto stack and then jump to address $$.
	const gbe::Byte kInstructionsNZ[] =
	{
		0xC4,	// Call to 0x104		(0x100)
		0x04,	//						(0x101)
		0x01	//						(0x102)
				//						(0x103)
	};

	const gbe::Byte kInstructionsZ[] =
	{
		0xCC,	// Call to 0x104		(0x100)
		0x04,	//						(0x101)
		0x01	//						(0x102)
				//						(0x103)
	};

	const gbe::Byte kInstructionsNC[] =
	{
		0xD4,	// Call to 0x104		(0x100)
		0x04,	//						(0x101)
		0x01	//						(0x102)
				//						(0x103)
	};

	const gbe::Byte kInstructionsC[] =
	{
		0xDC,	// Call to 0x104		(0x100)
		0x04,	//						(0x101)
		0x01	//						(0x102)
				//						(0x103)
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();
	gbe::Address kExpectedPC = 0x104;
	gbe::Address kExpectedPC_NoCall = 0x103;
	gbe::Address kExpectedStack = 0x103;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Not Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_EQ(kExpectedStack, cpu.StackPopWord_Mock());

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_EQ(kExpectedStack, cpu.StackPopWord_Mock());

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// No Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_EQ(kExpectedStack, cpu.StackPopWord_Mock());

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_EQ(kExpectedStack, cpu.StackPopWord_Mock());

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Restarts
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_RESTARTS, RST)
{
	const gbe::Byte kInstructions00[] =
	{
		0xC7	// RST00
	};

	const gbe::Byte kInstructions08[] =
	{
		0xCF	// RST08
	};

	const gbe::Byte kInstructions10[] =
	{
		0xD7	// RST10
	};

	const gbe::Byte kInstructions18[] =
	{
		0xDF	// RST18
	};

	const gbe::Byte kInstructions20[] =
	{
		0xE7	// RST20
	};

	const gbe::Byte kInstructions28[] =
	{
		0xEF	// RST28
	};

	const gbe::Byte kInstructions30[] =
	{
		0xF7	// RST30
	};

	const gbe::Byte kInstructions38[] =
	{
		0xFF	// RST38
	};

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	gbe::Address kExpectedPC00 = 0x00;
	gbe::Address kExpectedPC08 = 0x08;
	gbe::Address kExpectedPC10 = 0x10;
	gbe::Address kExpectedPC18 = 0x18;
	gbe::Address kExpectedPC20 = 0x20;
	gbe::Address kExpectedPC28 = 0x28;
	gbe::Address kExpectedPC30 = 0x30;
	gbe::Address kExpectedPC38 = 0x38;

	cpu.ExecuteInstructions(kInstructions00, sizeof(kInstructions00), 1);
	EXPECT_EQ(kExpectedPC00, regs.pc);
	EXPECT_EQ(0x101, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions08, sizeof(kInstructions08), 1);
	EXPECT_EQ(kExpectedPC08, regs.pc);
	EXPECT_EQ(0x01, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions18, sizeof(kInstructions18), 1);
	EXPECT_EQ(kExpectedPC18, regs.pc);
	EXPECT_EQ(0x09, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions20, sizeof(kInstructions20), 1);
	EXPECT_EQ(kExpectedPC20, regs.pc);
	EXPECT_EQ(0x19, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions28, sizeof(kInstructions28), 1);
	EXPECT_EQ(kExpectedPC28, regs.pc);
	EXPECT_EQ(0x21, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions30, sizeof(kInstructions30), 1);
	EXPECT_EQ(kExpectedPC30, regs.pc);
	EXPECT_EQ(0x29, cpu.StackPopWord_Mock());

	cpu.ExecuteInstructions(kInstructions38, sizeof(kInstructions38), 1);
	EXPECT_EQ(kExpectedPC38, regs.pc);
	EXPECT_EQ(0x31, cpu.StackPopWord_Mock());
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Returns
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_RETURNS, RET)
{
	const gbe::Byte kInstructions[] =
	{
		0xC9
	};

	MockCPU cpu;

	gbe::core::Registers& regs = cpu.GetRegisters();
	cpu.StackPushWord_Mock(0x110);
	gbe::Address kExpectedPC = 0x110;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
}

TEST(CPU_RETURNS, RET_CC)
{
	// Store address of next instruction onto stack and then jump to address $$.
	const gbe::Byte kInstructionsNZ[] =
	{
		0xC0
	};

	const gbe::Byte kInstructionsZ[] =
	{
		0xC8
	};

	const gbe::Byte kInstructionsNC[] =
	{
		0xD0
	};

	const gbe::Byte kInstructionsC[] =
	{
		0xD8
	};

	MockCPU cpu;

	// Copy regs to ensure nothing changes (except PC).
	gbe::core::Registers& regs = cpu.GetRegisters();

	gbe::Address kExpectedPC = 0x110;
	gbe::Address kExpectedPC_NoCall = 0x101;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Not Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	cpu.StackPushWord_Mock(0x110);
	cpu.ExecuteInstructions(kInstructionsNZ, sizeof(kInstructionsNZ), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Zero flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Zero);
	cpu.StackPushWord_Mock(0x110);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Zero);
	cpu.ExecuteInstructions(kInstructionsZ, sizeof(kInstructionsZ), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// No Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	cpu.StackPushWord_Mock(0x110);
	cpu.ExecuteInstructions(kInstructionsNC, sizeof(kInstructionsNC), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Carry flag test.
	regs.pc = 0x100;
	regs.SetFlag(gbe::core::RF::Carry);
	cpu.StackPushWord_Mock(0x110);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);

	// Reset
	regs.pc = 0x100;
	regs.ClearFlag(gbe::core::RF::Carry);
	cpu.ExecuteInstructions(kInstructionsC, sizeof(kInstructionsC), 1);
	EXPECT_EQ(kExpectedPC_NoCall, regs.pc);
}

TEST(CPU_RETURNS, RETI)
{
	const gbe::Byte kInstructions[] =
	{
		0xD9
	};

	MockCPU cpu;

	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.ime = false;
	cpu.StackPushWord_Mock(0x110);
	gbe::Address kExpectedPC = 0x110;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedPC, regs.pc);
	EXPECT_TRUE(regs.ime);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 8-bit Loads
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_8_bit_LOAD, N_to_N)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_LOAD, NNAddr_to_N)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_LOAD, N_to_NN_Addr)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_LOAD, Imm)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_LOAD, JustA)
{
	// All the instructions that read/write A or addr of A.
	std::cout << "TODO" << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 16- Loads
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_16_bit_LOAD, Imm)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_LOAD, HL_To_SP)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_LOAD, SP_To_ImmAddr)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_LOAD, PUSH_Imm)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_LOAD, POP_Imm)
{
	std::cout << "TODO" << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 8-bit ALU
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_8_bit_ALU, ADD)
{
	// Add A, B, C, D, E, H, L, $ to A
	const gbe::Byte kInstructions[] = 
	{
		0x87,	// Add A,A
		0x80,	// Add A,B
		0x81,	// Add A,C
		0x82,	// Add A,D
		0x83,	// Add A,E
		0x84,	// Add A,H
		0x85,	// Add A,L
		0xC6,	// Add A,$
		0x09	// Immediate value
	};

	// Add (HL) to A
	const gbe::Byte kInstructions2[] =
	{
		0x86	// Add A,(HL)
	};

	// Load up registers starting values.
	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.a = 1;
	regs.b = 2;
	regs.c = 3;
	regs.d = 4;
	regs.e = 5;
	regs.h = 6;
	regs.l = 7;
	cpu.WriteByte(0xC000, 8);

	gbe::Byte kExpectedA = 38;
	gbe::Byte kExpectedA2 = 46;

	// Run first set of instructions.
	gbe::Address expectedPC = regs.pc + 10;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 9);
	EXPECT_EQ(kExpectedA, regs.a);
	EXPECT_EQ(expectedPC, regs.pc);

	// Run second set of instructions.
	regs.hl = 0xC000;
	gbe::Address expectedPC2 = regs.pc + sizeof(kInstructions2);
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(kExpectedA2, regs.a);
	EXPECT_EQ(expectedPC2, regs.pc);
}

TEST(CPU_8_bit_ALU, ADD_Flags)
{
	const gbe::Byte kInstructions[] =
	{
		0x80	// Add A,B
	};

	const gbe::Byte kInstructions2[] =
	{
		0x81	// Add A,C
	};

	const gbe::Byte kInstructions3[] =
	{
		0x87	// Add A,A
	};

	const gbe::Byte kExpectedA = 16;
	const gbe::Byte kExpectedA2 = 14;
	const gbe::Byte kExpectedA3 = 0;

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.a = 15;
	regs.b = 1;
	regs.c = 254;

	// Should reset negative.
	regs.SetFlag(gbe::core::RF::Negative);

	// Run first set of instructions.
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedA, regs.a);
	cpu.ExpectFlags(false, false, true, false);

	// Run second set of instructions.
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(kExpectedA2, regs.a);
	cpu.ExpectFlags(false, false, false, true);

	// Run third set of instructions.
	regs.a = 0;
	cpu.ExecuteInstructions(kInstructions3, sizeof(kInstructions3), 1);
	EXPECT_EQ(kExpectedA3, regs.a);
	cpu.ExpectFlags(true, false, false, false);
}

TEST(CPU_8_bit_ALU, ADC)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_ALU, SUB)
{
	// Sub A, B, C, D, E, H, L, $ from A
	const gbe::Byte kInstructions[] =
	{
		0x90,	// Sub A,B
		0x91,	// Sub A,C
		0x92,	// Sub A,D
		0x93,	// Sub A,E
		0x94,	// Sub A,H
		0x95,	// Sub A,L
		0xD6,	// Sub A,$
		0x08	// Immediate value
	};

	// Sub (HL) from A
	const gbe::Byte kInstructions2[] =
	{
		0x96	// Sub A,(HL)
	};

	const gbe::Byte kInstructions3[] =
	{
		0x97	// Sub A,A
	};

	// Load up registers starting values.
	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.a = 250;
	regs.b = 1;
	regs.c = 2;
	regs.d = 3;
	regs.e = 4;
	regs.h = 5;
	regs.l = 6;
	cpu.WriteByte(0xC000, 7);

	gbe::Byte kExpectedA = 221;
	gbe::Byte kExpectedA2 = 214;
	gbe::Byte kExpectedA3 = 0;

	// Run first set of instructions.
	gbe::Address expectedPC = regs.pc + 9;
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 8);
	EXPECT_EQ(kExpectedA, regs.a);
	EXPECT_EQ(expectedPC, regs.pc);

	// Run second set of instructions.
	regs.hl = 0xC000;
	gbe::Address expectedPC2 = regs.pc + sizeof(kInstructions2);
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(kExpectedA2, regs.a);
	EXPECT_EQ(expectedPC2, regs.pc);

	// Run third set of instructions.
	gbe::Address expectedPC3 = regs.pc + sizeof(kInstructions3);
	cpu.ExecuteInstructions(kInstructions3, sizeof(kInstructions3), 1);
	EXPECT_EQ(kExpectedA3, regs.a);
	EXPECT_EQ(expectedPC3, regs.pc);
}

TEST(CPU_8_bit_ALU, SUB_Flags)
{
	const gbe::Byte kInstructions[] =
	{
		0x90	// Sub A,B
	};

	const gbe::Byte kInstructions2[] =
	{
		0x91	// Sub A,C
	};

	const gbe::Byte kInstructions3[] =
	{
		0x97	// Sub A,A
	};

	const gbe::Byte kExpectedA = 15;
	const gbe::Byte kExpectedA2 = 142;
	const gbe::Byte kExpectedA3 = 0;

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.a = 16;
	regs.b = 1;
	regs.c = 128;

	// Run first set of instructions. 16-1 == half-carry is set.
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedA, regs.a);
	cpu.ExpectFlags(false, true, true, false);

	// Run first set again, 15-1 == half-carry is not set.
	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(kExpectedA - 1, regs.a);
	cpu.ExpectFlags(false, true, false, false);

	// Run second set of instructions.
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(kExpectedA2, regs.a);
	cpu.ExpectFlags(false, true, false, true);

	// Run third set of instructions.
	cpu.ExecuteInstructions(kInstructions3, sizeof(kInstructions3), 1);
	EXPECT_EQ(kExpectedA3, regs.a);
	cpu.ExpectFlags(true, true, false, false);
}

TEST(CPU_8_bit_ALU, SBC)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_ALU, AND)
{
	// @todo: Test all variants.

	const gbe::Byte kInstructions[] =
	{
		0xA0	// AND A,B
	};

	const gbe::Byte kInstructions2[] =
	{
		0xA1	// AND A,C
	};

	const gbe::Byte kExpectedA = 5;
	const gbe::Byte kExpectedA2 = 0;

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.SetFlag(gbe::core::RF::Zero);		// Expected to clear
	regs.SetFlag(gbe::core::RF::Negative);	// Expected to reset
	regs.SetFlag(gbe::core::RF::Carry);		// Expected to reset
	regs.a = 117;
	regs.b = 15;
	regs.c = 240;

	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(regs.a, kExpectedA);
	cpu.ExpectFlags(false, false, true, false);

	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(regs.a, kExpectedA2);
	cpu.ExpectFlags(true, false, true, false);
}

TEST(CPU_8_bit_ALU, OR)
{
	// @todo: Test all variants.

	const gbe::Byte kInstructions[] =
	{
		0xB0	// OR A,B
	};

	const gbe::Byte kInstructions2[] =
	{
		0xB7	// OR A,A
	};

	const gbe::Byte kExpectedA = 15;
	const gbe::Byte kExpectedA2 = 0;

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.SetFlag(gbe::core::RF::Zero);			// Expected to clear
	regs.SetFlag(gbe::core::RF::Negative);		// Expected to reset
	regs.SetFlag(gbe::core::RF::HalfCarry);		// Expected to reset
	regs.SetFlag(gbe::core::RF::Carry);			// Expected to reset
	regs.a = 5;
	regs.b = 10;

	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(regs.a, kExpectedA);
	cpu.ExpectFlags(false, false, false, false);

	regs.a = 0;
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(regs.a, kExpectedA2);
	cpu.ExpectFlags(true, false, false, false);
}

TEST(CPU_8_bit_ALU, XOR)
{
	// @todo: Test all variants.

	const gbe::Byte kInstructions[] =
	{
		0xA8	// XOR A,B
	};

	const gbe::Byte kInstructions2[] =
	{
		0xAF	// XOR A,A
	};

	const gbe::Byte kExpectedA = 2;
	const gbe::Byte kExpectedA2 = 0;

	MockCPU cpu;
	gbe::core::Registers& regs = cpu.GetRegisters();
	regs.SetFlag(gbe::core::RF::Zero);			// Expected to clear
	regs.SetFlag(gbe::core::RF::Negative);		// Expected to reset
	regs.SetFlag(gbe::core::RF::HalfCarry);		// Expected to reset
	regs.SetFlag(gbe::core::RF::Carry);			// Expected to reset
	regs.a = 10;
	regs.b = 8;

	cpu.ExecuteInstructions(kInstructions, sizeof(kInstructions), 1);
	EXPECT_EQ(regs.a, kExpectedA);
	cpu.ExpectFlags(false, false, false, false);

	regs.a = 15;
	cpu.ExecuteInstructions(kInstructions2, sizeof(kInstructions2), 1);
	EXPECT_EQ(regs.a, kExpectedA2);
	cpu.ExpectFlags(true, false, false, false);
}

TEST(CPU_8_bit_ALU, CP)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_ALU, INC)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_8_bit_ALU, DEC)
{
	std::cout << "TODO" << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 16-bit ALU
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST(CPU_16_bit_ALU, ADD)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_ALU, INC)
{
	std::cout << "TODO" << std::endl;
}

TEST(CPU_16_bit_ALU, DEC)
{
	std::cout << "TODO" << std::endl;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Extended.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// @todo