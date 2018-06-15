#pragma once

#include "gbhw.h"

#include <QWidget>

namespace gbd
{
	class ImageWidget : public QWidget
	{
		Q_OBJECT

	public:
		ImageWidget(QWidget* parent = nullptr);
		~ImageWidget();

		void SetHardware(gbhw::Hardware* hardware);
		void SetTilePatternIndex(uint32_t tilePatternIndex);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage m_image;
		gbhw::Hardware* m_hardware;
		uint32_t m_tilePatternIndex;
	};
}
