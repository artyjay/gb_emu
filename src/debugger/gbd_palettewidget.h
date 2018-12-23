#pragma once

#include <gbhw_debug.h>
#include <QWidget>

namespace gbd
{
	struct PaletteFocusArgs
	{
		uint32_t value;
		uint32_t r_scaled;
		uint32_t g_scaled;
		uint32_t b_scaled;
	};

	class PaletteWidget : public QWidget
	{
		Q_OBJECT

	public:
		PaletteWidget(QWidget* parent = nullptr);
		~PaletteWidget();

		void initialise(gbhw_context_t hardware, gbhw::GPUPalette::Type type);

	protected:
		void paintEvent(QPaintEvent* evt);
		void mouseMoveEvent(QMouseEvent* evt);

	signals:
		void on_focus_change(PaletteFocusArgs args);

	private:
		QImage					m_image;
		gbhw_context_t			m_hardware;
		gbhw::GPUPalette::Type	m_type;

		uint32_t				m_focusEntryIndex;
		uint32_t				m_focusColourIndex;
	};
}