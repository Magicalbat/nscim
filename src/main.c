
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

    sheet_buffer* sheet = wb_get_active_sheet(wb, true);
    sheet->name = STR8_LIT("Test Sheet");
    sheet_set_row_height(sheet, 1, 2);
    sheet_set_row_height(sheet, 4, 3);
    sheet_set_row_height(sheet, SHEET_MAX_ROWS-1, 10);

    for (u32 i = 0; i < 10; i++) {
        sheet_cell_ref cell = sheet_get_cell(
            wb, sheet, (sheet_pos){ i, i }, true
        );
        *cell.type = SHEET_CELL_TYPE_STRING;
        sheet_string* str = wb_create_string(wb, 16);
        memcpy(str->str, "Test  ", 6);
        str->size = 6;
        str->str[5] = (u8)i + '0';
        *cell.str = str;
    }

    for (u32 i = 0; i < 10; i++) {
        sheet_set_col_width(sheet, i, (u8)(i + 5));
    }

    editor_context* editor = editor_init(perm_arena);

    mem_arena* frame_arena = arena_create(MiB(64), MiB(1), false);
    mem_arena* prev_frame_arena = arena_create(MiB(64), MiB(1), false);

    window* win = win_create(perm_arena);
    
    draw(win, wb, editor, frame_arena);
    
    while (
        (editor->flags & EDITOR_FLAG_SHOULD_QUIT) !=
        EDITOR_FLAG_SHOULD_QUIT
    ) {
        editor_update(win, editor, wb);

        if ((editor->flags & EDITOR_FLAG_SHOULD_DRAW)) {
            draw(win, wb, editor, frame_arena);

            {
                mem_arena* tmp = prev_frame_arena;
                prev_frame_arena = frame_arena;
                frame_arena = tmp;
                arena_clear(frame_arena);
            }
        }
    }

    win_destroy(win);

    arena_destroy(frame_arena);
    arena_destroy(prev_frame_arena);

    wb_destroy(wb);

    #endif

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


