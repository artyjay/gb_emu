#pragma once

#include "ui_gbd_memorywindow.h"
#include <gbhw_debug.h>
#include <vector>
#include <QtWidgets/QWidget>

typedef std::vector<QListWidgetItem*> MemoryLineList;

namespace gbd
{
	class MemoryWindow : public QWidget
	{
		Q_OBJECT

	public:
		MemoryWindow(QWidget* parent, gbhw_context_t hardware);
		~MemoryWindow();

		void UpdateView();

	private slots:
		void OnAddressChanged(const QString& str);

	private:
		Ui::GBDMemoryWindowClass	m_ui;
		gbhw_context_t				m_hardware;
		MemoryLineList				m_lines;
		gbhw::Address				m_enteredAddress;
	};
}