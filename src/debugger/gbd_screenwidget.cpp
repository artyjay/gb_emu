#include "gbd_log.h"
#include "gbd_screenwidget.h"

#include <QPainter>
#include <QMouseEvent>

using namespace gbhw;

namespace gbd
{
	namespace
	{
		static const uint32_t kImageWidth = 160;
		static const uint32_t kImageHeight = 144;
	}

	ScreenWidget::ScreenWidget(QWidget* parent)
		: QWidget(parent)
		, m_hardware(nullptr)
		, m_image(kImageWidth, kImageHeight, QImage::Format::Format_RGBX8888)
		, m_scaleX(2.0f)
		, m_scaleY(2.0f)
	{
		setMouseTracking(true);
		setEnabled(true);
		setFocusPolicy(Qt::FocusPolicy::ClickFocus);
	}

	ScreenWidget::~ScreenWidget()
	{
	}

	void ScreenWidget::initialise(gbhw_context_t hardware)
	{
		m_hardware = hardware;
	}

	void ScreenWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		if(m_hardware)
		{
			QRgb* dst = (QRgb*)m_image.scanLine(0);
			const uint8_t* src = nullptr;
			gbhw_get_screen(m_hardware, &src);

			for (uint32_t y = 0; y < kImageHeight; ++y)
			{
				for (uint32_t x = 0; x < kImageWidth; ++x)
				{
					*dst = qRgb(src[3], src[2], src[1]);
					dst++;
					src += 4;
				}
			}
		}

		painter.scale(m_scaleX, m_scaleY);
		painter.drawImage(QPoint(0, 0), m_image);
	}

	void ScreenWidget::keyPressEvent(QKeyEvent* evt)
	{
		on_key(evt, button_pressed);
	}

	void ScreenWidget::keyReleaseEvent(QKeyEvent* evt)
	{
		on_key(evt, button_released);
	}

	void ScreenWidget::on_key(QKeyEvent* evt, gbhw_button_state_t state)
	{
		if(!m_hardware)
			return;

		switch(evt->key())
		{
			case Qt::Key_Left:		gbhw_set_button_state(m_hardware, button_dpad_left, state); break;
			case Qt::Key_Right:		gbhw_set_button_state(m_hardware, button_dpad_right, state); break;
			case Qt::Key_Up:		gbhw_set_button_state(m_hardware, button_dpad_up, state); break;
			case Qt::Key_Down:		gbhw_set_button_state(m_hardware, button_dpad_down, state); break;
			case Qt::Key_Return:	gbhw_set_button_state(m_hardware, button_start, state); break;
			case Qt::Key_Backspace: gbhw_set_button_state(m_hardware, button_select, state); break;
			case Qt::Key_Space:		gbhw_set_button_state(m_hardware, button_a, state); break;
			case Qt::Key_B:			gbhw_set_button_state(m_hardware, button_b, state); break;
			default: break;
		}
	}
}