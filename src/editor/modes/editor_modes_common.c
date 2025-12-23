
void _editor_await_motion(editor_context* editor, win_input cur_input) {
    SET_FLAG_U32(editor->flags, _EDITOR_FLAG_PENDING_MOTION);
    SET_FLAG_U32(editor->flags, _EDITOR_FLAG_CONTINUE_ACTION);

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
    CLEAR_FLAG_U32(editor->flags, _EDITOR_FLAG_PENDING_MOTION);
    CLEAR_FLAG_U32(editor->flags, _EDITOR_FLAG_CONTINUE_ACTION);
    editor->pending_action_count = 1;
    editor->pending_action_inputs_size = 0;
}

void _editor_try_select_register(editor_context* editor, win_input input) {
    b32 lowercase = false;

    if (input >= 'a' && input <= 'z') {
        input -= 'a' - 'A';
        lowercase = true;
    }

    if (
        input < EDITOR_REGISTER_FIRST ||
        input > EDITOR_REGISTER_LAST
    ) {
        return;
    }

    u32 index = input - EDITOR_REGISTER_FIRST;
    if (
        editor->registers[index] == NULL ||
        editor->registers[index]->reg_type ==
            EDITOR_REGISTER_TYPE_INVALID
    ) {
        return;
    }

    editor->append_to_register = false;
    if (!lowercase && input >= 'A' && input <= 'Z') {
        editor->append_to_register = true;
    }

    editor->selected_register = input;
    SET_FLAG_U32(editor->flags, _EDITOR_FLAG_CONTINUE_ACTION);
}

