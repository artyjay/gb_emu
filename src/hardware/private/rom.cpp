#include "rom.h"
#include "log.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	namespace
	{
		static const uint32_t kTitleOffset						= 0x134;
		static const uint32_t kTitleLength						= 16;
		static const uint32_t kHWTypeOffset						= 0x143;
		static const uint32_t kLicenseeCodeHighNibbleOffset		= 0x144;
		static const uint32_t kLicenseeCodeLowNibbleOffset		= 0x145;
		static const uint32_t kSGBIndicatorOffset				= 0x146;
		static const uint32_t kCartridgeTypeOffset				= 0x147;
		static const uint32_t kRomSizeOffset					= 0x148;
		static const uint32_t kRamSizeOffset					= 0x149;
		static const uint32_t kDestinationCodeOffset			= 0x14A;
		static const uint32_t kLicenseeCodeOldOffset			= 0x14B;
		static const uint32_t kMaskRomVersionNumberOffset		= 0x14C;
		static const uint32_t kComplementCheckOffset			= 0x14D;
		static const uint32_t kCheckSumOffset					= 0x14E;
		static const uint32_t kCheckSumLength					= 2;
		static const uint32_t kBankSize							= 16384;
	}

	//--------------------------------------------------------------------------

	Rom::Rom()
	{
		reset();
	}

	Rom::~Rom()
	{
	}

	void Rom::reset()
	{
		m_romData.resize(0);
		m_title				= "";
		m_cartridgeType		= CartridgeType::Unknown;
		m_hardwareType		= HardwareType::Unknown;
		m_romSize			= RomSize::Unknown;
		m_ramSize			= RamSize::Unknown;
		m_destinationCode	= DestinationCode::Unknown;
		m_licenseeCodeOld	= LicenseeCodeOld::Unknown;
		m_banks.resize(0);
	}

	bool Rom::load(const uint8_t* data, uint32_t length)
	{
		reset();

		if(length == 0)
			return false;

		// Copy ROM into memory.
		m_romData = Buffer(&data[0], &data[length]);

		// Perform actual load.
		load_header();
		load_banks();

		return true;
	}

	uint8_t* Rom::get_bank(Byte bankIndex)
	{
		if (bankIndex < m_banks.size())
		{
			return m_banks[bankIndex].m_memory;
		}

		log_error("Failed to obtain rom bank, index out of range\n");
		return nullptr;
	}

	CartridgeType::Type Rom::get_cartridge_type() const
	{
		return m_cartridgeType;
	}

	void Rom::load_header()
	{
		// parse the rom header..
		m_title				= std::string(m_romData.begin() + kTitleOffset, m_romData.begin() + kTitleOffset + kTitleLength);
		m_cartridgeType		= static_cast<CartridgeType::Type>(m_romData[kCartridgeTypeOffset]);
		m_romSize			= static_cast<RomSize::Type>(m_romData[kRomSizeOffset]);
		m_ramSize			= static_cast<RamSize::Type>(m_romData[kRamSizeOffset]);
		m_destinationCode	= static_cast<DestinationCode::Type>(m_romData[kDestinationCodeOffset]);
		m_licenseeCodeOld	= static_cast<LicenseeCodeOld::Type>(m_romData[kLicenseeCodeOldOffset]);

		// @todo: sanity check rom size matches supplied data buffer.

		// Detect HW type
		Byte hwType = static_cast<Byte>(m_romData[kHWTypeOffset]);

		if (hwType == 0x80 || hwType == 0xC0)
		{
			m_hardwareType = HardwareType::GameboyColour;
		}
		else
		{
			if(m_romData[kSGBIndicatorOffset] == 0x03)
			{
				m_hardwareType = HardwareType::SuperGameboy;
			}
			else
			{
				m_hardwareType = HardwareType::Gameboy;
			}
		}

		log_debug("Loaded ROM header\n");
		log_debug("\tTitle:               %s\n", m_title.c_str());
		log_debug("\tCartridge type:      %s\n", CartridgeType::get_string(m_cartridgeType));
		log_debug("\tHardware type:       %s\n", HardwareType::get_string(m_hardwareType));
		log_debug("\tRom Size:            %s\n", RomSize::get_string(m_romSize));
		log_debug("\tRam Size:            %s\n", RamSize::get_string(m_ramSize));
		log_debug("\tDestination Code:    %s\n", DestinationCode::get_string(m_destinationCode));
		log_debug("\tLicensee Code (Old): %s\n", LicenseeCodeOld::get_string(m_licenseeCodeOld));
	}

	void Rom::load_banks()
	{
		m_banks.resize(RomSize::get_bank_count(m_romSize));
		uint8_t* dataPtr = m_romData.data();

		for (auto& bank : m_banks)
		{
			bank.m_memory = dataPtr;
			dataPtr += kBankSize;
		}
	}

	//--------------------------------------------------------------------------
}