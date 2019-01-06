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

	void GPUPalette::reset()
	{
		// Set to white.
		memset(entries, 0xFF, sizeof(GPUPaletteColour) * 8 * 4);
	}

	//--------------------------------------------------------------------------

	void GPUTileRam::reset()
	{
		bank = 0;
		memset(tileData, 0, sizeof(GPUTile) * kTileDataBankCount * kTileDataCount);
		memset(tileMap,  0, sizeof(Byte) * kTileMapCount * kTileMapSize);
		memset(tileAttr, 0, sizeof(GPUTileAttributes) * kTileMapCount * kTileMapSize);
	}

	//--------------------------------------------------------------------------

	GPU::GPU()
	{
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
#if 0
			// @todo: Figure out what the GPU needs to do when the display is turned off.
			switch(m_mode)
			{
				case Mode::ScanlineOAM:
				{
					// Apply OAM mode to LCDC status.
					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeOAM, HWLCDCStatus::InterruptOAM);

					if(m_modeCycles >= kScanlineReadOAMCycles)
					{
						m_modeCycles -= kScanlineReadOAMCycles;
						m_mode = Mode::ScanlineVRAM;
					}
					break;
				}
				case Mode::ScanlineVRAM:
				{
					// Apply OAM VRAM mode to LCDC status, no interrupt needed.
					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeOAMVRam, HWLCDCStatus::InterruptNone);

					if(m_modeCycles >= kScanlineReadVRAMCycles)
					{
						m_modeCycles -= kScanlineReadVRAMCycles;
						m_mode = Mode::HBlank;
					}
					break;
				}
				case Mode::HBlank:
				{
					// Apply HBlank mode to LCDC status.
					stat = UpdateLCDCStatusMode(m_cpu, stat, HWLCDCStatus::ModeHBlank, HWLCDCStatus::InterruptHBlank);

					if(m_modeCycles >= kHBlankCycles)
					{
						// Next scanline.
						m_modeCycles -= kHBlankCycles;
						m_mode = Mode::ScanlineOAM;
					}
					break;
				}
			}
