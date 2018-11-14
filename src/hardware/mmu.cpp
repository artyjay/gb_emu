#include "gpu.h"
#include "gbhw_mbc.h"
#include "mmu.h"
#include "gbhw_rom.h"

#include <memory>
#include <iostream>

namespace gbhw
{
	namespace
	{
		const Byte kButtonBits[HWButton::Count] =
		{
			0x01,
			0x02,
			0x04,
			0x08,
			0x01,
			0x02,
			0x04,
			0x08
		};

		const char* kRegionTypeStrings[] =
		{
			"Rom Bank 0",
			"Rom Bank 1",
			"Video Ram",
			"External Ram",
			"Working Ram",
			"Working Ram Echo",
			"Sprite Attribute",
			"IO",
			"Zero Page Ram"
		};
	}

	const char* MMU::RegionType::GetString(Type type)
	{
		return kRegionTypeStrings[type];
	}

	MMU::MMU()
		: m_regionsLUT { nullptr }
		, m_gpu(nullptr)
		, m_cpu(nullptr)
		, m_rom(nullptr)
		, m_mbc(nullptr)
	{
		// @todo Initialise all memory with "random" data.
		InitialiseRegion(RegionType::RomBank0,			0x0000, 16384, true, true);
		InitialiseRegion(RegionType::RomBank1,			0x4000, 16384, true, true);
		InitialiseRegion(RegionType::VideoRam,			0x8000, 8192,  true, false);
		InitialiseRegion(RegionType::ExternalRam,		0xA000, 8192, false, false);
		InitialiseRegion(RegionType::InternalRam,		0xC000, 8192,  true, false); // This is bankable, 0xC000->0xCFFF is fixed, 0xD000->0xDFFF is bankable 1-7 on CGB.
		InitialiseRegion(RegionType::InternalRamEcho,	0xE000, 7680,  true, false);
		InitialiseRegion(RegionType::SpriteAttribute,	0xFE00, 256,   true, false);
		InitialiseRegion(RegionType::IO,				0xFF00, 128,   true, false);
		InitialiseRegion(RegionType::ZeroPageRam,		0xFF80, 128,   true, false);
		InitialiseRam();

		// Echo working ram by sharing the same pointer. The 7680 byte region reserved for the
		// echo is just ignored.
		Region& workingRam		= m_regions[static_cast<uint32_t>(RegionType::InternalRam)];
		Region& workingRamEcho	= m_regions[static_cast<uint32_t>(RegionType::InternalRamEcho)];

		workingRamEcho.m_memory = workingRam.m_memory;

		Reset();
	}

	MMU::~MMU()
	{
		if(m_mbc)
		{
			delete m_mbc;
		}
	}

	void MMU::initialise(CPU_ptr cpu, GPU_ptr gpu, Rom_ptr rom)
	{
		m_cpu = cpu;
		m_gpu = gpu;
		m_rom = rom;
	}

	void MMU::release()
	{
		m_cpu = nullptr;
		m_gpu = nullptr;
		m_rom = nullptr;
	}

	void MMU::Reset(CartridgeType::Type cartridgeType)
	{
		if(m_mbc)
		{
			delete m_mbc;
		}

		m_mbc = CreateMBC(this, cartridgeType);

		Reset();

		// Load bank 0 and 1 into memory initially.
		LoadRomBank(0, true);
		LoadRomBank(1);
	}

	bool MMU::CheckResetBreakpoint()
	{
		bool res = m_bBreakpoint;
		m_bBreakpoint = false;
		return res;
	}
	void MMU::BreakpointSet(Address addressStart, Address addressEnd)
	{
		m_writeBreakpoints.push_back(Breakpoint(addressStart, addressEnd));
	}

	void MMU::BreakpointSetConditional(Address address, Byte conditionValue)
	{
		m_writeBreakpoints.push_back(Breakpoint(address, conditionValue));
	}

