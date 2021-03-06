#include "gbhw.h"
#include "gbhw_debug.h"
#include "cpu.h"
#include "gpu.h"
#include "log.h"
#include "mmu.h"
#include "rom.h"
#include "timer.h"

using namespace gbhw;

extern "C"
{
	typedef struct gbhw_context
	{
		CPU		cpu;
		GPU		gpu;
		MMU		mmu;
		Rom		rom;
		Timer	timer;
	} gbhw_context, *gbhw_context_t;

	HWPublicAPI gbhw_errorcode_t gbhw_create(gbhw_settings_t* settings, gbhw_context_t* ctx)
	{
		if(!settings || !ctx)
			return e_invalidparam;

		// Hook up logging mechanism.
		Log::instance().initialise(settings->log_level, settings->log_callback, settings->log_userdata);

		// Initialise components.
		gbhw_context_t res = new gbhw_context;
		res->cpu.initialise(&res->mmu);
		res->gpu.initialise(&res->cpu, &res->mmu);
		res->mmu.initialise(&res->cpu, &res->gpu, &res->rom);
		res->timer.initialise(&res->cpu, &res->mmu);
		*ctx = res;

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

		delete ctx;
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

		ctx->rom.load(memory, length);

		// Reset the mmu with rom cartridge type
		ctx->mmu.reset(ctx->rom.get_cartridge_type());

		// @todo: Reset a whole bunch of other stuff too.

		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_get_screen(gbhw_context_t ctx, const uint8_t** screen)
	{
		if(!ctx)
			return e_invalidparam;

		if(screen)
			*screen = ctx->gpu.get_screen_data();

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

		// single cycle.
		uint16_t cpucycles = 0;
		uint16_t maxcycles = 1;
		bool bLoop = false;

 		if (mode == step_vsync)
 		{
 			maxcycles = 1;
 			bLoop = true;
 		}

		// Just updates interrupts when stop or halt is called.
		if (ctx->cpu.is_stalled())
		{
			ctx->cpu.update_stalled();
		}
		else
		{
			do
			{
				cpucycles = ctx->cpu.update(maxcycles);
				ctx->timer.update(cpucycles);

				cpucycles >>= ctx->cpu.get_speed();

				ctx->mmu.update(cpucycles);
				ctx->gpu.update(cpucycles);

				if(ctx->cpu.is_bugchecked())
					return e_failed;

				if (ctx->cpu.is_stalled() || ctx->gpu.reset_vblank_notify())
					break;

			} while (bLoop);
		}

		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t ctx, gbhw_button_t button, gbhw_button_state_t state)
	{
		if(!ctx)
			return e_invalidparam;

		ctx->mmu.set_button_state(button, state);
		return e_success;
	}

#ifdef EMSCRIPTEN

	static void gbhw_log_callback_web(void* userdata, gbhw_log_level_t level, const char* msg)
	{
		printf("%s", msg);
		fflush(stdout);
	}

	HWPublicAPI gbhw_context_t gbhw_create_web(const uint8_t* rom, uint32_t rom_size)
	{
		gbhw_settings_t settings = {0};
		settings.rom			= rom;
		settings.rom_size		= rom_size;
		settings.log_callback	= gbhw_log_callback_web;
		settings.log_level		= l_debug;

		gbhw_context_t res = nullptr;
		gbhw_create(&settings, &res);
		return res;
	}

	HWPublicAPI const uint8_t* gbhw_get_screen_web(gbhw_context_t ctx)
	{
		const uint8_t* screen;
		gbhw_get_screen(ctx, &screen);
		return screen;
	}

	HWPublicAPI uint32_t gbhw_get_screen_resolution_width(gbhw_context_t ctx)
	{
		uint32_t w, h;
		gbhw_get_screen_resolution(ctx, &w, &h);
		return w;
	}

	HWPublicAPI uint32_t gbhw_get_screen_resolution_height(gbhw_context_t ctx)
	{
		uint32_t w, h;
		gbhw_get_screen_resolution(ctx, &w, &h);
		return h;
	}

#endif

	//--------------------------------------------------------------------------
	// Debug API
	//--------------------------------------------------------------------------

#if HWEnableDebug

	HWPublicAPI gbhw_errorcode_t gbhw_get_cpu(gbhw_context_t ctx, gbhw::CPU** cpu)
	{
		if(!ctx)
			return e_invalidparam;

		*cpu = &ctx->cpu;
		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_get_gpu(gbhw_context_t ctx, gbhw::GPU** gpu)
	{
		if(!ctx)
			return e_invalidparam;

		*gpu = &ctx->gpu;
		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_get_mmu(gbhw_context_t ctx, gbhw::MMU** mmu)
	{
		if(!ctx)
			return e_invalidparam;

		*mmu = &ctx->mmu;
		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_get_registers(gbhw_context_t ctx, gbhw::Registers** registers)
	{
		if(!ctx)
			return e_invalidparam;

		*registers = ctx->cpu.get_registers();
		return e_success;
	}

	HWPublicAPI gbhw_errorcode_t gbhw_disasm(gbhw_context_t ctx, gbhw::Address address, gbhw::InstructionList& instructions, int32_t instructionCount)
	{
		if(!ctx)
			return e_invalidparam;

		MMU& mmu = ctx->mmu;
		CPU& cpu = ctx->cpu;

		const uint8_t* instructionAddress = mmu.get_memory_ptr_from_addr(address);
		const uint8_t* baseInstruction = mmu.get_memory_ptr_from_addr(0);
		int32_t instructionsProcessed = 0;

		while (true)
		{
			Byte opcode = *instructionAddress;
			Instruction instruction;

			if (opcode == 0xCB)
			{
				Byte extended = *(instructionAddress + 1);
				instruction = cpu.get_instruction(extended, true);
			}
			else
			{
				instruction = cpu.get_instruction(opcode, false);
			}

			instruction.set(address);

			const uint8_t* oldInstructionAddress = instructionAddress;
			instructionAddress += instruction.byte_size();
			address += instruction.byte_size();

			if (instructionAddress == oldInstructionAddress)
			{
				instructionAddress++; // W\A - XX instructions should be parsed as 1 byte to prevent this.
			}

			instructions.push_back(instruction);
			instructionsProcessed++;

			if (instructionsProcessed >= instructionCount)
			{
				break;
			}
		}

		return e_success;
	}

	//--------------------------------------------------------------------------
#endif
}