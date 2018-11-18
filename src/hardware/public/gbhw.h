#pragma once

/*----------------------------------------------------------------------------*/

#include <stdint.h>

#ifdef _WIN32
#	ifdef GBExportHW
#		define HWPublicAPI __declspec(dllexport)
#	else
#		define HWPublicAPI __declspec(dllimport)
#	endif
#else
#	define HWPublicAPI __attribute__((visibility ("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/

typedef void* gbhw_context_t;

typedef enum gbhw_step_mode
{
	step_vsync		= 0,
	step_instruction
} gbhw_step_mode_t;

typedef enum gbhw_button
{
	button_a		= 0,
	button_b,
	button_select,
	button_start,
	button_dpad_right,
	button_dpad_left,
	button_dpad_up,
	button_dpad_down,
} gbhw_button_t;

typedef enum gbhw_button_state
{
	button_pressed	= 0,
	button_released
} gbhw_button_state_t;

typedef enum gbhw_errorcode
{
	e_success		= 0,
	e_failed		= -1,
	e_invalidparam	= -2
} gbhw_errorcode_t;

typedef enum gbhw_log_level
{
	l_debug = 0,
	l_warning,
	l_error,
	l_disabled
} gbhw_log_level_t;

typedef void(*gbhw_log_callback_t)(void* userdata, gbhw_log_level_t level, const char* msg);

typedef struct gbhw_settings
{
	const uint8_t*		rom;
	uint32_t			rom_size;
	gbhw_log_level_t	log_level;
	gbhw_log_callback_t	log_callback;
	void*				log_userdata;
} gbhw_settings_t;

/*----------------------------------------------------------------------------*/

HWPublicAPI gbhw_errorcode_t gbhw_create(gbhw_settings_t* settings, gbhw_context_t** context);

HWPublicAPI void gbhw_destroy(gbhw_context_t* context);

HWPublicAPI gbhw_errorcode_t gbhw_load_rom_file(gbhw_context_t* context, const char* path);

HWPublicAPI gbhw_errorcode_t gbhw_load_rom_memory(gbhw_context_t* context, const uint8_t* memory, uint32_t length);

HWPublicAPI gbhw_errorcode_t gbhw_get_screen(gbhw_context_t* context, const uint8_t** screen, uint32_t* width, uint32_t* height);

HWPublicAPI gbhw_errorcode_t gbhw_step(gbhw_context_t* context, gbhw_step_mode_t mode);

HWPublicAPI gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t* context, gbhw_button_t button, gbhw_button_state_t state);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif











#if 0

#include "gbhw_types.h"

namespace gbhw
{
	struct ExecuteMode
	{
		enum Type
		{
			SingleInstruction,
			SingleVSync
		};
	};

	class CPU;
	class GPU;
	class MMU;
	class Rom;
	class Timer;

	class Hardware
	{
	public:
		Hardware();
		~Hardware();

		void Initialise();
		void Release();

		bool LoadROM(const char* path);
		void Execute();

		ExecuteMode::Type GetExecuteMode() const;
		void SetExecuteMode(ExecuteMode::Type mode);
		void RegisterLogCallback(LogCallback callback);

		void PressButton(HWButton::Type button);
		void ReleaseButton(HWButton::Type button);

		const Byte* GetScreenData() const;

	private:
		ExecuteMode::Type	m_executeMode;
		CPU*				m_cpu;
		GPU*				m_gpu;
		MMU*				m_mmu;
		Rom*				m_rom;
		Timer*				m_timer;
	};
}

#endif