#pragma once

#include "mmu.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	class Rom
	{
	public:
		Rom();
		~Rom();

		bool load(const uint8_t* data, uint32_t length);

		uint8_t* get_bank(Byte bankIndex);
		CartridgeType::Type get_cartridge_type() const;

	private:
		void reset();
		void load_header();
		void load_banks();

		Buffer						m_romData;
		std::string					m_title;
		CartridgeType::Type			m_cartridgeType;
		HardwareType::Type			m_hardwareType;
		RomSize::Type				m_romSize;
		RamSize::Type				m_ramSize;
		DestinationCode::Type		m_destinationCode;
		LicenseeCodeOld::Type		m_licenseeCodeOld;
		MemoryBanks					m_banks;
	};

	//--------------------------------------------------------------------------
}