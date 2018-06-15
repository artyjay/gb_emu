#pragma once

#include "gbhw.h"

#include <QWidget>

namespace gbd
{
	class TileMapWidget : public QWidget
	{
		Q_OBJECT

	public:
		TileMapWidget(QWidget* parent = nullptr);
		~TileMapWidget();

		void SetHardware(gbhw::Hardware* hardware);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw::Hardware*		m_hardware;
	};
}
#pragma once
