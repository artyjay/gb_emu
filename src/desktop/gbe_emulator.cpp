#include "gbe_emulator.h"
#include "gbe_log.h"

#include <gbhw_gpu.h>

namespace gbe
{
	namespace
	{
		void LogCallback(const char* message)
		{
			Log::GetLog().MessageRaw(message);
		}
	}

	Emulator::Emulator()
		: m_bQuit(false)
		, m_window(nullptr)
		, m_screen(nullptr)
		, m_screenScale(4)
	{
		m_hardware.RegisterLogCallback(LogCallback);
	}

	Emulator::~Emulator()
	{
	}

	bool Emulator::Run()
	{
		gbe::Message("Running gameboy emulator\n");

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			gbe::Message("Failed to init SDL\n");
			return false;
		}
		else
		{
			//Create window
			m_window = SDL_CreateWindow("GBE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gbhw::GPU::kScreenWidth * m_screenScale, gbhw::GPU::kScreenHeight * m_screenScale, SDL_WINDOW_SHOWN);

			if (m_window == NULL)
			{
				gbe::Message("Failed to create window, error: %s\n", SDL_GetError());
				return false;
			}
			else
			{
				//Get window surface
				m_screen = SDL_GetWindowSurface(m_window);

				//Event handler
				SDL_Event e;

				m_hardware.Initialise();

				// Setup hardware
				//if (!m_hardware.LoadROM("../../../Roms/Tetris.gb")
				//if (!m_hardware.LoadROM("../../../Roms/ttt.gb"))
				if (!m_hardware.LoadROM("D:/Development/personal/gb_emu/roms/Zelda.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/KirbysDreamLand.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/ZeldaLinksAwakening.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/SuperMarioLand.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/tests/cpu_instrs/cpu_instrs.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/tests/cpu_instrs/individual/01-special.gb"))
				//if (!m_hardware.LoadROM("../../../Roms/tests/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"))
				{
					gbe::Message("Failed to load rom\n");
					return false;
				}

				m_hardware.SetExecuteMode(gbhw::ExecuteMode::SingleVSync);

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
						else if (e.type == SDL_KEYDOWN)
						{
							switch (e.key.keysym.sym)
							{
							case SDLK_LEFT:
								m_hardware.PressButton(gbhw::HWButton::Left); break;
							case SDLK_RIGHT:
								m_hardware.PressButton(gbhw::HWButton::Right); break;
							case SDLK_UP:
								m_hardware.PressButton(gbhw::HWButton::Up); break;
							case SDLK_DOWN:
								m_hardware.PressButton(gbhw::HWButton::Down); break;
							case SDLK_RETURN:
								m_hardware.PressButton(gbhw::HWButton::Start); break;
							case SDLK_BACKSPACE:
								m_hardware.PressButton(gbhw::HWButton::Select); break;
							case SDLK_SPACE:
								m_hardware.PressButton(gbhw::HWButton::A); break;
							case SDLK_b:
								m_hardware.PressButton(gbhw::HWButton::B); break;
							}
						}
						else if (e.type == SDL_KEYUP)
						{
							switch (e.key.keysym.sym)
							{
							case SDLK_LEFT:
								m_hardware.ReleaseButton(gbhw::HWButton::Left); break;
							case SDLK_RIGHT:
								m_hardware.ReleaseButton(gbhw::HWButton::Right); break;
							case SDLK_UP:
								m_hardware.ReleaseButton(gbhw::HWButton::Up); break;
							case SDLK_DOWN:
								m_hardware.ReleaseButton(gbhw::HWButton::Down); break;
							case SDLK_RETURN:
								m_hardware.ReleaseButton(gbhw::HWButton::Start); break;
							case SDLK_BACKSPACE:
								m_hardware.ReleaseButton(gbhw::HWButton::Select); break;
							case SDLK_SPACE:
								m_hardware.ReleaseButton(gbhw::HWButton::A); break;
							case SDLK_b:
								m_hardware.ReleaseButton(gbhw::HWButton::B); break;
							}
						}
					}

					// Run cycles for a single picture (i.e. a v-sync occurs).
					m_hardware.Execute();

					vsynccount++;

					sprintf_s(buffer, "GBE: Frame Count: %u", vsynccount);

					SDL_SetWindowTitle(m_window, buffer);

					UpdateSurface();

				}
			}
		}

		SDL_DestroyWindow(m_window);
		SDL_Quit();

		gbe::Message("Quit gameboy emulator\n");

		return true;
	}

	void Emulator::UpdateSurface()
	{
		//gbe::Message("Update surface data now\n");

		SDL_FillRect(m_screen, nullptr, SDL_MapRGB(m_screen->format, 0x0, 0x0, 0x0));
		SDL_LockSurface(m_screen);

		// very much a hacky way of doing this, will be improved.
		uint32_t* screenpixels = (uint32_t*)(m_screen->pixels);
		const gbhw::Byte* screendata = m_hardware.GetScreenData();
		const uint32_t dstWidth = m_screenScale * gbhw::GPU::kScreenWidth;

		for(uint32_t y = 0; y < gbhw::GPU::kScreenHeight; ++y)
		{
			for(uint32_t x = 0; x < gbhw::GPU::kScreenWidth; ++x)
			{
				gbhw::Byte hwpixel = (3 - screendata[(y * gbhw::GPU::kScreenWidth) + x]) * 85;
				//gbhw::Byte hwpixel = screendata[(y * gbhw::GPU::kScreenWidth) + x];

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