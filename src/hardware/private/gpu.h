#pragma once

#include "context.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	struct GPUTileData
	{
		bool m_bDirty;
		Byte m_pixels[8][8];	// Row major, indexed through y,x
	};

	//--------------------------------------------------------------------------

	class GPUTilePattern
	{
	public:
		GPUTilePattern();

		void reset();
		void check_dirty_tiles(MMU_ptr mmu);
		void check_dirty_tile(Byte tileIndex, MMU_ptr mmu);

		inline const Byte* get_tile_line(Byte tileIndex, const Byte tileLineY) const;
		inline void set_dirty_tile(Byte tileIndex);

		bool			m_bSignedIndex;
		Address			m_baseAddress;
		GPUTileData		m_tiles[256];
	};

	//--------------------------------------------------------------------------

	inline const Byte* GPUTilePattern::get_tile_line(Byte tileIndex, const Byte tileLineY) const
	{
		return m_tiles[m_bSignedIndex ? (tileIndex ^ 0x80) : tileIndex].m_pixels[tileLineY];
	}

	inline void GPUTilePattern::set_dirty_tile(Byte tileIndex)
	{
		m_tiles[tileIndex].m_bDirty = true;
	}

	//--------------------------------------------------------------------------

	class GPUSpriteData
	{
	public:
		inline GPUSpriteData();
		inline void reset();

		Byte x;
		Byte y;
		Byte tile;
		Byte flags;
	};

	//--------------------------------------------------------------------------

	inline GPUSpriteData::GPUSpriteData()
	{
		reset();
	}

	inline void GPUSpriteData::reset()
	{
		x		= 0;
		y		= 0;
		tile	= 0;
		flags	= 0;
	}

	//--------------------------------------------------------------------------

	struct GPUPixel
	{
		Byte x;
		Byte r;
		Byte g;
		Byte b;
	};

	//--------------------------------------------------------------------------

	struct GPUPaletteColour
	{
		GPUPixel	pixel;
		Byte		values[2];
	};

	struct GPUPalette
	{
		void reset();

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
		void reset();

		static const uint32_t kTileDataBankCount		= 2;
		static const uint32_t kTileDataCount			= 384;	// Addressable range is 0x8000->0x97FF
		static const uint32_t kTileMapWidth				= 32;
		static const uint32_t kTileMapHeight			= 32;
		static const uint32_t kTileMapSize				= kTileMapWidth * kTileMapHeight;
		static const uint32_t kTileMapCount				= 2;

		inline void get_tilemap_row(Byte index, Byte y, Byte** map, GPUTileAttributes** attr)
		{
			const Word offset = y * kTileMapWidth;
			*map = &tileMap[index][offset];
			*attr = &tileAttr[index][offset];
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

		void initialise(CPU_ptr cpu, MMU_ptr mmu);
		void release();

		void update(uint32_t cycles);
		void reset();

		void set_lcdc(Byte val);
		bool reset_vblank_notify();
		const Byte* get_screen_data() const;
		const GPUTilePattern* get_tile_pattern(Byte index);


		// New refactor to manage tile-data.
		void set_tile_ram_data(Address vramAddress, Byte data);
		void set_tile_ram_bank(Byte bank);
		inline const GPUTileRam* get_tile_ram() const;

		void set_palette(GPUPalette::Type type, Byte index, Byte value);
		inline const GPUPalette* get_palette(GPUPalette::Type type);












		void update_sprite_data(const Address spriteDataAddress, Byte value);
		void update_palette(const Address hwAddress, Byte palette);


		void update_tile_pattern_line(const Address tilePatternAddress, Byte value);

		static const uint32_t kScreenWidth			= 160;
		static const uint32_t kScreenHeight			= 144;
		static const uint32_t kTilePatternWidth		= 256;
		static const uint32_t kTilePatternHeight	= 256;
		static const uint32_t kTilePatternCount		= 256;
		static const uint32_t kTileSize				= 8;
		static const uint32_t kPaletteSize			= 64;

	private:

		void scan_line(Byte line);
		void scan_line_bg_tilemap();



		void update_tile_pattern_line(Byte patternIndex, const Byte tileIndex, const Byte tileLineY, const Byte lineLow, const Byte lineHigh);
		Byte update_lcdc_status_mode(Byte stat, HWLCDCStatus::Type mode, HWLCDCStatus::Type interrupt);

		void draw_scan_line(Byte line);
		void draw_scan_line_bg_tilemap(const Address tileMapAddress, const Byte scrollX, const Byte scrollY);
		void draw_scan_line_window_tilemap(const Address tileMapAddress, const int16_t windowX);
		void draw_scan_line_sprite();

		inline GPUPixel get_palette_colour(const Byte palette, const Byte paletteIndex) const;
		inline void update_scan_line_sprites();

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

		CPU_ptr					m_cpu;
		MMU_ptr					m_mmu;
		Mode::Enum				m_mode;
		uint32_t				m_modeCycles;
		GPUPixel				m_screenData[kScreenWidth * kScreenHeight];

		GPUTileRam				m_tileRam;




		GPUPixel				m_bgPalette[kPaletteSize];
		GPUPixel				m_spritePalette[kPaletteSize];

		bool					m_bVBlankNotify;

		Byte					m_lcdc;
		Byte					m_currentScanLine;
		Byte					m_windowPosY;
		Byte					m_windowReadY;

		std::vector<Byte>		m_scanLineSprites;

		GPUPalette				m_palette[GPUPalette::Count];

		Byte					m_paletteData[3][4];

		GPUTilePattern			m_tilePatterns[2];

		GPUSpriteData			m_spriteData[40];


		// @todo: palette data
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

	inline GPUPixel GPU::get_palette_colour(const Byte palette, const Byte paletteIndex) const
	{
		Byte value = m_paletteData[palette][paletteIndex & 0x03];

		// @todo: Temporary, once colour support is added this will be changed.
		value = (3 - value) * 85;
		return { 255, value, value, value };
	}

	inline void GPU::update_scan_line_sprites()
	{
		m_scanLineSprites.clear();

		const Byte spriteHeight = HWLCDC::get_sprite_double_height(m_lcdc) ? 16 : 8;

		for (Byte spriteIndex = 0; spriteIndex < 40; ++spriteIndex)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			const Byte		x			= sprite->x;
			const Byte		y			= sprite->y - 16;

			if ((x == 0 || x >= 168 || y == 0 || y >= 144) ||									// Visibility check
				(m_currentScanLine < y || m_currentScanLine > (y + (spriteHeight - 1))))		// Scanline check
			{
				continue;
			}

			// Just stop once we achieve goal.
			m_scanLineSprites.push_back(spriteIndex);

// 			if (m_scanLineSprites.size() == 10)
// 			{
// 				// @todo Priorities based upon position and address.
// 				break;
// 			}
		}

		// Currently just reverse the priorities, such that the left most is drawn last
		// and on-top of everything else.
// 		if (m_scanLineSprites.size() > 0)
// 		{
// 			std::reverse(m_scanLineSprites.begin(), m_scanLineSprites.end());
// 		}
	}

	//--------------------------------------------------------------------------
}