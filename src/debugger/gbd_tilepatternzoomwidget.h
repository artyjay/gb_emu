#pragma once

#include <gbhw.h>
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

		void SetHardware(gbhw::Hardware* hardware);

	public slots:
		void SetZoomedTile(uint32_t unsignedTileIndex, uint32_t tilePatternIndex);
		void SetLockedTile(bool bLocked);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw::Hardware*		m_hardware;
		uint32_t			m_tilePatternIndex;
		uint32_t			m_unsignedTileIndex;
		QPen				m_gridPen;
		bool				m_bLocked;
	};
}
#pragma once
