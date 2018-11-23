#pragma once

#include <gbhw_debug.h>
#include <QWidget>

namespace gbd
{
	class TileMapWidget : public QWidget
	{
		Q_OBJECT

	public:
		TileMapWidget(QWidget* parent = nullptr);
		~TileMapWidget();

		void SetHardware(gbhw_context_t hardware);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw_context_t		m_hardware;
	};
}
#pragma once
