
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
- The `chunk_map` array commits memory linearly as needed for the hash map
- The `column_widths` and `row_heights` work by committing individual
    pages of virtual memory as needed when resizing columns or rows
- The first page of the reserved section stores both the `sheet_buffer`
    struct itself as well as two bitfields: `_col_width_bitfield` and
    `_row_height_bitfield`
- The two bitfields store a 1 for pages in the corresponding arrays that are 
    commited and a 0 for pages that are not committed yet
- All of the offsets are only computed once and stored in a static struct

Is this kind of insane, overkill, and dangerous?
Yes, but its also fun, and should be incredibly fast.

*/

static struct {
    u64 page_size;

    u32 width_bitfield_u64s;
    u32 height_bitfield_u64s;

    u64 chunk_map_off;
    u64 column_widths_off;
    u64 row_heights_off;

    u64 total_reserve;

    u64 siphash_k0;
    u64 siphash_k1;

    b32 initialized;
} _sb_info = { 0 };

static void _sb_init_info(void) {
    _sb_info.page_size = plat_page_size();

    // Number of bytes one u64 in the bitfield can cover
    u32 bitfield_coverage = (u32)_sb_info.page_size * sizeof(u64) * 8;

    // Number of u64s in the width bitfield
    _sb_info.width_bitfield_u64s = (
        SHEET_MAX_COLS * sizeof(u8) + bitfield_coverage - 1
    ) / bitfield_coverage;

    // Number of u64 in the height bitfield
    _sb_info.height_bitfield_u64s = (
        SHEET_MAX_ROWS * sizeof(u8) + bitfield_coverage - 1
    ) / bitfield_coverage;

    u32 total_sb_size = sizeof(sheet_buffer) + (
        _sb_info.width_bitfield_u64s + _sb_info.height_bitfield_u64s
    ) * sizeof(u64);

#ifndef NDEBUG
    if (total_sb_size > _sb_info.page_size) {
        plat_fatal_error("sheet_buffer cannot fit in memory page", 1);
    }
#endif

    _sb_info.chunk_map_off = _sb_info.page_size;
    _sb_info.column_widths_off = ALIGN_UP_POW2(
        _sb_info.chunk_map_off + sizeof(sheet_chunk*) * SHEET_MAX_CHUNKS,
        _sb_info.page_size
    );
    _sb_info.row_heights_off = ALIGN_UP_POW2(
        _sb_info.column_widths_off + sizeof(u8) * SHEET_MAX_COLS,
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
        _sb_init_info();
    }

    u8* mem = plat_mem_reserve(_sb_info.total_reserve);

    if (mem == NULL || !plat_mem_commit(mem, _sb_info.page_size)) {
        plat_fatal_error("Failed to allocate memory for sheet buffer", 1);
    }

    MEM_ZERO(mem, _sb_info.page_size);
    sheet_buffer* sheet = (sheet_buffer*)mem;

    sheet->chunk_map = (sheet_chunk**)(mem + _sb_info.chunk_map_off);
    sheet->_column_widths = (u8*)(mem + _sb_info.column_widths_off);
    sheet->_row_heights = (u8*)(mem + _sb_info.row_heights_off);

    sheet->_col_width_bitfield = (u64*)(mem + sizeof(sheet_buffer));
    sheet->_row_height_bitfield = sheet->_col_width_bitfield +
        _sb_info.width_bitfield_u64s;

    MEM_ZERO(sheet->_col_width_bitfield,
             _sb_info.width_bitfield_u64s * sizeof(u64));
    MEM_ZERO(sheet->_row_height_bitfield,
             _sb_info.height_bitfield_u64s * sizeof(u64));

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

u64 _sb_chunk_hash(sheet_chunk_pos pos) {
    u32 nums[2] = { pos.row, pos.col };

    return siphash(
        (u8*)nums, sizeof(nums),
        _sb_info.siphash_k0, _sb_info.siphash_k1
    );
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

    if (chunk_pos.row >= SHEET_CHUNKS_Y || chunk_pos.col >= SHEET_CHUNKS_X) {
        return NULL;
    }

    if (
        sheet->last_chunk != NULL &&
        sheet->last_chunk->pos.row == chunk_pos.row &&
        sheet->last_chunk->pos.col == chunk_pos.col
    ) {
        return sheet->last_chunk;
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

    sheet->last_chunk = chunk;

    return chunk;
}

void _sb_chunk_free(workbook* wb, sheet_buffer* sheet, sheet_chunk* chunk) {
    u64 chunk_idx = chunk->hash % sheet->map_capacity;

    if (sheet->chunk_map[chunk_idx] == chunk) {
        sheet->chunk_map[chunk_idx] = chunk->next;
        sheet->num_chunks--;
    } else {
        sheet_chunk* parent = sheet->chunk_map[chunk_idx];

        while (parent != NULL && parent->next != chunk) {
            parent = parent->next;
        }

        if (parent != NULL) {
            parent->next = chunk->next;
            sheet->num_chunks--;
        }
    }

    if (chunk == sheet->last_chunk) {
        sheet->last_chunk = NULL;
    }

    wb_free_chunk(wb, chunk);
}

sheet_chunk* sheet_get_cells_chunk(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, b32 create_if_empty
) {
    if (pos.row >= SHEET_MAX_ROWS || pos.col >= SHEET_MAX_COLS) {
        return NULL;
    }

    sheet_chunk_pos chunk_pos = {
        pos.row / SHEET_CHUNK_ROWS,
        pos.col / SHEET_CHUNK_COLS
    };

    return sheet_get_chunk(wb, sheet, chunk_pos, create_if_empty);
}

sheet_chunk_arr sheet_get_range(
    mem_arena* arena, workbook* wb, sheet_buffer* sheet,
    sheet_range range, b32 create_if_empty
) {
    u32 min_cell_row = MIN(range.start.row, range.end.row);
    u32 min_cell_col = MIN(range.start.col, range.end.col);
    u32 max_cell_row = MAX(range.start.row, range.end.row);
    u32 max_cell_col = MAX(range.start.col, range.end.col);

    max_cell_row = MAX(SHEET_MAX_ROWS - 1, max_cell_row);
    max_cell_col = MAX(SHEET_MAX_COLS - 1, max_cell_col);

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
    sheet_chunk** temp_chunks = PUSH_ARRAY(
        scratch.arena, sheet_chunk*, max_chunks
    );

    for (u32 c_col = min_chunk_pos.col; c_col <= max_chunk_pos.col; c_col++) {
        for (u32 c_row = min_chunk_pos.row; c_row <= max_chunk_pos.row; c_row++) {
            sheet_chunk_pos pos = { c_row, c_col };

            sheet_chunk* cur_chunk =
                sheet_get_chunk(wb, sheet, pos, create_if_empty);

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

sheet_cell_view sheet_get_cell_view(
    workbook* wb, sheet_buffer* sheet, sheet_pos cell_pos
) {
    if (cell_pos.row >= SHEET_MAX_ROWS || cell_pos.col >= SHEET_MAX_COLS) {
        goto return_empty;
    }

    sheet_chunk_pos chunk_pos = {
        cell_pos.row / SHEET_CHUNK_ROWS,
        cell_pos.col / SHEET_CHUNK_COLS
    };

    sheet_chunk* chunk = NULL;

    chunk = sheet_get_chunk(wb, sheet, chunk_pos, false);

    if (chunk == NULL) {
        goto return_empty;
    }

    u32 local_row = cell_pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = cell_pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    sheet_cell_view cell_view = {
        .type = chunk->types[index],
    };

    switch (chunk->types[index]) {
        case SHEET_CELL_TYPE_NUM: {
            cell_view.num = chunk->nums[index];
        } break;
        case SHEET_CELL_TYPE_STRING: {
            cell_view.str = chunk->strings[index];
        } break;
    }

    return cell_view;

return_empty:
    return (sheet_cell_view) { SHEET_CELL_TYPE_EMPTY_CHUNK, { 0 } };
}

b32 sheet_is_cell_empty(sheet_buffer* sheet, sheet_pos pos) {
    // Pasing NULL for the workbook seems bad, but it should be fine
    // because we never create if empty
    sheet_chunk* chunk = sheet_get_cells_chunk(NULL, sheet, pos, false);

    if (chunk == NULL) { return true; }

    u32 local_row = pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    return chunk->types[index] == SHEET_CELL_TYPE_EMPTY;
}

void sheet_set_cell_num(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, f64 num
) {
    sheet_chunk* chunk = sheet_get_cells_chunk(wb, sheet, pos, true);

    if (chunk == NULL) { return; }

    u32 local_row = pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    if (chunk->types[index] == SHEET_CELL_TYPE_EMPTY) {
        chunk->set_cell_count++;
    }

    if (chunk->types[index] == SHEET_CELL_TYPE_STRING) {
        wb_free_string(wb, chunk->strings[index]);
        chunk->strings[index] = NULL;
    }

    chunk->types[index] = SHEET_CELL_TYPE_NUM;
    chunk->nums[index] = num;
}

void sheet_set_cell_str(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, string8 str
) {
    sheet_chunk* chunk = sheet_get_cells_chunk(wb, sheet, pos, true);

    if (chunk == NULL) { return; }

    u32 local_row = pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    if (chunk->types[index] == SHEET_CELL_TYPE_EMPTY) {
        chunk->set_cell_count++;
    }

    u32 capped_size = (u32)MIN(str.size, SHEET_MAX_STRLEN);

    b32 create_str = true;
    if (chunk->types[index] == SHEET_CELL_TYPE_STRING) {
        sheet_string* cell_str = chunk->strings[index];

        if (cell_str->capacity >= capped_size) {
            create_str = false;
        } else {
            wb_free_string(wb, cell_str);
            chunk->strings[index] = NULL;
        }
    }

    if (create_str) {
        chunk->strings[index] = wb_create_string(wb, capped_size);
    }

    chunk->types[index] = SHEET_CELL_TYPE_STRING;
    sheet_string* cell_str = chunk->strings[index];

    cell_str->size = capped_size;
    memcpy(cell_str->str, str.str, capped_size);
}

void sheet_clear_cell(workbook* wb, sheet_buffer* sheet, sheet_pos pos) {
    sheet_chunk* chunk = sheet_get_cells_chunk(wb, sheet, pos, true);

    if (chunk == NULL) { return; }

    u32 local_row = pos.row % SHEET_CHUNK_ROWS;
    u32 local_col = pos.col % SHEET_CHUNK_COLS;
    u32 index = local_row + local_col * SHEET_CHUNK_ROWS;

    if (chunk->types[index] == SHEET_CELL_TYPE_STRING) {
        wb_free_string(wb, chunk->strings[index]);
        chunk->strings[index] = NULL;
    }

    if (chunk->types[index] != SHEET_CELL_TYPE_EMPTY) {
        chunk->set_cell_count--;

        if (chunk->set_cell_count == 0) {
            _sb_chunk_free(wb, sheet, chunk);
            return;
        }
    }

    chunk->types[index] = SHEET_CELL_TYPE_EMPTY;
}

void sheet_clear_range(workbook* wb, sheet_buffer* sheet, sheet_range in_range) {
    if (sheet->map_capacity == 0) { return; }

    sheet_range range = sheets_fix_range(in_range);

    sheet_chunk_pos start_chunk = {
        range.start.row / SHEET_CHUNK_ROWS,
        range.start.col / SHEET_CHUNK_COLS,
    };

    sheet_chunk_pos end_chunk = {
        range.end.row / SHEET_CHUNK_ROWS,
        range.end.col / SHEET_CHUNK_COLS,
    };

    for (u32 c_col = start_chunk.col; c_col <= end_chunk.col; c_col++) {
        for (u32 c_row = start_chunk.row; c_row <= end_chunk.row; c_row++) {
            sheet_chunk* chunk = sheet_get_chunk(
                wb, sheet, (sheet_chunk_pos){ c_row, c_col }, false
            );

            if (chunk == NULL) { continue; }

            // Local row and col
            u32 l_start_row = 0;
            u32 l_start_col = 0;
            u32 l_end_row = SHEET_CHUNK_ROWS - 1;
            u32 l_end_col = SHEET_CHUNK_COLS - 1;

            if (c_row == start_chunk.row) {
                l_start_row = range.start.row % SHEET_CHUNK_ROWS;
            }
            if (c_row == end_chunk.row) {
                l_end_row = range.end.row % SHEET_CHUNK_ROWS;
            }
            if (c_col == start_chunk.col) {
                l_start_col = range.start.col % SHEET_CHUNK_COLS;
            }
            if (c_col == end_chunk.col) {
                l_end_col = range.end.col % SHEET_CHUNK_ROWS;
            }

            for (u32 l_col = l_start_col; l_col <= l_end_col; l_col++) {
                for (u32 l_row = l_start_row; l_row <= l_end_row; l_row++) {
                    u32 index = l_row + l_col * SHEET_CHUNK_ROWS;

                    if (chunk->types[index] == SHEET_CELL_TYPE_STRING) {
                        wb_free_string(wb, chunk->strings[index]);
                        chunk->strings[index] = NULL;
                    }

                    if (chunk->types[index] != SHEET_CELL_TYPE_EMPTY) {
                        chunk->set_cell_count--;
                    }

                    chunk->types[index] = SHEET_CELL_TYPE_EMPTY;
                }
            }

            if (chunk->set_cell_count == 0) {
                _sb_chunk_free(wb, sheet, chunk);
            }
        }
    }
}

#define _SB_GET_PAGE_BIT(field, byte_idx) \
    ((field[(byte_idx) / (_sb_info.page_size * sizeof(u64) * 8)] >> \
     (((byte_idx) / _sb_info.page_size) % (sizeof(u64) * 8))) & 0b1)

#define _SB_SET_PAGE_BIT(field, byte_idx) \
    field[(byte_idx) / (_sb_info.page_size * sizeof(u64) * 8)] |= \
    ((u64)1 << (((byte_idx) / _sb_info.page_size) % (sizeof(u64) * 8)))

u8 sheet_get_col_width(sheet_buffer* sheet, u32 col) {
    if (col >= SHEET_MAX_COLS){ return 0; }

    if (
        sheet->_col_width_bitfield == NULL ||
        _SB_GET_PAGE_BIT(sheet->_col_width_bitfield, col) == 0
    ) {
        return SHEET_DEF_COL_WIDTH;
    }

    return sheet->_column_widths[col];
}

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u8 width) {
    if (col >= SHEET_MAX_COLS) {
        return;
    }

    if (_SB_GET_PAGE_BIT(sheet->_col_width_bitfield, col) == 0) {
        u8* mem = (u8*)sheet->_column_widths;
        mem += col - (col % _sb_info.page_size);

        if (!plat_mem_commit(mem, _sb_info.page_size)) {
            plat_fatal_error("Failed to commit memory for sheet buffer", 1);
        }

        _SB_SET_PAGE_BIT(sheet->_col_width_bitfield, col);

        u8* widths = mem;
        for (u32 i = 0; i < (_sb_info.page_size); i++) {
            widths[i] = SHEET_DEF_COL_WIDTH;
        }
    }

    sheet->_column_widths[col] = width;
}

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row) {
    if (row >= SHEET_MAX_ROWS) { return 0; }

    if (
        sheet->_row_height_bitfield == NULL ||
        _SB_GET_PAGE_BIT(sheet->_row_height_bitfield, row) == 0
    ) {
        return SHEET_DEF_ROW_HEIGHT;
    }

    return sheet->_row_heights[row];
}

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height) {
    if (row >= SHEET_MAX_ROWS) {
        return;
    }

    if (_SB_GET_PAGE_BIT(sheet->_row_height_bitfield, row) == 0) {
        u8* mem = (u8*)sheet->_row_heights;
        mem += row - (row % _sb_info.page_size);

        if (!plat_mem_commit(mem, _sb_info.page_size)) {
            plat_fatal_error("Failed to commit memory for sheet buffer", 1);
        }

        _SB_SET_PAGE_BIT(sheet->_row_height_bitfield, row);

        u8* heights = mem;
        for (u32 i = 0; i < _sb_info.page_size; i++) {
            heights[i] = SHEET_DEF_ROW_HEIGHT;
        }
    }

    sheet->_row_heights[row] = height;

}


