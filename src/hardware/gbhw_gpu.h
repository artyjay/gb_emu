#pragma once

#include "gbhw_cpu.h"

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
		void DirtyTileCheckAll(MMU* mmu);
		void DirtyTileCheck(Byte tileIndex, MMU* mmu);

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

		void Initialise(CPU* cpu, MMU* mmu);

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

		CPU*					m_cpu;
		MMU*					m_mmu;
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

} // gbhw

#include "gbhw_gpu.inl"