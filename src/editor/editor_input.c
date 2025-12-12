
void _editor_push_input_raw(
    editor_context* editor, workbook* wb, win_input input
) {
    if (editor->input_queue_size >= EDITOR_INPUT_QUEUE_MAX) {
        _editor_process_inputs_raw(editor, wb);
    }

    editor->input_queue[editor->input_queue_end] = input;
    editor->input_queue_end =
        (editor->input_queue_end + 1) % EDITOR_INPUT_QUEUE_MAX;
    editor->input_queue_size++;
}

static _editor_do_mode_func* _mode_funcs[_EDITOR_MODE_COUNT] = {
    [EDITOR_MODE_NULL] = NULL,

    [EDITOR_MODE_NORMAL] = _editor_do_normal,
    [EDITOR_MODE_VISUAL] = _editor_do_visual,

    [EDITOR_MODE_CELL_EDIT] = _editor_do_cell_edit,
    [EDITOR_MODE_CELL_VISUAL] = _editor_do_cell_visual,
    [EDITOR_MODE_CELL_INSERT] = _editor_do_cell_insert,

    [EDITOR_MODE_CMD] = _editor_do_cmd,
};

void _editor_process_inputs_raw(editor_context* editor, workbook* wb) {
    while (editor->input_queue_size > 0) {
        win_input cur_input = editor->input_queue[editor->input_queue_start];
        editor->input_queue_start =
            (editor->input_queue_start + 1) % EDITOR_INPUT_QUEUE_MAX;
        editor->input_queue_size--;

        b32 capture_num = !(
            editor->mode == EDITOR_MODE_CELL_INSERT ||
            editor->mode == EDITOR_MODE_CMD
        );

        if (
            capture_num && (
                (cur_input >= '1' && cur_input <= '9') || 
                (cur_input == '0' && editor->flags & _EDITOR_FLAG_READING_NUM)
            )
        ) {
            if (
                !IS_FLAG_SET_U32(editor->flags, _EDITOR_FLAG_READING_NUM)
            ) {
                editor->count = 0;
            }

            SET_FLAG_U32(editor->flags, _EDITOR_FLAG_READING_NUM);

            // Ignore any counts over ten million
            if (editor->count >= 1e7) { goto ignore_input; }

            editor->count *= 10;
            editor->count += cur_input - '0';
        } else {
            CLEAR_FLAG_U32(editor->flags, _EDITOR_FLAG_READING_NUM);
            CLEAR_FLAG_U32(editor->flags, _EDITOR_FLAG_CONTINUE_ACTION);

            if (
                editor->mode == EDITOR_MODE_NULL ||
                editor->mode >= _EDITOR_MODE_COUNT
            ) {
                editor->mode = EDITOR_MODE_NORMAL;
            }

            // Actually executing whatever editor mode we are in
            // This is where the bulk of the editor logic lies
            b32 consumed_input = _mode_funcs[editor->mode](
                editor, wb, cur_input, editor->count
            );

            if (consumed_input) {
                editor->cur_inputs_size = 0;
                editor->count = 1;

                if (!IS_FLAG_SET_U32(
                    editor->flags, _EDITOR_FLAG_CONTINUE_ACTION
                )) {
                    editor->action_start_input = editor->input_queue_end;
                    
                    editor->selected_register = EDITOR_REGISTER_DEFAULT;
                }
            } else if (editor->cur_inputs_size < EDITOR_INPUT_SEQ_MAX) {
                editor->cur_inputs[editor->cur_inputs_size++] = cur_input;
            }
        }


        continue;

    ignore_input:
        u32 start = editor->input_queue_start;
        start = start == 0 ? EDITOR_INPUT_QUEUE_MAX - 1 : start - 1;
        u32 end = start;

        if (editor->input_queue_size > 0) {
            end = editor->input_queue_end;
            end = end == 0 ? EDITOR_INPUT_QUEUE_MAX - 1 : end - 1;

            for (u32 i = start; i < end; i++) {
                editor->input_queue[i] = editor->input_queue[i + 1];
            }
        }

        editor->input_queue_start = start;
        editor->input_queue_end = end;
    }
}

