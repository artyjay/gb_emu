#include "gbd_tilemapwidget.h"

#include <QPainter>

using namespace gbhw;

namespace gbd
{
	TileMapWidget::TileMapWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(256, 256, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
	{
	}

	TileMapWidget::~TileMapWidget()
	{

	}

	void TileMapWidget::SetHardware(gbhw_context_t hardware)
	{
		m_hardware = hardware;
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

		}
		else
		{
			m_image.fill(QColor(255, 255, 255, 255));
		}

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);
	}
} // gbd