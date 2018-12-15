#include "gbd_tilemapwidget.h"

#include <QPainter>

using namespace gbhw;

namespace gbd
{
	namespace
	{
		static const uint32_t kTilesAcross	= GPUTileRam::kTileMapWidth;
		static const uint32_t kTilesDown	= GPUTileRam::kTileMapHeight;
		static const uint32_t kTileSize		= 8;
		static const uint32_t kImageWidth	= kTilesAcross * kTileSize;
		static const uint32_t kImageHeight	= kTilesDown  * kTileSize;
		static const uint32_t kImageStride	= kImageWidth * sizeof(QRgb);
	}


	TileMapWidget::TileMapWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(kImageWidth, kImageHeight, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_map(0)
	{
		setMouseTracking(true);
	}

	TileMapWidget::~TileMapWidget()
	{
	}

	void TileMapWidget::initialise(gbhw_context_t hardware, uint32_t map)
	{
		m_hardware = hardware;
		m_map = map;
	}

	void TileMapWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			MMU* mmu = nullptr;
			if(gbhw_get_mmu(m_hardware, &mmu) != e_success)
				return;

			GPU* gpu = nullptr;
			if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
				return;

			// Read tile index from tilemap.

			const GPUTileRam*	ram			= gpu->get_tile_ram();
			const Byte*			tileMap		= ram->tileMap[m_map];
			const Byte*			tileAttr	= ram->tileAttr[m_map];
			const GPUTile*		tiles		= ram->tileData[ram->bank];
			QRgb*				destData	= (QRgb*)m_image.scanLine(0);

			// Map 0 tiles start at 0x8800
			// Map 1 tiles start at 0x8000
			// 16-bytes per tile. Total tile count is 384 (per-bank). 128-tile
			// overlap per-map.
			// Address already translated to indices in storage.
			const uint32_t		baseIndex	= m_map == 1 ? 0 : 128;

			for(uint32_t tileY = 0; tileY < kTilesDown; ++tileY)
			{

				for(uint32_t tileX = 0; tileX < kTilesAcross; ++tileX)
				{
					QRgb* tileStart = destData + (tileY * kTileSize * kImageWidth) + (tileX * kTileSize);
					Byte tileIndex = *tileMap++;

					// Tilemap 0 indices are signed. So convert to unsigned range
					// as base is normalised to the -128 address, i.e 0 index is
					// address 0x9000.
					if(m_map == 0)
						tileIndex ^= 0x80;

					const GPUTile* tile = &tiles[baseIndex + tileIndex];

					for(uint32_t pixelY = 0; pixelY < kTileSize; ++pixelY)
					{
						QRgb* tileLine = tileStart + (pixelY * kImageWidth);

						for(uint32_t pixelX = 0; pixelX < kTileSize; ++pixelX)
						{
							// @todo: Colourise.
							const Byte pixel = (3 - tile->m_pixels[pixelY][pixelX]) * 85;
							tileLine[pixelX] = qRgb(pixel, pixel, pixel);
						}
					}
				}
			}



#if 0

			Address tilemapAddress = gbhw::HWLCDC::get_bg_tile_map_address(mmu->read_byte(gbhw::HWRegs::LCDC));

			QFont font;
			font.setFamily(QStringLiteral("Consolas"));
			font.setPointSize(8);
			painter.setFont(font);

			uint32_t tileSizeX = 24;
			uint32_t tileSizeY = 16;

			for (uint32_t y = 0; y < 32; ++y)
			{
				for (uint32_t x = 0; x < 32; ++x)
				{
					gbhw::Byte tileIndex = mmu->read_byte(tilemapAddress + (y * 32) + x);
					painter.drawRect(x * tileSizeX, y * tileSizeY, tileSizeX, tileSizeY);
					painter.drawText((x * tileSizeX) + 2, (y * tileSizeY) + 8 + ((tileSizeY - 8) / 2), QString::asprintf("%d", tileIndex));
				}
			}

			GPU* gpu;
			if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
				return;

			const GPUTilePattern* tilePatternData = gpu->get_tile_pattern(0);
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for (uint32_t tileY = 0; tileY < 16; ++tileY)
			{
				QRgb* destLineStart = &destData[(tileY * 4096)];

				for (uint32_t tileX = 0; tileX < 16; ++tileX)
				{
					// Nearest upsample the tile-data.
					const gbhw::GPUTileData* tileData = &tilePatternData->m_tiles[tileY * 16 + tileX];

					for (uint32_t pixelY = 0; pixelY < 16; ++pixelY)
					{
						QRgb* destTileLine = destLineStart + (pixelY * 256) + (tileX * 16);

						for (uint32_t pixelX = 0; pixelX < 16; ++pixelX)
						{
							gbhw::Byte pixelColour = (3 - tileData->m_pixels[pixelY >> 1][pixelX >> 1]) * 85;
							destTileLine[pixelX] = qRgb(pixelColour, pixelColour, pixelColour);
						}
					}
				}
			}
#endif
		}
		else
		{
			m_image.fill(QColor(255, 255, 255, 255));
		}

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);
	}
} // gbd