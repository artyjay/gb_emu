#pragma once

#include "types.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	struct RegisterType
	{
		enum Enum
		{
			A = 0,
			B,
			C,
			D,
			E,
			F,
			H,
			L,
			AF,
			BC,
			DE,
			HL,
			StackPointer,
			ProgramCounter,
			Count
		};

		static const char* to_string(Enum type);
	};

	struct RegisterTypeDecode
	{
		enum Enum
		{
			None,
			A,
			B,
			C,
			D,
			E,
			F,
			H,
			L,
			AF,
			BC,
			DE,
			HL,
			SP,
			PC,
			Imm8,
			SImm8,
			Imm16,
			Addr8,
			Addr16,
			AddrC,
			AddrBC,
			AddrDE,
			AddrHL,
			FlagZ,
			FlagNZ,
			FlagC,
			FlagNC,
			Zero,
			One,
			Two,
			Three,
			Four,
			Five,
			Six,
			Seven,
			Eight,
			Nine
		};
	};

	struct RegisterFlag
	{
		enum Enum
		{
			Carry		= (1 << 4),
			HalfCarry	= (1 << 5),
			Negative	= (1 << 6),
			Zero		= (1 << 7)
		};

		static const uint32_t kCount = 4;
		static const char* to_string(Enum flag);
	};

	using RegisterFlags = uint8_t;

	struct RegisterFlagBehaviour
	{
		enum Enum
		{
			Set = 0,
			Reset,
			Unmodified,
			PerFunction
		};
	};

	//--------------------------------------------------------------------------

	using RT	= RegisterType;
	using RTD	= RegisterTypeDecode;
	using RF	= RegisterFlag;
	using RFB	= RegisterFlagBehaviour;

	//--------------------------------------------------------------------------
	// Helper to prevent incorrect usage at compile time through static assert.

	template<RT::Enum T, RT::Enum... Rest>
	struct IsRegisterAny : std::false_type {};

	template<RT::Enum T, RT::Enum First>
	struct IsRegisterAny<T, First> : std::integral_constant<bool, T == First> {};

	template<RT::Enum T, RT::Enum First, RT::Enum... Rest>
	struct IsRegisterAny<T, First, Rest...>
		: std::integral_constant<bool, T == First || IsRegisterAny<T, Rest...>::value>
	{};

	//--------------------------------------------------------------------------

	class Registers
	{
	public:
		Registers();

		void reset();

		inline bool operator==(const Registers& other);

		template<RegisterType::Enum Register> inline Byte& get_register();
		template<RegisterType::Enum Register> inline void  set_register(Byte value);

		template<RegisterType::Enum Register> inline Word& get_register_word();
		template<RegisterType::Enum Register> inline void  set_register_word(Word value);

		template<RegisterFlag::Enum FlagType> inline void set_flag_if(bool enabled);
		inline void set_flag(RegisterFlag::Enum flag);
		inline bool is_flag_set(RegisterFlag::Enum flag) const;
		inline void clear_flag(RegisterFlag::Enum flag);
		inline void clear_flags(RegisterFlags flags);

		// Registers
		union { struct { Byte f; Byte a; }; Word af; };
		union { struct { Byte c; Byte b; }; Word bc; };
		union { struct { Byte e; Byte d; }; Word de; };
		union { struct { Byte l; Byte h; }; Word hl; };
		Word sp;
		Word pc;
		bool ime;

	private:
		std::array<Byte*, 8> m_byteLUT;
		std::array<Word*, 6> m_wordLUT;
	};

	//--------------------------------------------------------------------------

	inline bool Registers::operator==(const Registers &other)
	{
		if (this == &other)
			return true;

		return	(af == other.af) &&
				(bc == other.bc) &&
				(de == other.de) &&
				(hl == other.hl) &&
				(sp == other.sp) &&
				(pc == other.pc) &&
				(ime == other.ime);
	}

	template<RegisterType::Enum Register>
	inline Byte& Registers::get_register()
	{
		static_assert(IsRegisterAny<Register, RT::A, RT::B, RT::C, RT::D, RT::E, RT::F, RT::H, RT::L>::value,
			"Invalid register type, it is not a byte type");
		return *m_byteLUT[Register];
	}

	template<RegisterType::Enum Register>
	inline void Registers::set_register(Byte value)
	{
		static_assert(IsRegisterAny<Register, RT::A, RT::B, RT::C, RT::D, RT::E, RT::F, RT::H, RT::L>::value,
			"Invalid register type, it is not a byte type");
		*m_byteLUT[Register] = value;
	}

	template<RegisterType::Enum Register>
	inline Word& Registers::get_register_word()
	{
		static_assert(IsRegisterAny<Register, RT::AF, RT::BC, RT::DE, RT::HL, RT::StackPointer, RT::ProgramCounter>::value,
			"Invalid register type, it is not a word type");
		return *m_wordLUT[Register - RegisterType::AF];
	}

	template<RegisterType::Enum Register>
	inline void Registers::set_register_word(Word value)
	{
		static_assert(IsRegisterAny<Register, RT::AF, RT::BC, RT::DE, RT::HL, RT::StackPointer, RT::ProgramCounter>::value,
			"Invalid register type, it is not a word type");
		*m_wordLUT[Register - RegisterType::AF] = value;
	}

	template<RegisterFlag::Enum FlagType>
	inline void Registers::set_flag_if(bool enabled)
	{
		if (enabled)
		{
			set_flag(FlagType);
		}
		else
		{
			clear_flag(FlagType);
		}
	}

	inline void Registers::set_flag(RegisterFlag::Enum flag)
	{
		f |= static_cast<Byte>(flag);
	}

	inline bool Registers::is_flag_set(RegisterFlag::Enum flag) const
	{
		return ((f & static_cast<Byte>(flag)) != 0);
	}

	inline void Registers::clear_flag(RegisterFlag::Enum flag)
	{
		f &= ~static_cast<Byte>(flag);
	}

	inline void Registers::clear_flags(RegisterFlags flags)
	{
		f &= ~static_cast<Byte>(flags);
	}

	//--------------------------------------------------------------------------
}