
workbook* wb_create(void) {
    mem_arena* arena = arena_create(
        WORKBOOK_RESERVE_SIZE, WORKBOOK_COMMIT_SIZE, true
    );

    workbook* wb = PUSH_STRUCT(arena, workbook);

    wb->arena = arena;
    wb->empty_buffer = PUSH_STRUCT(arena, sheet_buffer);

    sheet_window* root_win = PUSH_STRUCT(arena, sheet_window);
    root_win->buffer = wb->empty_buffer;

    wb->root_win = root_win;
    wb->active_win = root_win;

    u64 page_size = plat_page_size();
    wb->scratch_chunks_reserve = ALIGN_UP_POW2(
        sizeof(sheet_chunk*) * SHEET_MAX_CHUNKS,
        page_size
    );
    wb->scratch_chunks_commit = 0;
    wb->scratch_chunks = plat_mem_reserve(wb->scratch_chunks_reserve);

    u32 str_capacity = SHEET_MIN_STRLEN;
    for (u32 i = 0; i < SHEET_NUM_STRLENS; i++, str_capacity *= 2) {
        wb->free_strings[i].str_capacity = str_capacity;
    }

    return wb;
}

void wb_destroy(workbook* wb) {
    plat_mem_release(wb->scratch_chunks, wb->scratch_chunks_reserve);

    // TODO: destroy all buffers (both open and in free list)

    arena_destroy(wb->arena);
}

sheet_buffer* wb_create_buffer(workbook* wb);

sheet_buffer* wb_get_buffer(workbook* wb, string8 name);

void wb_free_buffer(workbook* wb, sheet_buffer* buffer);

sheet_chunk* wb_create_chunk(workbook* wb);

void wb_free_chunk(workbook* wb, sheet_chunk* chunk);

sheet_string wb_create_string(workbook* wb, u32 size);

void wb_free_string(workbook* wb, sheet_string str);


