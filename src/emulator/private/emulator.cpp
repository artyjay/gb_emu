#include "emulator.h"
#include "gbe.h"

namespace gbe
{
	//--------------------------------------------------------------------------

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

	//--------------------------------------------------------------------------

	Emulator::Emulator()
		: m_bQuit(false)
		, m_screenWidth(0)
		, m_screenHeight(0)
		, m_screenScale(4)
	{
	}

	Emulator::~Emulator()
	{
	}

	bool Emulator::main_loop()
	{
		log_message("Welcome to a Gameboy emulator\n");

		// Create hardware
		gbhw_settings_t settings = {0};
		settings.log_callback	= hw_log_callback;
		settings.log_level		= l_debug;
		settings.log_userdata	= this;
		//settings.rom_path		= "D:/Development/personal/gb_emu/roms/Zelda.gb";
		settings.rom_path		= "D:/Development/personal/gb_emu/roms/ZeldaDX.gbc";
//		settings.rom_path		= "D:/Development/personal/gb_emu/roms/PokemonRed.gb";
		//settings.rom_path		= "C:/Users/robert.johnson/Documents/personal/gb_emu/roms/Zelda.gb";
		//settings.rom_path		= "C:/Users/robert.johnson/Documents/personal/gb_emu/roms/ZeldaDX.gbc";
		//settings.rom_path		= "C:/Users/robert.johnson/Documents/personal/gb-test-roms/cpu_instrs/cpu_instrs.gb";

		if (gbhw_create(&settings, &m_hardware) != e_success)
			return false;

		if (gbhw_get_screen_resolution(m_hardware, &m_screenWidth, &m_screenHeight) != e_success)
			return false;

		// Setup SDL
		SDL_Init(SDL_INIT_EVERYTHING);

		m_window = SDL_CreateWindow("Gameboy Emulator",
									SDL_WINDOWPOS_UNDEFINED,
									SDL_WINDOWPOS_UNDEFINED,
									m_screenWidth * m_screenScale,
									m_screenHeight * m_screenScale,
									SDL_WINDOW_SHOWN);

		if(!m_window)
		{
			log_message("Failed to create window, error: %s\n", SDL_GetError());
			return false;
		}

		m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if(!m_renderer)
		{
			log_message("Failed to create renderer, error: %s\n", SDL_GetError());
			return false;
		}

		m_texture = SDL_CreateTexture(m_renderer,
									  SDL_PIXELFORMAT_BGRX8888,
									  SDL_TEXTUREACCESS_STREAMING,
									  m_screenWidth,
									  m_screenHeight);
		if(!m_texture)
		{
			log_message("Failed to create texture, error: %s\n", SDL_GetError());
			return false;
		}

		// Enter main-loop.
		SDL_Event e;

		while (!m_bQuit)
		{
			while (SDL_PollEvent(&e) != 0)
			{
				//User requests quit
				if (e.type == SDL_QUIT)
				{
					m_bQuit = true;
				}
				else if ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP))
				{
					gbhw_button_state_t state = (e.type == SDL_KEYDOWN) ? button_pressed : button_released;

					switch (e.key.keysym.sym)
					{
						case SDLK_LEFT:			gbhw_set_button_state(m_hardware, button_dpad_left, state); break;
						case SDLK_RIGHT:		gbhw_set_button_state(m_hardware, button_dpad_right, state); break;
						case SDLK_UP:			gbhw_set_button_state(m_hardware, button_dpad_up, state); break;
						case SDLK_DOWN:			gbhw_set_button_state(m_hardware, button_dpad_down, state); break;
						case SDLK_RETURN:		gbhw_set_button_state(m_hardware, button_start, state); break;
						case SDLK_BACKSPACE:	gbhw_set_button_state(m_hardware, button_select, state); break;
						case SDLK_SPACE:		gbhw_set_button_state(m_hardware, button_a, state); break;
						case SDLK_b:			gbhw_set_button_state(m_hardware, button_b, state); break;
						default: break;
					}
				}
			}

			// Tick hardware until there's a vsync.
			gbhw_step(m_hardware, step_vsync);

			// @todo: Rate limit.

			// Render.
			render_frame();
		}

		SDL_DestroyTexture(m_texture);
		SDL_DestroyRenderer(m_renderer);
		SDL_DestroyWindow(m_window);
		SDL_Quit();

		return true;
	}

	void Emulator::set_input(gbhw_button_t button, gbhw_button_state_t state)
	{
		gbhw_set_button_state(m_hardware, button, state);
	}

	void Emulator::render_frame()
	{
		const uint8_t* data	= nullptr;
		if(gbhw_get_screen(m_hardware, &data) != e_success)
			return;

		// Screen data is generated as XRGB8 data
		SDL_UpdateTexture(m_texture, nullptr, data, m_screenWidth * 4);
		SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
		SDL_RenderPresent(m_renderer);
	}
}

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

using namespace gbe;

extern "C"
{
	EPublicAPI gbe_context_t gbe_create()
	{
		Emulator* emu = new Emulator;
		return (gbe_context_t)emu;
	}

	EPublicAPI void gbe_destroy(gbe_context_t ctx)
	{
		if(!ctx)
			return;

		Emulator* emu = (Emulator*)ctx;
		delete emu;
	}

	EPublicAPI int32_t gbe_main_loop(gbe_context_t ctx)
	{
		if(!ctx)
			return -1;

		Emulator* emu = (Emulator*)ctx;
		return emu->main_loop() ? 0 : -1;
	}
}

//------------------------------------------------------------------------------