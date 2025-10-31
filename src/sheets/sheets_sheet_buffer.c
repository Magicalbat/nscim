
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

    // Used to create empty `sheet_cell_ref` structs with valid pointers
    struct {
        sheet_cell_type type;
        f64 num;
        sheet_string* str;
    } empty_cell;

    sheet_cell_ref empty_ref;

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

    _sb_info.empty_cell.type.t = SHEET_CELL_TYPE_NONE;
    _sb_info.empty_cell.num = 0.0;
    _sb_info.empty_cell.str = NULL;

    _sb_info.empty_ref = (sheet_cell_ref){
        .type = &_sb_info.empty_cell.type,
        .num = &_sb_info.empty_cell.num,
        .str = &_sb_info.empty_cell.str,
    };

    _sb_info.initialized = true;
}

// Grow size is in `sheet_chunk*`s, not bytes
void _sb_chunk_map_grow(sheet_buffer* sheet, u64 grow_size) {
    grow_size = ALIGN_UP_POW2(grow_size, _sb_info.page_size / sizeof(sheet_chunk*));

    if (sheet->map_capacity + grow_size > SHEET_MAX_CHUNKS) {
        grow_size = SHEET_MAX_CHUNKS - sheet->map_capacity;
    }

    sheet_chunk** cur_commit = sheet->chunk_map + sheet->map_capacity;

    if (!plat_mem_commit(cur_commit, grow_size * sizeof(sheet_chunk*))) {
        plat_fatal_error("Failed to commit memory for sheet chunks", 1);
    }

    MEM_ZERO(cur_commit, grow_size * sizeof(sheet_chunk*));

    sheet->map_capacity += grow_size;
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
    sheet->_column_widths = (u16*)(mem + _sb_info.column_widths_off);
    sheet->_row_heights = (u8*)(mem + _sb_info.row_heights_off);

    // Allocate initial page for chunks
    _sb_chunk_map_grow(sheet, _sb_info.page_size / sizeof(sheet_chunk*));

    return sheet;
}

void _sb_free_chunks(workbook* wb, sheet_buffer* sheet) {
    for (u32 i = 0; i < sheet->map_capacity; i++) {
        sheet_chunk* cur_chunk = sheet->chunk_map[i];
        sheet_chunk* next = NULL;

        while (cur_chunk != NULL) {
            next = cur_chunk->next;

            wb_free_chunk(wb, cur_chunk);

            cur_chunk = next;
        }
    }
}

void _sheet_buffer_reset(workbook* wb, sheet_buffer* sheet) {
    _sb_free_chunks(wb, sheet);

    if (sheet->map_capacity > 0) {
        plat_mem_decommit(sheet->chunk_map, sheet->map_capacity);
    }

    // Reset initial map space
    _sb_chunk_map_grow(sheet, _sb_info.page_size / sizeof(sheet_chunk*));

    if (sheet->num_column_widths > 0) {
        plat_mem_decommit(sheet->_column_widths, sheet->num_column_widths);
    }

    if (sheet->num_row_heights > 0) {
        plat_mem_decommit(sheet->_row_heights, sheet->num_row_heights);
    }

    sheet->map_capacity = 0;
    sheet->num_chunks = 0;
    sheet->num_column_widths = 0;
    sheet->num_row_heights = 0;
}

