
b32 win_col_eq(win_col a, win_col b) {
    return 
        a.r == b.r &&
        a.g == b.g &&
        a.b == b.b; 
}

window* win_create(mem_arena* arena) {
    window* win = PUSH_STRUCT(arena, window);

    _win_backend_create(arena, win);

    return win;
}

void win_destroy(window* win) {
    _win_backend_destroy(win);
}

void win_begin_frame(
    window* win, mem_arena* frame_arena,
    win_col clear_fg, win_col clear_bg, u8 clear_char
) {
    u32 width = 0, height = 0;
    _win_get_size(win, &width, &height);

    u32 buf_size = width * height;

    win->buffer = (win_buffer){
        .width = width,
        .height = height,
        .fg_cols = PUSH_ARRAY_NZ(frame_arena, win_col, buf_size),
        .bg_cols = PUSH_ARRAY_NZ(frame_arena, win_col, buf_size),
        .chars = PUSH_ARRAY_NZ(frame_arena, u8, buf_size),
    };

    for (u32 i = 0; i < buf_size; i++) {
        win->buffer.fg_cols[i] = clear_fg;
        win->buffer.bg_cols[i] = clear_bg;
        win->buffer.chars[i] = clear_char;
    }
}

