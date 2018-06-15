#pragma once

#include "gbhw.h"
#include "ui_gbd_memorywindow.h"
#include <QtWidgets/QWidget>
#include <vector>

typedef std::vector<QListWidgetItem*> MemoryLineList;

namespace gbd
{
	class MemoryWindow : public QWidget
	{
		Q_OBJECT

	public:
		MemoryWindow(QWidget* parent, gbhw::Hardware& hardware);
		~MemoryWindow();

		void UpdateView();

	private slots:
		void OnAddressChanged(const QString& str);

	private:
		Ui::GBDMemoryWindowClass	m_ui;
		gbhw::Hardware&				m_hardware;
		MemoryLineList				m_lines;
		gbhw::Address				m_enteredAddress;
	};
}