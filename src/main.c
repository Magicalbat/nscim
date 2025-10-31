
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
    u32 num_rows = win->height - WIN_INPUT_LINE_HEIGHT;
    end.row = start.row + num_rows - 1;

    u8 rc_chars[10] = { 0 };
    u32 max_row_chars = 0;
    u32 num_rc_chars = 0;

    uint_to_chars(end.row, &max_row_chars, rc_chars);

    u32 num_cols = (win->width - 7 + 9) / 10;
    end.col = start.col + num_cols - 1;

    draw_tile rc_blank = {
        .bg_col = 189,
        .fg_col = 89,
        .flags = 0,
        .c = ' '
    };

    draw_tile rc_tiles[10] = { 0 };

    u32 draw_height = win->height - WIN_INPUT_LINE_HEIGHT - 1;
    for (u32 y = 0; y < draw_height; y++ ) {
        uint_to_chars(y + start.row, &num_rc_chars, rc_chars);

        for (u32 i = 0; i < 7; i++) {
            rc_tiles[i].bits = rc_blank.bits;
        }
        for (u32 i = 0; i < num_rc_chars; i++) {
            rc_tiles[i + 7 - num_rc_chars].c = rc_chars[i];
        }

        for (u32 x = 0; x < 7; x++) {
            front_buf->tiles[
                x + (win->start_y + WIN_INPUT_LINE_HEIGHT + 1 + y) *
                front_buf->width
            ] = rc_tiles[x];
        }
    }

    for (u32 col = 0; col < num_cols; col++) {
        _get_col_chars(col + start.col, &num_rc_chars, rc_chars);

        for (u32 i = 0; i < 10; i++) {
            rc_tiles[i].bits = rc_blank.bits;
        }

        for (u32 i = 0; i < num_rc_chars; i++) {
            rc_tiles[i + 4-num_rc_chars/2].c = rc_chars[i];
        }

        for (u32 x = 0; x < 10; x++) {
            u32 draw_x = x + col * 10 + 7;

            if (draw_x >= front_buf->width) {
                break;
            }

            front_buf->tiles[
                draw_x + (win->start_y + WIN_INPUT_LINE_HEIGHT) *
                front_buf->width
            ] = rc_tiles[x];
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

    term_write(term, STR8_LIT("\x1b[2J\x1b[H\x1b[0m"));
    _set_term_col(term, empty_tile.fg_col, empty_tile.bg_col);

    draw_tile prev_tile = empty_tile;
    for (u32 y = 0; y < front_buf->height; y++) {
        for (u32 x = 0; x < front_buf->width; x++) {
            draw_tile tile = front_buf->tiles[x + y * front_buf->width];

            if (tile.fg_col != prev_tile.fg_col || tile.bg_col != prev_tile.bg_col) {
                _set_term_col(term, tile.fg_col, tile.bg_col);
            }

            term_write(term, (string8){ &tile.c, 1 });

            prev_tile = tile;
        }
        term_write(term, STR8_LIT("\x1b[1E"));
    }

    term_flush(term);

    draw_buf tmp = *front_buf;
    memcpy(front_buf, back_buf, sizeof(draw_buf));
    memcpy(back_buf, &tmp, sizeof(draw_buf));
}

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

    term_context* term = term_init(perm_arena, MiB(4));
    term_write(term, STR8_LIT("\x1b[?1049h"));
    term_flush(term);

    u8 input_buf[64] = { 0 };

    draw_buf front_buf = { 0 };
    draw_buf back_buf = { 0 };

    b32 running = true;
    while (running) {
        u32 num_read = term_read(term, input_buf, 64);
        for (u32 i = 0; i < num_read; i++) {
            u8 c = input_buf[i];

            switch (c) {
                case 'h': {
                    if (wb->active_win->scroll_pos.col > 0) {
                        wb->active_win->scroll_pos.col--;
                    }
                } break;

                case 'j': {
                    wb->active_win->scroll_pos.row += 10;
                } break;

                case 'k': {
                    if (wb->active_win->scroll_pos.row > 0) {
                        wb->active_win->scroll_pos.row--;
                    }
                } break;

                case 'l': {
                    wb->active_win->scroll_pos.col += 10;
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

