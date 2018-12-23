#pragma once

#include <gbhw_debug.h>
#include <QWidget>

namespace gbd
{
	struct TileMapFocusArgs
	{
		uint32_t map;
		uint32_t tile;
	};

	class TileMapWidget : public QWidget
	{
		Q_OBJECT

	public:
		TileMapWidget(QWidget* parent = nullptr);
		~TileMapWidget();

		void initialise(gbhw_context_t hardware, uint32_t map);

	protected:
		void paintEvent(QPaintEvent* evt);
		void mouseMoveEvent(QMouseEvent* evt);

	signals:
		void on_focus_change(TileMapFocusArgs args);

	private:
		QImage				m_image;
		gbhw_context_t		m_hardware;
		uint32_t			m_map;
		uint32_t			m_focusIndex;
	};
}