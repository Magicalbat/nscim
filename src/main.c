
#include "base/base.h"
#include "platform/platform.h"
#include "term/term.h"
#include "sheets/sheets.h"

#include "base/base.c"
#include "platform/platform.c"
#include "term/term.c"
#include "sheets/sheets.c"

int main(void) {
    {
        u64 seeds[2] = { 0 };
        plat_get_entropy(seeds, sizeof(seeds));
        prng_seed(seeds[0], seeds[1]);
    }

    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);

    workbook* wb = wb_create();

    sheet_buffer* sheet = wb_win_active_sheet(wb, true);

    for (u32 i = 0; i < 10000; i++) {
        sheet_cell_pos cell_pos = { .row = i * SHEET_CHUNK_ROWS, .col = 0 };

        sheet_cell_ref cell = sheet_get_cell(wb, sheet, cell_pos, true);

        cell.type->t = SHEET_CELL_TYPE_NUM;
        *cell.num = sin(i);
    }

    sheet_chunk_arr chunk_arr = sheet_get_range(
        perm_arena, wb, sheet, 
        (sheet_cell_range){ 
            .start = { 0, 0 },
            .end = { 10000 * SHEET_CHUNK_ROWS, 0 }
        }, false
    );

    for (u32 i = 0; i < chunk_arr.size; i++) {
        sheet_chunk* chunk = chunk_arr.chunks[i];

        u8 type = chunk->types[0].t;
        f64 num = chunk->nums[0];

        if ((i % 1000) == 0) {
            printf(
                "chunk (%u %u), cell (0, 0) - %u %f\n",
                chunk->pos.row, chunk->pos.col,
                type, num
            );
        }
    }

    wb_destroy(wb);

    /*term_context* term = term_init(perm_arena, MiB(4));

    u8 input_buf[64] = { 0 };

    u8 line_buf[10] = { 0 };
    string8 line = { line_buf, 8 };

    string8 cursor_down = STR8_LIT("\x1b[1E");

    b32 running = true;
    while (running) {
        u32 num_read = term_read(term, input_buf, 64);
        for (u32 i = 0; i < num_read; i++) {
            u8 c = input_buf[i];

            memcpy(line.str, "   : ' '\r\n", line.size);
            line.str[6] = c;

            u32 j = 2;
            while (c) {
                line.str[j] = c % 10 + '0';
                c /= 10;
                j--;
            }

            term_write(term, line);
            term_write(term, cursor_down);
            term_flush(term);

            if (input_buf[i] == 'q') {
                running = false;
            }
        }
    }

    term_quit(term);*/

    arena_destroy(perm_arena);

    return 0;
}

