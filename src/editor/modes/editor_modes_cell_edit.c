
b32 _editor_do_cell_edit(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    if (editor->raw_input_seq_size == 0) {
        switch (input) {
            case '\x1b': {
                _editor_store_cell_from_input(editor, wb);
                editor->mode = EDITOR_MODE_NORMAL;
            } break;

            case WIN_INPUT_ARROW_LEFT:
            case 'h': {
                u32 move_size = MIN(editor->cell_input_cursor, count);
                editor->cell_input_cursor -= move_size;
            } break;

            case WIN_INPUT_ARROW_DOWN:
            case '$':
            case 'j': {
                editor->cell_input_cursor = editor->cell_input_size - 1;
            } break;

            case WIN_INPUT_ARROW_UP:
            case '0':
            case 'k': {
                editor->cell_input_cursor = 0;
            } break;

            case WIN_INPUT_ARROW_RIGHT:
            case 'l': {
                editor->cell_input_cursor += count;

                if (editor->cell_input_cursor > editor->cell_input_size - 1) {
                    editor->cell_input_cursor = editor->cell_input_size - 1;
                }
            } break;

            case 'i': {
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;
        }
    }

    return true;
}
