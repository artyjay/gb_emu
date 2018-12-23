#pragma once

#include <gbhw_debug.h>
#include <QPen>
#include <QWidget>

namespace gbd
{
	class ScreenWidget : public QWidget
	{
		Q_OBJECT

	public:
		ScreenWidget(QWidget* parent = nullptr);
		~ScreenWidget();

		void initialise(gbhw_context_t hardware);

	protected:
		void paintEvent(QPaintEvent* evt);
		void keyPressEvent(QKeyEvent* evt);
		void keyReleaseEvent(QKeyEvent* evt);

	private:
		void on_key(QKeyEvent* evt, gbhw_button_state_t state);

		gbhw_context_t		m_hardware;
		QImage				m_image;
		float				m_scaleX;
		float				m_scaleY;
	};
}