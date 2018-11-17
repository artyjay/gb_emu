#pragma once

#include "context.h"

namespace gbhw
{
	class Rom
	{
	public:
		Rom();
		~Rom();

		void Reset();
		void Load(uint8_t* romData, uint32_t romLength);

		uint8_t* GetBank(Byte bankIndex);
		CartridgeType::Type GetCartridgeType() const;

		static const uint32_t kBankSize = 16384;

	private:
		void LoadHeader();
		void LoadBanks();

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