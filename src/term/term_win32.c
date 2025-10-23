
typedef struct _term_backend {
    HANDLE stdin_handle;
    HANDLE stdout_handle;
    DWORD orig_mode;
} _term_backend;

b32 _term_init_backend(mem_arena* arena, term_context* context) {
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (
        stdin_handle == INVALID_HANDLE_VALUE ||
        stdout_handle == INVALID_HANDLE_VALUE
    ) {
        return false;
    }

    DWORD orig_mode = 0;
    if (GetConsoleMode(stdin_handle, &orig_mode) == 0) {
        return false;
    }

    DWORD new_mode =
        ENABLE_WINDOW_INPUT |
        ENABLE_VIRTUAL_TERMINAL_INPUT;

    if (SetConsoleMode(stdin_handle, new_mode) == 0) {
        return false;
    }

    context->backend = PUSH_STRUCT(arena, _term_backend);

    context->backend->stdin_handle = stdin_handle;
    context->backend->stdout_handle = stdout_handle;
    context->backend->orig_mode = orig_mode;

    return true;
}

void term_quit(term_context* context) {
    SetConsoleMode(
        context->backend->stdin_handle,
        context->backend->orig_mode
    );
}

u32 term_read(term_context* context, u8* chars, u32 capacity) {
    u32 num_events = 0;

    if (GetNumberOfConsoleInputEvents(
        context->backend->stdin_handle,
        (DWORD*)&num_events
    ) == 0 || num_events == 0) {
        return 0;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    INPUT_RECORD* records = PUSH_ARRAY(scratch.arena, INPUT_RECORD, num_events);

    u32 num_read = 0;
    if (ReadConsoleInputW(
        context->backend->stdin_handle,
        records, num_events, (DWORD*)&num_read 
    ) == 0 || num_read != num_events) {
        return 0;
        arena_scratch_release(scratch);
    }

    u32 num_written = 0;
    for (u32 i = 0; i < num_read && num_written < capacity; i++) {
        if (
            records[i].EventType == KEY_EVENT &&
            records[i].Event.KeyEvent.bKeyDown
        ) {
            chars[num_written++] = 
                (u8)records[i].Event.KeyEvent.uChar.AsciiChar;
        }
    }

    arena_scratch_release(scratch);

    return num_written;
}

void term_flush(term_context* context) {
    DWORD written = 0;

    b32 ret = WriteConsoleA(
        context->backend->stdout_handle,
        context->draw_buf,
        context->draw_buf_pos,
        &written, NULL
    );

    if (ret == FALSE || written != context->draw_buf_pos) {
        plat_fatal_error("Failed to write to console", 1);
    }

    context->draw_buf_pos = 0;
}

