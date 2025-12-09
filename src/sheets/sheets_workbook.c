
workbook* wb_create(void) {
    mem_arena* arena = arena_create(
        SHEETS_WB_RESERVE_SIZE,
        SHEETS_WB_COMMIT_SIZE,
        ARENA_FLAG_GROWABLE
    );

    workbook* wb = PUSH_STRUCT(arena, workbook);

    wb->arena = arena;
    wb->empty_buffer = PUSH_STRUCT(arena, sheet_buffer);

    sheet_window* root_win = PUSH_STRUCT(arena, sheet_window);
    root_win->_sheet = wb->empty_buffer;

    wb->root_win = root_win;
    wb->active_win = root_win;

    u32 str_capacity = SHEET_MIN_STRLEN;
    for (u32 i = 0; i < SHEET_NUM_STRLENS; i++, str_capacity *= 2) {
        wb->free_strings[i].str_capacity = str_capacity;
    }

    u64 page_size = plat_page_size();
    wb->scratch_chunks_reserve = ALIGN_UP_POW2(
        sizeof(sheet_chunk*) * SHEET_MAX_CHUNKS,
        page_size
    );
    wb->scratch_chunks_commit = 0;
    wb->scratch_chunks = plat_mem_reserve(wb->scratch_chunks_reserve);

    return wb;
}

void wb_destroy(workbook* wb) {
    sheet_buffer* next = NULL;
    for (
        sheet_buffer* sheet = wb->first_sheet;
        sheet != NULL; sheet = next
    ) {
        next = sheet->next;
        _sheet_buffer_destroy(wb, sheet);
    }

    next = NULL;
    for (
        sheet_buffer* sheet = wb->first_free_sheet;
        sheet != NULL; sheet = next
    ) {
        next = sheet->next;
        _sheet_buffer_destroy(wb, sheet);
    }

    plat_mem_release(wb->scratch_chunks, wb->scratch_chunks_reserve);

    arena_destroy(wb->arena);
}

sheet_buffer* wb_get_active_sheet(workbook* wb, b32 create_if_empty) {
    return wb_win_get_sheet(wb, wb->active_win, create_if_empty);
}

sheet_buffer* wb_create_sheet_buffer(workbook* wb) {
    sheet_buffer* sheet = NULL;

    if (wb->first_free_sheet != NULL) {
        sheet = wb->first_free_sheet;
        DLL_REMOVE(wb->first_free_sheet, wb->last_free_sheet, sheet);
    } else {
        sheet = _sheet_buffer_create();
    }

    DLL_PUSH_BACK(wb->first_sheet, wb->last_sheet, sheet);

    wb->num_sheets++;

    return sheet;
}

sheet_buffer* wb_get_sheet_buffer(workbook* wb, string8 name) {
    for (
        sheet_buffer* sheet = wb->first_sheet;
        sheet != NULL; sheet = sheet->next
    ) {
        if (str8_equals(sheet->name, name)) {
            return sheet;
        }
    }

    return NULL;
}

void wb_free_sheet_buffer(workbook* wb, sheet_buffer* buffer) {
    wb->num_sheets--;
    _sheet_buffer_reset(wb, buffer);
    DLL_PUSH_BACK(wb->first_free_sheet, wb->last_free_sheet, buffer);
}

sheet_window* _wb_create_win(workbook* wb) {
    wb->num_windows++;

    if (wb->first_free_win != NULL) {
        sheet_window* win = wb->first_free_win;
        SLL_POP_FRONT(wb->first_free_win, wb->last_free_win);

        return win;
    }

    sheet_window* win = PUSH_STRUCT(wb->arena, sheet_window);
    win->_sheet = wb->empty_buffer;

    return win;
}

void _wb_free_win(workbook* wb, sheet_window* win) {
    wb->num_windows--;

    MEM_ZERO(win, sizeof(sheet_window));
    win->_sheet = wb->empty_buffer;

    SLL_PUSH_BACK(wb->first_free_win, wb->last_free_win, win);
}

sheet_chunk* wb_create_chunk(workbook* wb) {
    if (wb->first_free_chunk != NULL) {
        sheet_chunk* chunk = wb->first_free_chunk;
        SLL_POP_FRONT(wb->first_free_chunk, wb->last_free_chunk);

        return chunk;
    }

    sheet_chunk* chunk = PUSH_STRUCT(wb->arena, sheet_chunk);

    return chunk;
}

void wb_free_chunk(workbook* wb, sheet_chunk* chunk) {
    if (chunk->set_cell_count > 0) {
        for (u32 i = 0; i < SHEET_CHUNK_SIZE; i++) {
            if (
                chunk->types[i] == SHEET_CELL_TYPE_STRING &&
                chunk->strings[i] != NULL
            ) {
                wb_free_string(wb, chunk->strings[i]);
            }
        }
    }

    MEM_ZERO(chunk, sizeof(sheet_chunk));

    SLL_PUSH_BACK(wb->first_free_chunk, wb->last_free_chunk, chunk);
}

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

// Also updates capacity to match
u32 _wb_get_str_idx(u32* capacity) {
    *capacity = CLAMP(
        round_up_pow2_u32(*capacity), SHEET_MIN_STRLEN, SHEET_MAX_STRLEN
    );

    u32 capacity_log2 = log2_u32(*capacity);
    u32 list_idx = capacity_log2 - SHEET_MIN_STRLEN_EXP;

    return list_idx;
}

sheet_string* wb_create_string(workbook* wb, u32 capacity) {
    u32 list_idx = _wb_get_str_idx(&capacity);
    sheet_string_list* list = &wb->free_strings[list_idx];

    if (list->first != NULL) {
        sheet_string* out = list->first;
        SLL_POP_FRONT(list->first, list->last);
        out->next = NULL;

        return out;
    }

    // Also allocating the string data itself with this
    sheet_string* out = (sheet_string*)arena_push(
        wb->arena, sizeof(sheet_string) + sizeof(u8) * capacity, false
    );
    out->capacity = capacity;

    return out;
}

void wb_free_string(workbook* wb, sheet_string* str) {
    // TODO: check for correct capacity and have error info?

    u32 capacity_log2 = log2_u32(str->capacity);
    u32 list_idx = capacity_log2 - SHEET_MIN_STRLEN_EXP;
    sheet_string_list* list = &wb->free_strings[list_idx];

    str->size = 0;
    MEM_ZERO(str->str, sizeof(u8) * str->capacity);

    SLL_PUSH_BACK(list->first, list->last, str);
}

