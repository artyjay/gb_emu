#pragma once

#include <gbhw.h>
#include <QPen>
#include <QWidget>

namespace gbd
{
	class TilePatternWidget : public QWidget
	{
		Q_OBJECT

	public:
		TilePatternWidget(QWidget* parent = nullptr);
		~TilePatternWidget();

		void SetHardware(gbhw::Hardware* hardware);
		void SetTilePatternIndex(uint32_t tilePatternIndex);

	protected:
		void paintEvent(QPaintEvent* evt);
		void mouseMoveEvent(QMouseEvent* evt);
		void mousePressEvent(QMouseEvent* evt);

	signals:
		void OnTileCoordinateChanged(const QString& str);
		void OnHighlightedTileChanged(uint32_t unsignedTileIndex, uint32_t tilePatternIndex);
		void OnTileLocked(bool bLocked);

	private:
		void UpdateTileHighlight(int32_t tileIndex);
		void UpdateTileCoord(int32_t tileX, int32_t tileY, int32_t tileIndex, gbhw::Address tileAddress);

		void GetTileIndexFromMouseEvt(QMouseEvent* evt, int32_t& tileX, int32_t& tileY, int32_t& tileIndex, gbhw::Address& tileAddress);

		QImage				m_image;
		gbhw::Hardware*		m_hardware;
		uint32_t			m_tilePatternIndex;
		float				m_scaleX;
		float				m_scaleY;
		QPen				m_gridPen;
		QPen				m_lockedPen;

		int32_t				m_lockedTileIndex;
	};
}
