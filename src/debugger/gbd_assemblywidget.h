#pragma once

#include <gbhw_debug.h>
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

		void SetHardware(gbhw_context_t hardware);
		void UpdateView();
		void ToggleBreakpoint();

	private:
		void AddBreakpoint(uint16_t address);

		gbhw_context_t			m_hardware;
		gbhw::InstructionList	m_instructions;
		InstructionItemList		m_lines;
		BreakpointList			m_breakpoints;
		QBrush					m_brushDefault;
		QBrush					m_brushBreakpoint;
	};
}