#include <gbhw.h>
#include <SDL.h>
#include <stdio.h>
#include <type_traits>
#include <utility>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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
//------------------------------------------------------------------------------

int main(int argc, char* args[])
{
	log_message("Welcome to a Gameboy emulator\n");

	if (argc != 2)
	{
		log_message("Invalid command line specified\n");
		log_message("\tUsage: EXE <ROM PATH>\n");
		return -1;
	}

	// Create hardware
	gbhw_settings_t settings	= {0};
	settings.log_callback		= hw_log_callback;
	settings.log_level			= l_debug;
	settings.log_userdata		= nullptr;
	settings.rom_path			= args[1];

	gbhw_context_t hardware		= nullptr;

	if (gbhw_create(&settings, &hardware) != e_success)
		return -1;

	// Grab screen details and setup SDL.
	uint32_t screenWidth	= 0;
	uint32_t screenHeight	= 0;
	uint32_t screenScale	= 4;	// @todo: This should be more configurable.

	if (gbhw_get_screen_resolution(hardware, &screenWidth, &screenHeight) != e_success)
		return -1;

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window*	window = SDL_CreateWindow("Gameboy Emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		screenWidth * screenScale,
		screenHeight * screenScale,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (!window)
	{
		log_message("Failed to create window, error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);

	if (!renderer)
	{
		log_message("Failed to create renderer, error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGRX8888,
		SDL_TEXTUREACCESS_STREAMING,
		screenWidth,
		screenHeight);

	if (!texture)
	{
		log_message("Failed to create texture, error: %s\n", SDL_GetError());
		return -1;
	}

	// Enter main-loop.
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				bQuit = true;
			}
			else if ((e.type == SDL_KEYDOWN) || (e.type == SDL_KEYUP))
			{
				gbhw_button_state_t state = (e.type == SDL_KEYDOWN) ? button_pressed : button_released;

				switch (e.key.keysym.sym)
				{
					case SDLK_LEFT:			gbhw_set_button_state(hardware, button_dpad_left, state); break;
					case SDLK_RIGHT:		gbhw_set_button_state(hardware, button_dpad_right, state); break;
					case SDLK_UP:			gbhw_set_button_state(hardware, button_dpad_up, state); break;
					case SDLK_DOWN:			gbhw_set_button_state(hardware, button_dpad_down, state); break;
					case SDLK_RETURN:		gbhw_set_button_state(hardware, button_start, state); break;
					case SDLK_BACKSPACE:	gbhw_set_button_state(hardware, button_select, state); break;
					case SDLK_SPACE:		gbhw_set_button_state(hardware, button_a, state); break;
					case SDLK_b:			gbhw_set_button_state(hardware, button_b, state); break;
					default: break;
				}
			}
		}

		// Tick hardware until there's a vsync.
		if(gbhw_step(hardware, step_vsync) != e_success)
			return -1;

		// @todo: Rate limit according to screen refresh.

		const uint8_t* screen = nullptr;
		if (gbhw_get_screen(hardware, &screen) != e_success)
			return -1;

		// Screen data is generated as XRGB8 data packed into uint32_t.
		SDL_UpdateTexture(texture, nullptr, screen, screenWidth * sizeof(uint32_t));
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

//------------------------------------------------------------------------------