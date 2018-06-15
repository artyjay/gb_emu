#include "gbd_screen.h"

#include <QPainter>

namespace gbd
{

	Screen::Screen(QWidget * parent)
		: QWidget(parent)
		, m_image(160, 144, QImage::Format::Format_RGBX8888)
		, m_hardware(nullptr)
	{
	}

	Screen::~Screen()
	{
	}

	void Screen::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;
	}

	void Screen::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if(m_hardware)
		{
			const gbhw::Byte* screenData = m_hardware->GetGPU().GetScreenData();
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for (uint32_t y = 0; y < gbhw::GPU::kScreenHeight; ++y)
			{
				for (uint32_t x = 0; x < gbhw::GPU::kScreenWidth; ++x)
				{
					gbhw::Byte hwpixel = (3 - *screenData) * 85;

					*destData = qRgb(hwpixel, hwpixel, hwpixel);
					destData++;
					screenData++;
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

}