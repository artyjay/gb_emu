#pragma once

#include <gbhw.h>

#include <QWidget>

namespace gbd
{
	class Screen : public QWidget
	{
		Q_OBJECT

	public:
		Screen(QWidget * parent = nullptr);
		~Screen();

		void SetHardware(gbhw::Hardware* hardware);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw::Hardware*		m_hardware;
	};
}