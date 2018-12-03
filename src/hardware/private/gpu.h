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

		void update_sprite_data(const Address spriteDataAddress, Byte value);
		void update_palette(const Address hwAddress, Byte palette);
		void update_tile_pattern_line(const Address tilePatternAddress, Byte value);

		static const uint32_t kScreenWidth			= 160;
		static const uint32_t kScreenHeight			= 144;
		static const uint32_t kTilePatternWidth		= 256;
		static const uint32_t kTilePatternHeight	= 256;
		static const uint32_t kTilePatternCount		= 256;
		static const uint32_t kTileSize				= 8;

	private:
		void update_tile_pattern_line(Byte patternIndex, const Byte tileIndex, const Byte tileLineY, const Byte lineLow, const Byte lineHigh);
		Byte update_lcdc_status_mode(Byte stat, HWLCDCStatus::Type mode, HWLCDCStatus::Type interrupt);

		void draw_scan_line(Byte line);
		void draw_scan_line_bg_tilemap(const Address tileMapAddress, const Byte scrollX, const Byte scrollY);
		void draw_scan_line_window_tilemap(const Address tileMapAddress, const int16_t windowX);
		void draw_scan_line_sprite();

		inline Byte get_palette_colour(const Byte palette, const Byte paletteIndex) const;
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
		Byte					m_screenData[kScreenWidth * kScreenHeight];

		bool					m_bVBlankNotify;

		Byte					m_lcdc;
		Byte					m_currentScanLine;
		Byte					m_windowPosY;
		Byte					m_windowReadY;

		std::vector<Byte>		m_scanLineSprites;

		Byte					m_paletteData[3][4];

		GPUTilePattern			m_tilePatterns[2];

		GPUSpriteData			m_spriteData[40];


		// @todo: palette data
	};

	//--------------------------------------------------------------------------


	inline Byte GPU::get_palette_colour(const Byte palette, const Byte paletteIndex) const
	{
		return m_paletteData[palette][paletteIndex & 0x03];
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