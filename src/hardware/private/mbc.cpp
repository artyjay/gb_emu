#include "mbc.h"
#include "mmu.h"
#include "log.h"

namespace gbhw
{
	namespace
	{
		static inline bool range_check(const Address& address, uint32_t range)
		{
			return ((address & range) == range);
		}

		template<typename... Args>
		static inline void mbc_debug(const char* msg, Args... parameters)
		{
#if 0
			log_debug(msg, std::forward<Args>(parameters)...);
#endif
		}
	}

	//--------------------------------------------------------------------------
	// MBC
	//--------------------------------------------------------------------------

	MBC::MBC(MMU* mmu)
		: m_mmu(mmu)
	{
	}

	MBC::~MBC()
	{
	}

	bool MBC::write(const Address& address, Byte value)
	{
		return false;
	}

	//--------------------------------------------------------------------------
	// MBC1
	//--------------------------------------------------------------------------

	class MBC1 : public MBC
	{
	public:
		MBC1(MMU* mmu, bool bDefaultMode0)
		: MBC(mmu)
		, m_bMode0(bDefaultMode0)
		, m_romBank(0)
		{
		}

		bool write(const Address& address, Byte value)
		{
			// Mode 0 = Rom banking mode (8kB none-switchable ram, 2mB switchable rom)
			// Mode 1 = Ram banking mode, (32kB switchable ram, 512kB switchable rom)

			if(address < 0x8000)
			{
				if(range_check(address, 0x6000))
				{
					m_bMode0 = (value == 0);
					mbc_debug("MBC1: Swapped mode: %s\n", m_bMode0 ? "ROM banking" : "RAM banking");
				}
				else if(range_check(address, 0x4000))
				{
					// 2-bit value representing either Ram bank number or bits 5:6 of rom bank number.
					if(m_bMode0)
					{
						// @todo: 0x20, 0x40, 0x60 should have 1 added to it.
						m_romBank &= ~0x60;
						m_romBank |= ((value & 0x03) << 5);

						mbc_debug("MBC1: Loading Rom bank: %d\n", m_romBank);
						m_mmu->load_rom_bank(m_romBank);

					}
					else
					{
						mbc_debug("MBC1: Loading ERam bank: %d\n", value);
						m_mmu->load_eram_bank(value & 0x03);
					}
				}
				else if(range_check(address, 0x2000))
				{
					// Lower 5 bits of the ROM bank number (0x01->0x1F).
					if(value == 0)
					{
						// 0 = 1 in this case.
						m_romBank = 1;
					}
					else
					{
						// Lower 5 bits selects rom bank.
						m_romBank &= ~0x1F;
						m_romBank |= (value & 0x1F);
					}

					mbc_debug("MBC1: Loading Rom bank: %d\n", m_romBank);
					m_mmu->load_rom_bank(m_romBank);
				}
				else
				{
					// External-ram enable.
					mbc_debug("MBC1: Enabling ERam: %s\n", value == 0 ? "false" : "true");
					m_mmu->set_enable_eram(value != 0);
				}

				return true;
			}

			return false;
		}

