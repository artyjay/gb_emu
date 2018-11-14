#pragma once

#include "gbhw_types.h"

// Defines 2 registers, one for each name, plus a union pairing them off.
#define GBHW_MAKE_REGISTER_PAIR(NAMELOW, NAMEHIGH)	\
struct												\
{													\
	union											\
	{												\
		struct										\
		{											\
			Byte NAMELOW;							\
			Byte NAMEHIGH;							\
		};											\
		Word NAMEHIGH##NAMELOW;						\
	};												\
};

namespace gbhw
{
	class Registers
	{
	public:
		struct RegisterType
		{
			enum Type
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

			static const char* ToStr(Type registerType);
		};

		struct RegisterTypeDecode
		{
			enum Type
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

		struct Flags
		{
			enum Type
			{
				Carry		= (1 << 4),
				HalfCarry	= (1 << 5),
				Negative	= (1 << 6),
				Zero		= (1 << 7)
			};

			static const uint32_t kCount = 4;

			static const char* ToStr(Type flag);
		};

		struct FlagBehaviour
		{
			enum Type
			{
				Set,
				Reset,
				Unmodified,
				PerFunction
			};
		};


		Registers();

		// Helpers
		void Reset();

		inline bool IsFlagSet(Flags::Type flag) const;
		inline void SetFlag(Flags::Type flag);
		inline void ClearFlag(Flags::Type flag);
		inline void ClearFlags(uint8_t flags);

		template<Flags::Type FlagType> inline void SetFlagIf(bool enabled);
		template<RegisterType::Type Register> inline Byte& GetRegisterByte();
		template<RegisterType::Type Register> inline const Byte& GetRegisterByte() const;
		template<RegisterType::Type Register> inline Word& GetRegisterWord();
		template<RegisterType::Type Register> inline const Word& GetRegisterWord() const;

		// Registers
		GBHW_MAKE_REGISTER_PAIR(f, a)
		GBHW_MAKE_REGISTER_PAIR(c, b)
		GBHW_MAKE_REGISTER_PAIR(e, d)
		GBHW_MAKE_REGISTER_PAIR(l, h)
		Word sp;
		Word pc;
		bool ime;

	private:
		Byte* m_byteLUT[8];
		Word* m_wordLUT[6];
	};

	inline bool operator==(const Registers& lhs, const Registers& rhs)
	{
		if (&lhs == &rhs)
		{
			return true;
		}

		return	(lhs.af == rhs.af) &&
				(lhs.bc == rhs.bc) &&
				(lhs.de == rhs.de) &&
				(lhs.hl == rhs.hl) &&
				(lhs.sp == rhs.sp) &&
				(lhs.pc == rhs.pc) &&
				(lhs.ime == rhs.ime);
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Convenience shorthand
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	using RT	= Registers::RegisterType;
	using RTD	= Registers::RegisterTypeDecode;
	using RF	= Registers::Flags;
	using RFB	= Registers::FlagBehaviour;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Helper to prevent incorrect usage at compile time through static assertions.
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	template<RT::Type T, RT::Type... Rest>
	struct IsFirstRegisterAny : std::false_type {};

	template<RT::Type T, RT::Type First>
	struct IsFirstRegisterAny<T, First> : std::integral_constant<bool, T == First> {};

	template<RT::Type T, RT::Type First, RT::Type... Rest>
	struct IsFirstRegisterAny<T, First, Rest...>
		: std::integral_constant<bool, T == First || IsFirstRegisterAny<T, Rest...>::value>
	{};

} // gbhw

#include "registers.inl"

#undef GBHW_MAKE_REGISTER_PAIR