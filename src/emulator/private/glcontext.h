#pragma once

#include <common.h>
#include "glad.h"

namespace gbe
{
	//--------------------------------------------------------------------------

	class Emulator;
	class GLContext;
	using GLContext_ptr = std::shared_ptr<GLContext>;

	//--------------------------------------------------------------------------

	struct GLContextSettings
	{
		uint32_t	width;
		uint32_t	height;

		bool		bFullscreen;
		bool		bES;
		uint32_t	glMajor;
		uint32_t	glMinor;

		void*		windowHandle;
	};

	class GLContext
	{
	public:
		GLContext(Emulator* emulator);
		virtual ~GLContext();

		virtual bool	initialise(const GLContextSettings& settings) = 0;
		virtual void	release() = 0;
		virtual void	poll() = 0;
		virtual void	swap() = 0;
		virtual void	make_current() = 0;

		static GLContext_ptr create_context(Emulator* emulator, const GLContextSettings& settings);

	protected:
		Emulator*	m_emulator;
		uint32_t	m_windowWidth;
		uint32_t	m_windowHeight;
	};

	//--------------------------------------------------------------------------
}