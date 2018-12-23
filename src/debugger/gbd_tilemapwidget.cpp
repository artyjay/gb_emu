#include "gbd_tilemapwidget.h"
#include <QMouseEvent>
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
		, m_focusIndex(kTilesAcross * kTilesDown)
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

			const GPUTileRam*			ram			= gpu->get_tile_ram();
			const Byte*					tileMap		= ram->tileMap[m_map];
			const GPUTileAttributes*	tileAttr	= ram->tileAttr[m_map];
			const GPUPalette*			palette		= gpu->get_palette(GPUPalette::BG);
			QRgb*						destData	= (QRgb*)m_image.scanLine(0);

			// Map 0 tiles start at 0x8800
			// Map 1 tiles start at 0x8000
			// 16-bytes per tile. Total tile count is 384 (per-bank). 128-tile
			// overlap per-map.
			// Address already translated to indices in storage.
			const uint32_t				baseIndex	= m_map == 0 ? 128 : 0;

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

					//const GPUTileAttributes*	attr	= &tileAttr[baseIndex + tileIndex];
					const GPUTile*				tile	= &ram->tileData[tileAttr->bank][baseIndex + tileIndex];
 					const GPUPaletteColour*		colours = palette->entries[tileAttr->palette];
					tileAttr++;

					for(uint32_t pixelY = 0; pixelY < kTileSize; ++pixelY)
					{
						QRgb* tileLine = tileStart + (pixelY * kImageWidth);

						for(uint32_t pixelX = 0; pixelX < kTileSize; ++pixelX)
						{
							const Byte pixel = tile->pixels[pixelY][pixelX];
							const GPUPaletteColour* colour = &colours[pixel];
							tileLine[pixelX] = qRgb(colour->pixel.b, colour->pixel.g, colour->pixel.r);
						}
					}
				}
			}
		}
		else
		{
			m_image.fill(QColor(255, 255, 255, 255));
		}

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);
	}

	void TileMapWidget::mouseMoveEvent(QMouseEvent* evt)
	{
		if(m_hardware)
		{
			const uint32_t tileX = evt->pos().x() / kTileSize;
			const uint32_t tileY = evt->pos().y() / kTileSize;

			if(tileX > kTilesAcross || tileY > kTilesDown)
				return;

			const uint32_t tileIndex = (tileY * kTilesAcross) + tileX;

			if(m_focusIndex != tileIndex)
			{
				m_focusIndex = tileIndex;

				TileMapFocusArgs args = { m_map, m_focusIndex };

				emit on_focus_change(args);
			}
		}
	}
}