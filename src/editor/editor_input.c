
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
                (editor->flags & _EDITOR_FLAG_READING_NUM) !=
                _EDITOR_FLAG_READING_NUM
            ) {
                editor->count = 0;
            }

            editor->flags |= _EDITOR_FLAG_READING_NUM;

            // Ignore any counts over ten million
            if (editor->count >= 1e7) { goto ignore_input; }

            editor->count *= 10;
            editor->count += cur_input - '0';
        } else {
            editor->flags &= ~(u32)_EDITOR_FLAG_READING_NUM;

            if (
                editor->mode == EDITOR_MODE_NULL ||
                editor->mode >= _EDITOR_MODE_COUNT
            ) {
                editor->mode = EDITOR_MODE_NORMAL;
            }

            b32 consumed_input = _mode_funcs[editor->mode](
                editor, wb, cur_input, editor->count
            );

            if (consumed_input) {
                editor->cur_inputs_size = 0;
                editor->count = 1;

                if (
                    (editor->flags & _EDITOR_FLAG_PENDING_MOTION) !=
                    _EDITOR_FLAG_PENDING_MOTION
                ) {
                    editor->action_start_input = editor->input_queue_end;
                }
            } else if (editor->cur_inputs_size < EDITOR_INPUT_SEQ_MAX) {
                editor->cur_inputs[editor->cur_inputs_size++] = cur_input;
            }
        }


        continue;

    ignore_input:
        if (editor->input_queue_size == 0) {
            u32 pos = editor->input_queue_start;
            pos = pos == 0 ? EDITOR_INPUT_QUEUE_MAX - 1 : pos - 1;

            editor->input_queue_start = pos;
            editor->input_queue_end = pos;
        } else {
        }
    }
}

