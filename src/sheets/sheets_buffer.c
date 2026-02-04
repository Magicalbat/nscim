
/*

Sheet Buffer Memory Allocation Scheme

- Each sheet buffer reserves its own section of virtual address space
- The first few pages are used to store the `sheet_buffer` struct itself
    as well as the `page_bitfield` array
- The rest of the pages (starting at the `dynamic_mem` pointer) are committed
    individually, based on the needs of the sheet. The `page_bitfield` bitfield
    stores whether or not each page is committed
- There are three arrays in this dynamic memory:
    1) `chunks` - Stores the array of `sheet_chunk` pointers. Chunks are
        indexed based on their position in a way meant to keep close chunks
        close together in the `chunks` array. See `_sb_chunk_index` for
        specifics
    2) `column_widths` - Stores the widths of each column in the sheet.
        Pages that are not committed are assumed to be the default column
        width, `SHEET_DEF_COL_WIDTH`. When a page is committed, it is filled
        with this default.
    3) `row_heights` - Stores the heights of each row in the sheet.
        Basically the same as `columns_widths` except it uses the default
        `SHEET_DEF_ROW_HEIGHT`
- All of the offsets are only computed once and stored in a static struct

Is this kind of insane, overkill, and dangerous?
Yes, but its also fun, and should be incredibly fast.

*/

static struct {
    b32 initialized;
    u32 page_size;

    u64 bitfield_u64s;
    u64 bitfield_off;
    u64 sb_and_bitfield_size;

    u64 chunks_num_pages;
    u64 column_widths_num_pages;
    u64 row_heights_num_pages;

    // Byte offsetes from start of allocation
    u64 chunks_off;
    u64 column_widths_off;
    u64 row_heights_off;

    // Page offsets from begining of dynamic page section
    // i.e. start of chunks memory
    u64 chunks_page_off;
    u64 column_widths_page_off;
    u64 row_heights_page_off;

    u64 total_reserve;

    u64 siphash_k0;
    u64 siphash_k1;
} _sb_info = { 0 };

#define _SB_CHUNK_PAGE(sheet, chunk_idx) (_sb_info.chunks_page_off + \
    chunk_idx * sizeof(*sheet->chunks) / _sb_info.page_size)

#define _SB_COL_WIDTH_PAGE(sheet, col) (_sb_info.column_widths_page_off + \
    col * sizeof(*sheet->column_widths) / _sb_info.page_size)

#define _SB_ROW_HEIGHT_PAGE(sheet, row) (_sb_info.row_heights_page_off + \
    row * sizeof(*sheet->row_heights) / _sb_info.page_size)

// Based from the start of dynamic section
#define _SB_GET_PAGE_BIT(sheet, page) \
    (((sheet)->page_bitfield[(page) / 64] >> ((page) % 64)) & 1)

#define _SB_SET_PAGE_BIT(sheet, page) \
    ((sheet)->page_bitfield[(page) / 64] |= ((u64)1 << ((page) % 64)))

static void _sb_init_info(void) {
    _sb_info.page_size = plat_page_size();

    sheet_buffer* null_buf = NULL;

    u64 chunks_size = ALIGN_UP_POW2(
        SHEET_MAX_CHUNKS * sizeof(*null_buf->chunks),
        _sb_info.page_size
    );

    u64 column_widths_size = ALIGN_UP_POW2(
        SHEET_MAX_COLS * sizeof(*null_buf->column_widths),
        _sb_info.page_size
    );

    u64 row_heights_size = ALIGN_UP_POW2(
        SHEET_MAX_ROWS * sizeof(*null_buf->row_heights),
        _sb_info.page_size
    );

    _sb_info.chunks_num_pages = chunks_size / _sb_info.page_size;
    _sb_info.column_widths_num_pages = column_widths_size / _sb_info.page_size;
    _sb_info.row_heights_num_pages = row_heights_size / _sb_info.page_size;

    u64 total_dynamic_bytes = chunks_size +
        column_widths_size + row_heights_size;
    u32 bytes_per_u64 = _sb_info.page_size * sizeof(u64) * 8;

    _sb_info.bitfield_u64s =
        (total_dynamic_bytes + bytes_per_u64 - 1) / bytes_per_u64;
    _sb_info.bitfield_off = ALIGN_UP_POW2(sizeof(sheet_buffer), ARENA_ALIGN);
    _sb_info.sb_and_bitfield_size = ALIGN_UP_POW2(
        _sb_info.bitfield_off + _sb_info.bitfield_u64s * sizeof(u64),
        _sb_info.page_size
    );

    _sb_info.chunks_off = _sb_info.sb_and_bitfield_size;
    _sb_info.column_widths_off = ALIGN_UP_POW2(
        _sb_info.chunks_off + chunks_size, _sb_info.page_size
    );
    _sb_info.row_heights_off = ALIGN_UP_POW2(
        _sb_info.column_widths_off + column_widths_size, _sb_info.page_size
    );
    _sb_info.total_reserve = ALIGN_UP_POW2(
        _sb_info.row_heights_off + row_heights_size, _sb_info.page_size
    );

    _sb_info.chunks_page_off = 0;
    _sb_info.column_widths_page_off = (
        _sb_info.column_widths_off - _sb_info.chunks_off
    ) / _sb_info.page_size;
    _sb_info.row_heights_page_off = (
        _sb_info.row_heights_off - _sb_info.chunks_off
    ) / _sb_info.page_size;
    
    _sb_info.siphash_k0 = ((u64)prng_rand() << 32) | (u64)prng_rand();
    _sb_info.siphash_k1 = ((u64)prng_rand() << 32) | (u64)prng_rand();

    _sb_info.initialized = true;
}