	Byte MMU::ReadByte(Address address)
	{
		// @todo Check for out of bounds behaviour
		const uint32_t	lutindex	= (address >> kLutShiftGranularity);
		const Region*	region		= m_regionsLUT[lutindex];
		const Address	regionAddr	= address - region->m_baseAddress;

		if(!region->m_bEnabled)
		{
			return 0;
		}

		switch(region->m_type)
		{
			case RegionType::RomBank0:
			case RegionType::RomBank1:
			case RegionType::VideoRam:
			case RegionType::ExternalRam:
			case RegionType::InternalRam:
			case RegionType::InternalRamEcho:
			case RegionType::ZeroPageRam:	// @todo Safety checks on this.
			{
				return region->m_memory[regionAddr];
			}
			case RegionType::SpriteAttribute:
			{
				if(regionAddr < 160)
				{
					return region->m_memory[regionAddr];
				}

				return 0;
			}
			case RegionType::IO:
			{
				return region->m_memory[regionAddr];
			}
		}

		Error("Unhandled memory read occurred: [0x%04x]\n", address);

		return 0;
	}

	void MMU::WriteIO(HWRegs::Type reg, Byte byte)
	{
		// Special case by-pass for IO regs, this is essential a HW write rather than a SW write.
		m_memory[static_cast<Address>(reg)] = byte;
	}

	Word MMU::ReadWord(Address address)
	{
		return (ReadByte(address) + (static_cast<Word>(ReadByte(address + 1)) << 8));
	}

	void MMU::WriteByte(Address address, Byte byte)
	{
		// @todo Check for out of bounds behaviour
		const uint32_t	lutindex = (address >> kLutShiftGranularity);
			  Region*	region = m_regionsLUT[lutindex];
		const Address	regionAddr = address - region->m_baseAddress;

		HandleBreakpoint(address, byte);

		if(address >= 0x9800 && address < 0x9810)
		{
			int a = 0;
			a++;
		}

		// If this is an MBC write then consume the byte.
		// The MBC can prevent writes when they occur inside
		// certain address ranges that manipulate the MMU in
		// some way (i.e. Load ROM bank, enable eram, load eram
		// bank, etc...).
		if (m_mbc->HandleWrite(address, byte))
		{
			return;
		}

		if(region->m_bReadOnly || !region->m_bEnabled)
		{
			return;
		}

		switch (region->m_type)
		{
			case RegionType::RomBank0:
			{
				break;
			}
			case RegionType::VideoRam:
			{
				region->m_memory[regionAddr] = byte;

				if (regionAddr < 0x1800)
				{
					m_gpu->UpdateTilePatternLine(address, byte);
				}
				else
				{
					// update tilemap...
				}

				break;
			}
			case RegionType::ExternalRam:
			case RegionType::InternalRam:
			case RegionType::InternalRamEcho:
			case RegionType::ZeroPageRam:	// @todo Safety checks on this.
			{
				region->m_memory[regionAddr] = byte;
				break;
			}
			case RegionType::SpriteAttribute:
			{
				if (regionAddr < 160)
				{
					m_gpu->UpdateSpriteData(address, byte);
					region->m_memory[regionAddr] = byte;
				}
				break;
			}
			case RegionType::IO:
			{
				// IO has a bunch of special case handling for specific writes.
				// Due to the interactive nature of this region of memory.
				switch(address)
				{
					case HWRegs::P1:
					{
						m_buttonColumn = byte & 0x30;

						byte = 0;

						if(m_buttonColumn == 0x10)
						{
							byte |= m_buttonsFace;
						}
						else if(m_buttonColumn == 0x20)
						{
							byte |= m_buttonsDirection;
						}

						region->m_memory[regionAddr] = byte;
					} break;
					case HWRegs::DIV:
					{
						region->m_memory[regionAddr] = 0;	// DIV is reset when written to.
					} break;
					case HWRegs::DMA:
					{
						Address source = byte << 8;
						Address dest = 0xFE00;
						memcpy(&m_memory[dest], &m_memory[source], sizeof(Byte) * 160);

						for(Address i = 0; i < 160; ++i)
						{
							const Address dstAddress = dest + i;
							m_gpu->UpdateSpriteData(dstAddress, m_memory[dstAddress]);
						}


					} break;
					case HWRegs::LCDC:
					{
						// Inform GPU of display being turned on/off.
						m_gpu->SetLCDC(byte);
						region->m_memory[regionAddr] = byte;
					} break;
					case HWRegs::BGP:
					{
						m_gpu->UpdatePalette(HWRegs::BGP, byte);
						region->m_memory[regionAddr] = byte;
					} break;
					case HWRegs::OBJ0P:
					{
						m_gpu->UpdatePalette(HWRegs::OBJ0P, byte);
						region->m_memory[regionAddr] = byte;
					} break;
					case HWRegs::OBJ1P:
					{
						m_gpu->UpdatePalette(HWRegs::OBJ1P, byte);
						region->m_memory[regionAddr] = byte;
					} break;
					case HWRegs::Stat:
					{
						// Bottom 3 bits are read-only when from an instruction...
						region->m_memory[regionAddr] = (region->m_memory[regionAddr] & 0x07) | (byte & 0xF8);
					} break;
					
					default:
					{
						region->m_memory[regionAddr] = byte;
					} break;
				}

				// After writing special case handling...
				switch(address)
				{
					case HWRegs::ScrollY:
						break;
					case HWRegs::LYC:
						//std::cout << "Writing to lyc: " << std::hex << (int)byte << std::endl;
						break;
				}

				
				
				break;
			}
		}
	}
	
