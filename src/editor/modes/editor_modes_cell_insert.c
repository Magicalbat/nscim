
b32 _editor_do_cell_insert(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    UNUSED(count);

    if (editor->raw_input_seq_size == 0) {
        if (
            editor->cell_input_size < SHEET_MAX_STRLEN &&
            input >= ' ' && input <= '~'
        ) {
            for (
                u32 i = editor->cell_input_size;
                i > editor->cell_input_cursor; i--
            ) {
                editor->cell_input_buf[i] = editor->cell_input_buf[i-1]; 
            }

            editor->cell_input_buf[editor->cell_input_cursor] = input;

            editor->cell_input_cursor++;
            editor->cell_input_size++;

            return true;
        }

        switch (input) {
            case '\x1b': {
                if (editor->cell_input_cursor > 0) {
                    editor->cell_input_cursor--;
                }

                editor->mode = EDITOR_MODE_CELL_EDIT;
            } break;

            case '\r': {
                _editor_store_cell_from_input(editor, wb);
                editor->mode = EDITOR_MODE_NORMAL;
            } break;

            case WIN_INPUT_CTRL('h'): {
                _editor_input_active_left(editor, wb, 1, 0);
            } break;
            case WIN_INPUT_CTRL('j'): {
                _editor_input_active_down(editor, wb, 1, 0);
            } break;
            case WIN_INPUT_CTRL('k'): {
                _editor_input_active_up(editor, wb, 1, 0);
            } break;
            case WIN_INPUT_CTRL('l'): {
                _editor_input_active_right(editor, wb, 1, 0);
            } break;

            case WIN_INPUT_BACKSPACE: {
                if (editor->cell_input_cursor > 0) {
                    editor->cell_input_cursor--;

                    _editor_input_delete(editor, editor->cell_input_cursor, 1);
                }
            } break;

            case WIN_INPUT_ARROW_UP: {
                editor->cell_input_cursor = 0;
            } break;
            case WIN_INPUT_ARROW_DOWN: {
                editor->cell_input_cursor = editor->cell_input_size;
            } break;
            case WIN_INPUT_ARROW_LEFT: {
                _editor_input_cursor_left(editor, 1);
            } break;
            case WIN_INPUT_ARROW_RIGHT: {
                _editor_input_cursor_right(editor, 1, 0);
            } break;
        }
    } 

    return true;
}

