
void editor_update(window* win, editor_context* editor, workbook* wb) {
    win_input input = 0;

    editor->flags &= ~(u32)EDITOR_FLAG_SHOULD_DRAW;

    while ((input = win_next_input(win)) != 0) {
        editor->flags |= EDITOR_FLAG_SHOULD_DRAW;

        if (input == 'q') {
            editor->flags |= EDITOR_FLAG_SHOULD_QUIT;
            return;
        }

        _editor_push_input_raw(editor, wb, input);
    }

    _editor_process_inputs_raw(editor, wb);

    if (win_needs_resize(win)) {
        editor->flags |= EDITOR_FLAG_SHOULD_DRAW;
    }

    /*
    // TODO: remove (just temporary input handling)
    sheet_buffer* sheet = wb_get_active_sheet(wb, true);
    sheet_cell_pos* cursor_pos = &wb->active_win->cursor_pos;
    sheet_cell_pos* scroll_pos = &wb->active_win->scroll_pos;
    switch (input) {
        case 'h': {
            if (cursor_pos->col > 0)
                cursor_pos->col--;
        } break;
        case 'j': {
            cursor_pos->row++;
        } break;
        case 'k': {
            if (cursor_pos->row > 0)
                cursor_pos->row--;
        } break;
        case 'l': {
            cursor_pos->col++;
        } break;

        case WIN_INPUT_CTRL('h'): {
            wb_win_change_active_horz(wb, -1);
        } break;
        case WIN_INPUT_CTRL('j'): {
            wb_win_change_active_vert(wb, +1);
        } break;
        case WIN_INPUT_CTRL('k'): {
            wb_win_change_active_vert(wb, -1);
        } break;
        case WIN_INPUT_CTRL('l'): {
            wb_win_change_active_horz(wb, +1);
        } break;


        case 'w': {
            u8 cur_height = sheet_get_row_height(sheet, cursor_pos->row);
            sheet_set_row_height(
                sheet, cursor_pos->row, cur_height == 0 ? 0 : cur_height - 1
            );
        } break;
        case 'a': {
            u16 cur_width = sheet_get_col_width(sheet, cursor_pos->col);
            sheet_set_col_width(
                sheet, cursor_pos->col, cur_width == 0 ? 0 : cur_width - 1
            );
        } break;
        case 's': {
            u8 cur_height = sheet_get_row_height(sheet, cursor_pos->row);
            sheet_set_row_height(sheet, cursor_pos->row, cur_height + 1);
        } break;
        case 'd': {
            u16 cur_width = sheet_get_col_width(sheet, cursor_pos->col);
            sheet_set_col_width(sheet, cursor_pos->col, cur_width + 1);
        } break;

        case WIN_INPUT_ARROW_LEFT: {
            if (scroll_pos->col > 2048) {
                scroll_pos->col -= 2048;
                cursor_pos->col -= 2048;
            } else {
                scroll_pos->col = 0;
                cursor_pos->col = 0;
            }
        } break;
        case WIN_INPUT_ARROW_DOWN: {
            scroll_pos->row += 4096;
            cursor_pos->row += 4096;
        } break;
        case WIN_INPUT_ARROW_UP: {
            if (scroll_pos->row > 4096) {
                scroll_pos->row -= 4096;
                cursor_pos->row -= 4096;
            } else {
                scroll_pos->row = 0;
                cursor_pos->row = 0;
            }
        } break;
        case WIN_INPUT_ARROW_RIGHT: {
            scroll_pos->col += 2048;
            cursor_pos->col += 2048;
        } break;

    }*/
}

