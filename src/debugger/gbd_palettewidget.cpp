#include "gbd_palettewidget.h"

#include <QPainter>

using namespace gbhw;

namespace gbd
{
	namespace
	{
		static const uint32_t kEntries			= 8;
		static const uint32_t kColoursPerEntry	= 4;
		static const uint32_t kBlockSize		= 16;
		static const uint32_t kImageWidth		= kColoursPerEntry * kBlockSize;
		static const uint32_t kImageHeight		= kEntries * kBlockSize;
	}

	PaletteWidget::PaletteWidget(QWidget* parent)
		: QWidget(parent)
		, m_image(kImageWidth, kImageHeight, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
		, m_type(GPUPalette::Count)
	{
		setMouseTracking(true);
	}

	PaletteWidget::~PaletteWidget()
	{
	}

	void PaletteWidget::initialise(gbhw_context_t hardware, gbhw::GPUPalette::Type type)
	{
		m_hardware = hardware;
		m_type = type;
	}

	void PaletteWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if (m_hardware)
		{
			GPU* gpu = nullptr;
			if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
				return;

			if(m_type == GPUPalette::Count)
				return;

			const GPUPalette* palette = gpu->get_palette(m_type);

			// Read tile index from tilemap.
			QRgb* dest = (QRgb*)m_image.scanLine(0);

			for(uint32_t entry = 0; entry < kEntries; ++entry)
			{
				for(uint32_t colour = 0; colour < kColoursPerEntry; ++colour)
				{
					QRgb* entryStart = dest + (entry * kBlockSize * kImageWidth) + (colour * kBlockSize);
					const GPUPixel* pixel = &palette->entries[entry][colour].pixel;
					QRgb pixelColour = qRgb(pixel->r, pixel->g, pixel->b);

					// Block fill colour.
					// @todo: Improve this widgets drawing, this is likely very inefficient.
					for(uint32_t pixelY = 0; pixelY < kBlockSize; ++pixelY)
					{
						QRgb* blockLine = entryStart + (pixelY * kImageWidth);

						for(uint32_t pixelX = 0; pixelX < kBlockSize; ++pixelX)
						{
							blockLine[pixelX] = pixelColour;
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