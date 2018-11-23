#pragma once

#include <gbhw_debug.h>
#include <QWidget>

namespace gbd
{
	class Screen : public QWidget
	{
		Q_OBJECT

	public:
		Screen(QWidget * parent = nullptr);
		~Screen();

		void SetHardware(gbhw_context_t hardware);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage				m_image;
		gbhw_context_t		m_hardware;
	};
}