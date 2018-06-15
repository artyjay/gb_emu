#include "gbe_test_cpu.h"

#include <gtest/gtest.h>

MockCPU::MockCPU()
{

}

MockCPU::~MockCPU()
{

}

uint16_t MockCPU::ExecuteInstructions(const gbe::Byte* instructions, uint32_t instructionLength, uint32_t expectedExecutionCount)
{
	uint16_t cycles = 0;

	// Load instructions into PC.
	gbe::Word pc = m_registers.pc;
	uint32_t instructionsExecuted = 0;

	for (uint32_t i = 0; i < instructionLength; i++)
	{
		m_mmu.WriteByte(pc++, instructions[i]);
	}

	while (instructionsExecuted < expectedExecutionCount)
	{
		// Run the next instruction.
		m_currentInstruction = ImmediateByte();

		gbe::core::Instruction& instruction = m_instructions[m_currentInstruction];
		gbe::core::InstructionFunction& func = instruction.GetFunction();
		bool bActionPerformed = (this->*func)();
		gbe::Byte instCycles = instruction.GetCycles(bActionPerformed);
		assert(instCycles != 0);
		m_currentInstructionCycles += instCycles;
		instructionsExecuted++;

		// Update cycles
		cycles += m_currentInstructionCycles;
		m_currentInstructionCycles = 0;
	}

	EXPECT_EQ(expectedExecutionCount, instructionsExecuted);

	return cycles;
}

void MockCPU::ExpectFlags(bool bZero, bool bNegative, bool bHalf, bool bCarry)
{
	if(bZero)
	{
		EXPECT_TRUE(m_registers.IsFlagSet(gbe::core::RF::Zero));
	}
	else
	{
		EXPECT_FALSE(m_registers.IsFlagSet(gbe::core::RF::Zero));
	}

	if (bNegative)
	{
		EXPECT_TRUE(m_registers.IsFlagSet(gbe::core::RF::Negative));
	}
	else
	{
		EXPECT_FALSE(m_registers.IsFlagSet(gbe::core::RF::Negative));
	}

	if (bHalf)
	{
		EXPECT_TRUE(m_registers.IsFlagSet(gbe::core::RF::HalfCarry));
	}
	else
	{
		EXPECT_FALSE(m_registers.IsFlagSet(gbe::core::RF::HalfCarry));
	}

	if (bCarry)
	{
		EXPECT_TRUE(m_registers.IsFlagSet(gbe::core::RF::Carry));
	}
	else
	{
		EXPECT_FALSE(m_registers.IsFlagSet(gbe::core::RF::Carry));
	}
}

gbe::core::Registers& MockCPU::GetRegisters()
{
	return m_registers;
}

gbe::Byte MockCPU::ReadByte(gbe::Address address)
{
	return m_mmu.ReadByte(address);
}

gbe::Word MockCPU::ReadWord(gbe::Address address)
{
	return m_mmu.ReadWord(address);
}

void MockCPU::WriteByte(gbe::Address address, gbe::Byte byte)
{
	m_mmu.WriteByte(address, byte);
}

void MockCPU::WriteWord(gbe::Address address, gbe::Word word)
{
	m_mmu.WriteWord(address, word);
}

void MockCPU::StackPushWord_Mock(gbe::Word word)
{
	StackPushWord(word);
}

gbe::Word MockCPU::StackPopWord_Mock()
{
	return StackPopWord();
}