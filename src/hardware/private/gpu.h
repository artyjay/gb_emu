#pragma once

#include "types.h"

namespace gbhw
{
	class CPU;
	class MMU;

	//--------------------------------------------------------------------------

	struct GPUPixel
	{
#if EMSCRIPTEN
		Byte r;
		Byte g;
		Byte b;
		Byte x;
#else
		Byte x;
		Byte r;
		Byte g;
		Byte b;
#endif
	};

	//--------------------------------------------------------------------------

	struct GPUSpriteData
	{
		inline GPUSpriteData() : x(0), y(0), tile(0), flags(0) {}

		Byte x;
		Byte y;
		Byte tile;
		Byte flags;
	};

	//--------------------------------------------------------------------------

	struct GPUPaletteColour
	{
		GPUPixel	pixel;
		Byte		values[2];
	};

	struct GPUPalette
	{
		GPUPalette();

		enum Type
		{
			BG = 0,
			Sprite,
			Count
		};

		GPUPaletteColour entries[8][4];
	};

	//--------------------------------------------------------------------------

	struct GPUTile
	{
		Byte pixels[8][8];	// Row major, indexed through y,x
	};

	struct GPUTileAttributes
	{
		inline GPUTileAttributes(Byte data = 0)
		{
			palette		= data & 0x07;
			bank		= (data >> 3) & 0x01;
			hFlip		= static_cast<bool>((data >> 5) & 0x01);
			vFlip		= static_cast<bool>((data >> 6) & 0x01);
			priority	= static_cast<bool>((data >> 7) & 0x01);
		}

		Byte palette;
		Byte bank;
		bool hFlip;
		bool vFlip;
		bool priority;
	};

	struct GPUTileRam
	{
		GPUTileRam();

		static const uint32_t kTileDataBankCount		= 2;
		static const uint32_t kTileDataCount			= 384;					// Addressable range is 0x8000->0x97FF
		static const uint32_t kTileMapWidth				= 32;
		static const uint32_t kTileMapHeight			= 32;
		static const uint32_t kTileMapSize				= kTileMapWidth * kTileMapHeight;
		static const uint32_t kTileMapCount				= 2;

		inline void get_tilemap_row(Byte index, Byte y, Byte** map, GPUTileAttributes** attr)
		{
			const Word offset = y * kTileMapWidth;
			*map	= &tileMap[index][offset];
			*attr	= &tileAttr[index][offset];
		}

		inline const Byte* get_tiledata_row(Byte bank, Word index, Byte y) const
		{
			return &tileData[bank][index].pixels[y][0];
		}

		Byte				bank = 0;
		GPUTile				tileData[kTileDataBankCount][kTileDataCount];		// Banked for read & write.
		Byte				tileMap[kTileMapCount][kTileMapSize];				// Only written when bank = 0
		GPUTileAttributes	tileAttr[kTileMapCount][kTileMapSize];				// Only written when bank = 1. Should only be used for GBC rom.
	};

	//--------------------------------------------------------------------------

	class GPU
	{
	public:
		GPU();
		~GPU();

		void initialise(CPU* cpu, MMU* mmu);
		void update(uint32_t cycles);

		void set_lcdc(Byte val);
		bool reset_vblank_notify();
		const Byte* get_screen_data() const;

		// Tile Ram
		void set_tile_ram_data(Address vramAddress, Byte data);
		void set_tile_ram_bank(Byte bank);
		inline const GPUTileRam* get_tile_ram() const;

		// Sprite
		void set_sprite_data(const Address spriteAddress, Byte value);

		// Palette
		void set_palette(GPUPalette::Type type, Byte index, Byte value);
		inline const GPUPalette* get_palette(GPUPalette::Type type);

		static const uint32_t kScreenWidth	= 160;
		static const uint32_t kScreenHeight	= 144;

	private:
		Byte update_lcdc_status_mode(Byte stat, HWLCDCStatus::Type mode, HWLCDCStatus::Type interrupt);

		void scan_line(Byte line);
		void scan_line_bg();
		void scan_line_window();
		void scan_line_sprite();

		struct Mode
		{
			enum Enum
			{
				ScanlineOAM,
				ScanlineVRAM,
				HBlank,
				VBlank
			};
		};

		CPU*					m_cpu;
		MMU*					m_mmu;
		Mode::Enum				m_mode;
		uint32_t				m_modeCycles;
		bool					m_bVBlankNotify;
		Byte					m_lcdc;
		Byte					m_currentScanLine;
		Byte					m_windowPosY;
		Byte					m_windowReadY;
		GPUPixel*				m_screenData;
		GPUTileRam				m_tileRam;
		std::vector<Byte>		m_scanLineSprites;
		GPUSpriteData			m_spriteData[40];
		GPUPalette				m_palette[GPUPalette::Count];
	};

	//--------------------------------------------------------------------------

	inline const GPUTileRam* GPU::get_tile_ram() const
	{
		return &m_tileRam;
	}

	inline const GPUPalette* GPU::get_palette(GPUPalette::Type type)
	{
		return &m_palette[type];
	}

	//--------------------------------------------------------------------------
}