#endif
		}
		else
		{
			m_modeCycles += cycles;

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
						scan_line(ly);
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

		m_tileRam.reset();

		for(auto& palette : m_palette)
			palette.reset();
	}

	void GPU::set_lcdc(Byte val)
	{
		// Display is toggled, reset scanline.
		if((val & 0x80) != (m_mmu->read_io(HWRegs::LCDC) & 0x80))
		{
 			m_mmu->write_io(HWRegs::LY, 0);
 			m_mode = Mode::ScanlineOAM;
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
		return reinterpret_cast<const Byte*>(m_screenData);
	}

	void GPU::set_tile_ram_data(Address vramAddress, Byte data)
	{
		if(vramAddress < 0x9800)
		{
			// Offset into 0->0x17FF range.
			vramAddress -= 0x8000;

			// Data contains 4 pixels worth of data, let's expand to internal representation.
			const uint32_t	tileIndex	= vramAddress >> 4;
			const uint32_t	tileByte	= vramAddress % 16;
			const uint32_t	tileRow		= tileByte >> 1;	// Each row is 2 bytes.
			const Byte		tileBit		= (tileByte % 2);	// Indicates which bit we're affecting in each pixel on this row.
															// As the pixels bits are interleaved across low/high bytes.

			// @todo: Investigate if this needs optimising.
			Byte* tileCol = &m_tileRam.tileData[m_tileRam.bank][tileIndex].pixels[tileRow][7];

			for(uint32_t i = 0; i < 8; ++i)
			{
				// Bits are reversed.
				*tileCol = (*tileCol & ~(tileBit + 1)) | ((data & 0x1) << tileBit);
				data >>= 1;

				tileCol--;
			}
		}
		else
		{
			// Figure out tile index and tile map index based upon address ranges
			// 9800 -> 9BFF = tilemap 0
			// 9C00 -> 9FFF = tilemap 1

			// Offset into 0->0X7FF range.
			vramAddress -= 0x9800;
			const uint32_t tilemapIndex = vramAddress >> 10;						// 11th bit set when >= 1024.
			const uint32_t tileIndex	= vramAddress & ~GPUTileRam::kTileMapSize;	// Mask off top-bit

			if(m_tileRam.bank == 0)
			{
				m_tileRam.tileMap[tilemapIndex][tileIndex] = data;
			}
			else
			{
				GPUTileAttributes attr(data);
				m_tileRam.tileAttr[tilemapIndex][tileIndex] = attr;
			}
		}
	}

	void GPU::set_tile_ram_bank(Byte bank)
	{
		m_tileRam.bank = bank;
	}

	void GPU::set_sprite_data(const Address spriteDataAddress, Byte value)
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

	inline Byte palette_colour_scale(Byte val)
	{
		// 0->63 to 0->255.
		// @todo: Colouring is probably non-linear, a LUT is probably going to be
		//		  the best solution.
		return ((val + 1) * 8) - 1;
	}

	void GPU::set_palette(GPUPalette::Type type, Byte index, Byte value)
	{
		const Byte entryIndex	= index >> 3;		// 8-bytes per entry.
		const Byte byteIndex	= index % 2;		// Low/high byte.
		const Byte colourIndex = (index % 8) >> 1;	// 4 colours per entry.

		// @todo: Do we need to store the values.
		GPUPaletteColour& entry = m_palette[type].entries[entryIndex][colourIndex];
		entry.values[byteIndex] = value;

		// Construct pixel colour.
		GPUPixel& pixel = entry.pixel;
		const Word val = entry.values[0] | (entry.values[1] << 8);
		pixel.x = 255;
		pixel.r = palette_colour_scale(val & 0x1f);
		pixel.g = palette_colour_scale((val >> 5) & 0x1f);
		pixel.b = palette_colour_scale((val >> 10) & 0x1f);
	}

	Byte GPU::update_lcdc_status_mode(Byte stat, HWLCDCStatus::Type mode, HWLCDCStatus::Type interrupt)
	{
		if ((stat & HWLCDCStatus::ModeMask) != mode)
		{
			// Apply the mode to the status, retaining the rest of the bits.
			stat = (stat & (~HWLCDCStatus::ModeMask)) | mode;

			// Generate a CPU interrupt if needed.
			if ((interrupt != HWLCDCStatus::InterruptNone) && (stat & interrupt) != 0)
			{
				m_cpu->generate_interrupt(HWInterrupts::Stat);
			}

			// Special case for VBlank, always generate a VBlank interrupt
			// when we enter into vBlank.
			if (mode == HWLCDCStatus::ModeVBlank)
			{
				m_cpu->generate_interrupt(HWInterrupts::VBlank);
			}
		}

		return stat;
	}

	void GPU::scan_line(Byte line)
	{
		if (line >= kScreenHeight)
			return;

		// Store state
		m_lcdc				= m_mmu->read_io(HWRegs::LCDC);
		m_currentScanLine	= line;

		// This is only allowed to be modified on screen refresh, so cache first scanline
		if (m_currentScanLine == 0)
		{
			m_windowPosY = m_mmu->read_io(HWRegs::WindowY);	// Is this true?
			m_windowReadY = 0;	// Reset this. Window drawing will resume drawing from where it last read when disabled between h-blanks.
		}

		if (HWLCDC::bg_enabled(m_lcdc))
		{
			scan_line_bg();
		}

		if (HWLCDC::window_enabled(m_lcdc))
		{
			scan_line_window();
		}

		if (HWLCDC::sprite_enabled(m_lcdc))
		{
			scan_line_sprite();
		}
	}

	void GPU::scan_line_bg()
	{
		const Address lineOffset = m_currentScanLine * kScreenWidth;

		// Setup basics.
		const Byte scrollX	= m_mmu->read_io(HWRegs::ScrollX);
		const Byte scrollY	= m_mmu->read_io(HWRegs::ScrollY);
		const Byte tileY	= (m_currentScanLine + scrollY) % 8;	// Y-coordinate within the tile
			  Byte tileX	= scrollX % 8;							// X-coordinate within the tile to start off with.

		// Calculate tile map/pattern addresses.
		const Byte		tileDataIndex		= HWLCDC::tile_data_index(m_lcdc);
		const Byte		tileMapIndex		= HWLCDC::bg_tile_map_index(m_lcdc);
		const Word		tileOffset			= tileDataIndex == 0 ? 128 : 0;					// Pattern 1 is signed.
		const Byte		tileMapY			= ((m_currentScanLine + scrollY) % 256) >> 3;	// Calc y tile this scan-line lands on. wrapped(scanline + scrollY) / 8
		Byte			tileMapX			= scrollX >> 3;									// Calc first x tile to use. scrollX / 8.

		// Grab tilemap line data.
		Byte*				mapRow	= nullptr;
		GPUTileAttributes*	attrRow = nullptr;
		m_tileRam.get_tilemap_row(tileMapIndex, tileMapY, &mapRow, &attrRow);

		GPUPaletteColour* colours = m_palette[GPUPalette::BG].entries[attrRow[tileMapX].palette];
		Byte tileIndex = mapRow[tileMapX];

		if(tileDataIndex == 0)
			tileIndex ^= 0x80;

		const Byte* tileRow = m_tileRam.get_tiledata_row(attrRow[tileMapX].bank, tileIndex + tileOffset, tileY);

		for (uint32_t screenX = 0; screenX < kScreenWidth; ++screenX)
		{
			m_screenData[lineOffset + screenX] = colours[tileRow[tileX]].pixel;

			if (tileX++ == 7)
			{
				// Move across the tile source x, catching end of tile.
				tileX = 0;
				tileMapX = (tileMapX + 1) & 0x1f;	// Wrap tile x to 0->31.

				tileIndex = mapRow[tileMapX];

				if(tileDataIndex == 0)
					tileIndex ^= 0x80;

				tileRow = m_tileRam.get_tiledata_row(attrRow[tileMapX].bank, tileIndex + tileOffset, tileY);
				colours = m_palette[GPUPalette::BG].entries[attrRow[tileMapX].palette];
			}
		}
	}

	void GPU::scan_line_window()
	{
		// @todo: Re-implement this properly. Can be modified between interrupts.
		Byte windowX = static_cast<SWord>(m_mmu->read_io(HWRegs::WindowX));

		// Only draw on this scanline when visible.
		if (windowX > 166 || m_windowPosY > 144)
			return;

		// Offset accordingly.
		windowX -= 7;

		// Setup basics.
		const Address	lineOffset = m_currentScanLine * kScreenWidth;
		Byte			tileX = 0;											// X-coordinate within the tile to start off with.
		Byte			tileY = m_windowReadY % 8;										// Y-coordinate within the tile

		// Calculate tile map/pattern addresses.

		static Byte dataidx = 0;
		static Byte mapidx = 0;
		const Byte		tileDataIndex		= dataidx; // HWLCDC::tile_data_index(m_lcdc);
		const Byte		tileMapIndex		= mapidx; // HWLCDC::window_tile_map_index(m_lcdc);
		const Word		tileOffset			= tileDataIndex == 0 ? 128 : 0;
		const Byte		tileMapY			= m_windowReadY >> 3;
		Byte			tileMapX			= 0;

		// Step down next read line.
		m_windowReadY++;

		Byte* mapRow = nullptr;
		GPUTileAttributes* attrRow = nullptr;
		m_tileRam.get_tilemap_row(tileMapIndex, tileMapY, &mapRow, &attrRow);

		GPUPaletteColour* colours = m_palette[GPUPalette::BG].entries[attrRow[tileMapX].palette];
		Byte tileIndex = mapRow[tileMapX];

		if(tileDataIndex == 0)
			tileIndex ^= 0x80;

		const Byte* tileRow = m_tileRam.get_tiledata_row(attrRow[tileMapX].bank, tileIndex + tileOffset, tileY);

		for (Byte screenX = windowX; screenX < kScreenWidth; ++screenX)
		{
#if 1
			Byte col = (3 - tileRow[tileX]) * 85;
			m_screenData[lineOffset + screenX] = { 255, col, col, col };
#else
			m_screenData[lineOffset + screenX] = colours[tileRow[tileX]].pixel;
#endif
			if (tileX++ == 7)
			{
				// Move across the tile source x, catching end of tile.
				tileX = 0;
				tileMapX++; // = (tileMapX + 1) & 0x1f;	// Wrap tile x to 0->31.

				tileIndex = mapRow[tileMapX];

				if (tileDataIndex == 0)
					tileIndex ^= 0x80;

				tileRow = m_tileRam.get_tiledata_row(attrRow[tileMapX].bank, tileIndex + tileOffset, tileY);
				colours = m_palette[GPUPalette::BG].entries[attrRow[tileMapX].palette];
			}
		}
	}

#if 0
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
#endif

	void GPU::scan_line_sprite()
	{
		// Calculate sprites visible on scanline.
		m_scanLineSprites.clear();

		const bool bDouble	= HWLCDC::sprite_double_height(m_lcdc);
		const Byte spriteHeight = HWLCDC::sprite_double_height(m_lcdc) ? 16 : 8;

		for (Byte spriteIndex = 0; spriteIndex < 40; ++spriteIndex)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			const Byte x = sprite->x;
			const Byte y = sprite->y - 16;

			if ((x == 0 || x >= 168 || y == 0 || y >= 144) ||								// Visibility check
				(m_currentScanLine < y || m_currentScanLine > (y + (spriteHeight - 1))))	// Scanline check
			{
				continue;
			}

			m_scanLineSprites.push_back(spriteIndex);
		}

		// Draw each visible sprite
		const Address lineOffset = m_currentScanLine * kScreenWidth;

		for (auto& spriteIndex : m_scanLineSprites)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			int16_t spriteX = sprite->x - 8;
			int16_t spriteY = sprite->y - 16;
			Byte tileIndex	= sprite->tile;
			Byte flags		= sprite->flags;

			const bool bFlipX	= (flags & HWSpriteFlags::FlipX) != 0;
			const bool bFlipY	= (flags & HWSpriteFlags::FlipY) != 0;
			const Byte palette	= (flags & HWSpriteFlags::PaletteCGB);
			const Byte bank		= (flags & HWSpriteFlags::BankCGB) >> HWSpriteFlags::BankCGBShift;

			// Always from tile pattern 1 (i.e. 0x8000)
			const Byte tileY			= m_currentScanLine - spriteY;
			const Byte tileYIndex		= bFlipY ? (bDouble ? (15 - tileY) : (7 - tileY)) : tileY;
			const Byte tileOffset		= (bDouble && (tileYIndex > 7)) ? 1 : 0;

			GPUPaletteColour* colours = m_palette[GPUPalette::Sprite].entries[palette];

			const Byte* tileRow = m_tileRam.get_tiledata_row(bank, tileIndex + tileOffset, tileYIndex & 0x7);

			Byte tileXIndex;
			Byte tileX = (spriteX < 0) ? (-spriteX) : 0;

			// Read the line data
			for (; tileX < 8; tileX++)
			{
				if (bFlipX)
				{
					tileXIndex = 7 - tileX;
				}
				else
				{
					tileXIndex = tileX;
				}

				// Colour 0 is always transparent for sprites.
				Byte paletteIndex = tileRow[tileXIndex];
				if (paletteIndex)
				{
					m_screenData[lineOffset + spriteX + tileX] = colours[paletteIndex].pixel;
				}
			}
		}
	}

	//--------------------------------------------------------------------------
}