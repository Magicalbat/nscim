
void editor_update(
    window* win, editor_context* editor,
    workbook* wb, f32 delta
) {
    win_input input = 0;

    CLEAR_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);

    while ((input = win_next_input(win)) != 0) {
        SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);

        if (input == 'q') {
            SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_QUIT);
            return;
        }

        _editor_push_input_raw(editor, wb, input);
    }

    _editor_process_inputs_raw(editor, wb);

    // TODO: keep track of animations for drawing
    if (true || win_needs_resize(win)) {
        SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);

        editor_win_update_anims(editor, delta);
    }
}

