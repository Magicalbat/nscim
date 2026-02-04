
typedef struct _win_backend {
    term_context* term;
} _win_backend;

void _win_backend_create(mem_arena* arena, window* win) {
    if (win == NULL) { return; }

    win->_backend = PUSH_STRUCT(arena, _win_backend);

    win->_backend->term = term_init(arena, MiB(4));

    term_write(win->_backend->term, STR8_LIT("\x1b[?1049h"));
    term_flush(win->_backend->term);
}

void _win_backend_destroy(window* win) {
    if (win == NULL) { return; }

    term_write(win->_backend->term, STR8_LIT("\x1b[?1049l\x1b[?25h"));
    term_flush(win->_backend->term);
    term_quit(win->_backend->term);
}

void _win_get_size(window* win, u32* width, u32* height) {
    if (win == NULL) { return; }

    term_get_size(win->_backend->term, width, height);
}

win_input win_next_input(window* win) {
    if (win == NULL) { return 0; }

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
    if (win == NULL) { return false; }

    u32 width, height;
    term_get_size(win->_backend->term, &width, &height);

    return width != win->buffer.width || height != win->buffer.height;
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

void win_draw(window* win) {
    if (win == NULL) { return; }

    term_context* term = win->_backend->term;

    win_col prev_fg = { { 0xff, 0xff, 0xff } };
    win_col prev_bg = { { 0x00, 0x00, 0x00 } };

    // Reset cursor pos, hide cursor
    term_write(term, STR8_LIT("\x1b[H\x1b[?25l"));
    _win_term_set_col(term, prev_fg, true);
    _win_term_set_col(term, prev_bg, false);

    for (u32 y = 0; y < win->buffer.height; y++) {
        for (u32 x = 0; x < win->buffer.width; x++) {
            u32 index = x + y * win->buffer.width;

            if (!win_col_eq(prev_fg, win->buffer.fg_cols[index])) {
                _win_term_set_col(term, win->buffer.fg_cols[index], true);
                prev_fg = win->buffer.fg_cols[index];
            } 
            
            if (!win_col_eq(prev_bg, win->buffer.bg_cols[index])) {
                _win_term_set_col(term, win->buffer.bg_cols[index], false);
                prev_bg = win->buffer.bg_cols[index];
            }

            term_write_c(term, win->buffer.chars[index]);
        }
        term_write(term, STR8_LIT("\x1b[1E"));
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


