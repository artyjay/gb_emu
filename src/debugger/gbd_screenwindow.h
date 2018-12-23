#pragma once

#include "ui_gbd_screenwindow.h"
#include <QtWidgets/QWidget>

namespace gbd
{
	class ScreenWindow : public QWidget
	{
		Q_OBJECT

	public:
		ScreenWindow(QWidget* parent, gbhw_context_t hardware);
		~ScreenWindow();

	private:
		void on_palette_focus_change(PaletteFocusArgs args);
		void on_tilemap_focus_change(TileMapFocusArgs args);

		Ui::GBDScreenWindowClass	m_ui;
		gbhw_context_t				m_hardware;
	};
}