#include "mmu.h"
#include "cpu.h"
#include "gpu.h"
#include "log.h"
#include "mbc.h"
#include "rom.h"

namespace gbhw
{
	namespace
	{
		const Byte kButtonBits[button_dpad_down + 1] =
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

	const char* MMU::RegionType::GetString(Enum type)
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
		// @todo: Initialise all memory with "random" data.
		initialise_region(RegionType::RomBank0,			0x0000, 16384, true, true);
		initialise_region(RegionType::RomBank1,			0x4000, 16384, true, true);
		initialise_region(RegionType::VideoRam,			0x8000, 8192,  true, false);
		initialise_region(RegionType::ExternalRam,		0xA000, 8192, false, false);
		initialise_region(RegionType::WorkingRam0,		0xC000, 4096,  true, false);
		initialise_region(RegionType::WorkingRam1,		0xD000, 4096,  true, false); // On CGB this is banked (1->7).
		initialise_region(RegionType::WorkingRamEcho0,	0xE000, 4096,  true, false);
		initialise_region(RegionType::WorkingRamEcho1,	0xF000, 3584,  true, false);
		initialise_region(RegionType::SpriteAttribute,	0xFE00, 256,   true, false);
		initialise_region(RegionType::IO,				0xFF00, 128,   true, false);
		initialise_region(RegionType::ZeroPageRam,		0xFF80, 128,   true, false);
		initialise_ram();

		// Echo first wram bank
		echo_region(RegionType::WorkingRam0, RegionType::WorkingRamEcho0);

		// Load second wram bank from first index.
		load_wram_bank(1);

		reset();
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

	void MMU::reset(CartridgeType::Type cartridgeType)
	{
		if(m_mbc)
		{
			delete m_mbc;
		}

		m_mbc = MBC::create(MMU_ptr(this), cartridgeType);

		reset();

		// Load bank 0 and 1 into memory initially, Bank0 is fixed across all
		// known MBC types. So it will always point at the beginning of a rom.
		load_rom_bank(0, RegionType::RomBank0);
		load_rom_bank(1);
	}

	Byte MMU::read_byte(Address address) const
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
			case RegionType::WorkingRam0:
			case RegionType::WorkingRam1:
			case RegionType::WorkingRamEcho0:
			case RegionType::WorkingRamEcho1:
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

		log_error("Unhandled memory read occurred: [0x%04x]\n", address);

		return 0;
	}


	Word MMU::read_word(Address address) const
	{
		return (read_byte(address) + (static_cast<Word>(read_byte(address + 1)) << 8));
	}

	Byte MMU::read_io(HWRegs::Type reg)
	{
		return read_byte(static_cast<Address>(reg));
	}

	void MMU::write_byte(Address address, Byte byte)
	{
		// @todo Check for out of bounds behaviour
		const uint32_t	lutindex = (address >> kLutShiftGranularity);
			  Region*	region = m_regionsLUT[lutindex];
		const Address	regionAddr = address - region->m_baseAddress;

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
		if (m_mbc->write(address, byte))
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
					m_gpu->update_tile_pattern_line(address, byte);
				}
				else
				{
					// update tilemap...
				}

