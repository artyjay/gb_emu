#include "gbhw_mbc.h"
#include "gbhw_mmu.h"
#include "gbhw_log.h"

namespace gbhw
{
	namespace
	{
		static GBInline bool RangeCheck(const Address& address, uint32_t range)
		{
			return ((address & range) == range);
		}

		template<typename... Args>
		static GBInline void MBCMessage(const char* msg, Args... parameters)
		{
#if 1
#else
			Message(msg, std::forward<Args>(parameters)...);
#endif
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// MBC
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	MBC::MBC(MMU* mmu)
		: m_mmu(mmu)
	{
	}

	MBC::~MBC()
	{
	}

	bool MBC::HandleWrite(const Address& address, Byte value)
	{
		// Default ignored.
		return false;
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// MBC1
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	class MBC1 : public MBC
	{
	public:
		MBC1(MMU* mmu, bool bDefaultMode0)
		: MBC(mmu)
		, m_bMode0(bDefaultMode0)
		, m_romBank(0)
		{
		}

		bool MBC1::HandleWrite(const Address& address, Byte value)
		{
			// Mode 0 = Rom banking mode (8kB none-switchable ram, 2mB switchable rom)
			// Mode 1 = Ram banking mode, (32kB switchable ram, 512kB switchable rom)

			if(address < 0x8000)
			{
				if(RangeCheck(address, 0x6000))
				{
					m_bMode0 = (value == 0);
					MBCMessage("MBC1: Swapped mode: %s\n", m_bMode0 ? "ROM banking" : "RAM banking");
				}
				else if(RangeCheck(address, 0x4000))
				{
					// 2-bit value representing either Ram bank number or bits 5:6 of rom bank number.
					if(m_bMode0)
					{
						// @todo: 0x20, 0x40, 0x60 should have 1 added to it.
						m_romBank &= ~0x60;
						m_romBank |= ((value & 0x03) << 5);

						MBCMessage("MBC1: Loading Rom bank: %d\n", m_romBank);
						m_mmu->LoadRomBank(m_romBank);

					}
					else
					{
						MBCMessage("MBC1: Loading ERam bank: %d\n", value);
						m_mmu->LoadERamBank(value & 0x03);
					}
				}
				else if(RangeCheck(address, 0x2000))
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

					MBCMessage("MBC1: Loading Rom bank: %d\n", m_romBank);
					m_mmu->LoadRomBank(m_romBank);
				}
				else
				{
					// External-ram enable.
					MBCMessage("MBC1: Enabling ERam: %s\n", value == 0 ? "false" : "true");
					m_mmu->SetEnableERam(value != 0);
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

		bool MBC3::HandleWrite(const Address& address, Byte value)
		{
			if (address < 0x8000)
			{
				if(RangeCheck(address, 0x6000))
				{
					// Ignored, used by RTC when timer is present in the cartridge.
					Error("MBC with timer unsupported\n");
				}
				else if(RangeCheck(address, 0x4000))
				{
					Byte ramBank = value & 0x03;
					MBCMessage("MBC3: Loading ERam bank: %d\n", ramBank);
					m_mmu->LoadERamBank(ramBank);
				}
				else if(RangeCheck(address, 0x2000))
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

					MBCMessage("MBC3: Loading Rom bank: %d\n", romBank);
					m_mmu->LoadRomBank(romBank);
				}
				else
				{
					// External-ram enable.
					MBCMessage("MBC3: Enabling ERam: %s\n", value == 0x0 ? "false" : "true");
					m_mmu->SetEnableERam(value != 0x0);
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

		bool HandleWrite(const Address& address, Byte value)
		{
			if (address < 0x6000)
			{
				if(RangeCheck(address, 0x4000))
				{
					Byte ramBank = value & 0x0F;
					MBCMessage("MBC5: Loading ERam bank: %d\n", ramBank);
					m_mmu->LoadERamBank(ramBank);
				}
				else if(RangeCheck(address, 0x3000))
				{
					m_romBankHigh = value;
					LoadROMBank();
				}
				else if(RangeCheck(address, 0x2000))
				{
					m_romBankLow = value;
					LoadROMBank();
				}
				else
				{
					// External-ram enable.
					MBCMessage("MBC5: Enabling ERam: %s\n", value == 0x0 ? "false" : "true");
					m_mmu->SetEnableERam(value != 0x0);
				}

				return true;
			}

			return false;
		}

	private:
		void LoadROMBank()
		{
			uint32_t romBank = (static_cast<uint32_t>(m_romBankHigh) << 8) | m_romBankLow;
			MBCMessage("MBC5: Loading Rom bank: %u\n", romBank);
			m_mmu->LoadRomBank(romBank);
		}

		Byte m_romBankLow;
		Byte m_romBankHigh;
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	MBC* CreateMBC(MMU* mmu, CartridgeType::Type cartridgeType)
	{
		switch (cartridgeType)
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
			case CartridgeType::RomMBC3TimerBatt:
			case CartridgeType::RomMBC3TimerRamBatt:
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
				Error("Unsupported MBC\n");
				break;
			}
		}

		return nullptr;
	}
}