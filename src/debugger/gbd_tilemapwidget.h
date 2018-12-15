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

		void initialise(gbhw_context_t hardware, uint32_t map);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw_context_t		m_hardware;
		uint32_t			m_map;
	};
}