				break;
			}
			case RegionType::ExternalRam:
			case RegionType::WorkingRam0:
			case RegionType::WorkingRam1:
			case RegionType::WorkingRamEcho0:
			case RegionType::WorkingRamEcho1:
			case RegionType::ZeroPageRam:	// @todo Safety checks on this.
			{
				region->m_memory[regionAddr] = byte;
				break;
			}
			case RegionType::SpriteAttribute:
			{
				if (regionAddr < 160)
				{
					m_gpu->update_sprite_data(address, byte);
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

						break;
					}
					case HWRegs::DIV:
					{
						region->m_memory[regionAddr] = 0;	// DIV is reset when written to.
						break;
					}
					case HWRegs::DMA:
					{
						Address source = byte << 8;
						Address dest = 0xFE00;
						memcpy(&m_memory[dest], &m_memory[source], sizeof(Byte) * 160);

						for(Address i = 0; i < 160; ++i)
						{
							const Address dstAddress = dest + i;
							m_gpu->update_sprite_data(dstAddress, m_memory[dstAddress]);
						}

						break;
					}
					case HWRegs::LCDC:
					{
						// Inform GPU of display being turned on/off.
						m_gpu->set_lcdc(byte);
						region->m_memory[regionAddr] = byte;
						break;
					}
					case HWRegs::BGP:
					{
						m_gpu->update_palette(HWRegs::BGP, byte);
						region->m_memory[regionAddr] = byte;
						break;
					}
					case HWRegs::OBJ0P:
					{
						m_gpu->update_palette(HWRegs::OBJ0P, byte);
						region->m_memory[regionAddr] = byte;
						break;
					}
					case HWRegs::OBJ1P:
					{
						m_gpu->update_palette(HWRegs::OBJ1P, byte);
						region->m_memory[regionAddr] = byte;
						break;
					}
					case HWRegs::Stat:
					{
						// Bottom 3 bits are read-only when from an instruction...
						region->m_memory[regionAddr] = (region->m_memory[regionAddr] & 0x07) | (byte & 0xF8);
						break;
					}
					case HWRegs::SVBK:
					{
						// Bit 3 bits, Read/Writes, indicates bank to be selected.
						// @todo: If 0 is specified do we want to write 0 or the actual bank 1?
						byte &= 0x07;
						load_wram_bank(byte);
						region->m_memory[regionAddr] = byte;
					}
					default:
					{
						region->m_memory[regionAddr] = byte;
						break;
					}
				}

				break;
			}
		}
	}

	void MMU::write_word(Address address, Word word)
	{
		write_byte(address,		static_cast<Byte>(word & 0xFF));
		write_byte(address + 1,	static_cast<Byte>((word >> 8) & 0xFF));
	}

	void MMU::write_io(HWRegs::Type reg, Byte byte)
	{
		// Special case by-pass for IO regs, this is essentially a HW write rather
		// than a SW write.
		m_memory[static_cast<Address>(reg)] = byte;
	}

	void MMU::set_button_state(gbhw_button_t button, gbhw_button_state_t state)
	{
		if(state == button_pressed)
		{
			if (button < button_dpad_right)
			{
				m_buttonsFace &= ~(kButtonBits[button]);
			}
			else
			{
				m_buttonsDirection &= ~(kButtonBits[button]);
			}

			m_cpu->generate_interrupt(HWInterrupts::Button);
		}
		else
		{
			if(button < button_dpad_right)
			{
				m_buttonsFace |= kButtonBits[button];
			}
			else
			{
				m_buttonsDirection |= kButtonBits[button];
			}
		}
	}

	void MMU::load_rom_bank(uint32_t sourceBankIndex, RegionType::Enum destRegion)
	{
		uint8_t* romBankData = m_rom->get_bank(sourceBankIndex);

		if (romBankData)
		{
			m_regions[destRegion].m_memory = romBankData;
		}
		else
		{
			// @todo return error
			log_error("Failed to load ROM bank\n");
		}
	}

	void MMU::load_wram_bank(uint32_t index)
	{
		// 0 indexed lookup, if 0 is specified this clamps to first.
		if(index > 0)
			index -= 1;

		uint8_t* data = m_wramBanks[index].m_memory;

		if(data)
		{
			m_regions[RegionType::WorkingRam1].m_memory = data;
			echo_region(RegionType::WorkingRam1, RegionType::WorkingRamEcho1);
		}
		else
		{
			log_error("Failed to load WRAM bank\n");
		}
	}

	void MMU::load_eram_bank(uint32_t index)
	{
		uint8_t* bank = m_eramBanks[index].m_memory;

		if(bank)
			m_regions[RegionType::ExternalRam].m_memory = bank;
		else
			log_error("Failed to load ERAM bank\n");
	}

	void MMU::set_enable_eram(bool bEnabled)
	{
		m_regions[RegionType::ExternalRam].m_bEnabled = bEnabled;
	}

	const uint8_t* MMU::get_memory_ptr_from_addr(Address address)
	{
		// @todo: Check for out of bounds behaviour
		const Region* region = m_regionsLUT[(address >> kLutShiftGranularity)];
		return (region->m_memory + (address - region->m_baseAddress));
	}

	void MMU::initialise_region(RegionType::Enum type, Address baseaddress, uint16_t size, bool bEnabled, bool bReadOnly)
	{
		Region* region = &m_regions[type];

		// Setup region.
		region->m_type			= type;
		region->m_memory		= m_memory + baseaddress;
		region->m_size			= size;
		region->m_baseAddress	= baseaddress;
		region->m_bEnabled		= bEnabled;
		region->m_bReadOnly		= bReadOnly;

		// Setup LUT.
		uint32_t lutindex = (baseaddress >> kLutShiftGranularity);
		const uint32_t lutend = (size >> kLutShiftGranularity) + lutindex;

		for(; lutindex < lutend; ++lutindex)
		{
			m_regionsLUT[lutindex] = region;
		}
	}

	void MMU::initialise_ram()
	{
		// 7 x 4KiB banks available for 2nd half of WRAM.
		for(uint32_t i = 0; i < 7; ++i)
		{
			MemoryBank bank;
			bank.m_memory = new uint8_t[4096];
			m_wramBanks.push_back(bank);
		}

		// 16 x 8KiB banks available for eram.
		for(uint32_t i = 0; i < 16; ++i)
		{
			MemoryBank bank;
			bank.m_memory = new uint8_t[8192];
			m_eramBanks.push_back(bank);
		}
	}

	void MMU::echo_region(RegionType::Enum src, RegionType::Enum dst)
	{
		Region& srcRegion = m_regions[static_cast<uint32_t>(src)];
		Region& dstRegion = m_regions[static_cast<uint32_t>(dst)];
		dstRegion.m_memory = srcRegion.m_memory;
	}

	void MMU::reset()
	{
		memset(m_memory, 0, kMemorySize);

		for(auto& bank : m_wramBanks)
			memset(bank.m_memory, 0, 4096);

		for(auto& bank : m_eramBanks)
			memset(bank.m_memory, 0, 8192);

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
	}
}