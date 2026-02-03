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
    log_frame_begin();

    plat_init();

    {
        u64 seeds[2] = { 0 };
        plat_get_entropy(seeds, sizeof(seeds));
        prng_seed(seeds[0], seeds[1]);
    }

    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), ARENA_FLAG_GROWABLE);

    workbook* wb = wb_create();
    editor_context* editor = editor_create();

    sheet_buffer* sheet = editor_get_active_sheet(editor, wb, true);
    sheet->name = STR8_LIT("Test Sheet");

#if 1
    u32 table_row_off = 1;//SHEET_MAX_ROWS - 302;
    u32 table_col_off = 1;//SHEET_MAX_COLS - 3;

    sheet_set_cell_str(
        wb, sheet, (sheet_pos){
            table_row_off, table_col_off + 0 
        }, STR8_LIT("Time")
    );
    sheet_set_cell_str(
        wb, sheet, (sheet_pos){
            table_row_off, table_col_off + 1 
        }, STR8_LIT("Data")
    );
    sheet_set_cell_str(
        wb, sheet, (sheet_pos){
            table_row_off, table_col_off + 2 
        }, STR8_LIT("More data")
    );

    for (u32 i = 0; i < 300; i++) {
        sheet_set_cell_num(
            wb, sheet, (sheet_pos){
                table_row_off + 1 + i, table_col_off 
            }, (f64)i * 0.1
        );

        f64 x = (f64)i * 0.1;
        sheet_set_cell_num(
            wb, sheet, (sheet_pos){
                table_row_off + 1 + i, table_col_off + 1 
            },
            10.0 * sin(x) - 5.0 * x + 100.0
        );

        sheet_set_cell_num(
            wb, sheet, (sheet_pos){
                table_row_off + 1 + i, table_col_off + 2 
            }, (f64)i * 2.5
        );
    }
#endif

    mem_arena* frame_arena = arena_create(MiB(64), MiB(1), 0);

    window* win = win_create(perm_arena);

    draw(win, wb, editor, frame_arena);

    u64 prev_frame_start = plat_now_usec();

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 errors = log_frame_peek(scratch.arena, LOG_ERROR, LOG_RES_CONCAT, true);

        if (errors.size) {
            win_destroy(win);

            fprintf(stderr, "\x1b[31mFailed to initialize nscim\n");
            fprintf(stderr, "%.*s\n\x1b[39m", STR8_FMT(errors));
        }

        arena_scratch_release(scratch);
    }
    
    while (
        (editor->flags & EDITOR_FLAG_SHOULD_QUIT) !=
        EDITOR_FLAG_SHOULD_QUIT
    ) {
        log_frame_begin();

        u64 cur_frame_start = plat_now_usec();
        f32 delta = (f32)(cur_frame_start - prev_frame_start) * 1e-6f;

        prev_frame_start = cur_frame_start;

        editor_update(win, editor, wb, delta);

        if ((editor->flags & EDITOR_FLAG_SHOULD_DRAW)) {
            draw(win, wb, editor, frame_arena);

            arena_clear(frame_arena);
        }

        u64 frame_end = plat_now_usec();
        u32 frame_time_ms = (u32)((frame_end - cur_frame_start) / 1000);
        if (frame_time_ms < 16) {
            plat_sleep_ms(16 - frame_time_ms);
        }

        {
            mem_arena_temp scratch = arena_scratch_get(NULL, 0);

            string8 errors = log_frame_peek(scratch.arena, LOG_ERROR, LOG_RES_CONCAT, true);

            if (errors.size) {
                win_destroy(win);

                u8* errors_cstr = str8_to_cstr(scratch.arena, errors);
                plat_fatal_error((const char*)errors_cstr, 1);
            }

            arena_scratch_release(scratch);
        }
    }

    win_destroy(win);

    arena_destroy(frame_arena);

    editor_destroy(editor);
    wb_destroy(wb);

    arena_destroy(perm_arena);

    return 0;
}

void draw(
    window* win, workbook* wb,
    editor_context* editor, mem_arena* frame_arena
) {
    win_begin_frame(
        win, frame_arena,
        (win_col){ { 0xff, 0xff, 0xff } },
        (win_col){ { 0x00, 0x00, 0x00 } },
        ' '
    );

    editor_draw(win, editor, wb);

    win_draw(win);
}

