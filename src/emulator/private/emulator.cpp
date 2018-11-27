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
		settings.rom_path		= "D:/Development/personal/gb_emu/roms/Zelda.gb";
		//settings.rom_path		= "C:/Users/robert.johnson/Documents/personal/gb_emu/roms/Zelda.gb";
		//settings.rom_path		= "C:/Users/robert.johnson/Documents/personal/gb-test-roms/cpu_instrs/cpu_instrs.gb";

		if (gbhw_create(&settings, &m_hardware) != e_success)
			return false;

		if (gbhw_get_screen_resolution(m_hardware, &m_screenWidth, &m_screenHeight) != e_success)
			return false;

		m_screenWidth *= m_screenScale;
		m_screenHeight *= m_screenScale;

		// Create GL
		GLContextSettings glSettings = {0};
		glSettings.bES			= false;
		glSettings.bFullscreen	= false;
		glSettings.glMajor		= -1;
		glSettings.glMinor		= -1;
		glSettings.width		= m_screenWidth;
		glSettings.height		= m_screenHeight;
		glSettings.windowHandle	= nullptr;
		m_gl = GLContext::create_context(this, glSettings);

		// Initialise rendering.
		if(!initialise_rendering())
			return false;


		while (!m_bQuit)
		{
			m_gl->poll();

#if 0
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
#endif

			// Tick hardware until there's a vsync.
			gbhw_step(m_hardware, step_vsync);

			// @todo: Rate limit.

			// Render.
			render_frame();
		}

		return true;
	}

	void Emulator::set_input(gbhw_button_t button, gbhw_button_state_t state)
	{
		gbhw_set_button_state(m_hardware, button, state);
	}

	bool Emulator::initialise_rendering()
	{
		return true;
	}

	void Emulator::render_frame()
	{
#if 0
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
#endif
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