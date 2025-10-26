
#include "base/base.h"
#include "platform/platform.h"
#include "term/term.h"
#include "sheets/sheets.h"

#include "base/base.c"
#include "platform/platform.c"
#include "term/term.c"
#include "sheets/sheets.c"

int main(void) {
    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);

    term_context* term = term_init(perm_arena, MiB(4));

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

    term_quit(term);

    arena_destroy(perm_arena);

    return 0;
}

