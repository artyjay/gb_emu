#include "gbhw_log.h"
#include "gbhw_mbc.h"
#include "gbhw_rom.h"

namespace gbhw
{
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
	}

	Rom::Rom()
	{
		Reset();
	}

	Rom::~Rom()
	{
	}

	void Rom::Initialise(CPU* cpu, MMU* mmu)
	{
		m_cpu = cpu;
		m_mmu = mmu;
	}

	void Rom::Reset()
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

	void Rom::Load( uint8_t* romData, uint32_t romLength)
	{
		Reset();

		// Copy ROM into memory.
		m_romData = std::vector<uint8_t>(&romData[0], &romData[romLength]);

		// Perform actual load.
		LoadHeader();
		LoadBanks();
	}

	void Rom::GetInstructions(const Address& address, InstructionList& instructions, int32_t instructionCount)
	{
		const uint8_t* instructionsAddress = m_mmu->GetMemoryPtrFromAddress(address);
		GenerateInstructions(address, instructions, instructionsAddress, instructionCount);
	}

	uint8_t* Rom::GetBank(Byte bankIndex)
	{
		if (bankIndex < m_banks.size())
		{
			return m_banks[bankIndex].m_memory;
		}

		// @todo error...
		return nullptr;
	}

	CartridgeType::Type Rom::GetCartridgeType() const
	{
		return m_cartridgeType;
	}

	void Rom::LoadHeader()
	{
		// parse the rom header..
		m_title				= std::string(m_romData.begin() + kTitleOffset, m_romData.begin() + kTitleOffset + kTitleLength);
		m_cartridgeType		= static_cast<CartridgeType::Type>(m_romData[kCartridgeTypeOffset]);
		m_romSize			= static_cast<RomSize::Type>(m_romData[kRomSizeOffset]);
		m_ramSize			= static_cast<RamSize::Type>(m_romData[kRamSizeOffset]);
		m_destinationCode	= static_cast<DestinationCode::Type>(m_romData[kDestinationCodeOffset]);
		m_licenseeCodeOld	= static_cast<LicenseeCodeOld::Type>(m_romData[kLicenseeCodeOldOffset]);

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

		Message("Loading ROM header\n");
		Message("\tTitle:               %s\n", m_title.c_str());
		Message("\tCartridge type:      %s\n", CartridgeType::GetString(m_cartridgeType));
		Message("\tHardware type:       %s\n", HardwareType::GetString(m_hardwareType));
		Message("\tRom Size:            %s\n", RomSize::GetString(m_romSize));
		Message("\tRam Size:            %s\n", RamSize::GetString(m_ramSize));
		Message("\tDestination Code:    %s\n", DestinationCode::GetString(m_destinationCode));
		Message("\tLicensee Code (Old): %s\n", LicenseeCodeOld::GetString(m_licenseeCodeOld));
	}

	void Rom::LoadBanks()
	{
		m_banks.resize(RomSize::GetBankCount(m_romSize));
		uint8_t* dataPtr = m_romData.data();

		for (auto& bank : m_banks)
		{
			bank.m_memory = dataPtr;
			dataPtr += kBankSize;
		}
	}

	void Rom::GenerateInstructions(Address address, InstructionList& instructions, const uint8_t* instructionAddress, int32_t instructionCount)
	{
		const uint8_t* baseInstruction = m_mmu->GetMemoryPtrFromAddress(0);
		int32_t instructionsProcessed = 0;

		while (true)
		{
			Byte opcode = *instructionAddress;
			Instruction instruction;

			if (opcode == 0xCB)
			{
				Byte extended = *(instructionAddress + 1);
				instruction = m_cpu->GetInstructionExtended(extended);
			}
			else
			{
				instruction = m_cpu->GetInstruction(opcode);
			}

			instruction.Set(address);

			const uint8_t* oldInstructionAddress = instructionAddress;
			instructionAddress += instruction.GetByteSize();
			address += instruction.GetByteSize();

			if (instructionAddress == oldInstructionAddress)
			{
				instructionAddress++; // W\A - XX instructions should be parsed as 1 byte to prevent this.
			}

			instructions.push_back(instruction);
			instructionsProcessed++;

			if (instructionsProcessed >= instructionCount)
			{
				break;
			}
		}
	}

} // gbhw