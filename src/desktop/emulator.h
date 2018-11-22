#pragma once

#include <common.h>
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
		gbhw_context_t		m_hardware;

		uint32_t			m_screenWidth;
		uint32_t			m_screenHeight;
		uint32_t			m_screenScale;

	};
}