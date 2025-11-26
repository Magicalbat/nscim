
typedef struct _win_backend {
    term_context* term;
} _win_backend;

void _win_backend_create(mem_arena* arena, window* win) {
    win->_backend = PUSH_STRUCT(arena, _win_backend);

    win->_backend->term = term_init(arena, MiB(4));

    term_write(win->_backend->term, STR8_LIT("\x1b[?1049h"));
    term_flush(win->_backend->term);
}

void _win_backend_destroy(window* win) {
    term_write(win->_backend->term, STR8_LIT("\x1b[?1049l\x1b[?25h"));
    term_flush(win->_backend->term);
    term_quit(win->_backend->term);
}

void _win_get_size(window* win, u32* width, u32* height) {
    term_get_size(win->_backend->term, width, height);
}

win_input win_next_input(window* win) {
    term_context* term = win->_backend->term;

    u8 c = 0;

    if (term_read(term, &c, 1) == 0) {
        return 0;
    }

    if (c != 27 || term_read(term, &c, 1) == 0) {
        return c;
    }

    if (c == '[') {
        if (term_read(term, &c, 1) == 0) {
            return 0;
        }

        switch (c) {
            case 'A': { return WIN_INPUT_ARROW_UP; } break;
            case 'B': { return WIN_INPUT_ARROW_DOWN; } break;
            case 'C': { return WIN_INPUT_ARROW_RIGHT; } break;
            case 'D': { return WIN_INPUT_ARROW_LEFT; } break;

            case '1': {
                if (term_read(term, &c, 1) == 0) return 0;
                if (c != ';') return 0;
                if (term_read(term, &c, 1) == 0) return 0;
                if (c != '5') return 0;
                if (term_read(term, &c, 1) == 0) return 0;

                switch (c) {
                    case 'A': { return WIN_INPUT_CTRL_ARROW_UP; } break;
                    case 'B': { return WIN_INPUT_CTRL_ARROW_DOWN; } break;
                    case 'C': { return WIN_INPUT_CTRL_ARROW_RIGHT; } break;
                    case 'D': { return WIN_INPUT_CTRL_ARROW_LEFT; } break;
                }
            } break;
        }
    }

    return 0;
}

b32 win_needs_resize(window* win) {
    u32 width, height;
    term_get_size(win->_backend->term, &width, &height);

    return width != win->front_buf.width || height != win->front_buf.height;
}

void _win_term_move_cursor(term_context* term, i32 change, b32 vert) {
    if (change == 0) {
        return;
    }

    u32 n = 0;

    b32 is_neg = false;
    if (change < 0) {
        is_neg = true;
        n = (u32)(-change);
    } else {
        n = (u32)change;
    }

    n = MIN(999, n);

    u8 num_str[3] = { 0 };
    u64 size = (u64)chars_from_u32(n, num_str, 3);

    term_write(term, STR8_LIT("\x1b["));
    term_write(term, (string8){ num_str, size });
    term_write_c(
        term, vert ?
            (is_neg ? 'A' : 'B') :
            (is_neg ? 'D' : 'C')
    );
}

void _win_term_set_col(term_context* term, win_col col, b32 fg) {
    if (fg) {
        term_write(term, STR8_LIT("\x1b[38;2"));
    } else {
        term_write(term, STR8_LIT("\x1b[48;2"));
    }

    u8 num_str[4] = { ';', 0, 0, 0 };

    for (u32 i = 0; i < 3; i++) {
        u64 size = (u64)chars_from_u32(col.c[i], num_str + 1, 3);

        term_write(term, (string8){ num_str, size + 1 });
    }

    term_write_c(term, 'm');
}

void _win_draw_front_buf(window* win) {
    term_context* term = win->_backend->term;

    b32 redraw = win->_first_draw || (
        win->back_buf.width != win->front_buf.width ||
        win->back_buf.height != win->front_buf.height
    );
    win->_first_draw = false;

    if (redraw) {
        term_write(term, STR8_LIT("\x1b[2J"));
    }
    
    win_tile prev_tile = WIN_EMPTY_TILE;

    // Reset cursor pos
    term_write(term, STR8_LIT("\x1b[H\x1b[?25l"));
    _win_term_set_col(term, prev_tile.fg, true);
    _win_term_set_col(term, prev_tile.bg, false);

    u32 cursor_x = 0;
    u32 cursor_y = 0;

    for (u32 y = 0; y < win->front_buf.height; y++) {
        for (u32 x = 0; x < win->front_buf.width; x++) {
            u32 index = x + y * win->front_buf.width;
            win_tile tile = win->front_buf.tiles[index];

            b32 draw = redraw || !win_tile_eq(tile, win->back_buf.tiles[index]);

            if (!draw) continue;

            if (x != cursor_x) {
                _win_term_move_cursor(term, (i32)x-(i32)cursor_x, false);
                cursor_x = x;
            }

            if (y != cursor_y) {
                _win_term_move_cursor(term, (i32)y-(i32)cursor_y, true);
                cursor_y = y;
            }

            if (!win_col_eq(prev_tile.fg, tile.fg)) {
                _win_term_set_col(term, tile.fg, true);
            }
            
            if (!win_col_eq(prev_tile.bg, tile.bg)) {
                _win_term_set_col(term, tile.bg, false);
            }

            term_write_c(term, tile.c);

            if (x != win->front_buf.width - 1) {
                cursor_x++;
            }

            prev_tile = tile;
        }
    }

    

    if (win->cursor_mode != WIN_CURSOR_MODE_HIDDEN) {
        term_write(term, STR8_LIT("\x1b[?25h"));

        term_write(term, STR8_LIT("\x1b[H"));
        _win_term_move_cursor(term, (i32)win->cursor_row, true);
        _win_term_move_cursor(term, (i32)win->cursor_col, false);

        if (win->cursor_mode != win->_prev_cursor_mode) {
            term_write(term, STR8_LIT("\x1b["));
            term_write_c(term, (u8)win->cursor_mode + '0');
            term_write(term, STR8_LIT(" q"));
            win->_prev_cursor_mode = win->cursor_mode;
        }    
    }

    term_flush(term);
}