	void MMU::WriteWord(Address address, Word word)
	{
		WriteByte(address,		static_cast<Byte>(word & 0xFF));
		WriteByte(address + 1,	static_cast<Byte>((word >> 8) & 0xFF));
	}

	void MMU::PressButton(HWButton::Type button)
	{
		if (button < HWButton::Right)
		{
			m_buttonsFace &= ~(kButtonBits[button]);
		}
		else
		{
			m_buttonsDirection &= ~(kButtonBits[button]);
		}

		m_cpu->GenerateInterrupt(HWInterrupts::Button);
	}

	void MMU::ReleaseButton(HWButton::Type button)
	{
		if(button < HWButton::Right)
		{
			m_buttonsFace |= kButtonBits[button];
		}
		else
		{
			m_buttonsDirection |= kButtonBits[button];
		}
	}

	void MMU::LoadRomBank(uint32_t sourceBankIndex, bool bFirstBank)
	{
		uint8_t* romBankData = m_rom->GetBank(sourceBankIndex);

		if (romBankData)
		{
			if(bFirstBank)
			{
				m_regions[RegionType::RomBank0].m_memory = romBankData;
			}
			else
			{
				m_regions[RegionType::RomBank1].m_memory = romBankData;
			}
			//Message("Loading ROM bank %u into %s\n", sourceBankIndex, RegionType::GetString(destinationBank.m_type));
			//memcpy_s(destinationBank.m_memory, Rom::kBankSize, romBankData, Rom::kBankSize);
		}
		else
		{
			// @todo error....
			Message("*CRITICAL* Failed to load ROM bank\n");
		}
	}

	void MMU::LoadERamBank(uint32_t sourceBankIndex)
	{
		uint8_t* ramBankData = m_ramBanks[sourceBankIndex].m_memory;

		if(ramBankData)
		{
			m_regions[RegionType::ExternalRam].m_memory = ramBankData;
		}
		else
		{
			Message("*CRITICAL* Failed to load RAM bank\n");
		}
	}

	void MMU::SetEnableERam(bool bEnabled)
	{
		m_regions[RegionType::ExternalRam].m_bEnabled = bEnabled;
	}

