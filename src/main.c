
#include "base/base.h"
#include "platform/platform.h"
#include "term/term.h"
#include "sheets/sheets.h"

#include "base/base.c"
#include "platform/platform.c"
#include "term/term.c"
#include "sheets/sheets.c"

typedef enum {
    TILE_FLAG_NONE      = 0b00000,
    TILE_FLAG_BOLD      = 0b00001,
    TILE_FLAG_DIM       = 0b00010,
    TILE_FLAG_ITALIC    = 0b00100,
    TILE_FLAG_UNDERLINE = 0b01000,
    TILE_FLAG_STRIKE    = 0b10000,
} draw_tile_flags;

typedef union {
    struct {
        u8 fg_col;
        u8 bg_col;
        u8 flags;
        u8 c;
    };

    u32 bits;
} draw_tile;

static const draw_tile empty_tile = {
    .fg_col = 7,
    .bg_col = 0,
    .flags = 0,
    .c = ' '
};

typedef struct {
    u32 width, height;
    draw_tile* tiles;
} draw_buf;

// One at the top for commands
// And one at the bottom for output and info
#define APP_INFO_ROWS_TOP 1
#define APP_INFO_ROWS_BOTTOM 1
#define APP_INFO_ROWS (APP_INFO_ROWS_TOP + APP_INFO_ROWS_BOTTOM)

void _get_col_chars(u32 col, u32* num, u8 chars[3]) {
    *num = 0;

    for (u32 i = 0; i < 3; i++) {
        chars[(*num)++] = (col % 26) + 'A';
        col /= 26;

        if (col == 0) {
            break;
        } else {
            col -= 1;
        }
    }

    if (*num > 1) {
        u8 tmp = chars[0];
        chars[0] = chars[*num - 1];
        chars[*num - 1] = tmp;
    }
}

void uint_to_chars(u32 n, u32* size, u8 chars[10]) {
    *size = 0;

    do {
        chars[(*size)++] = n % 10 + '0';
        n /= 10;
    } while (n);

    for (u32 i = 0; i < *size / 2; i++) {
        u8 tmp = chars[*size - i - 1];
        chars[*size - i - 1] = chars[i];
        chars[i] = tmp;
    }
}

#define WIN_INPUT_LINE_HEIGHT 1

void draw_win(workbook* wb, sheet_window* win, draw_buf* front_buf) {
    // TODO: input line
    
    if (win->width <= 0 || win->height <= 1) {
        return;
    }


    sheet_cell_pos start = win->scroll_pos;
    sheet_cell_pos end = win->scroll_pos;
    u32 num_rows = win->height - WIN_INPUT_LINE_HEIGHT - 1;
    end.row = start.row + num_rows - 1;

    u8 rc_chars[10] = { 0 };
    u32 max_row_chars = 0;
    u32 num_rc_chars = 0;

    uint_to_chars(end.row, &max_row_chars, rc_chars);

    u32 num_cols = (win->width - 7 + 9) / 10;
    end.col = start.col + num_cols - 1;

    const draw_tile rc_blank = {
        .bg_col = 189,
        .fg_col = 89,
        .c = ' '
    };

    const draw_tile rc_current = {
        .bg_col = 89,
        .fg_col = 189,
        .c = ' '
    };

    const draw_tile empty_cell_tile = {
        .fg_col = 7,
        .c = ' '
    };

    const draw_tile cur_cell_tile = {
        .bg_col = 189,
        .fg_col = 89,
        .c = ' '
    };

    draw_tile scratch_tiles[10] = { 0 };

    b32 active = win == wb->active_win;

    for (u32 col = 0; col < num_cols; col++) {
        _get_col_chars(col + start.col, &num_rc_chars, rc_chars);

        draw_tile ref_tile = active && 
            col + start.col == win->cursor_pos.col ?
            rc_current : rc_blank;

        for (u32 i = 0; i < 10; i++) {
            scratch_tiles[i].bits = ref_tile.bits;
        }

        for (u32 i = 0; i < num_rc_chars; i++) {
            scratch_tiles[i + 4-num_rc_chars/2].c = rc_chars[i];
        }

        for (u32 x = 0; x < 10; x++) {
            u32 draw_x = x + win->start_x + col * 10 + 7;

            if (
                draw_x >= front_buf->width ||
                draw_x >= win->start_x + win->width
            ) {
                break;
            }

            front_buf->tiles[
                draw_x + (win->start_y + WIN_INPUT_LINE_HEIGHT) *
                front_buf->width
            ] = scratch_tiles[x];
        }
    }

    sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

    for (u32 row = 0; row < num_rows; row++ ) {
        uint_to_chars(row + start.row, &num_rc_chars, rc_chars);

        draw_tile ref_tile = active && 
            row + start.row == win->cursor_pos.row ?
            rc_current : rc_blank;

        for (u32 i = 0; i < 7; i++) {
            scratch_tiles[i].bits = ref_tile.bits;
        }
        for (u32 i = 0; i < num_rc_chars; i++) {
            scratch_tiles[i + 7 - num_rc_chars].c = rc_chars[i];
        }

        u32 draw_y_off = (win->start_y + WIN_INPUT_LINE_HEIGHT + 1 + row) *
            front_buf->width;

        for (u32 x = 0; x < 7; x++) {
            front_buf->tiles[x + win->start_x + draw_y_off] = scratch_tiles[x];
        }

        for (u32 col = 0; col < num_cols; col++) {
            ref_tile = active && 
                row + start.row == win->cursor_pos.row &&
                col + start.col == win->cursor_pos.col ?
                cur_cell_tile : empty_cell_tile;

            for (u32 i = 0; i < 10; i++) {
                scratch_tiles[i].bits = ref_tile.bits;
            }

            sheet_cell_ref cell = sheet_get_cell(
                wb, sheet, (sheet_cell_pos){
                    row + start.row, col + start.col
                }, false
            );

            switch (cell.type->t) {
                case SHEET_CELL_TYPE_NONE: {
                } break;

                case SHEET_CELL_TYPE_NUM: {
                    // TODO
                } break;

                case SHEET_CELL_TYPE_STRING: {
                    sheet_string* str = *cell.str;

                    if (str->size > 10) {
                        for (u32 i = 0; i < 7; i++) {
                            scratch_tiles[i].c = str->str[i];
                        }
                        scratch_tiles[7].c = '.';
                        scratch_tiles[8].c = '.';
                        scratch_tiles[9].c = '.';
                    } else {
                        for (u32 i = 0; i < str->size; i++) {
                            scratch_tiles[i].c = str->str[i];
                        }
                    }
                } break;
            }

            for (u32 x = 0; x < 10; x++) {
                u32 draw_x = x + win->start_x + col * 10 + 7;

                if (
                    draw_x >= front_buf->width ||
                    draw_x >= win->start_x + win->width
                ) {
                    break;
                }

                front_buf->tiles[draw_x + draw_y_off] = scratch_tiles[x];
            }
        }
    }
}