void _sheet_buffer_destroy(workbook* wb, sheet_buffer* sheet) {
    _sb_free_chunks(wb, sheet);

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

// TODO: profile this function
// Would it be worth it to multithread?
void _sb_chunk_rehash(workbook* wb, sheet_buffer* sheet) {
    sheet_chunk** scratch_chunks = _wb_get_scratch_chunks(
        wb, sheet->num_chunks * sizeof(sheet_chunk*)
    );

    u32 chunk_index = 0;

    for (u32 i = 0; i < sheet->map_capacity; i++) {
        sheet_chunk* cur_chunk = sheet->chunk_map[i];
        sheet_chunk* next = NULL;

        while (cur_chunk != NULL) {
            next = cur_chunk->next;
            cur_chunk->next = NULL;

            scratch_chunks[chunk_index++] = cur_chunk;

            cur_chunk = next;
        }
    }

    ASSERT(chunk_index == sheet->num_chunks);

    MEM_ZERO(sheet->chunk_map, sizeof(sheet_chunk*) * sheet->map_capacity);

    // Double in size
    // TODO: test other growth factors?
    _sb_chunk_map_grow(sheet, sheet->map_capacity);

    for (u32 i = 0; i < sheet->num_chunks; i++) {
        sheet_chunk* chunk = scratch_chunks[i];

        u32 chunk_idx = chunk->hash % sheet->map_capacity;

        // If there is no chunk there, the pointer should be NULL,
        // so this line will still work correctly
        chunk->next = sheet->chunk_map[chunk_idx];
        sheet->chunk_map[chunk_idx] = chunk;
    }
}

sheet_chunk* sheet_get_chunk(
    workbook* wb, sheet_buffer* sheet,
    sheet_chunk_pos chunk_pos, b32 create_if_empty
) {
    // This is the empty sheet
    if (sheet->map_capacity == 0) {
        return NULL;
    }

    u64 chunk_hash = _sb_chunk_hash(chunk_pos);
    u64 chunk_idx = chunk_hash % sheet->map_capacity;

    sheet_chunk* chunk = sheet->chunk_map[chunk_idx];
    while (
        chunk != NULL && 
        (chunk->pos.row != chunk_pos.row ||
        chunk->pos.col != chunk_pos.col)
    ) {
        chunk = chunk->next;
    }

    if (chunk == NULL && create_if_empty) {
        chunk = wb_create_chunk(wb);
        chunk->pos = chunk_pos;
        chunk->hash = chunk_hash;

        sheet->num_chunks++;

        // If there is no chunk there, the pointer should be NULL,
        // so this line will still work correctly
        chunk->next = sheet->chunk_map[chunk_idx];
        sheet->chunk_map[chunk_idx] = chunk;

        // TODO: test other load factor thresholds?
        if (sheet->num_chunks > sheet->map_capacity) {
            _sb_chunk_rehash(wb, sheet);
        }
    }

    return chunk;
}

sheet_chunk_arr sheet_get_range(
    mem_arena* arena, workbook* wb, sheet_buffer* sheet,
    sheet_cell_range cell_range, b32 create_if_empty
) {
    u32 min_cell_row = MIN(cell_range.start.row, cell_range.end.row);
    u32 min_cell_col = MIN(cell_range.start.col, cell_range.end.col);
    u32 max_cell_row = MAX(cell_range.start.row, cell_range.end.row);
    u32 max_cell_col = MAX(cell_range.start.col, cell_range.end.col);

    sheet_chunk_pos min_chunk_pos = {
        min_cell_row / SHEET_CHUNK_ROWS,
        min_cell_col / SHEET_CHUNK_COLS
    };

    sheet_chunk_pos max_chunk_pos = {
        max_cell_row / SHEET_CHUNK_ROWS,
        max_cell_col / SHEET_CHUNK_COLS
    };

    u32 max_chunks = (max_chunk_pos.row - min_chunk_pos.row + 1) *
        (max_chunk_pos.col - max_chunk_pos.col + 1);

    mem_arena_temp scratch = arena_scratch_get(&arena, 1);

    u32 num_chunks = 0;
    sheet_chunk** temp_chunks = PUSH_ARRAY(scratch.arena, sheet_chunk*, max_chunks);

    for (u32 c_col = min_chunk_pos.col; c_col <= max_chunk_pos.col; c_col++) {
        for (u32 c_row = min_chunk_pos.row; c_row <= max_chunk_pos.row; c_row++) {
            sheet_chunk_pos pos = { c_row, c_col };

            sheet_chunk* cur_chunk = sheet_get_chunk(wb, sheet, pos, create_if_empty);

            if (cur_chunk != NULL) {
                temp_chunks[num_chunks++] = cur_chunk;
            }
        }
    }

    sheet_chunk_arr chunk_arr = {
        .size = num_chunks,
        .chunks = PUSH_ARRAY_NZ(arena, sheet_chunk*, num_chunks)
    };

    memcpy(chunk_arr.chunks, temp_chunks, sizeof(sheet_chunk*) * num_chunks);

    arena_scratch_release(scratch);

    return chunk_arr;
}

sheet_cell_ref sheet_get_cell(
    workbook* wb, sheet_buffer* sheet,
    sheet_cell_pos cell_pos, b32 create_if_empty
) {
    sheet_chunk_pos chunk_pos = {
        cell_pos.row / SHEET_CHUNK_ROWS,
        cell_pos.col / SHEET_CHUNK_COLS
    };

    sheet_chunk* chunk = NULL;

    if (
        sheet->last_chunk != NULL &&
        sheet->last_chunk->pos.row == chunk_pos.row &&
        sheet->last_chunk->pos.col == chunk_pos.col
    ) {
        chunk = sheet->last_chunk;
    } else {
        chunk = sheet_get_chunk(wb, sheet, chunk_pos, create_if_empty);
    }

    if (chunk == NULL) {
        _sb_info.empty_cell.type.t = SHEET_CELL_TYPE_NONE;
        _sb_info.empty_cell.num = 0.0;
        _sb_info.empty_cell.str = NULL;

        return _sb_info.empty_ref;
    }

    // Update last chunk for the next time
    sheet->last_chunk = chunk;

    u32 local_row = cell_pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = cell_pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    sheet_cell_ref cell_ref = {
        .type = &chunk->types[index],
        .num = &chunk->nums[index],
        .str = &chunk->strings[index]
    };

    return cell_ref;
}

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

    return sheet->_column_widths[col];
}

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u16 width) {
    if (col >= SHEET_MAX_COLS) {
        return;
    }

    if (col >= sheet->num_column_widths) {
        u32 old_size = sheet->num_column_widths;
        sheet->num_column_widths = col + 1;

        _sb_grow_array(
            sheet->_column_widths, sizeof(u16),
            old_size, &sheet->num_column_widths
        );

        for (u32 i = old_size; i < sheet->num_column_widths; i++) {
            sheet->_column_widths[i] = SHEET_DEF_COL_WIDTH;
        }
    }

    sheet->_column_widths[col] = width;
}

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row) {
    if (row >= sheet->num_row_heights) {
        return SHEET_DEF_ROW_HEIGHT;
    }

    return sheet->_row_heights[row];
}

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height) {
    if (row >= SHEET_MAX_ROWS) {
        return;
    }

    if (row >= sheet->num_row_heights) {
        u32 old_size = sheet->num_row_heights;
        sheet->num_row_heights = row + 1;

        _sb_grow_array(
            sheet->_row_heights, sizeof(u8),
            old_size, &sheet->num_row_heights
        );

        for (u32 i = old_size; i < sheet->num_row_heights; i++) {
            sheet->_row_heights[i] = SHEET_DEF_ROW_HEIGHT;
        }
    }

    sheet->_row_heights[row] = height;

}


