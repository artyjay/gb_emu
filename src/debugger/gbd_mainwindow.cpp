#include "gbd_mainwindow.h"
#include "gbd_log.h"

#include <iostream>
#include <QtWidgets/QFileDialog>

using namespace gbhw;

namespace
{
	template<typename... Args>
	inline void log_message(const char* format, Args... parameters)
	{
		static char buffer[4096];

		int32_t len = sprintf(buffer, format, std::forward<Args>(parameters)...);

		fwrite(buffer, sizeof(char), len, stdout);

#ifdef WIN32
		OutputDebugString(buffer);
#endif
	}

	void hw_log_callback(void* userdata, gbhw_log_level_t level, const char* msg)
	{
		log_message(msg);
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

		// Create hardware.
		gbhw_settings_t settings = {0};
		settings.log_callback = hw_log_callback;
		gbhw_create(&settings, &m_hardware);

		m_ui.m_assembly->SetHardware(m_hardware);

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
		if(gbhw_load_rom_file(m_hardware, path.c_str()) == e_success)
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
		Registers* registers;
		if(gbhw_get_registers(m_hardware, &registers) != e_success)
		{
			return;
		}

		m_ui.reg_a_->setText(QString::asprintf("0x%02x", registers->a));
		m_ui.reg_b_->setText(QString::asprintf("0x%02x", registers->b));
		m_ui.reg_c_->setText(QString::asprintf("0x%02x", registers->c));
		m_ui.reg_d_->setText(QString::asprintf("0x%02x", registers->d));
		m_ui.reg_e_->setText(QString::asprintf("0x%02x", registers->e));
		m_ui.reg_f_->setText(QString::asprintf("0x%02x", registers->f));
		m_ui.reg_h_->setText(QString::asprintf("0x%02x", registers->h));
		m_ui.reg_l_->setText(QString::asprintf("0x%02x", registers->l));

		m_ui.reg_af_->setText(QString::asprintf("0x%04x", registers->af));
		m_ui.reg_bc_->setText(QString::asprintf("0x%04x", registers->bc));
		m_ui.reg_de_->setText(QString::asprintf("0x%04x", registers->de));
		m_ui.reg_hl_->setText(QString::asprintf("0x%04x", registers->hl));

		m_ui.reg_pc_->setText(QString::asprintf("0x%04x", registers->pc));
		m_ui.reg_sp_->setText(QString::asprintf("0x%04x", registers->sp));

		m_ui.m_flagZero->setChecked(registers->is_flag_set(RF::Zero));
		m_ui.m_flagNegative->setChecked(registers->is_flag_set(RF::Negative));
		m_ui.m_flagHalfCarry->setChecked(registers->is_flag_set(RF::HalfCarry));
		m_ui.m_flagCarry->setChecked(registers->is_flag_set(RF::Carry));
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

			//m_hardware.SetExecuteMode(gbhw::ExecuteMode::SingleVSync);
			//m_hardware.GetCPU().BreakpointSkip();

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
		auto filename = QFileDialog::getOpenFileName(this, tr("Open Rom File"), "../../../roms/", tr("Gameboy Files (*.gb *.gbc)")).toStdString();
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
			gbhw_step(m_hardware, step_instruction);

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
			gbhw_step(m_hardware, step_vsync);
			UpdateGPUState();
		}
// @todo: Handle breakpoints.
//
// 		if(m_hardware.GetCPU().IsBreakpoint())
// 		{
// 			PauseEmulation();
// 		}
	}
} // gbd