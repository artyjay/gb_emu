#pragma once

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <type_traits>
#include <array>
#include <cstring>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#endif

namespace gbhw
{
	using Buffer	= std::vector<uint8_t>;
	using Byte		= uint8_t;
	using SByte		= int8_t;
	using Word		= uint16_t;
	using SWord		= int16_t;
	using Address	= uint16_t;

	union AddressU
	{
		Address addr;
		struct
		{
			Byte low;
			Byte high;
		};
	};

	struct CartridgeType
	{
		enum Type
		{
			RomOnly = 0x00,
			RomMBC1 = 0x01,
			RomMBC1Ram = 0x02,
			RomMBC1RamBatt = 0x03,
			RomMBC2 = 0x05,
			RomMBC2Batt = 0x06,
			RomRam = 0x08,
			RomRamBatt = 0x09,
			RomMMMM01 = 0x0B,
			RomMMMM01Ram = 0x0C,
			RomMMMM01RamBatt = 0x0D,
			RomMBC3TimerBatt = 0x0F,
			RomMBC3TimerRamBatt = 0x10,
			RomMBC3 = 0x11,
			RomMBC3Ram = 0x12,
			RomMBC3RamBatt = 0x13,
			RomMBC4 = 0x15,
			RomMBC4Ram = 0x16,
			RomMBC4RamBatt = 0x17,
			RomMBC5 = 0x19,
			RomMBC5Ram = 0x1A,
			RomMBC5RamBatt = 0x1B,
			RomMBC5Rumble = 0x1C,
			RomMBC5RumbleSRam = 0x1D,
			RomMBC5RumbleSRamBatt = 0x1E,
			PocketCamera = 0xFC,
			BandaiTAMA5 = 0xFD,
			HudsonHuC3 = 0xFE,
			HudsonHuC1 = 0xFF,
			Unknown
		};

		static const char* get_string(Type type);
	};

	struct HardwareType
	{
		enum Type
		{
			Gameboy = 0x00,
			GameboyColour = 0x01,
			SuperGameboy = 0x02,
			Unknown
		};

		static const char* get_string(Type type);
	};

	struct RomSize
	{
		enum Type
		{
			Size_32kB = 0x00,
			Size_64kB = 0x01,
			Size_128kB = 0x02,
			Size_256kB = 0x03,
			Size_512kB = 0x04,
			Size_1024kB = 0x05,
			Size_2048kB = 0x06,
			Size_4096kB = 0x07,
			Size_1152kB = 0x52,
			Size_1280kB = 0x53,
			Size_1536kB = 0x54,
			Unknown
		};

		static uint32_t get_bank_count(Type type);
		static const char* get_string(Type type);
	};

	struct RamSize
	{
		enum Type
		{
			None = 0x00,
			Size_2kB = 0x01,
			Size_8kB = 0x02,
			Size_32kB = 0x03,
			Size_128kB = 0x04,
			Size_64kB = 0x05,
			Unknown
		};

		static uint32_t get_bank_count(Type type);
		static const char* get_string(Type type);
	};

	struct DestinationCode
	{
		enum Type
		{
			Japanese = 0x00,
			NonJapanese = 0x01,
			Unknown
		};

		static const char* get_string(Type type);
	};

	struct LicenseeCodeOld
	{
		enum Type
		{
			CheckNew = 0x33,
			Accolade = 0x79,
			Konami = 0xA4,
			Unknown
		};

		static const char* get_string(Type type);
	};

	struct HWRegs
	{
		enum Type
		{
			P1		= 0xFF00,
			DIV		= 0xFF04,
			TIMA	= 0xFF05,
			TMA		= 0xFF06,
			TAC		= 0xFF07,
			IF		= 0xFF0F,
			LCDC	= 0xFF40,
			Stat	= 0xFF41,
			ScrollY = 0xFF42,
			ScrollX = 0xFF43,
			LY		= 0xFF44,
			LYC		= 0xFF45,
			DMA		= 0xFF46,
			BGP		= 0xFF47,
			OBJ0P	= 0xFF48,
			OBJ1P	= 0xFF49,
			WindowY = 0xFF4A,
			WindowX = 0xFF4B,
			Key1	= 0xFF4D,
			HDMA1	= 0xFF51,
			HDMA2	= 0xFF52,
			HDMA3	= 0xFF53,
			HDMA4	= 0xFF54,
			HDMA5	= 0xFF55,
			BGPI	= 0xFF68,	// Background palette index
			BGPD	= 0xFF69,	// Background palette data
			OBPI	= 0xFF6A,	// Sprite palette index
			OBPD	= 0xFF6B,	// Sprite palette data
			VBK		= 0xFF4F,	// VRAM bank
			SVBK	= 0xFF70,	// WRAM bank
			IE		= 0xFFFF
		};
	};

	struct HWInterrupts
	{
		enum Type
		{
			VBlank = 0x01,
			Stat = 0x02,
			Timer = 0x04,
			Serial = 0x08,
			Button = 0x10
		};

		static const char* get_string(Type interrupt);
	};

	struct HWInterruptRoutines
	{
		enum Type
		{
			VBlank = 0x0040,
			Stat = 0x0048,
			Timer = 0x0050,
			Serial = 0x0058,
			Button = 0x0060
		};
	};

	struct HWLCDC
	{
		enum Type
		{
			BGEnabled = 0x01,
			SpriteEnabled = 0x02,
			SpriteSize = 0x04,
			BGTileMap = 0x08,
			TileData = 0x10,
			WindowEnabled = 0x20,
			WindowTileMap = 0x40,
			Enabled = 0x80
		};

		enum Addresses
		{
			TilePattern0 = 0x8800,	// Index is -128 to 127. (So index zero is 0x9000)
			TilePattern1 = 0x8000,	// Sprites use this area.
			TileMap0 = 0x9800,
			TileMap1 = 0x9C00,
			SpriteData = 0xFE00,
		};

		static bool bg_enabled(Byte lcdc);
		static bool sprite_enabled(Byte lcdc);
		static bool sprite_double_height(Byte lcdc);
		static Byte bg_tile_map_index(Byte lcdc);
		static Byte tile_data_index(Byte lcdc);
		static bool window_enabled(Byte lcdc);
		static Byte window_tile_map_index(Byte lcdc);
		static bool enabled(Byte lcdc);

	};

	struct HWLCDCStatus
	{
		enum Type
		{
			ModeHBlank				= 0x00,
			ModeVBlank				= 0x01,
			ModeOAM					= 0x02,
			ModeOAMVRam				= 0x03,
			ModeMask				= 0x03,
			Coincidence				= 0x04,
			InterruptHBlank			= 0x08,
			InterruptVBlank			= 0x10,
			InterruptOAM			= 0x20,
			InterruptCoincidence	= 0x40,
			InterruptNone			= 0xFF	// Used to not send an interrupt.
		};
	};
}