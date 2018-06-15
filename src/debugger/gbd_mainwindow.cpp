#include "gbd_mainwindow.h"
#include "gbd_log.h"

#include <QtWidgets/QFileDialog>

#include <iostream>

namespace
{
	void LogCallback(const char* message)
	{
		gbd::MessageRaw(message);
	}
}

namespace gbd
{
	MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent)
	{
		m_ui.setupUi(this);
		m_bPaused = true;

		// Connect to ui elements for callbacks...
		m_ui.m_toolbar->addAction(m_ui.m_actionContinueEmulation);
		m_ui.m_toolbar->addAction(m_ui.m_actionPauseEmulation);
		m_ui.m_toolbar->addAction(m_ui.m_actionStepInstruction);
		m_ui.m_toolbar->addAction(m_ui.m_actionToggleBreakpoint);

		QObject::connect(m_ui.m_actionOpenRomFile,			&QAction::triggered, this, &MainWindow::OnOpenRomFileTriggered);
		QObject::connect(m_ui.m_actionContinueEmulation,	&QAction::triggered, this, &MainWindow::OnContinueEmulationTriggered);
		QObject::connect(m_ui.m_actionPauseEmulation,		&QAction::triggered, this, &MainWindow::OnPauseEmulationTriggered);
		QObject::connect(m_ui.m_actionStepInstruction,		&QAction::triggered, this, &MainWindow::OnStepInstructionTriggered);
		QObject::connect(m_ui.m_actionToggleBreakpoint,		&QAction::triggered, this, &MainWindow::OnToggleBreakpointTriggered);

		m_hardware.RegisterLogCallback(&LogCallback);

		m_ui.m_assembly->SetHardware(&m_hardware);

		// Create screen window
		m_screenWindow = new ScreenWindow(this, m_hardware);
		m_screenWindow->show();

		// Create memory window
		m_memoryWindow = new MemoryWindow(this, m_hardware);
		m_memoryWindow->show();
	}

	MainWindow::~MainWindow()
	{
	}

	void MainWindow::OpenRomFile(const std::string& path)
	{
		std::cout << "Opening rom file: " << path << std::endl;

		// load into the emulator, then parse the contents.
		if(m_hardware.LoadROM(path.c_str()))
		{
			std::cout << "Successfully loaded rom file" << std::endl;
			UpdateCPUState();
			UpdateAssemblyState();
			m_memoryWindow->UpdateView();
		}
		else
		{
			std::cout << "Failed to open rom!" << std::endl;
		}
	}

	void MainWindow::UpdateCPUState()
	{
		auto& registers = m_hardware.GetCPU().GetRegisters();
	
		m_ui.reg_a_->setText(QString::asprintf("0x%02x", registers.a));
		m_ui.reg_b_->setText(QString::asprintf("0x%02x", registers.b));
		m_ui.reg_c_->setText(QString::asprintf("0x%02x", registers.c));
		m_ui.reg_d_->setText(QString::asprintf("0x%02x", registers.d));
		m_ui.reg_e_->setText(QString::asprintf("0x%02x", registers.e));
		m_ui.reg_f_->setText(QString::asprintf("0x%02x", registers.f));
		m_ui.reg_h_->setText(QString::asprintf("0x%02x", registers.h));
		m_ui.reg_l_->setText(QString::asprintf("0x%02x", registers.l));

		m_ui.reg_af_->setText(QString::asprintf("0x%04x", registers.af));
		m_ui.reg_bc_->setText(QString::asprintf("0x%04x", registers.bc));
		m_ui.reg_de_->setText(QString::asprintf("0x%04x", registers.de));
		m_ui.reg_hl_->setText(QString::asprintf("0x%04x", registers.hl));

		m_ui.reg_pc_->setText(QString::asprintf("0x%04x", registers.pc));
		m_ui.reg_sp_->setText(QString::asprintf("0x%04x", registers.sp));

		m_ui.m_flagZero->setChecked(registers.IsFlagSet(gbhw::RF::Zero));
		m_ui.m_flagNegative->setChecked(registers.IsFlagSet(gbhw::RF::Negative));
		m_ui.m_flagHalfCarry->setChecked(registers.IsFlagSet(gbhw::RF::HalfCarry));
		m_ui.m_flagCarry->setChecked(registers.IsFlagSet(gbhw::RF::Carry));
	}

	void MainWindow::UpdateGPUState()
	{
		m_screenWindow->repaint();
	}

	void MainWindow::UpdateAssemblyState()
	{
		m_ui.m_assembly->UpdateView();
	}

	void MainWindow::UpdateStatusLabel()
	{
		if(m_bPaused)
		{
			m_ui.m_status->setText("Paused");
		}
		else
		{
			m_ui.m_status->setText("Running");
		}
	}

	void MainWindow::RunEmulation()
	{
		if (m_bPaused == true)
		{
			m_bPaused = false;

			UpdateStatusLabel();

			m_hardware.SetExecuteMode(gbhw::ExecuteMode::SingleVSync);
			m_hardware.GetCPU().BreakpointSkip();

			QObject::connect(&m_hardwareTimer, &QTimer::timeout, this, &MainWindow::OnRunningUpdate);
			m_hardwareTimer.start(10);
		}
	}

	void MainWindow::PauseEmulation()
	{
		if (m_bPaused == false)
		{
			m_hardwareTimer.stop();
			QObject::disconnect(&m_hardwareTimer, &QTimer::timeout, this, &MainWindow::OnRunningUpdate);
			m_bPaused = true;

			UpdateStatusLabel();
			UpdateCPUState();
			UpdateAssemblyState();
			m_memoryWindow->UpdateView();
		}
	}

	void MainWindow::OnOpenRomFileTriggered(bool checked)
	{
		auto filename = QFileDialog::getOpenFileName(this, tr("Open Rom File"), "../../../roms/", tr("Gameboy Rom Files (*.gb)")).toStdString();
		OpenRomFile(filename);
	}

	void MainWindow::OnContinueEmulationTriggered(bool checked)
	{
		RunEmulation();
	}

	void MainWindow::OnPauseEmulationTriggered(bool checked)
	{
		PauseEmulation();
	}

	void MainWindow::OnStepInstructionTriggered(bool checked)
	{
		if(m_bPaused == true)
		{
			// Perform a single instruction step, ensuring we skip the current breakpoint (if we're on one).
			m_hardware.SetExecuteMode(gbhw::ExecuteMode::SingleInstruction);
			m_hardware.GetCPU().BreakpointSkip();
			m_hardware.Execute();
			UpdateCPUState();
			UpdateGPUState();
			UpdateAssemblyState();
			m_memoryWindow->UpdateView();
		}
	}

	void MainWindow::OnToggleBreakpointTriggered(bool checked)
	{
		m_ui.m_assembly->ToggleBreakpoint();
	}

	void MainWindow::OnRunningUpdate()
	{
		// @todo Be accurate about this by pulling the cycle count from the emulator.
		if(!m_bPaused)
		{
			m_hardware.Execute();
			UpdateGPUState();
		}

		if(m_hardware.GetCPU().IsBreakpoint())
		{
			PauseEmulation();
		}
	}
} // gbd