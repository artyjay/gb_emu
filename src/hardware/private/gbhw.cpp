#include "gbhw.h"
#include "context.h"
#include "gpu.h"
#include "mmu.h"
#include "rom.h"

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

		FILE* romFile = fopen(path, "rb");
		if(!romFile)
			return e_failed;

		fseek(romFile, 0, SEEK_END);
		auto romLength = ftell(romFile);
		fseek(romFile, 0, SEEK_SET);

		Buffer romData = Buffer(romLength);
		fread(&romData[0], 1, romLength, romFile);
		fclose(romFile);

		return gbhw_load_rom_memory(ctx, &romData[0], romLength);
	}

	gbhw_errorcode_t gbhw_load_rom_memory(gbhw_context_t* ctx, const uint8_t* memory, uint32_t length)
	{
		if(!ctx || !memory || !length)
			return e_invalidparam;

		Context* context = (Context*)ctx;
		context->rom->load(memory, length);

		// Reset the mmu with rom cartridge type
		context->mmu->Reset(context->rom->get_cartridge_type());

		// @todo: Reset a whole bunch of other stuff too.

		return e_success;
	}

	gbhw_errorcode_t gbhw_get_screen(gbhw_context_t* ctx, const uint8_t** screen, uint32_t* width, uint32_t* height)
	{
		if(!ctx || !screen || !width || !height)
			return e_invalidparam;

		Context* context = (Context*)ctx;
		*screen = context->gpu->GetScreenData();
		*width	= 0;
		*height = 0;

		return e_success;
	}

	gbhw_errorcode_t gbhw_step(gbhw_context_t* ctx, gbhw_step_mode_t mode)
	{
		if(!ctx)
			return e_invalidparam;

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

		return e_success;
	}

	gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t* ctx, gbhw_button_t button, gbhw_button_state_t state)
	{
		if(!ctx)
			return e_invalidparam;

		Context* context = (Context*)ctx;
		context->mmu->set_button_state(button, state);
		return e_success;
	}
}