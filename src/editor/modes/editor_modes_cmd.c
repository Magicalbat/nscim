
void _editor_exec_cmd(editor_context* editor, workbook* wb);

b32 _editor_do_cmd(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    UNUSED(count);

    if (editor->cur_inputs_size != 0) {
        return true;
    }

    if (
        editor->cmd_size < EDITOR_CMD_MAX_STRLEN &&
        input >= ' ' && input <= '~'
    ) {
        for (
            u32 i = editor->cmd_size;
            i > editor->cmd_cursor; i--
        ) {
            editor->cmd_buf[i] = editor->cmd_buf[i-1]; 
        }

        editor->cmd_buf[editor->cmd_cursor] = input;

        editor->cmd_cursor++;
        editor->cmd_size++;

        return true;
    }

    switch (input) {
        case '\x1b': {
            editor->mode = EDITOR_MODE_NORMAL;
        } break;

        case '\r': {
            _editor_exec_cmd(editor, wb);
            editor->mode = EDITOR_MODE_NORMAL;
        } break;

        case WIN_INPUT_BACKSPACE: {
            if (editor->cmd_cursor > 0 && editor->cmd_size > 0) {
                for (u32 i = editor->cmd_cursor - 1; i < editor->cmd_size - 1; i++) {
                    editor->cmd_buf[i] = editor->cmd_buf[i + 1];
                }

                editor->cmd_cursor--;
                editor->cmd_size--;
            }
        } break;

        case WIN_INPUT_ARROW_LEFT: {
            if (editor->cmd_cursor > 0) {
                editor->cmd_cursor--; 
            }
        } break;

        case WIN_INPUT_ARROW_RIGHT: {
            if (editor->cmd_cursor < editor->cmd_size) {
                editor->cmd_cursor++; 
            }
        } break;
    }

    return true;
}

void _editor_exec_cmd(editor_context* editor, workbook* wb) {
    string8 cmd = (string8){
        .str = editor->cmd_buf,
        .size = editor->cmd_size 
    };

    if (str8_equals(STR8_LIT("q"), cmd)) {
        SET_FLAG_U32(editor->flags, EDITOR_FLAG_SHOULD_QUIT);
    }
}