sheet_buffer* _sheet_buffer_create(void) {
    if (!_sb_info.initialized) {
        _sb_init_info();
    }

    u8* mem = plat_mem_reserve(_sb_info.total_reserve);

    if (mem == NULL || !plat_mem_commit(mem, _sb_info.sb_and_bitfield_size)) {
        plat_fatal_error("Failed to allocate memory for sheet buffer", 1);
    }

    MEM_ZERO(mem, _sb_info.sb_and_bitfield_size);
    sheet_buffer* sheet = (sheet_buffer*)mem;

    sheet->page_bitfield = (u64*)(mem + _sb_info.bitfield_off);
    sheet->dynamic_mem = (u8*)(mem + _sb_info.sb_and_bitfield_size);
    sheet->chunks = (sheet_chunk**)(mem + _sb_info.chunks_off);
    sheet->column_widths = (u8*)(mem + _sb_info.column_widths_off);
    sheet->row_heights = (u8*)(mem + _sb_info.row_heights_off);

    return sheet;
}

void _sb_free_chunks(workbook* wb, sheet_buffer* sheet) {
    u32 chunks_per_page = _sb_info.page_size / sizeof(*sheet->chunks);

    for (
        u32 i = 0, page = 0;
        i < _sb_info.chunks_num_pages / 64;
        i++, page += 64
    ) {
        if (sheet->page_bitfield[i] == 0) {
            continue; 
        }

        u64 cur_bitfield = sheet->page_bitfield[i];
        while (cur_bitfield) {
            u32 cur_bit = 63 - clz_u64(cur_bitfield);
            cur_bitfield &= ~((u64)1 << cur_bit);

            u32 cur_page = page + cur_bit;
            sheet_chunk** page_chunks = sheet->chunks +
                (cur_page * chunks_per_page);

            for (u32 j = 0; j < chunks_per_page; j++) {
                if (page_chunks[j] == NULL) { continue; }

                wb_free_chunk(wb, page_chunks[j]);
            }
        }
    }
}

void _sheet_buffer_reset(workbook* wb, sheet_buffer* sheet) {
    _sb_free_chunks(wb, sheet);

    memset(sheet->page_bitfield, 0, _sb_info.bitfield_u64s * sizeof(u64));
    plat_mem_decommit(
        sheet->dynamic_mem,
        _sb_info.total_reserve - _sb_info.sb_and_bitfield_size
    );
}

void _sheet_buffer_destroy(workbook* wb, sheet_buffer* sheet) {
    _sb_free_chunks(wb, sheet);

    plat_mem_release(sheet, _sb_info.total_reserve);
}

u64 _sb_chunk_index(sheet_chunk_pos chunk_pos) {
    u32 meta_chunk_row = chunk_pos.row / SHEET_META_CHUNK_ROWS;
    u32 meta_chunk_col = chunk_pos.col / SHEET_META_CHUNK_COLS;
    u32 local_chunk_row = chunk_pos.row % SHEET_META_CHUNK_ROWS;
    u32 local_chunk_col = chunk_pos.col % SHEET_META_CHUNK_ROWS;

    return SHEET_META_CHUNK_SIZE *
        (meta_chunk_row + meta_chunk_col * SHEET_META_CHUNKS_Y) +
        local_chunk_row + local_chunk_col * SHEET_META_CHUNK_COLS;
}

sheet_chunk* sheet_get_chunk(
    workbook* wb, sheet_buffer* sheet,
    sheet_chunk_pos chunk_pos, b32 create_if_empty
) {
    // This is the empty sheet
    if (sheet->chunks == NULL) { return NULL; }

    if (chunk_pos.row >= SHEET_CHUNKS_Y || chunk_pos.col >= SHEET_CHUNKS_X) {
        return NULL;
    }

    u64 chunk_idx = _sb_chunk_index(chunk_pos);
    u64 chunk_page = _SB_CHUNK_PAGE(sheet, chunk_idx);
    b32 page_committed = _SB_GET_PAGE_BIT(sheet, chunk_page);

    sheet_chunk* chunk = NULL;

    if (page_committed) {
        chunk = sheet->chunks[chunk_idx];
    }

    if (chunk == NULL && create_if_empty) {
        if (!page_committed) {
            u8* page_start = sheet->dynamic_mem +
                (chunk_page * _sb_info.page_size);

            if (!plat_mem_commit(page_start, _sb_info.page_size)) {
                plat_fatal_error("Failed to allocate page for chunk array", 1);
                return NULL;
            }

            _SB_SET_PAGE_BIT(sheet, chunk_page);

            memset(page_start, 0, _sb_info.page_size);
        }

        chunk = wb_create_chunk(wb);
        chunk->pos = chunk_pos;
        
        sheet->chunks[chunk_idx] = chunk;
    }

    return chunk;
}

