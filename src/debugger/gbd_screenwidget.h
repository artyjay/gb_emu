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

		void SetHardware(gbhw_context_t hardware);
		void SetShowTilemapGrid(bool bShow);

	protected:
		void paintEvent(QPaintEvent* evt);
		void mouseMoveEvent(QMouseEvent* evt);

	signals:
		void UpdateCoordText(const QString& str);

	private:
		gbhw_context_t		m_hardware;
		QImage				m_image;
		bool				m_bShowTilemapGrid;
		float				m_scaleX;
		float				m_scaleY;
		QPen				m_gridPen;
	};
}