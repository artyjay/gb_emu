#include "gbhw.h"
#include "context.h"
#include "cpu.h"
#include "gpu.h"
#include "log.h"
#include "mmu.h"
#include "rom.h"
#include "timer.h"

using namespace gbhw;

extern "C"
{
	HWPublicAPI gbhw_errorcode_t gbhw_create(gbhw_settings_t* settings, gbhw_context_t* ctx)
	{
		if(!settings || !ctx)
			return e_invalidparam;

		// Hook up logging mechanism.
		Log::instance().initialise(settings->log_level, settings->log_callback, settings->log_userdata);

		// Create context.
		Context* context = new Context();
		if(!context->initialise())
		{
			delete ctx;
			return e_failed;
		}

		*ctx = (gbhw_context_t*)context;

		// Attempt to load ROM.
		if(settings->rom_path)
			gbhw_load_rom_file(*ctx, settings->rom_path);
		else if(settings->rom)
			gbhw_load_rom_memory(*ctx, settings->rom, settings->rom_size);

		return e_success;
	}

	HWPublicAPI void gbhw_destroy(gbhw_context_t ctx)
	{
		if(!ctx)
			return;

		Context* context = (Context*)ctx;
		delete context;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_load_rom_file(gbhw_context_t ctx, const char* path)
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

	HWPublicAPI gbhw_errorcode_t gbhw_load_rom_memory(gbhw_context_t ctx, const uint8_t* memory, uint32_t length)
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

	HWPublicAPI gbhw_errorcode_t gbhw_get_screen(gbhw_context_t ctx, const uint8_t** screen)
	{
		if(!ctx)
			return e_invalidparam;

		Context* context = (Context*)ctx;

		if(screen)
			*screen = context->gpu->GetScreenData();

		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_get_screen_resolution(gbhw_context_t ctx, uint32_t* width, uint32_t* height)
	{
		if(!ctx)
			return e_invalidparam;

		if(width)
			*width = GPU::kScreenWidth;

		if(height)
			*height = GPU::kScreenHeight;

		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_step(gbhw_context_t ctx, gbhw_step_mode_t mode)
	{
		if(!ctx)
			return e_invalidparam;

		Context* context = (Context*)ctx;

		// single cycle.
		uint16_t cpucycles = 0;
		uint16_t maxcycles = 1;
		bool bLoop = false;

 		if (mode == step_vsync)
 		{
 			maxcycles = 40;
 			bLoop = true;
 		}

		// Just updates interrupts when stop or halt is called.
		if (context->cpu->is_stalled())
		{
			context->cpu->update_stalled();
		}
		else
		{
			// V-sync is 59.73 Hz
			// 0.016742 seconds
			// 16.74200569228194 ms per v-blank.
			// 16742 microseconds
			// 16742005 nanoseconds.
			do
			{
				cpucycles = context->cpu->update(maxcycles);
				context->timer->update(cpucycles);
				context->gpu->Update(cpucycles);

				if (context->cpu->is_stalled())
				{
					break;
				}

				// If a vblank occurred, then we want to sync externally.
				if (context->gpu->GetResetVBlankNotify())
				{
					break;
				}

			} while (bLoop);
		}

		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t ctx, gbhw_button_t button, gbhw_button_state_t state)
	{
		if(!ctx)
			return e_invalidparam;

		Context* context = (Context*)ctx;
		context->mmu->set_button_state(button, state);
		return e_success;
	}
}