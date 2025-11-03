
void _win_backend_create(mem_arena* arena, window* win);
void _win_backend_destroy(window* win);
// Size in tiles, not pixels
void _win_get_size(window* win, u32* width, u32* height);
void _win_draw_front_buf(window* win);

#include "win_common.c"
#include "win_term.c"

