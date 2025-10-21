#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"
#include "term/term.h"

#include "base/base.c"
#include "platform/platform.c"
#include "term/term.c"

int main(void) {
    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);

    term_context* term = term_init(perm_arena);

    u8 input_buf[64] = { 0 };

    b32 running = true;
    while (running) {
        u32 num_read = term_read(term, input_buf, 64);
        for (u32 i = 0; i < num_read; i++) {
            printf("%d: '%c'\r\n", input_buf[i], input_buf[i]);

            if (input_buf[i] == 'q') {
                running = false;
            }
        }
    }

    term_quit(term);

    arena_destroy(perm_arena);

    return 0;
}

