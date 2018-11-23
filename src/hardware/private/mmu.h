#pragma once

#include "context.h"
#include "gbhw.h"
#include "mbc.h"

namespace gbhw
{
	// The gameboy has a total working size of 65536, which is divided into regions.
	// Behavior changes depending on the region accessed. Below is an outline of the
	// memory regions.
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
			enum Type
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

			static const char* GetString(Type type);
		};

		struct Region
		{
			RegionType::Type	m_type;
			uint8_t*			m_memory;
			uint16_t			m_size;
			Address				m_baseAddress;
			bool				m_bEnabled;
			bool				m_bReadOnly;
		};

		struct Breakpoint
		{
			Breakpoint(Address s, Address e) : m_startAddress(s), m_endAddress(e), m_conditionValue(0), m_bConditional(false) {}
			Breakpoint(Address addr, Byte conditionValue) : m_startAddress(addr), m_endAddress(addr), m_conditionValue(conditionValue), m_bConditional(true) {}

			Address m_startAddress;
			Address m_endAddress;	// inclusive.
			Byte	m_conditionValue;
			bool	m_bConditional;
		};

	public:
		MMU();
		~MMU();

		void initialise(CPU_ptr cpu, GPU_ptr gpu, Rom_ptr rom);
		void release();

		void Reset(CartridgeType::Type cartridgeType);

		bool CheckResetBreakpoint();
		void BreakpointSet(Address addressStart, Address addressEnd);
		void BreakpointSetConditional(Address address, Byte conditionValue);

		Byte ReadByte(Address address) const;
		Word ReadWord(Address address) const;

		void WriteIO(HWRegs::Type reg, Byte byte);
		void WriteByte(Address address, Byte byte);
		void WriteWord(Address address, Word word);

		void set_button_state(gbhw_button_t button, gbhw_button_state_t state);

		void LoadRomBank(uint32_t sourceBankIndex, bool bFirstBank = false);
		void LoadERamBank(uint32_t sourceBankIndex);
		void SetEnableERam(bool bEnabled);

		const uint8_t* GetMemoryPtrFromAddress(Address address);

	private:
		void HandleBreakpoint(Address writeAddress, Byte value);
		void InitialiseRegion(RegionType::Type type, Address baseaddress, uint16_t size, bool bEnabled, bool bReadOnly);
		void InitialiseRam();
		void Reset();

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


		std::vector<Breakpoint>	m_writeBreakpoints;
		bool					m_bBreakpoint;
		Byte					m_buttonColumn;
		Byte					m_buttonsDirection;
		Byte					m_buttonsFace;
	};
}