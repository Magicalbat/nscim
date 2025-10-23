
b32 _term_init_backend(mem_arena* arena, term_context* context);

#if defined(PLATFORM_WIN32)
#include "term_win32.c"
#else
#include "term_unix.c"
#endif

#include "term_common.c"

