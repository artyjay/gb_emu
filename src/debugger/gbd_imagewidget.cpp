#include "gbd_imagewidget.h"

#include <QPainter>

namespace gbd
{
	ImageWidget::ImageWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(256, 256, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_tilePatternIndex(0)
	{

	}

	ImageWidget::~ImageWidget()
	{

	}

	void ImageWidget::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;
	}

	void ImageWidget::SetTilePatternIndex(uint32_t tilePatternIndex)
	{
		m_tilePatternIndex = tilePatternIndex;
	}

	void ImageWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			const gbhw::GPUTilePattern* tilePatternData = m_hardware->GetGPU().GetTilePattern(m_tilePatternIndex);
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for(uint32_t tileY = 0; tileY < 16; ++tileY)
			{
				QRgb* destLineStart = &destData[(tileY * 4096)];

				for(uint32_t tileX = 0; tileX < 16; ++tileX)
				{
					// Nearest upsample the tile-data.
					const gbhw::GPUTileData* tileData = &tilePatternData->m_tiles[tileY * 16 + tileX];
					
					for(uint32_t pixelY = 0; pixelY < 16; ++pixelY)
					{
						QRgb* destTileLine = destLineStart + (pixelY * 256) + (tileX * 16);

						for(uint32_t pixelX = 0; pixelX < 16; ++pixelX)
						{
							gbhw::Byte pixelColour = (3-tileData->m_pixels[pixelY>>1][pixelX>>1]) * 85;
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