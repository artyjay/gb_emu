#pragma once

#include <common.h>
#include <gbhw.h>
#include "glcontext.h"

namespace gbe
{
	class Emulator
	{
	public:
		Emulator();
		~Emulator();

		bool main_loop();

		void set_input(gbhw_button_t button, gbhw_button_state_t state);


	private:
		bool initialise_rendering();
		void render_frame();

		bool				m_bQuit;
		gbhw_context_t		m_hardware;
		GLContext_ptr		m_gl;

		uint32_t			m_screenWidth;
		uint32_t			m_screenHeight;
		uint32_t			m_screenScale;
	};
}