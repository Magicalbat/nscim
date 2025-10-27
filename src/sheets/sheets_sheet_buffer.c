
/*

Sheet Buffer Memory Allocation Scheme

- Each sheet buffer reserves its own section of virtual address space
- In this region, the sheet buffer structure itself is stored,
    as well as three dynamic arrays:
    1) `chunk_map`, which is the storage for the chunk hash map
    2) `column_widths`, which stores the column widths
    3) `row_heights`, which stores the row heights
- Each of these three arrays will dynamically commit virtual
    address space as needed so as not to use too much memory
    on small sheets
- The maximum size of each array is a power of two, such that they
    should fit nicely on page boundaries
- The first page of the buffer only contains the sheet_buffer struct
    itself followed by page_size - sizeof(sheet_buffer) padding bytes
- All of the offsets are only computed once and stored in a static struct


Is this kind of insane, overkill, and dangerous?
Yes, but its also fun, and should be incredibly fast.

*/

static struct {
    u64 page_size;

    u64 chunk_map_off;
    u64 column_widths_off;
    u64 row_heights_off;

    u64 total_reserve;

    u64 siphash_k0;
    u64 siphash_k1;

    b32 initialized;
} _sb_info = { 0 };

static void _sb_init_mem_info(void) {
    _sb_info.page_size = plat_page_size();

#ifndef NDEBUG
    if (sizeof(sheet_buffer) > _sb_info.page_size) {
        plat_fatal_error("sheet_buffer cannot fit in memory page", 1);
    }
#endif

    _sb_info.chunk_map_off = _sb_info.page_size;
    _sb_info.column_widths_off = ALIGN_UP_POW2(
        _sb_info.chunk_map_off + sizeof(sheet_chunk*) * SHEET_MAX_CHUNKS,
        _sb_info.page_size
    );
    _sb_info.row_heights_off = ALIGN_UP_POW2(
        _sb_info.column_widths_off + sizeof(u16) * SHEET_MAX_COLS,
        _sb_info.page_size
    );

    _sb_info.total_reserve = ALIGN_UP_POW2(
        _sb_info.row_heights_off + sizeof(u8) * SHEET_MAX_ROWS,
        _sb_info.page_size
    );

    _sb_info.siphash_k0 = ((u64)prng_rand() << 32) | (u64)prng_rand();
    _sb_info.siphash_k1 = ((u64)prng_rand() << 32) | (u64)prng_rand();

    _sb_info.initialized = true;
}

sheet_buffer* _sheet_buffer_create(void) {
    if (!_sb_info.initialized) {
        _sb_init_mem_info();
    }

    u8* mem = plat_mem_reserve(_sb_info.total_reserve);

    if (mem == NULL || !plat_mem_commit(mem, _sb_info.page_size)) {
        plat_fatal_error("Failed to allocate memory for sheet buffer", 1);
    }

    MEM_ZERO(mem, _sb_info.page_size);
    sheet_buffer* sheet = (sheet_buffer*)mem;

    sheet->chunk_map = (sheet_chunk**)(mem + _sb_info.chunk_map_off);
    sheet->column_widths = (u16*)(mem + _sb_info.column_widths_off);
    sheet->row_heights = (u8*)(mem + _sb_info.row_heights_off);

    return sheet;
}

void _sheet_buffer_reset(sheet_buffer* sheet) {
    if (sheet->map_capacity > 0) {
        plat_mem_decommit(sheet->chunk_map, sheet->map_capacity);
    }

    if (sheet->num_column_widths > 0) {
        plat_mem_decommit(sheet->column_widths, sheet->num_column_widths);
    }

    if (sheet->num_row_heights > 0) {
        plat_mem_decommit(sheet->row_heights, sheet->num_row_heights);
    }

    sheet->map_capacity = 0;
    sheet->num_chunks = 0;
    sheet->num_column_widths = 0;
    sheet->num_row_heights = 0;
}

void _sheet_buffer_destroy(sheet_buffer* sheet) {
    plat_mem_release(sheet, _sb_info.total_reserve);
}

#define _SB_SIPHASH_CROUNDS 2
#define _SB_SIPHASH_DROUNDS 2

#define _SB_SIPHASH_ROTL(x, b) (u64)(((x) << (b)) | ((x) >> (64 - (b))))

