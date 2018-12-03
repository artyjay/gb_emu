#include "gpu.h"
#include "cpu.h"
#include "mmu.h"
#include "log.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	namespace
	{
		static const uint32_t kScanlineReadOAMCycles	= 80;
		static const uint32_t kScanlineReadVRAMCycles	= 172;
		static const uint32_t kHBlankCycles				= 204;
	}

	//--------------------------------------------------------------------------

	GPUTilePattern::GPUTilePattern()
		: m_bSignedIndex(false)
		, m_baseAddress(0)
	{
		reset();
	}

	void GPUTilePattern::reset()
	{
		memset(m_tiles, 0, sizeof(GPUTileData) * 256);
	}

	void GPUTilePattern::check_dirty_tiles(MMU_ptr mmu)
	{
		for (int32_t i = 0; i < 256; ++i)
		{
			check_dirty_tile(i, mmu);
		}
	}

	void GPUTilePattern::check_dirty_tile(Byte tileIndex, MMU_ptr mmu)
	{
		GPUTileData& tileData = m_tiles[tileIndex];
		Address tileAddress = m_baseAddress + (tileIndex * 16);

		if (tileData.m_bDirty)
		{
			for (int32_t y = 0; y < 8; ++y)
			{
				Address lineAddress = tileAddress + (y * 2);

				Byte lineLow = mmu->read_byte(lineAddress);
				Byte lineHigh = mmu->read_byte(lineAddress + 1);
				Byte* tileLinePixels = tileData.m_pixels[y];

				for (Byte tilePixelX = 0; tilePixelX < 8; ++tilePixelX)
				{
					const Byte pixelBit = 7 - tilePixelX;
					const Byte pixelMask = 1 << pixelBit;

					Byte pixelLow = (lineLow & pixelMask) >> pixelBit;
					Byte pixelHigh = (lineHigh & pixelMask) >> pixelBit;

					tileLinePixels[tilePixelX] = (pixelLow | (pixelHigh << 1));
				}
			}

			tileData.m_bDirty = false;
		}
	}

	//--------------------------------------------------------------------------

	GPU::GPU()
	{
		// Setup tile patterns.
		m_tilePatterns[0].m_bSignedIndex = true;
		m_tilePatterns[0].m_baseAddress = HWLCDC::Addresses::TilePattern0;
		m_tilePatterns[1].m_bSignedIndex = false;
		m_tilePatterns[1].m_baseAddress = HWLCDC::Addresses::TilePattern1;

		reset();
	}

	void GPU::initialise(CPU_ptr cpu, MMU_ptr mmu)
	{
		m_cpu = cpu;
		m_mmu = mmu;
	}

	void GPU::release()
	{
		m_cpu = nullptr;
		m_mmu = nullptr;
	}

	void GPU::update(uint32_t cycles)
	{
		Byte ly		= m_mmu->read_io(HWRegs::LY);
		Byte stat	= m_mmu->read_io(HWRegs::Stat);
		Byte lcdc	= m_mmu->read_io(HWRegs::LCDC);

		// LCD if off.
		if((lcdc & 0x80) == 0)
		{
// 			switch(m_mode)
// 			{
// 				case Mode::ScanlineOAM:
// 				{
// 					// Apply OAM mode to LCDC status.
// 					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeOAM, HWLCDCStatus::InterruptOAM);
//
// 					if(m_modeCycles >= kScanlineReadOAMCycles)
// 					{
// 						m_modeCycles -= kScanlineReadOAMCycles;
// 						m_mode = Mode::ScanlineVRAM;
// 					}
//					break;
// 				}
// 				case Mode::ScanlineVRAM:
// 				{
// 					// Apply OAM VRAM mode to LCDC status, no interrupt needed.
// 					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeOAMVRam, HWLCDCStatus::InterruptNone);
//
// 					if(m_modeCycles >= kScanlineReadVRAMCycles)
// 					{
// 						m_modeCycles -= kScanlineReadVRAMCycles;
// 						m_mode = Mode::HBlank;
// 					}
//					break;
// 				}
// 				case Mode::HBlank:
// 				{
// 					// Apply HBlank mode to LCDC status.
// 					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeHBlank, HWLCDCStatus::InterruptHBlank);
//
// 					if(m_modeCycles >= kHBlankCycles)
// 					{
// 						// Next scanline.
// 						m_modeCycles -= kHBlankCycles;
// 						m_mode = Mode::ScanlineOAM;
// 					}
//					break;
// 				}
// 			}
		}
		else
		{
			m_modeCycles += cycles;

			if (m_modeCycles > 400)
			{
				log_warning("GPU is lagging quite badly, this needs some re-thinking\n");
			}

			// Display is on
			switch (m_mode)
			{
				case Mode::ScanlineOAM:
				{
					stat = update_lcdc_status_mode(stat, HWLCDCStatus::ModeOAM, HWLCDCStatus::InterruptOAM);

					if(m_modeCycles >= kScanlineReadOAMCycles)
					{
						m_modeCycles -= kScanlineReadOAMCycles;
						m_mode = Mode::ScanlineVRAM;
					}
					break;
				}
				case Mode::ScanlineVRAM:
				{
					stat = update_lcdc_status_mode(stat, HWLCDCStatus::ModeOAMVRam, HWLCDCStatus::InterruptNone);

					if(m_modeCycles >= kScanlineReadVRAMCycles)
					{
						// Draw the current scan-line now.
						draw_scan_line(ly);

						m_modeCycles -= kScanlineReadOAMCycles;
						m_mode = Mode::HBlank;
					}
					break;
				}
				case Mode::HBlank:
				{
					stat = update_lcdc_status_mode(stat, HWLCDCStatus::ModeHBlank, HWLCDCStatus::InterruptHBlank);

					if(m_modeCycles > kHBlankCycles)
					{
						m_modeCycles -= kHBlankCycles;

						// Move down a line.
						++ly;

						if(ly >= 144)
						{
							// ly = 144 -> 153 indicates v-blank period.
							m_mode = Mode::VBlank;
							m_bVBlankNotify = true;
						}
						else
						{
							// Start new scanline.
							m_mode = Mode::ScanlineOAM;
						}

						// @todo: HBlank DMA for GBC?
					}
					break;
				}
				case Mode::VBlank:
				{
					stat = update_lcdc_status_mode(stat, HWLCDCStatus::ModeVBlank, HWLCDCStatus::InterruptVBlank);

					if(m_modeCycles >= kHBlankCycles)
					{
						m_modeCycles -= kHBlankCycles;

						// Move down a line.
						++ly;

						if(ly == 154)
						{
							// VBlank has finished now.
							ly = 0;
							m_mode = Mode::ScanlineOAM;
						}
					}
					break;
				}
			}
		}

		m_mmu->write_io(HWRegs::LY, ly);
		m_mmu->write_io(HWRegs::Stat, stat);
	}

	void GPU::reset()
	{
		m_mode				= Mode::ScanlineOAM;
		m_modeCycles		= 0;
		m_bVBlankNotify		= false;
		m_scanLineSprites.reserve(10);
	}

	void GPU::set_lcdc(Byte val)
	{
		// todo:
		if((val & 0x80) != (m_mmu->read_io(HWRegs::LCDC) & 0x80))
		{
			if(val & 0x80)
			{
				//gbhw::Message("Turned display on\n");
			}
			else
			{
				//gbhw::Message("Turned display off\n");
			}

// 			m_cpu->WriteIO(HWRegs::LY, 0);
// 			m_mode = Mode::ScanlineOAM;
		}
	}

	bool GPU::reset_vblank_notify()
	{
		bool bRes = m_bVBlankNotify;
		m_bVBlankNotify = false;
		return bRes;
	}

	const Byte* GPU::get_screen_data() const
	{
		return m_screenData;
	}

	const GPUTilePattern* GPU::get_tile_pattern(Byte index)
	{
		return &m_tilePatterns[index];
	}

	void GPU::update_sprite_data(const Address spriteDataAddress, Byte value)
	{
		Byte spriteIndex = (spriteDataAddress - HWLCDC::Addresses::SpriteData) >> 2;

		if(spriteIndex < 40)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			switch(spriteDataAddress % 4)
			{
				case 0: sprite->y = value; break;
				case 1: sprite->x = value; break;
				case 2: sprite->tile = value; break;
				case 3: sprite->flags = value; break;
				default: log_error("Invalid sprite data address\n"); break;
			}
		}
		else
		{
			log_error("Invalid sprite index specified: %d\n", spriteIndex);
		}
	}

	void GPU::update_palette(const Address hwAddress, Byte palette)
	{
		//Byte					m_paletteData[3][4];
		// @todo...
		Address index = hwAddress - HWRegs::BGP;

		for (Byte i = 0; i < 4; i++)
		{
			m_paletteData[index][i] = (palette >> (i * 2)) & 3;
		}
	}

	void GPU::update_tile_pattern_line(const Address tilePatternAddress, Byte value)
	{
		if (tilePatternAddress >= HWLCDC::TilePattern0 && tilePatternAddress < 0x9000)
		{
			// in both patterns due to overlap.
			m_tilePatterns[0].set_dirty_tile((tilePatternAddress - HWLCDC::TilePattern0) >> 4);
			m_tilePatterns[1].set_dirty_tile((tilePatternAddress - HWLCDC::TilePattern1) >> 4);
		}
		else if (tilePatternAddress >= HWLCDC::TilePattern0)
		{
			// in 0 pattern
			m_tilePatterns[0].set_dirty_tile((tilePatternAddress - HWLCDC::TilePattern0) >> 4);
		}
		else if (tilePatternAddress >= HWLCDC::TilePattern1)
		{
			// in 1 pattern
			m_tilePatterns[1].set_dirty_tile((tilePatternAddress - HWLCDC::TilePattern1) >> 4);
		}
		else
		{
			log_error("Invalid tile pattern address [0x%04x]\n", tilePatternAddress);
		}
	}

	void GPU::update_tile_pattern_line(Byte patternIndex, const Byte tileIndex, const Byte tileLineY, const Byte lineLow, const Byte lineHigh)
	{
		Byte* tileLinePixels = m_tilePatterns[patternIndex].m_tiles[tileIndex].m_pixels[tileLineY];

		for (Byte tilePixelX = 0; tilePixelX < 8; ++tilePixelX)
		{
			const Byte pixelBit = 7 - tilePixelX;
			const Byte pixelMask = 1 << pixelBit;

			Byte pixelLow = (lineLow & pixelMask) >> pixelBit;
			Byte pixelHigh = (lineHigh & pixelMask) >> pixelBit;

			tileLinePixels[tilePixelX] = (pixelLow | (pixelHigh << 1));
		}
	}

	Byte GPU::update_lcdc_status_mode(Byte stat, HWLCDCStatus::Type mode, HWLCDCStatus::Type interrupt)
	{
		if((stat & HWLCDCStatus::ModeMask) != mode)
		{
			// Apply the mode to the status, retaining the rest of the bits.
			stat = (stat & (~HWLCDCStatus::ModeMask)) | mode;

			// Generate a CPU interrupt if needed.
			if((interrupt != HWLCDCStatus::InterruptNone) && (stat & interrupt) != 0)
			{
				m_cpu->generate_interrupt(HWInterrupts::Stat);
			}

			// Special case for VBlank, always generate a VBlank interrupt
			// when we enter into vBlank.
			if(mode == HWLCDCStatus::ModeVBlank)
			{
				m_cpu->generate_interrupt(HWInterrupts::VBlank);
			}
		}

		return stat;
	}

	void GPU::draw_scan_line(Byte line)
	{
		if (line >= kScreenHeight)
		{
			return;
		}

		// Store state
		m_lcdc				= m_mmu->read_io(HWRegs::LCDC);
		m_currentScanLine	= line;

		// This is only allowed to be modified on screen refresh, so cache first scanline
		if (m_currentScanLine == 0)
		{
			m_windowPosY = m_mmu->read_io(HWRegs::WindowY);
			m_windowReadY = 0;	// Reset this. Window drawing will resume drawing from where it last read when disabled between h-blanks.
		}

		// @todo: More efficient check for dirty tiles.
		m_tilePatterns[0].check_dirty_tiles(m_mmu);
		m_tilePatterns[1].check_dirty_tiles(m_mmu);

		if (HWLCDC::get_bg_enabled(m_lcdc))
		{
			draw_scan_line_bg_tilemap(HWLCDC::get_bg_tile_map_address(m_lcdc), m_mmu->read_io(HWRegs::ScrollX), m_mmu->read_io(HWRegs::ScrollY));
		}

		if (HWLCDC::get_window_enabled(m_lcdc))
		{
			// Can be modified between interrupts.
			Byte windowX = m_mmu->read_io(HWRegs::WindowX);

			// Only draw on this scanline when visible.
			if (windowX <= 166 && m_currentScanLine >= m_windowPosY)
			{
				draw_scan_line_window_tilemap(HWLCDC::get_window_tile_map_address(m_lcdc), windowX - 7);
			}
		}

		if (HWLCDC::get_sprite_enabled(m_lcdc))
		{
			draw_scan_line_sprite();
		}
	}

	void GPU::draw_scan_line_bg_tilemap(const Address tileMapAddress, const Byte scrollX, const Byte scrollY)
	{
		// Setup basics.
		const Address	screenAddr			= m_currentScanLine * kScreenWidth;
		Byte			tileX				= scrollX % 8;															// X-coordinate within the tile to start off with.
		Byte			tileY				= (m_currentScanLine + scrollY) % 8;									// Y-coordinate within the tile

		// Calculate tile map/pattern addresses.
		const Byte		tilePatternIndex	= HWLCDC::get_tile_pattern_index(m_lcdc);
		const Address	tileMapBase			= tileMapAddress + ((((m_currentScanLine + scrollY) % 256) >> 3) << 5);	// Get line of tiles to use. base + (mapline / 8) * 32.
		Byte			tileMapX			= scrollX >> 3;															// Get first tile to use. offsetx / 8.

		// Load first tile in the line.
		Byte tileIndex = m_mmu->read_byte(tileMapBase + tileMapX);
		const Byte* tileLine = m_tilePatterns[tilePatternIndex].get_tile_line(tileIndex, tileY);

		for (uint32_t screenX = 0; screenX < kScreenWidth; ++screenX)
		{
			m_screenData[screenAddr + screenX] = get_palette_colour(0, tileLine[tileX]);

			if (tileX++ == 7)
			{
				// Move across the tile source x, catching end of tile.
				tileX = 0;
				tileMapX = (tileMapX + 1) & 0x1f;	// Wrap tile x to 0->31.
				tileIndex = m_mmu->read_byte(tileMapBase + tileMapX);
				tileLine = m_tilePatterns[tilePatternIndex].get_tile_line(tileIndex, tileY);
			}
		}
	}

	void GPU::draw_scan_line_window_tilemap(const Address tileMapAddress, const int16_t windowX)
	{
		// Setup basics.
		const Address	screenAddr = m_currentScanLine * kScreenWidth;
		Byte			tileX = 0;														// X-coordinate within the tile to start off with.
		Byte			tileY = (m_windowReadY) % 8;									// Y-coordinate within the tile

		// Calculate tile map/pattern addresses.
		const Byte		tilePatternIndex = HWLCDC::get_tile_pattern_index(m_lcdc);
		const Address	tileMapBase = tileMapAddress + ((m_windowReadY >> 3) << 5);		// Get line of tiles to use. base + (mapline / 8) * 32.
		Byte			tileMapX = 0;													// Get first tile to use. offsetx / 8.

		m_windowReadY++;

																						// Load first tile in the line.
		Byte tileIndex = m_mmu->read_byte(tileMapBase + tileMapX);
		const Byte* tileLine = m_tilePatterns[tilePatternIndex].get_tile_line(tileIndex, tileY);

		for (int16_t screenX = windowX; screenX < (int16_t)kScreenWidth; ++screenX)
		{
			if (screenX < 0)
				continue;

			m_screenData[screenAddr + screenX] = get_palette_colour(0, tileLine[tileX]);

			if (tileX++ == 7)
			{
				// Move across the tile source x, catching end of tile.
				tileX = 0;
				tileMapX++;// = (tileMapX + 1) & 0x1f;	// Wrap tile x to 0->31.
				tileIndex = m_mmu->read_byte(tileMapBase + tileMapX);
				tileLine = m_tilePatterns[tilePatternIndex].get_tile_line(tileIndex, tileY);
			}
		}
	}

	void GPU::draw_scan_line_sprite()
	{
		const Address	screenAddr			= m_currentScanLine * kScreenWidth;
		const Byte		tilePatternIndex	= HWLCDC::get_tile_pattern_index(m_lcdc);

		// Find the sprites for this scanline
		update_scan_line_sprites();

		// Actually draw each visible sprite
		for(auto& spriteIndex : m_scanLineSprites)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			int16_t spriteX	= sprite->x - 8;
			int16_t spriteY	= sprite->y - 16;
			Byte tileIndex	= sprite->tile;
			Byte flags		= sprite->flags;

			// @todo palette & priority....

			bool bFlipX = (flags & HWSpriteFlags::FlipX) != 0;
			bool bFlipY = (flags & HWSpriteFlags::FlipY) != 0;
			Byte palette = ((flags & HWSpriteFlags::Palette) ? 1 : 0) + 1;	// Palette data for all 3 palettes starts with the bg palette.

			// Always from tile pattern 1 (i.e. 0x8000)
			const Byte tileY = m_currentScanLine - spriteY;
			const Byte tileYIndex = bFlipY ? (HWLCDC::get_sprite_double_height(m_lcdc) ? (15 - tileY) : (7 - tileY)) : tileY;
			const Byte tileExtra = (HWLCDC::get_sprite_double_height(m_lcdc) && (tileYIndex > 7)) ? 1 : 0;

			if(tileY >= (HWLCDC::get_sprite_double_height(m_lcdc) ? 16 : 8))
			{
				log_error("Tile line is invalid");
			}

			const Byte* tileLine = m_tilePatterns[1].get_tile_line(tileIndex + tileExtra, tileYIndex & 0x7);

			Byte tileXIndex;
			Byte tileX = (spriteX < 0) ? (-spriteX) : 0;

			// Read the line data
			for (; tileX < 8; tileX++)
			{
				if(bFlipX)
				{
					tileXIndex = 7 - tileX;
				}
				else
				{
					tileXIndex = tileX;
				}

				Byte colour = tileLine[tileXIndex];

				if (colour)
				{
					m_screenData[screenAddr + spriteX + tileX] = get_palette_colour(palette, colour);
				}
			}
		}
	}
}