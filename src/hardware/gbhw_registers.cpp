#include "gbhw_registers.h"

namespace gbhw
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Helpers and types
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

	const char * Registers::Flags::ToStr(Type registerType)
	{
		switch(registerType)
		{
			case Carry:
			{
				return kFlagsNameTable[0];
			}
			case HalfCarry:
			{
				return kFlagsNameTable[1];
			}
			case Negative:
			{
				return kFlagsNameTable[2];
			}
			case Zero:
			{
				return kFlagsNameTable[3];
			}
		}
		
		return "ERROR-FLAG";
	}

	const char * Registers::RegisterType::ToStr(Type flag)
	{
		if(flag < Count)
		{
			return kRegisterTypeNameTable[static_cast<uint32_t>(flag)];
		}

		return "ERROR-REG-TYPE";
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Registers implementation
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Registers::Registers()
		: m_byteLUT { &a, &b, &c, &d, &e, &f, &h, &l }
		, m_wordLUT { &af, &bc, &de, &hl, &sp, &pc }
	{
		Reset();
	}

	void Registers::Reset()
	{
		// @todo correct this!
		af = 0x01B0; // GBP = 0xFFB0, GBC = 0x11B0
		bc = 0x0013;
		de = 0x00D8;
		hl = 0x014D;
		sp = 0xFFFE;
		pc = 0x0100;

		// @todo mmu

		// @todo: stop/halt.
		ime = false;


// 		this->Set_AF(0x01B0);
// 		this->Set_BC(0x0013);
// 		this->Set_DE(0x00D8);
// 		this->Set_HL(0x014D);
// 		this->Set_PC(0x0100);
// 		this->Set_SP(0xFFFE);
// 		this->Set_Halt(false);
// 		this->Set_Stop(false);
// 		this->Set_IME(false);
	}

} // gbhw