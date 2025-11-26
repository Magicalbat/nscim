
b32 _editor_do_cell_insert(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    if (editor->raw_input_seq_size == 0) {
        switch (input) {
            case '\r': {
                editor->mode = EDITOR_MODE_NORMAL;
            } break;
        }
    } 

    return true;
}

