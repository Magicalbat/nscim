
void _editor_await_motion(editor_context* editor, win_input cur_input) {
    editor->flags |= _EDITOR_FLAG_PENDING_MOTION;

    if (editor->cur_inputs_size < EDITOR_INPUT_SEQ_MAX) {
        editor->cur_inputs[editor->cur_inputs_size++] = cur_input;
    }


    editor->pending_action_inputs_size = editor->cur_inputs_size;

    memcpy(
        editor->pending_action_inputs, editor->cur_inputs,
        sizeof(win_input) * editor->cur_inputs_size
    );

    editor->cur_inputs_size = 0;
    editor->pending_action_count = editor->count;
}

void _editor_consume_motion(editor_context* editor) {
    editor->flags &= ~(u32)_EDITOR_FLAG_PENDING_MOTION;
    editor->pending_action_count = 1;
    editor->pending_action_inputs_size = 0;
}

