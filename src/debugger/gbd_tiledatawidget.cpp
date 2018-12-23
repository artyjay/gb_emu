#include "gbd_tiledatawidget.h"

#include <QPainter>
#include <QMouseEvent>

using namespace gbhw;

namespace gbd
{
	namespace
	{
		int32_t GetSignedTileIndex(int32_t unsignedTileIndex)
		{
			int32_t signedTileIndex = unsignedTileIndex ^ 0x80;

			if (signedTileIndex >= 128)
			{
				signedTileIndex -= 256;
			}

			return signedTileIndex;
		}

		static const uint32_t kTilesAcross	= 16;
		static const uint32_t kTilesDown	= 24;
		static const uint32_t kTileSize		= 8;
		static const uint32_t kImageWidth	= kTilesAcross * kTileSize;
		static const uint32_t kImageHeight	= kTilesDown * kTileSize;
	}

	TileDataWidget::TileDataWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(kImageWidth, kImageHeight, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_bank(0)
		, m_scaleX(2.0f)
		, m_scaleY(2.0f)
		, m_lockedTileIndex(-1)
	{
		setMouseTracking(true);
		m_lockedPen.setColor(QColor(0, 255, 0, 180));
		m_lockedPen.setWidthF(0.0f);
	}

	TileDataWidget::~TileDataWidget()
	{
	}

	void TileDataWidget::initialise(gbhw_context_t hardware, uint32_t bank)
	{
		m_hardware = hardware;
		m_bank = bank;
	}

	void TileDataWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			GPU* gpu;
			if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
				return;

			const GPUTileRam*	ram			= gpu->get_tile_ram();
			const GPUTile*		tiles		= &ram->tileData[m_bank][0];
			QRgb*				destData	= (QRgb*)m_image.scanLine(0);

			// @todo: Use palettes to colour

			for(uint32_t tileIndex = 0; tileIndex < GPUTileRam::kTileDataCount; ++tileIndex)
			{
				uint32_t tileX = tileIndex % kTilesAcross;
				uint32_t tileY = tileIndex / kTilesAcross;

				// Offset to top line of tile.
				QRgb* tileStart = destData + (tileY * kTileSize * kImageWidth) + (tileX * kTileSize);
				const GPUTile* tile = &tiles[tileIndex];

				for(uint32_t pixelY = 0; pixelY < kTileSize; ++pixelY)
				{
					QRgb* tileLine = tileStart + (pixelY * kImageWidth);

					for(uint32_t pixelX = 0; pixelX < kTileSize; ++pixelX)
					{
 						const Byte pixel = (3 - tile->pixels[pixelY][pixelX]) * 85;
 						tileLine[pixelX] = qRgb(pixel, pixel, pixel);
					}
				}
			}
		}

		painter.scale(m_scaleX, m_scaleY);

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);

		if(m_lockedTileIndex != -1)
		{
			int32_t lockedTileX = m_lockedTileIndex % kTilesAcross;
			int32_t lockedTileY = m_lockedTileIndex / kTilesAcross;

			painter.setPen(m_lockedPen);
			painter.drawRect(lockedTileX * kTileSize, lockedTileY * kTileSize, kTileSize, kTileSize);
		}
	}

	void TileDataWidget::mousePressEvent(QMouseEvent* evt)
	{
		if(evt->button() != Qt::LeftButton)
		{
			return;
		}

		int32_t tileX, tileY, tileIndex;
		gbhw::Address tileAddress;
		GetTileIndexFromMouseEvt(evt, tileX, tileY, tileIndex, tileAddress);

		if(m_lockedTileIndex == tileIndex)
		{
			m_lockedTileIndex = -1;
			emit OnTileLocked(false);
		}
		else
		{
			// Unlock briefly whilst we update.
			emit OnTileLocked(false);
			m_lockedTileIndex = tileIndex;
		}

		UpdateTileHighlight(tileIndex);
		UpdateTileCoord(tileX, tileY, tileIndex, tileAddress);

		if(m_lockedTileIndex != -1)
		{
			emit OnTileLocked(true);
		}

		repaint();
	}

	void TileDataWidget::UpdateTileHighlight(int32_t tileIndex)
	{
		emit OnHighlightedTileChanged((uint32_t)tileIndex, 0);
	}

	void TileDataWidget::UpdateTileCoord(int32_t tileX, int32_t tileY, int32_t tileIndex, gbhw::Address tileAddress)
	{
#if 0
		QString str;

		if (tileIndex >= )
		{
			int32_t signedTileIndex = GetSignedTileIndex(tileIndex);
			str = QString::asprintf("Tile: %d, %d - Index: %d (%d) - 0x%04x", tileX, tileY, tileIndex, signedTileIndex, tileAddress);
		}
		else
		{
			str = QString::asprintf("Tile: %d, %d - Index: %d - 0x%04x", tileX, tileY, tileIndex, tileAddress);
		}

		emit OnTileCoordinateChanged(str);
#endif
	}

	void TileDataWidget::GetTileIndexFromMouseEvt(QMouseEvent* evt, int32_t& tileX, int32_t& tileY, int32_t& tileIndex, gbhw::Address& tileAddress)
	{
		const int32_t posX = evt->pos().x() / m_scaleX;
		const int32_t posY = evt->pos().y() / m_scaleY;
		tileX = posX / kTileSize;
		tileY = posY / kTileSize;

		tileIndex = (tileY * kTilesAcross) + tileX;
		tileAddress = 0; //gbhw::HWLCDC::get_tile_pattern_address(m_tilePatternIndex) + (tileIndex * 16);
	}
}