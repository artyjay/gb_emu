#pragma once

#include <gbhw_debug.h>
#include <QPen>
#include <QWidget>

namespace gbd
{
	class TileDataWidget : public QWidget
	{
		Q_OBJECT

	public:
		TileDataWidget(QWidget* parent = nullptr);
		~TileDataWidget();

		void initialise(gbhw_context_t hardware, uint32_t bank);

	protected:
		void paintEvent(QPaintEvent* evt);
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
		gbhw_context_t		m_hardware;
		uint32_t			m_bank;
		float				m_scaleX;
		float				m_scaleY;
		QPen				m_lockedPen;
		int32_t				m_lockedTileIndex;
	};
}
