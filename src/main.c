
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

    sheet_buffer* test_sheet = _sheet_buffer_create();

    u64* counts = PUSH_ARRAY(perm_arena, u64, 256);

    for (u32 col = 0; col < SHEET_CHUNKS_Y; col++) {
        for (u32 row = 0; row < SHEET_CHUNKS_X; row++) {
            sheet_chunk_pos pos = { row, col };
            u64 hash = _sb_chunk_hash(pos);

            counts[hash & 0xff]++;
        }
    }

    f64 mean = 0.0f;
    for (u32 i = 0; i < 256; i++) {
        mean += (f64)counts[i];
    }
    mean /= 256.0f;

    f64 std_dev = 0.0f;
    for (u32 i = 0; i < 256; i++) {
        std_dev += ((f64)counts[i] - mean) * ((f64)counts[i] - mean);
    }
    std_dev /= 256.0f;
    std_dev = sqrt(std_dev);

    printf("mean: %f, std_dev: %f\n", mean, std_dev);

    _sheet_buffer_destroy(test_sheet);

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

