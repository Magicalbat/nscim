
b32 _editor_do_cell_edit(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    if (editor->cur_inputs_size == 0) {
        switch (input) {
            case '\x1b': {
                _editor_store_cell_from_input(editor, wb);
                editor->mode = EDITOR_MODE_NORMAL;
            } break;

            case 'x': {
                _editor_input_delete(editor, editor->cell_input_cursor, count);

                if (editor->cell_input_size == 0) {
                    editor->cell_input_cursor = 0;
                } else if (
                    editor->cell_input_cursor > editor->cell_input_size - 1
                ) {
                    editor->cell_input_cursor = editor->cell_input_size - 1;
                }
            } break;

            case WIN_INPUT_ARROW_LEFT:
            case 'h': {
                _editor_input_cursor_left(editor, count);
            } break;

            case WIN_INPUT_ARROW_DOWN:
            case '$':
            case 'j': {
                if (editor->cell_input_size == 0) {
                    editor->cell_input_cursor = 0;
                } else {
                    editor->cell_input_cursor = editor->cell_input_size - 1;
                }
            } break;

            case WIN_INPUT_ARROW_UP:
            case '0':
            case 'k': {
                editor->cell_input_cursor = 0;
            } break;

            case WIN_INPUT_ARROW_RIGHT:
            case 'l': {
                _editor_input_cursor_right(editor, count, 1);
            } break;

            case WIN_INPUT_CTRL('h'): {
                _editor_input_active_left(editor, wb, 1, 1);
            } break;
            case WIN_INPUT_CTRL('j'): {
                _editor_input_active_down(editor, wb, 1, 1);
            } break;
            case WIN_INPUT_CTRL('k'): {
                _editor_input_active_up(editor, wb, 1, 1);
            } break;
            case WIN_INPUT_CTRL('l'): {
                _editor_input_active_right(editor, wb, 1, 1);
            } break;

            case 'i': {
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;
        }
    }

    return true;
}
