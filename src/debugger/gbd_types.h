#pragma once

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#ifdef _WIN32 

// Windows Visual Studio compiler - which does not support C99 inttypes
#define PRId32 "d"
#define PRIu32 "u"
#define PRId64 "ld"
#define PRIx64 "llx"
#define PRIu64 "llu"

#else

// Posix
# define __STDC_FORMAT_MACROS
# include <inttypes.h>

#endif

