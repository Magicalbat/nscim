
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

    sheet_buffer* next = NULL;
    for (
        sheet_buffer* sheet = wb->first_buffer;
        sheet != NULL; sheet = next
    ) {
        next = sheet->next;
        _sheet_buffer_destroy(sheet);
    }

    next = NULL;
    for (
        sheet_buffer* sheet = wb->first_free_buffer;
        sheet != NULL; sheet = next
    ) {
        next = sheet->next;
        _sheet_buffer_destroy(sheet);
    }

    arena_destroy(wb->arena);
}

sheet_buffer* wb_create_buffer(workbook* wb) {
    sheet_buffer* sheet = NULL;

    if (wb->first_free_buffer != NULL) {
        sheet = wb->first_free_buffer;
        DLL_REMOVE(wb->first_free_buffer, wb->last_free_buffer, sheet);
    } else {
        sheet = _sheet_buffer_create();
    }

    DLL_PUSH_BACK(wb->first_buffer, wb->last_buffer, sheet);

    return sheet;
}

sheet_buffer* wb_get_buffer(workbook* wb, string8 name) {
    for (
        sheet_buffer* sheet = wb->first_buffer;
        sheet != NULL; sheet = sheet->next
    ) {
        if (str8_equals(sheet->name, name)) {
            return sheet;
        }
    }

    return NULL;
}

void wb_free_buffer(workbook* wb, sheet_buffer* buffer) {
    _sheet_buffer_reset(buffer);
    DLL_PUSH_BACK(wb->first_free_buffer, wb->last_free_buffer, buffer);
}

sheet_window* _wb_create_win(workbook* wb) {
}

void _wb_free_win(workbook* wb, sheet_window* win) {
}

sheet_chunk* wb_create_chunk(workbook* wb);

void wb_free_chunk(workbook* wb, sheet_chunk* chunk);

sheet_chunk** _wb_get_scratch_chunks(workbook* wb, u64 ensure_size) {
    // This technically should not be possible
    if (ensure_size > wb->scratch_chunks_reserve) {
        plat_fatal_error("Unable to get memory for chunk rehashing", 1);
        return NULL;
    }

    if (wb->scratch_chunks_commit < ensure_size) {
        u32 page_size = plat_page_size();

        u64 to_commit = ALIGN_UP_POW2(
            ensure_size - wb->scratch_chunks_commit,
            page_size
        );

        u8* ptr = (u8*)wb->scratch_chunks + wb->scratch_chunks_commit;

        if (!plat_mem_commit(ptr, to_commit)) {
            plat_fatal_error("Failed to commit memory for chunk rehashign", 1);
            return NULL;
        }

        wb->scratch_chunks_commit += to_commit;
    }

    return wb->scratch_chunks;
}

sheet_string wb_create_string(workbook* wb, u32 size);

void wb_free_string(workbook* wb, sheet_string str);


