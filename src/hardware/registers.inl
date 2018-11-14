#pragma once

#include <stdexcept>

namespace gbhw
{

	inline bool Registers::IsFlagSet(Flags::Type flag) const
	{
		return ((f & static_cast<Byte>(flag)) != 0);
	}

	inline void Registers::SetFlag(Flags::Type flag)
	{
		f |= static_cast<Byte>(flag);
	}

	inline void Registers::ClearFlag(Flags::Type flag)
	{
		f &= ~static_cast<Byte>(flag);
	}

	inline void Registers::ClearFlags(uint8_t flags)
	{
		f &= ~static_cast<Byte>(flags);
	}

	template<Registers::Flags::Type FlagType>
	inline void Registers::SetFlagIf(bool enabled)
	{
		if(enabled)
		{
			SetFlag(FlagType);
		}
		else
		{
			ClearFlag(FlagType);
		}
	}

	template<Registers::RegisterType::Type Register>
	inline Byte& Registers::GetRegisterByte()
	{
		// ImmByte should not be used
		static_assert(IsFirstRegisterAny<Register, RT::A, RT::B, RT::C, RT::D, RT::E, RT::F, RT::H, RT::L>::value, "Invalid register type, it is not a byte type");
		return *(m_byteLUT[Register]);
	}

	template<Registers::RegisterType::Type Register>
	inline const Byte& Registers::GetRegisterByte() const
	{
		// ImmByte should not be used
		static_assert(IsFirstRegisterAny<Register, RT::A, RT::B, RT::C, RT::D, RT::E, RT::F, RT::H, RT::L>::value, "Invalid register type, it is not a byte type");
		return *(m_byteLUT[Register]);
	}

	template<Registers::RegisterType::Type Register>
	inline Word& Registers::GetRegisterWord()
	{
		// ImmWord should not be used
		static_assert(IsFirstRegisterAny<Register, RT::AF, RT::BC, RT::DE, RT::HL, RT::StackPointer, RT::ProgramCounter>::value, "Invalid register type, it is not a word type");
		return *(m_wordLUT[Register - RT::AF]);
	}

	template<Registers::RegisterType::Type Register>
	inline const Word& Registers::GetRegisterWord() const
	{
		// ImmWord should not be used
		static_assert(IsFirstRegisterAny<Register, RT::AF, RT::BC, RT::DE, RT::HL, RT::StackPointer, RT::ProgramCounter>::value, "Invalid register type, it is not a word type");
		return *(m_wordLUT[Register - RT::AF]);
	}

} // gbhw