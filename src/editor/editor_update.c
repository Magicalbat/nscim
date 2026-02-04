
void editor_update(
    window* win, editor_context* editor,
    workbook* wb, f32 delta
) {
    log_frame_begin();

    win_input input = 0;

    CLEAR_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);

    while ((input = win_next_input(win)) != 0) {
        SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);

        /*if (input == 'q') {
            SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_QUIT);
            return;
        }*/

        _editor_push_input_raw(editor, wb, input);
    }

    _editor_process_inputs_raw(editor, wb);

    b8 anims_finished, anims_just_finished;
    editor_win_update_anims(
        editor, delta,
        &anims_finished, &anims_just_finished
    );

    if (!anims_finished || anims_just_finished || win_needs_resize(win)) {
        SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_DRAW);
    }

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 error = log_frame_end(scratch.arena, LOG_ERROR, LOG_RES_FIRST, true);

        if (error.size) {
            sheet_buffer* sheet = editor_get_active_sheet(editor, wb, true);
            sheet_set_cell_str(wb, sheet, (sheet_pos){ 0 }, error);
        }

        arena_scratch_release(scratch);
    }
}

