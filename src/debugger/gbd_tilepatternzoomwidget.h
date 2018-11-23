#pragma once

#include <gbhw_debug.h>
#include <QPen>
#include <QWidget>

namespace gbd
{
	class TilePatternZoomWidget : public QWidget
	{
		Q_OBJECT

	public:
		TilePatternZoomWidget(QWidget* parent = nullptr);
		~TilePatternZoomWidget();

		void SetHardware(gbhw_context_t hardware);

	public slots:
		void SetZoomedTile(uint32_t unsignedTileIndex, uint32_t tilePatternIndex);
		void SetLockedTile(bool bLocked);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw_context_t		m_hardware;
		uint32_t			m_tilePatternIndex;
		uint32_t			m_unsignedTileIndex;
		QPen				m_gridPen;
		bool				m_bLocked;
	};
}
#pragma once