static u8 num_chars[10] = { 0 };
static u32 num_chars_size = 0;

void _set_term_col(term_context* term, u8 fg_col, u8 bg_col) {
    term_write(term, STR8_LIT("\x1b[38;5;"));
    uint_to_chars(fg_col, &num_chars_size, num_chars);
    term_write(term, (string8){ num_chars, (u64)num_chars_size });

    term_write(term, STR8_LIT("m\x1b[48;5;"));
    uint_to_chars(bg_col, &num_chars_size, num_chars);
    term_write(term, (string8){ num_chars, (u64)num_chars_size });
    term_write(term, STR8_LIT("m"));
}

void  _set_term_cursor(term_context* term, u32 x, u32 y) {
    x++; y++;

    term_write(term, STR8_LIT("\x1b["));
    uint_to_chars(y, &num_chars_size, num_chars);
    term_write(term, (string8){ num_chars, (u64)num_chars_size });
    term_write(term, STR8_LIT(";"));
    uint_to_chars(x, &num_chars_size, num_chars);
    term_write(term, (string8){ num_chars, (u64)num_chars_size });
    term_write(term, STR8_LIT("H"));
}

void draw(
    workbook* wb, term_context* term, 
    draw_buf* front_buf, draw_buf* back_buf,
    mem_arena* frame_arena, mem_arena* prev_frame_arena
) {
    u32 width = 0, height = 0;
    term_get_size(term, &width, &height);

    front_buf->width = width;
    front_buf->height = height;
    front_buf->tiles = PUSH_ARRAY_NZ(frame_arena, draw_tile, width * height);

    for (u32 i = 0; i < width * height; i++) {
        front_buf->tiles[i].bits = empty_tile.bits;
    }

    if (width == 0 || height <= APP_INFO_ROWS) {
        return;
    }

    wb_win_compute_sizes(wb, width, height - APP_INFO_ROWS);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 stack_size = 0;
    sheet_window** stack = PUSH_ARRAY(scratch.arena, sheet_window*, wb->num_windows);
    stack[stack_size++] = wb->root_win;

    while (stack_size) {
        sheet_window* win = stack[--stack_size];

        if (win->internal) {
            stack[stack_size++] = win->child1;
            stack[stack_size++] = win->child0;
        } else {
            draw_win(wb, win, front_buf);
        }
    }

    arena_scratch_release(scratch);

    b32 redraw = back_buf->width != front_buf->width ||
        back_buf->height != front_buf->height;

    if (redraw) {
        term_write(term, STR8_LIT("\x1b[2J"));
    }

    term_write(term, STR8_LIT("\x1b[H\x1b[0m"));
    _set_term_col(term, empty_tile.fg_col, empty_tile.bg_col);

    u32 prev_x = 0;
    u32 prev_y = 0;

    draw_tile prev_tile = empty_tile;
    for (u32 y = 0; y < front_buf->height; y++) {
        for (u32 x = 0; x < front_buf->width; x++) {
            u32 idx = x + y * front_buf->width;

            draw_tile tile = front_buf->tiles[idx];

            if (redraw || tile.bits != back_buf->tiles[idx].bits) {
                if (tile.fg_col != prev_tile.fg_col || tile.bg_col != prev_tile.bg_col) {
                    _set_term_col(term, tile.fg_col, tile.bg_col);
                }

                if (x != prev_x + 1 || y != prev_y) {
                    _set_term_cursor(term, x, y);
                }

                term_write(term, (string8){ &tile.c, 1 });

                prev_tile = tile;
                prev_x = x;
                prev_y = y;
            }
        }
        term_write(term, STR8_LIT("\x1b[1E"));
    }

    term_flush(term);

    draw_buf tmp = *front_buf;
    memcpy(front_buf, back_buf, sizeof(draw_buf));
    memcpy(back_buf, &tmp, sizeof(draw_buf));
}

