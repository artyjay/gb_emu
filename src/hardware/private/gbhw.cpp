#include "gbhw.h"
#include "context.h"

#include <chrono>

using namespace gbhw;

extern "C"
{
	gbhw_errorcode_t gbhw_create(gbhw_settings_t* settings, gbhw_context_t** ctx)
	{
		if(!settings || !ctx)
			return e_invalidparam;

		Context* context = new Context();
		if(!context->initialise())
		{
			delete ctx;
			return e_failed;
		}

		*ctx = (gbhw_context_t*)context;

		return e_success;
	}

	void gbhw_destroy(gbhw_context_t* ctx)
	{
		if(!ctx)
			return;

		Context* context = (Context*)ctx;
		delete context;
	}

	gbhw_errorcode_t gbhw_load_rom_file(gbhw_context_t* ctx, const char* path)
	{
		if(!ctx || !path)
			return e_invalidparam;

		// @todo: Implement me.

		return e_success;
	}

	gbhw_errorcode_t gbhw_load_rom_memory(gbhw_context_t* ctx, const uint8_t* memory, uint32_t length)
	{
		if(!ctx || !memory || !length)
			return e_invalidparam;

		return e_success;
	}

	gbhw_errorcode_t gbhw_get_screen(gbhw_context_t* ctx, const uint8_t** screen, uint32_t* width, uint32_t* height)
	{
		if(!ctx || !screen || !width || !height)
			return e_invalidparam;

		return e_success;
	}

	gbhw_errorcode_t gbhw_step(gbhw_context_t* ctx, gbhw_step_mode_t mode)
	{
		if(!ctx)
			return e_invalidparam;

		return e_success;
	}

	gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t* ctx, gbhw_button_t button, gbhw_button_state_t state)
	{
		if(!ctx)
			return e_invalidparam;

		return e_success;
	}
}

#if 0
namespace gbhw
{
	namespace
	{
		uint64_t GetWallTime()
		{
			return std::chrono::steady_clock::now().time_since_epoch().count();
		}
	}

	Hardware::Hardware()
		: m_executeMode(ExecuteMode::SingleInstruction)
	{
	}

	Hardware::~Hardware()
	{
	}

	void Hardware::Initialise()
	{
		// Create components.
		m_cpu = new CPU();
		m_gpu = new GPU();
		m_mmu = new MMU();
		m_rom = new Rom();
		m_timer = new Timer();

		// Assign various components to each others for communication.
		m_cpu->Initialise(m_mmu);
		m_gpu->Initialise(m_cpu, m_mmu);
		m_mmu->Initialise(m_cpu, m_gpu, m_rom);
		m_rom->Initialise(m_cpu, m_mmu);
		m_timer->Initialise(m_cpu);
	}

	void Hardware::Release()
	{
		delete m_cpu;
		delete m_gpu;
		delete m_mmu;
		delete m_rom;
		delete m_timer;
	}

	bool Hardware::LoadROM(const char* path)
	{
		FILE* romfile = fopen(path, "rb");

		if(romfile)
		{
			fseek(romfile, 0, SEEK_END);
			auto romlength = ftell(romfile);
			fseek(romfile, 0, SEEK_SET);

			uint8_t* romdata = new uint8_t[romlength];
			fread(romdata, 1, romlength, romfile);
			fclose(romfile);

			m_rom->Load(romdata, romlength);

			// Reset the mmu with rom cartridge type.
			m_mmu->Reset(m_rom->GetCartridgeType());

			return true;
		}

		return false;
	}

	void Hardware::Execute()
	{
		// single cycle.
		uint16_t cpucycles = 0;
		uint16_t maxcycles = 1;
		bool bLoop = false;

		if (m_executeMode == gbhw::ExecuteMode::SingleVSync)
		{
			maxcycles = 40;
			bLoop = true;
		}

		// Just updates interrupts when stop or halt is called.
		if (m_cpu->IsStalled())
		{
			m_cpu->UpdateStalled();
		}
		else
		{
			// V-sync is 59.73 Hz
			// 0.016742 seconds
			// 16.74200569228194 ms per v-blank.
			// 16742 microseconds
			// 16742005 nanoseconds.
			//uint64_t start = GetWallTime();

			do
			{
				cpucycles = m_cpu->Update(maxcycles);
				m_timer->Update(cpucycles);
				m_gpu->Update(cpucycles);

				if (m_cpu->IsStalled() || m_cpu->IsBreakpoint())
				{
					break;
				}

				// If a vblank occured, then we want to sync externally.
				if (m_gpu->GetResetVBlankNotify())
				{
					break;
				}

			} while (bLoop);

			// improve for debugger usage...
// 			uint64_t end = GetWallTime();
// 			uint64_t delta = end - start;
// 			while (delta < 8742005)
// 			{
// 				delta = GetWallTime() - start;
// 			}
		}
	}

	ExecuteMode::Type Hardware::GetExecuteMode() const
	{
		return m_executeMode;
	}

	void Hardware::SetExecuteMode(ExecuteMode::Type mode)
	{
		m_executeMode = mode;
	}

	void Hardware::RegisterLogCallback(LogCallback callback)
	{
		Log::GetLog().RegisterCallback(callback);
	}

	void Hardware::PressButton(HWButton::Type button)
	{
		m_mmu->PressButton(button);
	}

	void Hardware::ReleaseButton(HWButton::Type button)
	{
		m_mmu->ReleaseButton(button);
	}

	const Byte* Hardware::GetScreenData() const
	{
		return m_gpu->GetScreenData();
	}
}

#endif