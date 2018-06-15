#include "gbd_tilepatternwidget.h"

#include <QPainter>
#include <QMouseEvent>

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
	}

	TilePatternWidget::TilePatternWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(128, 128, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_tilePatternIndex(0)
		, m_scaleX(2.0f)
		, m_scaleY(2.0f)
		, m_lockedTileIndex(-1)
	{
		setMouseTracking(true);
		m_gridPen.setColor(QColor(100, 100, 100, 100));
		m_lockedPen.setColor(QColor(220, 20, 20, 180));
	}

	TilePatternWidget::~TilePatternWidget()
	{
	}

	void TilePatternWidget::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;
	}

	void TilePatternWidget::SetTilePatternIndex(uint32_t tilePatternIndex)
	{
		m_tilePatternIndex = tilePatternIndex;
	}

	void TilePatternWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			const gbhw::GPUTilePattern* tilePatternData = m_hardware->GetGPU().GetTilePattern(m_tilePatternIndex);
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for(uint32_t tileY = 0; tileY < 16; ++tileY)
			{
				QRgb* destLineStart = &destData[(tileY * 1024)];

				for(uint32_t tileX = 0; tileX < 16; ++tileX)
				{
					const gbhw::GPUTileData* tileData = &tilePatternData->m_tiles[tileY * 16 + tileX];
					
					for(uint32_t pixelY = 0; pixelY < 8; ++pixelY)
					{
						QRgb* destTileLine = destLineStart + (pixelY * 128) + (tileX * 8);

						for(uint32_t pixelX = 0; pixelX < 8; ++pixelX)
						{
							gbhw::Byte pixelColour = (3-tileData->m_pixels[pixelY][pixelX]) * 85;
							destTileLine[pixelX] = qRgb(pixelColour, pixelColour, pixelColour);
						}
					}
				}
			}
		}

		painter.scale(m_scaleX, m_scaleY);

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);

		// Draw grid
		painter.setPen(m_gridPen);

		int lineX = 0;
		int lineY = 0;

		// Draw horizontal lines
		for (uint32_t y = 0; y < 19; ++y)
		{
			painter.drawLine(0, lineY, 160, lineY);
			lineY += 8;
		}

		// Draw vertical lines
		for (uint32_t x = 0; x < 21; ++x)
		{
			painter.drawLine(lineX, 0, lineX, 144);
			lineX += 8;
		}

		if(m_lockedTileIndex != -1)
		{
			int32_t lockedTileX = m_lockedTileIndex % 16;
			int32_t lockedTileY = m_lockedTileIndex / 16;

			painter.setPen(m_lockedPen);
			painter.drawRect(lockedTileX * 8, lockedTileY * 8, 8, 8);
		}
	}

	void TilePatternWidget::mouseMoveEvent(QMouseEvent* evt)
	{
		// Only update when we haven't locked the widget in-place.
		if(m_lockedTileIndex == -1)
		{
			int32_t tileX, tileY, tileIndex;
			gbhw::Address tileAddress;
			GetTileIndexFromMouseEvt(evt, tileX, tileY, tileIndex, tileAddress);

			UpdateTileHighlight(tileIndex);
			UpdateTileCoord(tileX, tileY, tileIndex, tileAddress);

			repaint();
		}
	}

	void TilePatternWidget::mousePressEvent(QMouseEvent* evt)
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

	void TilePatternWidget::UpdateTileHighlight(int32_t tileIndex)
	{
		emit OnHighlightedTileChanged((uint32_t)tileIndex, m_tilePatternIndex);
	}

	void TilePatternWidget::UpdateTileCoord(int32_t tileX, int32_t tileY, int32_t tileIndex, gbhw::Address tileAddress)
	{
		QString str;

		if (m_tilePatternIndex == 0)
		{
			int32_t signedTileIndex = GetSignedTileIndex(tileIndex);
			str = QString::asprintf("Tile: %d, %d - Index: %d (%d) - 0x%04x", tileX, tileY, signedTileIndex, tileIndex, tileAddress);
		}
		else
		{
			str = QString::asprintf("Tile: %d, %d - Index: %d - 0x%04x", tileX, tileY, tileIndex, tileAddress);
		}

		emit OnTileCoordinateChanged(str);
	}

	void TilePatternWidget::GetTileIndexFromMouseEvt(QMouseEvent* evt, int32_t& tileX, int32_t& tileY, int32_t& tileIndex, gbhw::Address& tileAddress)
	{
		int32_t posX = evt->pos().x();
		int32_t posY = evt->pos().y();
		tileX = posX / (8 * m_scaleX);
		tileY = posY / (8 * m_scaleY);
		tileIndex = tileY * 16 + tileX;
		tileAddress = gbhw::HWLCDC::GetTilePatternAddress(m_tilePatternIndex) + (tileIndex * 16);
	}

} // gbd