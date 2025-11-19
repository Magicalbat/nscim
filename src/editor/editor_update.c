
void editor_update(window* win, editor_context* editor, workbook* wb) {
    win_input input = 0;

    editor->flags &= ~(u32)EDITOR_FLAG_SHOULD_DRAW;

    while ((input = win_next_input(win)) != 0) {
        editor->flags |= EDITOR_FLAG_SHOULD_DRAW;

        if (input == 'q') {
            editor->flags |= EDITOR_FLAG_SHOULD_QUIT;
            return;
        }

        switch (input) {
            case WIN_INPUT_CTRL('h'): {
                _editor_push_input_raw(editor, wb, WIN_INPUT_CTRL('w'));
                _editor_push_input_raw(editor, wb, 'h');
            } break;
            case WIN_INPUT_CTRL('j'): {
                _editor_push_input_raw(editor, wb, WIN_INPUT_CTRL('w'));
                _editor_push_input_raw(editor, wb, 'j');
            } break;
            case WIN_INPUT_CTRL('k'): {
                _editor_push_input_raw(editor, wb, WIN_INPUT_CTRL('w'));
                _editor_push_input_raw(editor, wb, 'k');
            } break;
            case WIN_INPUT_CTRL('l'): {
                _editor_push_input_raw(editor, wb, WIN_INPUT_CTRL('w'));
                _editor_push_input_raw(editor, wb, 'l');
            } break;


            default: {
                _editor_push_input_raw(editor, wb, input);
            } break;
        }
    }

    _editor_process_inputs_raw(editor, wb);

    if (win_needs_resize(win)) {
        editor->flags |= EDITOR_FLAG_SHOULD_DRAW;
    }
}

