#pragma once

/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include <gbhw.h>

#ifdef EExportDLL
#ifdef _WIN32
#	ifdef EExportSymbols
#		define EPublicAPI __declspec(dllexport)
#	else
#		define EPublicAPI __declspec(dllimport)
#	endif
#else
#	define EPublicAPI __attribute__((visibility ("default")))
#endif
#else
#	define EPublicAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/

typedef void* gbe_context_t;

EPublicAPI gbe_context_t gbe_create();
EPublicAPI void gbe_destroy(gbe_context_t ctx);
EPublicAPI int32_t gbe_main_loop(gbe_context_t ctx);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif