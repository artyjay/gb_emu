#include "glcontext.h"

#ifdef GBEnableGLFW
#include <GLFW/glfw3.h>
#endif

namespace gbe
{
	// -------------------------------------------------------------------------

#ifdef GBEnableGLFW

	struct vec2
	{
		float x;
		float y;
	};

	static void glfw_error_callback(int error, const char* description)
	{
		// log_error("GLFW error (%d): %s\n", error, description);
	}

	// -----------------------------------------------------------------------------------------------------------------4

	class GLContextGLFW : public GLContext
	{
	public:
		GLContextGLFW(Emulator* emulator)
			: GLContext(emulator)
		{
		}

		~GLContextGLFW()
		{
		}

		bool initialise(const GLContextSettings& settings)
		{
			if (!glfwInit())
				return false;

			glfwSetErrorCallback(glfw_error_callback);
			glfwWindowHint(GLFW_CLIENT_API, settings.bES ? GLFW_OPENGL_ES_API : GLFW_OPENGL_API);

			if (!settings.bES)
			{
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
			}

			if(settings.bFullscreen)
				m_window = glfwCreateWindow((int)settings.width, (int)settings.height, "Gameboy Emulator", glfwGetPrimaryMonitor(), nullptr);
			else
				m_window = glfwCreateWindow((int)settings.width, (int)settings.height, "Gameboy Emulator", nullptr, nullptr);

			m_windowWidth	= settings.width;
			m_windowHeight	= settings.height;

			if (!m_window)
				return false;

			glfwMakeContextCurrent(m_window);

			if (settings.bES)
				gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);
			else
				gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


			GLFWAPI GLFWkeyfun glfwSetKeyCallback(GLFWwindow* window, GLFWkeyfun cbfun);
			glfwSwapInterval(0);

			return true;
		}

		void release()
		{
			if (m_window)
				glfwDestroyWindow(m_window);

			glfwTerminate();
		}

		void poll()
		{
			glfwPollEvents();
		}

		void swap()
		{
			glfwSwapBuffers(m_window);
		}

		void make_current()
		{
			glfwMakeContextCurrent(m_window);
		}

	private:
		GLFWwindow* m_window;
	};

#endif

	// -----------------------------------------------------------------------------------------------------------------

