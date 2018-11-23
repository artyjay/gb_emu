#pragma once

#include "gbd_memorywindow.h"
#include "gbd_screenwindow.h"
#include "ui_gbd_mainwindow.h"

#include <gbhw_debug.h>
#include <QTimer>
#include <QtWidgets/QMainWindow>
#include <map>
#include <vector>

namespace gbd
{
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		MainWindow(QWidget *parent = nullptr);
		~MainWindow();

	private:
		void OpenRomFile(const std::string& path);
		void UpdateCPUState();
		void UpdateGPUState();
		void UpdateAssemblyState();
		void UpdateStatusLabel();

		void RunEmulation();
		void PauseEmulation();

		// Callbacks
		void OnOpenRomFileTriggered(bool checked);

		void OnContinueEmulationTriggered(bool checked);
		void OnPauseEmulationTriggered(bool checked);
		void OnStepInstructionTriggered(bool checked);
		void OnToggleBreakpointTriggered(bool checked);

		void OnRunningUpdate();

		Ui::GBDMainWindowClass	m_ui;
		gbhw_context_t			m_hardware;
		bool					m_bPaused;

		QBrush					m_brushDefault;

		QTimer					m_hardwareTimer;

		ScreenWindow*			m_screenWindow;
		MemoryWindow*			m_memoryWindow;
	};
} // gbd