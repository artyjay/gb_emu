#include "gbd_palettewidget.h"
#include <QMouseEvent>
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
		, m_focusEntryIndex(kEntries)
		, m_focusColourIndex(kColoursPerEntry)
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
					QRgb pixelColour = qRgb(pixel->b, pixel->g, pixel->r);

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

		painter.drawImage(QPoint(0, 0), m_image);
	}

	void PaletteWidget::mouseMoveEvent(QMouseEvent* evt)
	{
		// Update the image
		if (m_hardware)
		{
			uint32_t colourIndex = evt->pos().x() / kBlockSize;
			uint32_t entryIndex = evt->pos().y() / kBlockSize;

			if(colourIndex > kColoursPerEntry || entryIndex > kEntries)
				return;

			if(entryIndex != m_focusEntryIndex ||
			   colourIndex != m_focusColourIndex)
			{
				m_focusEntryIndex = entryIndex;
				m_focusColourIndex = colourIndex;

				// Emit colour.
				GPU* gpu = nullptr;
				if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
					return;

				const auto palette = gpu->get_palette(m_type);
				auto& colour = palette->entries[entryIndex][colourIndex];

				PaletteFocusArgs args = { (uint32_t)(colour.values[0] | (colour.values[1] << 8)),
										  colour.pixel.r, colour.pixel.g, colour.pixel.b };

				emit on_focus_change(args);
			}
		}
	}
}