#pragma once

#include <gbhw_debug.h>
#include <QWidget>

namespace gbd
{
	class PaletteWidget : public QWidget
	{
		Q_OBJECT

	public:
		PaletteWidget(QWidget* parent = nullptr);
		~PaletteWidget();

		void initialise(gbhw_context_t hardware, gbhw::GPUPalette::Type type);

	protected:
		void paintEvent(QPaintEvent* evt);

	private:
		QImage					m_image;
		gbhw_context_t			m_hardware;
		gbhw::GPUPalette::Type	m_type;
	};
}