#include "gbhw_types.h"

namespace gbhw
{
	const char* CartridgeType::GetString(Type type)
	{
		switch (type)
		{
		case CartridgeType::RomOnly:				return "Rom-Only";
		case CartridgeType::RomMBC1:				return "MBC1";
		case CartridgeType::RomMBC1Ram:				return "MBC1 + Ram";
		case CartridgeType::RomMBC1RamBatt:			return "MBC1 + Ram + Battery";
		case CartridgeType::RomMBC2:				return "MBC2";
		case CartridgeType::RomMBC2Batt:			return "MBC2 + Battery";
		case CartridgeType::RomRam:					return "Ram";
		case CartridgeType::RomRamBatt:				return "Ram + Battery";
		case CartridgeType::RomMMMM01:				return "MMM01";
		case CartridgeType::RomMMMM01Ram:			return "MMM01 + Ram";
		case CartridgeType::RomMMMM01RamBatt:		return "MMM01 + Ram + Battery";
		case CartridgeType::RomMBC3TimerBatt:		return "MBC3 + Timer + Battery";
		case CartridgeType::RomMBC3TimerRamBatt:	return "MBC3 + Timer + Ram + Battery";
		case CartridgeType::RomMBC3:				return "MBC3";
		case CartridgeType::RomMBC3Ram:				return "MBC3 + Ram";
		case CartridgeType::RomMBC3RamBatt:			return "MBC3 + Ram + Battery";
		case CartridgeType::RomMBC4:				return "MBC4";
		case CartridgeType::RomMBC4Ram:				return "MBC4 + Ram";
		case CartridgeType::RomMBC4RamBatt:			return "MBC4 + Ram + Battery";
		case CartridgeType::RomMBC5:				return "MBC5";
		case CartridgeType::RomMBC5Ram:				return "MBC5 + Ram";
		case CartridgeType::RomMBC5RamBatt:			return "MBC5 + Ram + Battery";
		case CartridgeType::RomMBC5Rumble:			return "MBC5 + Rumble";
		case CartridgeType::RomMBC5RumbleSRam:		return "MBC5 + Rumble + Ram";
		case CartridgeType::RomMBC5RumbleSRamBatt:	return "MBC5 + Rumble + Ram + Battery";
		case CartridgeType::PocketCamera:			return "Pocket Camera";
		case CartridgeType::BandaiTAMA5:			return "Bandai TAMA5";
		case CartridgeType::HudsonHuC3:				return "Hudson HuC3";
		case CartridgeType::HudsonHuC1:				return "Hudson HuC1";
		}
		return "UnknownCartridgeType";
	}

	const char* HardwareType::GetString(Type type)
	{
		switch (type)
		{
		case HardwareType::Gameboy: return "Gameboy";
		case HardwareType::GameboyColour: return "Gameboy Colour";
		case HardwareType::SuperGameboy: return "Super Gameboy";
		}
		return "UnknownHardwareType";
	}

	uint32_t RomSize::GetBankCount(Type type)
	{
		switch (type)
		{
		case Size_32kB:		return 2;
		case Size_64kB:		return 4;
		case Size_128kB:	return 8;
		case Size_256kB:	return 16;
		case Size_512kB:	return 32;
		case Size_1024kB:	return 64;
		case Size_2048kB:	return 128;
		case Size_1152kB:	return 72;
		case Size_1280kB:	return 80;
		case Size_1536kB:	return 96;
		}

		// @todo error.
		return 0;
	}

	const char* RomSize::GetString(Type type)
	{
		switch (type)
		{
		case RomSize::Size_32kB:	return "32 KB  [ no banks]";
		case RomSize::Size_64kB:	return "64 KB  [  4 banks]";
		case RomSize::Size_128kB:	return "128 KB [  8 banks]";
		case RomSize::Size_256kB:	return "256 KB [ 16 banks]";
		case RomSize::Size_512kB:	return "512 KB [ 32 banks]";
		case RomSize::Size_1024kB:	return "1 MB   [ 64 banks]";
		case RomSize::Size_2048kB:	return "2 MB   [128 banks]";
		case RomSize::Size_1152kB:	return "1.1 MB [ 72 banks]";
		case RomSize::Size_1280kB:	return "1.2 MB [ 80 banks]";
		case RomSize::Size_1536kB:	return "1.5 MB [ 96 banks]";
		}
		return "UnknownRomSize";
	}

	uint32_t RamSize::GetBankCount(Type type)
	{
		switch (type)
		{
		case RamSize::None:			return 0;
		case RamSize::Size_2kB:		return 0;
		case RamSize::Size_8kB:		return 0;
		case RamSize::Size_32kB:	return 4;
		case RamSize::Size_128kB:	return 16;
		case RamSize::Size_64kB:	return 8;
		}

		// @todo error.
		return 0;
	}

	const char* RamSize::GetString(Type type)
	{
		switch (type)
		{
		case RamSize::None:			return "None";
		case RamSize::Size_2kB:		return "  2 KB";
		case RamSize::Size_8kB:		return "  8 KB";
		case RamSize::Size_32kB:	return " 32 KB [ 4 banks]";
		case RamSize::Size_128kB:	return "128 KB [16 banks]";
		case RamSize::Size_64kB:	return " 64 KB [ 8 banks]";
		}
		return "UnknownRamSize";
	}

	const char* DestinationCode::GetString(Type type)
	{
		switch (type)
		{
		case DestinationCode::Japanese:		return "Japanese";
		case DestinationCode::NonJapanese:	return "Non-Japanese";
		}
		return "UnknownDestinationCode";
	}

	const char* LicenseeCodeOld::GetString(Type type)
	{
		switch (type)
		{
		case LicenseeCodeOld::CheckNew:		return "Use new licensee";
		case LicenseeCodeOld::Accolade:		return "Accolade";
		case LicenseeCodeOld::Konami:		return "Konami";
		}
		return "UnknownLicenseeCodeOld";
	}

	const char* HWInterrupts::GetString(Type interrupt)
	{
		switch (interrupt)
		{
		case HWInterrupts::VBlank:	return "VBlank";
		case HWInterrupts::Stat:	return "Stat";
		case HWInterrupts::Timer:	return "Timer";
		case HWInterrupts::Serial:	return "Serial";
		case HWInterrupts::Button:	return "Button";
		}
		return "UnknownInterruptType";
	}

	Byte HWLCDC::GetTilePatternIndex(Byte lcdc)
	{
		return ((lcdc & TilePattern) == 0) ? 0 : 1;
	}

	Address HWLCDC::GetTilePatternAddress(Byte index)
	{
		return (index == 0) ? Addresses::TilePattern0 : Addresses::TilePattern1;
	}

	Address HWLCDC::GetBGTileMapAddress(Byte lcdc)
	{
		return ((lcdc & BGTileMap) == 0) ? TileMap0 : TileMap1;
	}

	Address HWLCDC::GetWindowTileMapAddress(Byte lcdc)
	{
		return ((lcdc & WindowTileMap) == 0) ? TileMap0 : TileMap1;
	}

	bool HWLCDC::GetBGEnabled(Byte lcdc)
	{
		return ((lcdc & BGEnabled) != 0);
	}

	bool HWLCDC::GetWindowEnabled(Byte lcdc)
	{
		return ((lcdc & WindowEnabled) != 0);
	}

	bool HWLCDC::GetSpriteEnabled(Byte lcdc)
	{
		return ((lcdc & SpriteEnabled) != 0);
	}

	bool HWLCDC::GetSpriteDoubleHeight(Byte lcdc)
	{
		return ((lcdc & SpriteSize) != 0);
	}
}