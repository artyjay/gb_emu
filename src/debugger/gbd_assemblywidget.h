#pragma once

#include "gbhw.h"

#include <QListWidget>

typedef std::vector<QListWidgetItem*> InstructionItemList;
typedef std::vector<gbhw::Address> BreakpointList;

namespace gbd
{
	class AssemblyWidget : public QListWidget
	{
		Q_OBJECT

	public:
		AssemblyWidget(QWidget* parent = nullptr);
		~AssemblyWidget();

		void SetHardware(gbhw::Hardware* hardware);
		void UpdateView();
		void ToggleBreakpoint();

	private:
		void AddBreakpoint(gbhw::Address address);

		gbhw::Hardware*			m_hardware;
		gbhw::InstructionList	m_instructions;
		InstructionItemList		m_lines;
		BreakpointList			m_breakpoints;
		QBrush					m_brushDefault;
		QBrush					m_brushBreakpoint;
	};
}