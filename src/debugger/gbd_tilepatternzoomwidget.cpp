#include "gbd_tilepatternzoomwidget.h"

#include <QPainter>
#include <QMouseEvent>

namespace gbd
{
	TilePatternZoomWidget::TilePatternZoomWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(8, 8, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_tilePatternIndex(0)
		, m_unsignedTileIndex(0)
		, m_bLocked(false)
	{
		m_gridPen.setColor(QColor(100, 100, 100, 100));
	}

	TilePatternZoomWidget::~TilePatternZoomWidget()
	{
	}

	void TilePatternZoomWidget::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;
	}

	void TilePatternZoomWidget::SetZoomedTile(uint32_t unsignedTileIndex, uint32_t tilePatternIndex)
	{
		if(!m_bLocked)
		{
			m_tilePatternIndex = tilePatternIndex;
			m_unsignedTileIndex = unsignedTileIndex;
			repaint();
		}
	}

	void TilePatternZoomWidget::SetLockedTile(bool bLocked)
	{
		m_bLocked = bLocked;
	}

	void TilePatternZoomWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			const gbhw::GPUTilePattern* tilePatternData = m_hardware->GetGPU().GetTilePattern(m_tilePatternIndex);
			const gbhw::GPUTileData* tileData = &tilePatternData->m_tiles[m_unsignedTileIndex];
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for (uint32_t pixelY = 0; pixelY < 8; ++pixelY)
			{
				for (uint32_t pixelX = 0; pixelX < 8; ++pixelX)
				{
					gbhw::Byte pixelColour = (3 - tileData->m_pixels[pixelY][pixelX]) * 85;
					*destData = qRgb(pixelColour, pixelColour, pixelColour);
					destData++;
				}
			}
		}

		painter.scale(16, 16);

		// Draw the image
		painter.drawImage(QPoint(0, 0), m_image);

		QPainter painter2(this);
		// Draw grid
		painter2.setPen(m_gridPen);

		int lineX = 0;
		int lineY = 0;

		// Draw horizontal lines
		for (uint32_t y = 0; y < 9; ++y)
		{
			painter2.drawLine(0, lineY, 128, lineY);
			lineY += 16;
		}

		// Draw vertical lines
		for (uint32_t x = 0; x < 9; ++x)
		{
			painter2.drawLine(lineX, 0, lineX, 128);
			lineX += 16;
		}
	}

} // gbd