#define CTRL_KEY(k) ((k) & 0x1f)

int main(void) {
    {
        u64 seeds[2] = { 0 };
        plat_get_entropy(seeds, sizeof(seeds));
        prng_seed(seeds[0], seeds[1]);
    }

    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);
    
    mem_arena* frame_arena = arena_create(MiB(64), MiB(1), false);
    mem_arena* prev_frame_arena = arena_create(MiB(64), MiB(1), false);

    workbook* wb = wb_create();

    sheet_buffer* sheet = wb_win_active_sheet(wb, true);

    {
        sheet_cell_ref cell0 = sheet_get_cell(wb, sheet, (sheet_cell_pos){ 2, 1 }, true);
        cell0.type->t = SHEET_CELL_TYPE_STRING;
        *cell0.str = wb_create_string(wb, SHEET_MIN_STRLEN);
        memcpy((*cell0.str)->str, "Hello", 5);
        (*cell0.str)->size = 5;
        
        sheet_cell_ref cell1 = sheet_get_cell(wb, sheet, (sheet_cell_pos){ 3, 4 }, true);
        cell1.type->t = SHEET_CELL_TYPE_STRING;
        *cell1.str = wb_create_string(wb, SHEET_MIN_STRLEN);
        memcpy((*cell1.str)->str, "Hello World", 11);
        (*cell1.str)->size = 11;
    }

    term_context* term = term_init(perm_arena, MiB(4));
    term_write(term, STR8_LIT("\x1b[?1049h"));
    term_flush(term);

    u8 input_buf[64] = { 0 };

    draw_buf front_buf = { 0 };
    draw_buf back_buf = { 0 };

    draw(wb, term, &front_buf, &back_buf, frame_arena, prev_frame_arena);

    b32 win_input = false;

    b32 running = true;
    while (running) {
        u32 num_read = term_read(term, input_buf, 64);
        for (u32 i = 0; i < num_read; i++) {
            u8 c = input_buf[i];

            if (win_input) {
                switch (c) {
                    case 'v': {
                        wb_win_split(
                            wb, (sheet_window_split){
                                .s = SHEET_WIN_SPLIT_VERT
                            }, true
                        );
                    } break;

                    case 'n': {
                        wb_win_split(
                            wb, (sheet_window_split){
                                .s = SHEET_WIN_SPLIT_HORZ
                            }, false
                        );
                    } break;

                    case 'c': {
                        wb_win_close(wb);
                    } break;
                }
            }

            switch (c) {
                case 'h': {
                    if (wb->active_win->cursor_pos.col > 0) {
                        wb->active_win->cursor_pos.col--;
                    }
                } break;

                case 'j': {
                    if (wb->active_win->cursor_pos.row < SHEET_MAX_ROWS) {
                        wb->active_win->cursor_pos.row++;
                    }
                } break;

                case 'k': {
                    if (wb->active_win->cursor_pos.row > 0) {
                        wb->active_win->cursor_pos.row--;
                    }
                } break;

                case 'l': {
                    if (wb->active_win->cursor_pos.col < SHEET_MAX_COLS) {
                        wb->active_win->cursor_pos.col++;
                    }
                } break;

                case CTRL_KEY('h'): {
                    wb_win_change_active_horz(wb, -1);
                } break;

                case CTRL_KEY('j'): {
                    wb_win_change_active_vert(wb, +1);
                } break;

                case CTRL_KEY('k'): {
                    wb_win_change_active_vert(wb, -1);
                } break;

                case CTRL_KEY('l'): {
                    wb_win_change_active_horz(wb, +1);
                } break;

                case CTRL_KEY('w'): {
                    win_input = true;
                } break;
            }

            if (c == 'q') {
                running = false;
            }
        }

        if (num_read) {
            mem_arena* tmp = prev_frame_arena;
            prev_frame_arena = frame_arena;
            frame_arena = tmp;

            arena_pop_to(frame_arena, 0);

            draw(
                wb, term, &front_buf, &back_buf,
                frame_arena, prev_frame_arena
            );

            plat_sleep_ms(16);
        }
    }

    term_write(term, STR8_LIT("\x1b[?1049l"));
    term_flush(term);

    term_quit(term);

    wb_destroy(wb);

    arena_destroy(frame_arena);
    arena_destroy(prev_frame_arena);
    arena_destroy(perm_arena);

    return 0;
}