	private:
		bool m_bMode0;
		Byte m_romBank;
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// MBC3
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	class MBC3 : public MBC
	{
	public:
		MBC3(MMU* mmu)
		: MBC(mmu)
		{
		}

		bool write(const Address& address, Byte value)
		{
			if (address < 0x8000)
			{
				if(range_check(address, 0x6000))
				{
					// Ignored, used by RTC when timer is present in the cartridge.
					log_error("MBC with timer unsupported\n");
				}
				else if(range_check(address, 0x4000))
				{
					Byte ramBank = value & 0x03;
					mbc_debug("MBC3: Loading ERam bank: %d\n", ramBank);
					m_mmu->load_eram_bank(ramBank);
				}
				else if(range_check(address, 0x2000))
				{
					Byte romBank;

					if (value == 0)
					{
						// 0 = 1 in this case.
						romBank = 1;
					}
					else
					{
						romBank = value & 0x7F;
					}

					mbc_debug("MBC3: Loading Rom bank: %d\n", romBank);
					m_mmu->load_rom_bank(romBank);
				}
				else
				{
					// External-ram enable.
					mbc_debug("MBC3: Enabling ERam: %s\n", value == 0x0 ? "false" : "true");
					m_mmu->set_enable_eram(value != 0x0);
				}

				return true;
			}

			return false;
		}
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// MBC5
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	class MBC5 : public MBC
	{
	public:
		MBC5(MMU* mmu)
			: MBC(mmu)
			, m_romBankLow(0)
			, m_romBankHigh(0)
		{
		}

		bool write(const Address& address, Byte value)
		{
			if (address < 0x6000)
			{
				if(range_check(address, 0x4000))
				{
					Byte ramBank = value & 0x0F;
					mbc_debug("MBC5: Loading ERam bank: %d\n", ramBank);
					m_mmu->load_eram_bank(ramBank);
				}
				else if(range_check(address, 0x3000))
				{
					m_romBankHigh = value;
					load_rom_bank();
				}
				else if(range_check(address, 0x2000))
				{
					m_romBankLow = value;
					load_rom_bank();
				}
				else
				{
					// External-ram enable.
					mbc_debug("MBC5: Enabling ERam: %s\n", value == 0x0 ? "false" : "true");
					m_mmu->set_enable_eram(value != 0x0);
				}

				return true;
			}

			return false;
		}

	private:
		void load_rom_bank()
		{
			uint32_t romBank = (static_cast<uint32_t>(m_romBankHigh) << 8) | m_romBankLow;
			mbc_debug("MBC5: Loading Rom bank: %u\n", romBank);
			m_mmu->load_rom_bank(romBank);
		}

		Byte m_romBankLow;
		Byte m_romBankHigh;
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	MBC* MBC::create(MMU* mmu, CartridgeType::Type cartridge)
	{
		switch (cartridge)
		{
			case CartridgeType::RomOnly:
			{
				return new MBC(mmu);	// Do nothing.
			}
			case CartridgeType::RomMBC1:
			{
				return new MBC1(mmu, true);
			}
			case CartridgeType::RomMBC1Ram:
			case CartridgeType::RomMBC1RamBatt:
			{
				return new MBC1(mmu, false);
			}
			case CartridgeType::RomMBC3:
			case CartridgeType::RomMBC3Ram:
			case CartridgeType::RomMBC3RamBatt:
			case CartridgeType::RomMBC3TimerBatt:		// @todo: Implement timer functionality, some roms will still work without it.
			case CartridgeType::RomMBC3TimerRamBatt:
			{
				return new MBC3(mmu);
			}
			case CartridgeType::RomMBC5:
			case CartridgeType::RomMBC5Ram:
			case CartridgeType::RomMBC5RamBatt:
			{
				return new MBC5(mmu);
			}
			case CartridgeType::RomMBC2:
			case CartridgeType::RomMBC2Batt:
			case CartridgeType::RomRam:
			case CartridgeType::RomRamBatt:
			case CartridgeType::RomMMMM01:
			case CartridgeType::RomMMMM01Ram:
			case CartridgeType::RomMMMM01RamBatt:
			case CartridgeType::RomMBC4:
			case CartridgeType::RomMBC4Ram:
			case CartridgeType::RomMBC4RamBatt:
			case CartridgeType::RomMBC5Rumble:
			case CartridgeType::RomMBC5RumbleSRam:
			case CartridgeType::RomMBC5RumbleSRamBatt:
			case CartridgeType::PocketCamera:
			case CartridgeType::BandaiTAMA5:
			case CartridgeType::HudsonHuC3:
			case CartridgeType::HudsonHuC1:
			default:
			{
				log_error("Unsupported MBC\n");
				break;
			}
		}

		return nullptr;
	}
}