#pragma once

/*----------------------------------------------------------------------------*/

#include <stdint.h>

#ifdef HWExportDLL
#ifdef _WIN32
#	ifdef HWExportSymbols
#		define HWPublicAPI __declspec(dllexport)
#	else
#		define HWPublicAPI __declspec(dllimport)
#	endif
#else
#	define HWPublicAPI __attribute__((visibility ("default")))
#endif
#else
#	define HWPublicAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/

typedef struct gbhw_context *gbhw_context_t;

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
	const char*			rom_path;
	gbhw_log_level_t	log_level;
	gbhw_log_callback_t	log_callback;
	void*				log_userdata;
} gbhw_settings_t;

/*----------------------------------------------------------------------------*/

HWPublicAPI gbhw_errorcode_t gbhw_create(gbhw_settings_t* settings, gbhw_context_t* ctx);

HWPublicAPI void gbhw_destroy(gbhw_context_t ctx);

HWPublicAPI gbhw_errorcode_t gbhw_load_rom_file(gbhw_context_t ctx, const char* path);

HWPublicAPI gbhw_errorcode_t gbhw_load_rom_memory(gbhw_context_t ctx, const uint8_t* memory, uint32_t length);

HWPublicAPI gbhw_errorcode_t gbhw_get_screen(gbhw_context_t ctx, const uint8_t** screen);

HWPublicAPI gbhw_errorcode_t gbhw_get_screen_resolution(gbhw_context_t ctx, uint32_t* width, uint32_t* height);

HWPublicAPI gbhw_errorcode_t gbhw_step(gbhw_context_t ctx, gbhw_step_mode_t mode);

HWPublicAPI gbhw_errorcode_t gbhw_set_button_state(gbhw_context_t ctx, gbhw_button_t button, gbhw_button_state_t state);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
#ifdef EMSCRIPTEN

HWPublicAPI gbhw_context_t gbhw_create_web(const uint8_t* rom, uint32_t rom_size);
HWPublicAPI const uint8_t* gbhw_get_screen_web(gbhw_context_t ctx);
HWPublicAPI uint32_t gbhw_get_screen_resolution_width(gbhw_context_t ctx);
HWPublicAPI uint32_t gbhw_get_screen_resolution_height(gbhw_context_t ctx);

#endif

}
#endif