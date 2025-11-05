
b32 win_col_eq(win_col a, win_col b) {
    return 
        a.r == b.r &&
        a.g == b.g &&
        a.b == b.b; 
}

b32 win_tile_eq(win_tile a, win_tile b) {
    return 
        win_col_eq(a.fg, b.fg) && 
        win_col_eq(a.bg, b.bg) && 
        a.c == b.c;
}

window* win_create(mem_arena* arena) {
    window* win = PUSH_STRUCT(arena, window);

    win->first_draw = true;

    _win_backend_create(arena, win);

    return win;
}

void win_destroy(window* win) {
    _win_backend_destroy(win);
}

void win_begin_frame(window* win, mem_arena* frame_arena) {
    u32 width = 0, height = 0;
    _win_get_size(win, &width, &height);

    win->back_buf = (win_buffer){
        width, height,
        PUSH_ARRAY_NZ(frame_arena, win_tile, width * height)
    };

    win_tile empty_tile = WIN_EMPTY_TILE;

    for (u32 i = 0; i < width * height; i++) {
        win->back_buf.tiles[i] = empty_tile;
    }
}

void win_update(window* win) {
    win_buffer tmp = win->front_buf;
    win->front_buf = win->back_buf;
    win->back_buf = tmp;

    _win_draw_front_buf(win);
}

