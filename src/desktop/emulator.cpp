#include "emulator.h"

namespace gbe
{
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

	Emulator::Emulator()
		: m_bQuit(false)
		, m_window(nullptr)
		, m_screen(nullptr)
		, m_screenWidth(0)
		, m_screenHeight(0)
		, m_screenScale(4)
	{
	}

	Emulator::~Emulator()
	{
	}

	bool Emulator::Run()
	{
		log_message("Welcome to a Gameboy emulator\n");

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			log_message("Failed to init SDL\n");
			return false;
		}
		else
		{
			gbhw_settings_t settings = { 0 };
			settings.rom_path = "C:/Users/robert.johnson/Documents/personal/gb_emu/roms/Zelda.gb";
			//settings.rom_path = "C:/Users/robert.johnson/Documents/personal/gb-test-roms/cpu_instrs/cpu_instrs.gb";
			settings.log_callback = hw_log_callback;
			settings.log_level = l_debug;
			settings.log_userdata = this;

			if (gbhw_create(&settings, &m_hardware) != e_success)
				return false;

			if (gbhw_get_screen_resolution(m_hardware, &m_screenWidth, &m_screenHeight) != e_success)
				return false;

			//Create window
			m_window = SDL_CreateWindow("GBE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_screenWidth * m_screenScale, m_screenHeight * m_screenScale, SDL_WINDOW_SHOWN);

			if (m_window == NULL)
			{
				log_message("Failed to create window, error: %s\n", SDL_GetError());
				return false;
			}
			else
			{
				//Get window surface
				m_screen = SDL_GetWindowSurface(m_window);

				//Event handler
				SDL_Event e;

				uint32_t vsynccount = 0;
				char buffer[1024];

				while (!m_bQuit)
				{
					//Handle events on queue
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

					// Run cycles for a single picture (i.e. a v-sync occurs).
					gbhw_step(m_hardware, step_vsync);

					vsynccount++;

					sprintf_s(buffer, "GBE: Frame Count: %u", vsynccount);
					SDL_SetWindowTitle(m_window, buffer);
					UpdateSurface();

				}
			}
		}

		SDL_DestroyWindow(m_window);
		SDL_Quit();

		log_message("Quit gameboy emulator\n");

		return true;
	}

	void Emulator::UpdateSurface()
	{
		SDL_FillRect(m_screen, nullptr, SDL_MapRGB(m_screen->format, 0x0, 0x0, 0x0));
		SDL_LockSurface(m_screen);

		// very much a hacky way of doing this, will be improved.
		uint32_t* screenpixels = (uint32_t*)(m_screen->pixels);
		const uint8_t* data	= nullptr;

		// @todo: Error message.
		if(gbhw_get_screen(m_hardware, &data) != e_success)
			return;

		const uint32_t dstWidth = m_screenScale * m_screenWidth;

		for(uint32_t y = 0; y < m_screenHeight; ++y)
		{
			for(uint32_t x = 0; x < m_screenWidth; ++x)
			{
				//const uint8_t hwpixel = data[(y * m_screenWidth) + x];
				uint8_t hwpixel = (3 - data[(y * m_screenWidth) + x]) * 85;	// Arbitrary colour scaling.

				for(uint32_t ys = 0; ys < m_screenScale; ++ys)
				{
					uint32_t dsty = (y * m_screenScale) + ys;

					for(uint32_t xs = 0; xs < m_screenScale; ++xs)
					{
						uint32_t dstx = (x * m_screenScale) + xs;
						uint32_t* screenpixel = &screenpixels[(dsty * dstWidth) + dstx];
						*screenpixel = SDL_MapRGB(m_screen->format, hwpixel, hwpixel, hwpixel);
					}
				}
			}
		}

		SDL_UnlockSurface(m_screen);
		SDL_UpdateWindowSurface(m_window);
	}
}