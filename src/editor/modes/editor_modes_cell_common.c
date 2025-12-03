
void _editor_load_cell_to_input(
    editor_context* editor, workbook* wb, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_get_active_sheet(wb, false);

    sheet_cell_view cell = sheet_get_cell_view(wb, sheet, win->cursor_pos);

    u32 size = sheets_cell_to_chars(
        cell, editor->cell_input_buf,
        sizeof(editor->cell_input_buf)
    );

    editor->cell_input_size = size;

    if (editor->cell_input_size <= max_cursor_off) {
        editor->cell_input_cursor = 0;
    } else {
        editor->cell_input_cursor = editor->cell_input_size - max_cursor_off;
    }
}

void _editor_store_cell_from_input(editor_context* editor, workbook* wb) {
    sheet_window* win = wb->active_win;

    string8 str = { editor->cell_input_buf, (u64)editor->cell_input_size };

    if (str.size == 0) {
        // Fetching the sheet separately in case it is empty
        // It is not necessary to create the sheet just to clear a
        // cell that must already be empty
        sheet_buffer* tmp_sheet = wb_get_active_sheet(wb, false);
        sheet_clear_cell(wb, tmp_sheet, win->cursor_pos);
        return;
    }

    sheet_buffer* sheet = wb_get_active_sheet(wb, true);

    sheets_parse_store_str(wb, sheet, str, win->cursor_pos);
}

void _editor_input_cursor_left(editor_context* editor, u32 n) {
    u32 move_size = MIN(n, editor->cell_input_cursor);
    editor->cell_input_cursor -= move_size;
}

void _editor_input_cursor_right(
    editor_context* editor, u32 n, u32 max_cursor_off
) {
    u32 max_cursor = editor->cell_input_size > max_cursor_off ? 
        editor->cell_input_size - max_cursor_off : 0;

    u32 move_size = MIN(n, max_cursor - editor->cell_input_cursor);
    editor->cell_input_cursor += move_size;
}

void _editor_input_delete(editor_context* editor, u32 start, u32 n) {
    start = MIN(start, editor->cell_input_size - 1);
    n = MIN(n, editor->cell_input_size - start);

    if (n == 0) { return; }

    for (u32 i = start; i < editor->cell_input_size - n; i++) {
        editor->cell_input_buf[i] = editor->cell_input_buf[i + n];
    }

    editor->cell_input_size -= n;
}

void _editor_input_active_up(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.row == 0) { return; }

    u32 move_size = MIN(n, win->cursor_pos.row);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.row -= move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_down(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.row == SHEET_MAX_ROWS - 1) { return; }

    u32 move_size = MIN(n, SHEET_MAX_ROWS - 1 - win->cursor_pos.row);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.row += move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_left(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.col == 0) { return; }

    u32 move_size = MIN(n, win->cursor_pos.col);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.col -= move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_right(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.col == SHEET_MAX_COLS - 1) { return; }

    u32 move_size = MIN(n, SHEET_MAX_COLS - 1 - win->cursor_pos.col);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.col += move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