#if defined(GBEnableEGL) && defined(__ANDROID__)
	#include <android/native_window_jni.h>
	#include <EGL/egl.h>

	static const char* egl_error_to_str(EGLint error)
	{
		switch(error)
		{
			case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
			case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
			case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
			case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
			case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
			case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
			case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
			case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
			case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
			case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
			case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
			case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
			case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
			case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
			default: break;
		}
		return "unknown";
	}

	#define VNCheckEGLB(op) if(!(op)) { VNLogError("EGL call failed (%u - %s): %s\n", __LINE__, egl_error_to_str(eglGetError()), #op); return false; }
	#define VNCheckEGL(op)  if(!(op)) { VNLogError("EGL call failed (%u - %s): %s\n", __LINE__, egl_error_to_str(eglGetError()), #op); return; }

	namespace
	{
		const EGLint kConfigAtrribs[] =
		{
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_BLUE_SIZE,	8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE,	8,
			EGL_NONE
		};

		const EGLint kContextAttribsES3[] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
		};

		const EGLint kContextAttribsES2[] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
		};

		static VNInline const char* get_egl_client_type_string(EGLint clientType)
		{
			switch(clientType)
			{
				case EGL_OPENGL_API:		return "OpenGL";
				case EGL_OPENGL_ES_API:		return "OpenGL ES";
				case EGL_OPENVG_API:		return "OpenVG";
				default: break;
			}

			return "unknown";

		}
	}

	// -----------------------------------------------------------------------------------------------------------------

	class DecoderGLContextEGL : public DecoderGLContext
	{
	public:
		DecoderGLContextEGL()
			: m_window(nullptr)
			, m_display(nullptr)
			, m_config(nullptr)
			, m_numConfigs(0)
			, m_format(0)
			, m_surface(nullptr)
			, m_context(nullptr)
		{
		}

		~DecoderGLContextEGL()
		{
		}

		bool initialise(DecoderGL* decoder, const DecoderGLContextSettings* settings)
		{
			VNLogDebug("Creating EGL context\n");

			VNCheckEGLB((m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) != EGL_NO_DISPLAY);
			VNCheckEGLB(eglInitialize(m_display, 0, 0));
			VNCheckEGLB(eglChooseConfig(m_display, kConfigAtrribs, &m_config, 1, &m_numConfigs));

			// Setup display and config
			VNLogDebug("Got configs: %d\n", m_numConfigs);

			VNCheckEGLB(eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &m_format));

			// Start with Es3 and backoff to ES2 if it fails.
			if((m_context = eglCreateContext(m_display, m_config, 0, kContextAttribsES3)) == nullptr)
			{
				VNLogDebug("Failed to create EGL context for ES3. Attempting ES2\n");
				VNCheckEGLB((m_context = eglCreateContext(m_display, m_config, 0, kContextAttribsES2)) != nullptr);
			}

			m_decoder = decoder;
			m_window = (ANativeWindow*)settings->windowHandle;

			// Android specific, better to do the stretching in the GL rendering
			//ANativeWindow_setBuffersGeometry(m_window, settings->width, settings->height, m_format);

			// Create window and context
			VNCheckEGLB((m_surface = eglCreateWindowSurface(m_display, m_config, m_window, 0)) != nullptr);
			VNCheckEGLB(eglMakeCurrent(m_display, m_surface, m_surface, m_context));
			VNCheckEGLB(eglSwapInterval(m_display, 1));

			// Query window size (@todo: Utilise this to perform some letterboxing of video based upon video res).
			VNCheckEGLB(eglQuerySurface(m_display, m_surface, EGL_WIDTH, (EGLint*)&m_windowWidth));
			VNCheckEGLB(eglQuerySurface(m_display, m_surface, EGL_HEIGHT, (EGLint*)&m_windowHeight));

			// Grab some important details about the EGL implementation.
			EGLint clientType, clientVersion;
			eglQueryContext(m_display, m_context, EGL_CONTEXT_CLIENT_TYPE, &clientType);
			eglQueryContext(m_display, m_context, EGL_CONTEXT_CLIENT_VERSION, &clientVersion);

			// Log this info.
			VNLogDebug("EGL initialised\n");
			VNLogDebug("EGL_VERSION:                %s\n", eglQueryString(m_display, EGL_VERSION));
			VNLogDebug("EGL_VENDOR:                 %s\n", eglQueryString(m_display, EGL_VENDOR));
			VNLogDebug("EGL_CLIENT_APIS:            %s\n", eglQueryString(m_display, EGL_CLIENT_APIS));
			VNLogDebug("EGL_EXTENSIONS:             %s\n", eglQueryString(m_display, EGL_EXTENSIONS));
			VNLogDebug("EGL_CONTEXT_CLIENT_TYPE:    %s\n", get_egl_client_type_string(clientType));
			VNLogDebug("EGL_CONTEXT_CLIENT_VERSION: %d\n", clientVersion);
			VNLogDebug("Display resolution:         %ux%u\n", m_windowWidth, m_windowHeight);

			if (settings->bES)
				gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress);
			else
				gladLoadGLLoader((GLADloadproc)eglGetProcAddress);

			return true;
		}

		void release()
		{
			eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(m_display, m_context);
			eglDestroySurface(m_display, m_surface);
			eglTerminate(m_display);

			m_display = EGL_NO_DISPLAY;
			m_surface = EGL_NO_SURFACE;
			m_context = EGL_NO_CONTEXT;
		}

		void poll()
		{
		}

		void swap()
		{
			VNCheckEGL(eglSwapBuffers(m_display, m_surface));
		}

		void new_ui_frame()
		{
#ifdef VNEnableDecUI
			m_impl.new_frame();
#endif
		}

		void make_current()
		{
			VNCheckEGL(eglMakeCurrent(m_display, m_surface, m_surface, m_context));
		}

		void* get_gl_context()
		{
			return m_context;
		}

		void* get_gl_display()
		{
			return m_display;
		}


	private:
		DecoderGL*			m_decoder;
		DecoderUIImplEGL	m_impl;
		ANativeWindow*		m_window;
		EGLDisplay			m_display;
		EGLConfig			m_config;
		EGLint				m_numConfigs;
		EGLint				m_format;
		EGLSurface			m_surface;
		EGLContext			m_context;
	};

#endif
	// -----------------------------------------------------------------------------------------------------------------

	GLContext::GLContext(Emulator_ptr emulator)
		: m_emulator(emulator)
		, m_windowWidth(0)
		, m_windowHeight(0)
	{
	}

	GLContext::~GLContext()
	{
	}

	GLContext_ptr GLContext::create_context(Emulator* emulator, const GLContextSettings& settings)
	{
		GLContext* context = nullptr;

#if defined(GBEnableGLFW)
		context = new GLContextGLFW(emulator);
#elif defined(GBEnableEGL)
		context = new GLContextEGL();
#else
		#error "Unimplemented GL context"
#endif

		if(!context->initialise(settings))
		{
			delete context;
			context = nullptr;
		}

		return GLContext_ptr(context);
	}

	// -----------------------------------------------------------------------------------------------------------------
}