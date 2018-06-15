#pragma once

#include "gbhw_cpu.h"
#include "gbhw_types.h"

#include <string>
#include <vector>

namespace gbhw
{
	class CPU;
	class MMU;

	class Rom
	{
	public:
		Rom();
		~Rom();

		void Initialise(CPU* cpu, MMU* mmu);

		void Reset();
		void Load(uint8_t* romData, uint32_t romLength);
		void GetInstructions(const Address& address, InstructionList& instructions, int32_t instructionCount);
		uint8_t* GetBank(Byte bankIndex);
		CartridgeType::Type GetCartridgeType() const;

		static const uint32_t kBankSize = 16384;

	private:
		void LoadHeader();
		void LoadBanks();

		void GenerateInstructions(Address address, InstructionList& instructions, const uint8_t* instructionAddress, int32_t instructionCount);

		CPU*						m_cpu;
		MMU*						m_mmu;
		std::vector<uint8_t>		m_romData;
		std::string					m_title;
		CartridgeType::Type			m_cartridgeType;
		HardwareType::Type			m_hardwareType;
		RomSize::Type				m_romSize;
		RamSize::Type				m_ramSize;
		DestinationCode::Type		m_destinationCode;
		LicenseeCodeOld::Type		m_licenseeCodeOld;
		std::vector<MemoryBank>		m_banks;
	};
}