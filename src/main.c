
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

    wb_destroy(wb);

    arena_destroy(perm_arena);

    return 0;
}

