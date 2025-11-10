
#include "base/base.h"
#include "platform/platform.h"
#include "term/term.h"
#include "win/win.h"
#include "sheets/sheets.h"
#include "editor/editor.h"

#include "base/base.c"
#include "platform/platform.c"
#include "term/term.c"
#include "win/win.c"
#include "sheets/sheets.c"
#include "editor/editor.c"

void draw(
    window* win, workbook* wb,
    editor_context* editor, mem_arena* frame_arena
);

int main(void) {
    {
        u64 seeds[2] = { 0 };
        plat_get_entropy(seeds, sizeof(seeds));
        prng_seed(seeds[0], seeds[1]);
    }

    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);


    #if 0

    term_context* term = term_init(perm_arena, MiB(4));
    term_write(term, STR8_LIT("\x1b[?1049h"));
    term_flush(term);

    u8 input_buf[64] = { 0 };

    u8 draw_buf[256] = { 0 };

    b32 running = true;
    while (running) {
        u32 num_read = term_read(term, input_buf, 64);

        for (u32 i = 0; i < num_read; i++) {
            u8 c = input_buf[i];

            i32 size = snprintf((char*)draw_buf, sizeof(draw_buf)-1, "%3d: '%c'", c, c);
            term_write(term, (string8){ draw_buf, size });
            term_write(term, STR8_LIT("\x1b[1E"));

            if (c == 'q') {
                running = false;
            }
        }


        if (num_read) {
            term_flush(term);
            term_write(term, STR8_LIT("\x1b[2J\x1b[H"));
        }
    }

    term_write(term, STR8_LIT("\x1b[?1049l"));
    term_flush(term);

    term_quit(term);

    #else

    workbook* wb = wb_create();

    wb_win_split(wb, (sheet_window_split){ .s = SHEET_WIN_SPLIT_VERT }, false);
    wb_win_split(wb, (sheet_window_split){ .s = SHEET_WIN_SPLIT_HORZ }, false);

    sheet_buffer* sheet = wb_get_active_sheet(wb, true);
    sheet_set_row_height(sheet, 1, 2);
    sheet_set_row_height(sheet, 4, 3);

    for (u32 i = 0; i < 10; i++) {
        sheet_set_col_width(sheet, i, (u16)(i + 1));
    }

    editor_context* editor = editor_init(perm_arena);

    mem_arena* frame_arena = arena_create(MiB(64), MiB(1), false);
    mem_arena* prev_frame_arena = arena_create(MiB(64), MiB(1), false);

    window* win = win_create(perm_arena);
    
    draw(win, wb, editor, frame_arena);
    
    b32 running = true;

    while (running) {
        win_input input = win_next_input(win);

        sheet_cell_pos* cursor_pos = &wb->active_win->cursor_pos;

        switch (input) {
            case 'h': {
                if (cursor_pos->col > 0)
                    cursor_pos->col--;
            } break;
            case 'j': {
                cursor_pos->row++;
            } break;
            case 'k': {
                if (cursor_pos->row > 0)
                    cursor_pos->row--;
            } break;
            case 'l': {
                cursor_pos->col++;
            } break;

            case WIN_INPUT_CTRL('h'): {
                wb_win_change_active_horz(wb, -1);
            } break;
            case WIN_INPUT_CTRL('j'): {
                wb_win_change_active_vert(wb, +1);
            } break;
            case WIN_INPUT_CTRL('k'): {
                wb_win_change_active_vert(wb, -1);
            } break;
            case WIN_INPUT_CTRL('l'): {
                wb_win_change_active_horz(wb, +1);
            } break;
        }

        if (input == 'q') {
            running = false;
            break;
        }

        if (input) {
            draw(win, wb, editor, frame_arena);

            {
                mem_arena* tmp = prev_frame_arena;
                prev_frame_arena = frame_arena;
                frame_arena = tmp;
            }
        }
    }

    win_destroy(win);

    arena_destroy(frame_arena);
    arena_destroy(prev_frame_arena);

    #endif

    wb_destroy(wb);

    arena_destroy(perm_arena);

    return 0;
}

void draw(
    window* win, workbook* wb,
    editor_context* editor, mem_arena* frame_arena
) {
    win_begin_frame(win, frame_arena);

    editor_draw(win, editor, wb);

    win_update(win);
}