	const uint8_t* MMU::GetMemoryPtrFromAddress(Address address)
	{
		// @todo Check for out of bounds behaviour
		const uint32_t	lutindex = (address >> kLutShiftGranularity);
		Region*	region = m_regionsLUT[lutindex];
		const Address	regionAddr = address - region->m_baseAddress;

		return (region->m_memory + regionAddr);
	}

	void MMU::HandleBreakpoint(Address writeAddress, Byte value)
	{
		m_bBreakpoint = false;

		for (auto& bp : m_writeBreakpoints)
		{
			if(bp.m_bConditional)
			{
				if(writeAddress == bp.m_startAddress && value == bp.m_conditionValue)
				{
					m_bBreakpoint = true;
					break;
				}
			}
			else
			{
				if (writeAddress >= bp.m_startAddress && writeAddress <= bp.m_endAddress)
				{
					m_bBreakpoint = true;
					break;
				}
			}
		}
	}

	void MMU::InitialiseRegion(RegionType::Type type, Address baseaddress, uint16_t size, bool bEnabled, bool bReadOnly)
	{
		Region* region = &m_regions[type];

		// Setup region.
		region->m_type			= type;
		region->m_memory		= m_memory + baseaddress;
		region->m_size			= size;
		region->m_baseAddress	= baseaddress;
		region->m_bEnabled		= bEnabled;
		region->m_bReadOnly		= bReadOnly;

		// Setup lut.
		uint32_t lutindex = (baseaddress >> kLutShiftGranularity);
		const uint32_t lutend = (size >> kLutShiftGranularity) + lutindex;

		for(; lutindex < lutend; ++lutindex)
		{
			m_regionsLUT[lutindex] = region;
		}
	}

	void MMU::InitialiseRam()
	{
		for(uint32_t i = 0; i < 16; ++i)
		{
			MemoryBank bank;
			bank.m_memory = new uint8_t[8192];
			m_ramBanks.push_back(bank);
		}
	}

	void MMU::Reset()
	{
		memset(m_memory, 0, kMemorySize);

		m_memory[HWRegs::P1] = 0xFF;

		m_memory[HWRegs::TIMA] = 0x00;
		m_memory[HWRegs::TMA] = 0x00;
		m_memory[HWRegs::TAC] = 0x00;

		m_memory[0xFF10] = 0x80;
		m_memory[0xFF11] = 0xBF;
		m_memory[0xFF12] = 0x3F;

		m_memory[0xFF14] = 0xBF;
		m_memory[0xFF16] = 0x3F;
		m_memory[0xFF17] = 0x00;

		m_memory[0xFF19] = 0xBF;
		m_memory[0xFF1A] = 0x7F;
		m_memory[0xFF1B] = 0xFF;
		m_memory[0xFF1C] = 0x9F;
		m_memory[0xFF1E] = 0xBF;

		m_memory[0xFF20] = 0xFF;
		m_memory[0xFF21] = 0x00;
		m_memory[0xFF22] = 0x00;
		m_memory[0xFF23] = 0xBF;
		m_memory[0xFF24] = 0x77;
		m_memory[0xFF25] = 0xF3;
		m_memory[0xFF26] = 0xF1; // SGB = F0

		m_memory[0xFF40] = 0x91;
		m_memory[0xFF42] = 0x00;
		m_memory[0xFF43] = 0x00;
		m_memory[0xFF45] = 0x00;
		m_memory[0xFF47] = 0xFC;
		m_memory[0xFF48] = 0xFF;
		m_memory[0xFF49] = 0xFF;

		m_memory[0xFF4A] = 0x00;
		m_memory[0xFF4B] = 0x00;
		m_memory[HWRegs::IE] = 0x00;

		m_buttonColumn = 0;
		m_buttonsDirection = 0x0F;
		m_buttonsFace = 0x0F;

		m_bBreakpoint = false;
	}
}