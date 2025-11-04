
editor_context* editor_init(mem_arena* arena) {
    editor_context* editor = PUSH_STRUCT(arena, editor_context);

    editor->mode = EDITOR_MODE_NORMAL;

    return editor;
}

void editor_update(window* win, editor_context* editor, workbook* wb) {
}

#define EDITOR_STATUS_ROWS 2

void _editor_draw_sheet_win(
    win_buffer* buf, editor_context* editor,
    workbook* wb, sheet_window* win
) {
}

void editor_draw(window* win, editor_context* editor, workbook* wb) {
    win_buffer* buf = &win->back_buf;

    if (buf->height < EDITOR_STATUS_ROWS) {
        return;
    }

    // TODO: draw status rows

    wb_win_compute_sizes(wb, buf->width, buf->height - EDITOR_STATUS_ROWS);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 stack_size = 0;
    sheet_window** stack = PUSH_ARRAY(scratch.arena, sheet_window*, wb->num_windows);
    stack[stack_size++] = wb->root_win;

    while (stack_size) {
        sheet_window* cur = stack[--stack_size];

        if (cur->internal) {
            if (cur->child0 != NULL)
                stack[stack_size++] = cur->child0;

            if (cur->child1 != NULL)
                stack[stack_size++] = cur->child1;
        } else {
            _editor_draw_sheet_win(buf, editor, wb, cur);
        }
    }

    arena_scratch_release(scratch);
}

