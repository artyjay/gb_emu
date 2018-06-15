#pragma once

#include <gbhw.h>
#include <SDL.h>

namespace gbe
{
	class Emulator
	{
	public:
		Emulator();
		~Emulator();

		bool Run();

	private:
		void UpdateSurface();

		bool				m_bQuit;
		SDL_Window*			m_window;
		SDL_Surface*		m_screen;
		SDL_Texture*		m_texture;
		SDL_PixelFormat*	m_format;
		gbhw::Hardware		m_hardware;
		uint32_t			m_screenScale;
	};
}