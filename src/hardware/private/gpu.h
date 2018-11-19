#pragma once

#include "context.h"

namespace gbhw
{
	struct GPUTileData
	{
		bool m_bDirty;

		// Row major, indexed through y,x
		Byte m_pixels[8][8];
	};

	class GPUTilePattern
	{
	public:
		GPUTilePattern();

		void Reset();
		void DirtyTileCheckAll(MMU_ptr mmu);
		void DirtyTileCheck(Byte tileIndex, MMU_ptr mmu);

		inline const Byte* GetTileLine(Byte tileIndex, const Byte tileLineY) const;
		inline void DirtyTile(Byte tileIndex);

		bool m_bSignedIndex;
		Address m_baseAddress;
		GPUTileData m_tiles[256];
	};

	class GPUSpriteData
	{
	public:
		inline GPUSpriteData();

		inline void Reset();

		inline Byte GetX() const;
		inline Byte GetY() const;
		inline Byte GetTileIndex() const;
		inline Byte GetFlags() const;

		inline void SetX(Byte x);
		inline void SetY(Byte y);
		inline void SetTile(Byte tile);
		inline void SetFlags(Byte flags);

	private:
		Byte m_x;
		Byte m_y;
		Byte m_tileIndex;
		Byte m_flags;
	};

	class GPU
	{
	public:
		GPU();

		void initialise(CPU_ptr cpu, MMU_ptr mmu);
		void release();

		void Update(uint32_t cycles);
		void Reset();

		void SetLCDC(Byte val);

		bool GetResetVBlankNotify();
		const Byte* GetScreenData() const;

		// Debug getters
		const GPUTilePattern* GetTilePattern(Byte index);

		// Update GPU data
		void UpdateSpriteData(const Address spriteDataAddress, Byte value);
		void UpdatePalette(const Address hwAddress, Byte palette);
		void UpdateTilePatternLine(const Address tilePatternAddress, Byte value);

		static const uint32_t kScreenWidth		= 160;
		static const uint32_t kScreenHeight		= 144;

		static const uint32_t kTilePatternWidth		= 256;
		static const uint32_t kTilePatternHeight	= 256;

		static const uint32_t kTilePatternCount = 256;
		static const uint32_t kTileSize = 8;

	private:

		void UpdateTilePatternLine(Byte patternIndex, const Byte tileIndex, const Byte tileLineY, const Byte lineLow, const Byte lineHigh);

		void DrawScanLine(Byte line);
		void DrawScanLineBGTileMap(const Address tileMapAddress, const Byte scrollX, const Byte scrollY);
		void DrawScanLineWindowTileMap(const Address tileMapAddress, const int16_t windowX);
		void DrawScanLineSprite();

		inline Byte GetPaletteColour(const Byte palette, const Byte paletteIndex) const;
		inline void UpdateScanLineSprites();

		void DrawSpriteScanLine(Byte line);

		void DrawTile(Byte tilePatternIndex, Byte screenX, Byte screenY, Byte line);

		struct Mode
		{
			enum Type
			{
				ScanlineOAM,
				ScanlineVRAM,
				HBlank,
				VBlank
			};
		};

		CPU_ptr					m_cpu;
		MMU_ptr					m_mmu;
		Mode::Type				m_mode;
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


		// @todo palette data
	};

		inline const Byte* GPUTilePattern::GetTileLine(Byte tileIndex, const Byte tileLineY) const
	{
		if (m_bSignedIndex)
		{
			tileIndex ^= 0x80;
		}

		return m_tiles[tileIndex].m_pixels[tileLineY];
	}

	inline void GPUTilePattern::DirtyTile(Byte tileIndex)
	{
		m_tiles[tileIndex].m_bDirty = true;
	}

	inline GPUSpriteData::GPUSpriteData()
	{
		Reset();
	}

	inline void GPUSpriteData::Reset()
	{
		m_x			= 0;
		m_y			= 0;
		m_tileIndex = 0;
		m_flags		= 0;
	}

	inline Byte GPUSpriteData::GetX() const
	{
		return m_x;
	}

	inline Byte GPUSpriteData::GetY() const
	{
		return m_y;
	}

	inline Byte GPUSpriteData::GetTileIndex() const
	{
		return m_tileIndex;
	}

	inline Byte GPUSpriteData::GetFlags() const
	{
		return m_flags;
	}

	inline void GPUSpriteData::SetX(Byte x)
	{
		m_x = x;
	}

	inline void GPUSpriteData::SetY(Byte y)
	{
		m_y = y;
	}

	inline void GPUSpriteData::SetTile(Byte tile)
	{
		m_tileIndex = tile;
	}

	inline void GPUSpriteData::SetFlags(Byte flags)
	{
		m_flags = flags;
	}

	inline Byte GPU::GetPaletteColour(const Byte palette, const Byte paletteIndex) const
	{
		return m_paletteData[palette][paletteIndex & 0x03];
	}

	inline void GPU::UpdateScanLineSprites()
	{
		m_scanLineSprites.clear();

		const Byte spriteHeight = HWLCDC::get_sprite_double_height(m_lcdc) ? 16 : 8;

		for (Byte spriteIndex = 0; spriteIndex < 40; ++spriteIndex)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			const Byte		x			= sprite->GetX();
			const Byte		y			= sprite->GetY();
			const int16_t	yProper		= y - 16;

			if ((x == 0 || x >= 168 || y == 0 || y >= 160) ||												// Visibility check
				(m_currentScanLine < yProper || m_currentScanLine > (yProper + (spriteHeight - 1))))		// Scanline check
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
}