void _sb_chunk_free(workbook* wb, sheet_buffer* sheet, sheet_chunk* chunk) {
    u64 chunk_idx = _sb_chunk_index(chunk->pos);
    u64 chunk_page = _SB_CHUNK_PAGE(sheet, chunk_idx);
    b32 page_committed = _SB_GET_PAGE_BIT(sheet, chunk_page);

    if (!page_committed || sheet->chunks[chunk_idx] == NULL) {
        return;
    }

    wb_free_chunk(wb, sheet->chunks[chunk_idx]);
    sheet->chunks[chunk_idx] = NULL;
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

void _sheet_clear_chunk(
    workbook* wb, sheet_buffer* sheet, sheet_chunk* chunk,
    sheet_range range, sheet_chunk_pos start_chunk, sheet_chunk_pos end_chunk
) {
    // Local row and col
    u32 l_start_row = 0;
    u32 l_start_col = 0;
    u32 l_end_row = SHEET_CHUNK_ROWS - 1;
    u32 l_end_col = SHEET_CHUNK_COLS - 1;

    if (chunk->pos.row == start_chunk.row) {
        l_start_row = range.start.row % SHEET_CHUNK_ROWS;
    }
    if (chunk->pos.row == end_chunk.row) {
        l_end_row = range.end.row % SHEET_CHUNK_ROWS;
    }
    if (chunk->pos.col == start_chunk.col) {
        l_start_col = range.start.col % SHEET_CHUNK_COLS;
    }
    if (chunk->pos.col == end_chunk.col) {
        l_end_col = range.end.col % SHEET_CHUNK_COLS;
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

void sheet_clear_range(workbook* wb, sheet_buffer* sheet, sheet_range in_range) {
    if (sheet->chunks == NULL) { return; }

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

            _sheet_clear_chunk(
                wb, sheet, chunk, range, start_chunk, end_chunk
            );
        }
    }
}

u8 sheet_get_col_width(sheet_buffer* sheet, u32 col) {
    if (col >= SHEET_MAX_COLS){ return 0; }
    if (sheet->page_bitfield == NULL) { return SHEET_DEF_COL_WIDTH; }

    u64 page = _SB_COL_WIDTH_PAGE(sheet, col);
    if (_SB_GET_PAGE_BIT(sheet, page) == 0) {
        return SHEET_DEF_COL_WIDTH;
    }

    return sheet->column_widths[col];
}

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u8 width) {
    if (col >= SHEET_MAX_COLS) { return; }

    u64 page = _SB_COL_WIDTH_PAGE(sheet, col);
    if (_SB_GET_PAGE_BIT(sheet, page) == 0) {
        u8* mem = sheet->dynamic_mem + page * _sb_info.page_size;

        if (!plat_mem_commit(mem, _sb_info.page_size)) {
            plat_fatal_error(
                "Failed to commit memory for column widths in sheet buffer", 1
            );
        }

        _SB_SET_PAGE_BIT(sheet, page);

        u8* widths = mem;
        for (u32 i = 0; i < _sb_info.page_size; i++) {
            widths[i] = SHEET_DEF_COL_WIDTH;
        }
    }

    sheet->column_widths[col] = width;
}

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row) {
    if (row >= SHEET_MAX_ROWS){ return 0; }
    if (sheet->page_bitfield == NULL) { return SHEET_DEF_ROW_HEIGHT; }

    u64 page = _SB_ROW_HEIGHT_PAGE(sheet, row);
    if (_SB_GET_PAGE_BIT(sheet, page) == 0) {
        return SHEET_DEF_ROW_HEIGHT;
    }

    return sheet->row_heights[row];
}

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height) {
    if (row >= SHEET_MAX_ROWS) { return; }

    u64 page = _SB_ROW_HEIGHT_PAGE(sheet, row);
    if (_SB_GET_PAGE_BIT(sheet, page) == 0) {
        u8* mem = sheet->dynamic_mem + page * _sb_info.page_size;

        if (!plat_mem_commit(mem, _sb_info.page_size)) {
            plat_fatal_error(
                "Failed to commit memory for row heights in sheet buffer", 1
            );
        }

        _SB_SET_PAGE_BIT(sheet, page);

        u8* widths = mem;
        for (u32 i = 0; i < _sb_info.page_size; i++) {
            widths[i] = SHEET_DEF_ROW_HEIGHT;
        }
    }

    sheet->row_heights[row] = height;
}


