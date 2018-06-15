#pragma once

#include "gbhw_cpu.h"

class MockCPU : public gbhw::CPU
{
public:
	MockCPU();
	virtual ~MockCPU();

	uint16_t ExecuteInstructions(const gbe::Byte* instructions, uint32_t instructionLength, uint32_t expectedExecutionCount);
	void ExpectFlags(bool bZero, bool bNegative, bool bHalf, bool bCarry);

	gbe::core::Registers& GetRegisters();
	gbe::Byte ReadByte(gbe::Address address);
	gbe::Word ReadWord(gbe::Address address);
	void WriteByte(gbe::Address address, gbe::Byte byte);
	void WriteWord(gbe::Address address, gbe::Word word);

	void StackPushWord_Mock(gbe::Word word);
	gbe::Word StackPopWord_Mock();
};
