#include "registers.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	namespace
	{
		static const char* kFlagsNameTable[] =
		{
			"Carry",
			"HalfCarry",
			"Negative",
			"Zero"
		};

		static const char* kRegisterTypeNameTable[] =
		{
			"A",
			"B",
			"C",
			"D",
			"E",
			"F",
			"L",
			"H",
			"AF",
			"BC",
			"DE",
			"HL",
			"StackPointer",
			"ProgramCounter"
		};
	}

	const char* RegisterType::to_string(Enum reg)
	{
		if (reg < Count)
		{
			return kRegisterTypeNameTable[static_cast<uint32_t>(reg)];
		}

		return "RegisterType - Error";
	}

	const char* RegisterFlag::to_string(Enum flag)
	{
		switch(flag)
		{
			case Carry:			return kFlagsNameTable[0];
			case HalfCarry:		return kFlagsNameTable[1];
			case Negative:		return kFlagsNameTable[2];
			case Zero:			return kFlagsNameTable[3];
			default: break;
		}

		return "RegisterFlags - Error";
	}

	//--------------------------------------------------------------------------

	Registers::Registers()
		: m_byteLUT { &a, &b, &c, &d, &e, &f, &h, &l }
		, m_wordLUT { &af, &bc, &de, &hl, &sp, &pc }
	{
		reset();
	}

	void Registers::reset()
	{
		af = 0x01B0; // @todo: GBP = 0xFFB0, GBC = 0x11B0
		bc = 0x0013;
		de = 0x00D8;
		hl = 0x014D;
		sp = 0xFFFE;
		pc = 0x0100;
		ime = false;
	}

	//--------------------------------------------------------------------------
}