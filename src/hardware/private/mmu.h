#pragma once

#include "context.h"
#include "gbhw.h"
#include "mbc.h"

namespace gbhw
{
	// The Gameboy has a total addressable memory size of 65536, which is divided
	// into regions. Behavior changes depending on the region accessed. Below is
	// an outline of the memory regions.
	//
	// 16kB				- [0x0000 -> 0x3FFF]	Cartridge ROM (bank 0)
	// 16kB				- [0x4000 -> 0x7FFF]	Cartridge ROM (switchable bank)
	// 8kB				- [0x8000 -> 0x9FFF]	Video RAM
	// 8kB				- [0xA000 -> 0xBFFF]	External RAM
	// 8KB				- [0xC000 -> 0xDFFF]	Internal RAM
	// 8kB - 512 bytes	- [0xE000 -> 0xFDFF]	Echo of internal RAM - upto last 512 bytes, at which point other things happen. Ignore this range and translate address to internal RAM.
	// 160 bytes		- [0xFE00 -> 0xFE9F]	Sprite attribute memory (OAM)
	// 96 bytes			- [0xFEA0 -> 0xFEFF]	Empty but unusable for I/o
	// 128 bytes		- [0xFF00 -> 0xFF7F]	Memory mapped I/O
	// 128 bytes		- [0xFF80 -> 0xFFFF]	Zero-page RAM

	// Cartridge rom is outlined below:
	// 256 bytes		- [0x0000 -> 0x00FF]	BIOS
	// 80 bytes			- [0x0100 -> 0x014F]	Cartridge header
	// 16048 bytes		- [0x0150 -> 0x3FFF]	Program

	// Store details about a specific memory bank
	struct MemoryBank
	{
		inline MemoryBank() : m_memory(nullptr) {}
		uint8_t* m_memory;
	};

	using MemoryBanks = std::vector<MemoryBank>;

	class CPU;
	class GPU;
	class Rom;
	class MMU
	{
		struct RegionType
		{
			enum Enum
			{
				RomBank0 = 0,
				RomBank1,
				VideoRam,
				ExternalRam,
				InternalRam,
				InternalRamEcho,
				SpriteAttribute,
				IO,
				ZeroPageRam,
				Count
			};

			static const char* GetString(Enum type);
		};

		struct Region
		{
			RegionType::Enum	m_type;
			uint8_t*			m_memory;
			uint16_t			m_size;
			Address				m_baseAddress;
			bool				m_bEnabled;
			bool				m_bReadOnly;
		};

	public:
		MMU();
		~MMU();

		void initialise(CPU_ptr cpu, GPU_ptr gpu, Rom_ptr rom);
		void release();

		void reset(CartridgeType::Type cartridgeType);

		Byte read_byte(Address address) const;
		Word read_word(Address address) const;
		Byte read_io(HWRegs::Type reg);

		void write_byte(Address address, Byte byte);
		void write_word(Address address, Word word);
		void write_io(HWRegs::Type reg, Byte byte);

		void set_button_state(gbhw_button_t button, gbhw_button_state_t state);

		void load_rom_bank(uint32_t sourceBankIndex, RegionType::Enum destRegion = RegionType::RomBank1);
		void load_eram_bank(uint32_t sourceBankIndex);
		void set_enable_eram(bool bEnabled);

		const uint8_t* get_memory_ptr_from_addr(Address address);

	private:
		void initialise_region(RegionType::Enum type, Address baseaddress, uint16_t size, bool bEnabled, bool bReadOnly);
		void initialise_ram();
		void reset();

		static const uint32_t	kMemorySize				= 65536;
		static const uint32_t	kLutShiftGranularity	= 7;	// Shift right for / 128.
		static const uint32_t	kRegionLutCount			= kMemorySize >> kLutShiftGranularity;

		GPU_ptr					m_gpu;
		CPU_ptr					m_cpu;
		Rom_ptr					m_rom;
		uint8_t					m_memory[kMemorySize];
		Region					m_regions[static_cast<uint32_t>(RegionType::Count)];
		Region*					m_regionsLUT[kRegionLutCount];
		MBC*					m_mbc;
		MemoryBanks				m_ramBanks;

		Byte					m_buttonColumn;
		Byte					m_buttonsDirection;
		Byte					m_buttonsFace;
	};
}