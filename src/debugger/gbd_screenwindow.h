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
		void OnShowTileGrid(bool bChecked);

		Ui::GBDScreenWindowClass	m_ui;
		gbhw_context_t				m_hardware;
	};
}