#define _SB_SIPHASH_SIPROUND             \
    do {                                 \
        v0 += v1;                        \
        v1 = _SB_SIPHASH_ROTL(v1, 13);   \
        v1 ^= v0;                        \
        v0 = _SB_SIPHASH_ROTL(v0, 32);   \
        v2 += v3;                        \
        v3 = _SB_SIPHASH_ROTL(v3, 16);   \
        v3 ^= v2;                        \
        v0 += v3;                        \
        v3 = _SB_SIPHASH_ROTL(v3, 21);   \
        v3 ^= v0;                        \
        v2 += v1;                        \
        v1 = _SB_SIPHASH_ROTL(v1, 17);   \
        v1 ^= v2;                        \
        v2 = _SB_SIPHASH_ROTL(v2, 32);   \
    } while (0);

// This is based on siphash
// https://github.com/veorq/SipHash/blob/master/siphash.c
u64 _sb_chunk_hash(sheet_chunk_pos pos) {
    uint64_t v0 = UINT64_C(0x736f6d6570736575);
    uint64_t v1 = UINT64_C(0x646f72616e646f6d);
    uint64_t v2 = UINT64_C(0x6c7967656e657261);
    uint64_t v3 = UINT64_C(0x7465646279746573);
    uint64_t k0 = _sb_info.siphash_k0;
    uint64_t k1 = _sb_info.siphash_k1;

    u64 b = (u64)sizeof(u64) << 56;

    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    u64 m = ((u64)pos.row << 32) | (u64)pos.col;

    v3 ^= m;
    for (u32 i = 0; i < _SB_SIPHASH_CROUNDS; i++) {
        _SB_SIPHASH_SIPROUND;
    }
    v0 ^= m;

    v3 ^= b;
    for (u32 i = 0; i < _SB_SIPHASH_CROUNDS; i++) {
        _SB_SIPHASH_SIPROUND;
    }
    v0 ^= b;

    v2 ^= 0xff;

    for (u32 i = 0; i < _SB_SIPHASH_DROUNDS; i++) {
        _SB_SIPHASH_SIPROUND;
    }

    u64 out = v0 ^ v1 ^ v2 ^ v3;
    return out;
}

sheet_chunk* sheet_get_chunk(
    workbook* wb, sheet_buffer* sheet, b32 create_if_empty
);

sheet_chunk_arr sheet_get_chunks_range(
    workbook* wb, sheet_buffer* sheet, sheet_cell_range range, b32 create_if_empty
);

sheet_cell_ref sheet_get_cell(
    workbook* wb, sheet_buffer* sheet, sheet_cell_pos pos, b32 create_if_empty
);

// Bounds checking already happens in the functions that call this
void _sb_grow_array(void* mem, u32 elem_size, u32 old_size, u32* new_size) {
    u8* ptr = (u8*)mem + (old_size * elem_size);

    u32 to_commit = (u32)ALIGN_UP_POW2(
        (*new_size - old_size) * elem_size,
        _sb_info.page_size
    );
    *new_size = old_size + to_commit / elem_size;

    if (!plat_mem_commit(ptr, to_commit)) {
        plat_fatal_error("Failed to allocate memory for sheet buffer", 1);
    }
}

u16 sheet_get_col_width(sheet_buffer* sheet, u32 col) {
    if (col >= sheet->num_column_widths) {
        return SHEET_DEF_COL_WIDTH;
    }

    return sheet->column_widths[col];
}

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u16 width) {
    if (col >= SHEET_MAX_COLS) {
        return;
    }

    if (col >= sheet->num_column_widths) {
        u32 old_size = sheet->num_column_widths;
        sheet->num_column_widths = col + 1;

        _sb_grow_array(
            sheet->column_widths, sizeof(u16),
            old_size, &sheet->num_column_widths
        );

        for (u32 i = old_size; i < sheet->num_column_widths; i++) {
            sheet->column_widths[i] = SHEET_DEF_COL_WIDTH;
        }
    }

    sheet->column_widths[col] = width;
}

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row) {
    if (row >= sheet->num_row_heights) {
        return SHEET_DEF_ROW_HEIGHT;
    }

    return sheet->row_heights[row];
}

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height) {
    if (row >= SHEET_MAX_ROWS) {
        return;
    }

    if (row >= sheet->num_row_heights) {
        u32 old_size = sheet->num_row_heights;
        sheet->num_row_heights = row + 1;

        _sb_grow_array(
            sheet->row_heights, sizeof(u8),
            old_size, &sheet->num_row_heights
        );

        for (u32 i = old_size; i < sheet->num_row_heights; i++) {
            sheet->row_heights[i] = SHEET_DEF_ROW_HEIGHT;
        }
    }

    sheet->row_heights[row] = height;

}


