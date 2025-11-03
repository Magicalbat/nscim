
typedef struct _win_backend {
    term_context* term;
} _win_backend;

void _win_backend_create(mem_arena* arena, window* win) {
    win->backend = PUSH_STRUCT(arena, _win_backend);

    win->backend->term = term_init(arena, MiB(4));

    term_write(win->backend->term, STR8_LIT("\x1b[?1049h"));
    term_flush(win->backend->term);
}

void _win_backend_destroy(window* win) {
    term_write(win->backend->term, STR8_LIT("\x1b[?1049l"));
    term_flush(win->backend->term);
    term_quit(win->backend->term);
}

void _win_get_size(window* win, u32* width, u32* height) {
    term_get_size(win->backend->term, width, height);
}

win_input win_next_input(window* win);

// Moves down or right depending on bool
void _win_term_move_cursor(term_context* term, u32 n, b32 down) {
    if (n == 0) {
        return;
    }

    n = MIN(99, n);

    u64 size = 0;
    u8 num_str[2] = { 0 };

    while (n) {
        num_str[size++] = n % 10 + '0';
        n /= 10;
    }

    if (size == 2) {
        u8 tmp = num_str[1];
        num_str[1] = num_str[0];
        num_str[0] = tmp;
    }

    term_write(term, STR8_LIT("\x1b["));
    term_write(term, (string8){ num_str, size });
    term_write_c(term, down ? 'B' : 'C');
}

void _win_term_set_col(term_context* term, win_col col, b32 fg) {
    if (fg) {
        term_write(term, STR8_LIT("\x1b[38;2"));
    } else {
        term_write(term, STR8_LIT("\x1b[48;2"));
    }

    u64 size = 0;
    u8 num_str[4] = { ';', 0, 0, 0 };

    for (u32 i = 0; i < 3; i++) {
        size = 1;

        while (col.c[i]) {
            num_str[size++] = col.c[i] % 10 + '0';
            col.c[i] /= 10;
        }

        for (u32 j = 0; j < size/2; j++) {
            u8 tmp = num_str[size-j-1];
            num_str[size-j-1] = num_str[j];
            num_str[j] = tmp;
        }

        term_write(term, (string8){ num_str, size });
    }

    term_write_c(term, 'm');
}

void _win_draw_front_buf(window* win) {
    term_context* term = win->backend->term;

    b32 redraw = win->back_buf.width != win->front_buf.width ||
        win->back_buf.height != win->front_buf.height;

    if (redraw) {
        term_write(term, STR8_LIT("\x1b[2J"));
    }
    
    win_tile prev_tile = WIN_EMPTY_TILE;

    // Reset cursor pos
    term_write(term, STR8_LIT("\x1b[H"));
    _win_term_set_col(term, prev_tile.fg, true);
    _win_term_set_col(term, prev_tile.bg, false);

    u32 cursor_x = 0;

    for (u32 y = 0; y < win->front_buf.height; y++) {
        for (u32 x = 0; x < win->front_buf.width; x++) {
            u32 index = x + y * win->front_buf.width;
            win_tile tile = win->front_buf.tiles[index];

            b32 draw = redraw || !win_tile_eq(tile, win->back_buf.tiles[index]);

            if (!draw) continue;

            _win_term_move_cursor(term, x-cursor_x, false);
            cursor_x++;

            if (!win_col_eq(prev_tile.fg, tile.fg)) {
                _win_term_set_col(term, tile.fg, true);
            }
            
            if (!win_col_eq(prev_tile.bg, tile.bg)) {
                _win_term_set_col(term, tile.bg, false);
            }

            term_write_c(term, tile.c);
        }

        if (y != win->front_buf.height - 1) {
            term_write(term, STR8_LIT("\x1b[1E"));
            cursor_x = 0;
        }
    }
}


