#pragma once

#include "ui_gbd_screenwindow.h"
#include <QtWidgets/QWidget>

namespace gbd
{
	class ScreenWindow : public QWidget
	{
		Q_OBJECT

	public:
		ScreenWindow(QWidget* parent, gbhw::Hardware& hardware);
		~ScreenWindow();

	private:
		void OnShowTileGrid(bool bChecked);

		Ui::GBDScreenWindowClass	m_ui;
		gbhw::Hardware&				m_hardware;
	};
}