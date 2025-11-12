
void editor_update(window* win, editor_context* editor, workbook* wb) {
    win_input input = win_next_input(win);

    if (input == 0) {
        editor->should_draw = false;
        return;
    } else {
        editor->should_draw = true;
    }

    if (input == 'q') {
        editor->should_quit = true;
        return;
    }

    // NOTE: Temporary input handling for testing purposes
    switch (input) {
        case WIN_INPUT_ARROW_LEFT:
        case 'h': {
            editor_execute_action(editor, wb, EDITOR_ACTION_MOVE_LEFT, 1);
        } break;
            
        case WIN_INPUT_ARROW_DOWN:
        case 'j': {
            editor_execute_action(editor, wb, EDITOR_ACTION_MOVE_DOWN, 1);
        } break;

        case WIN_INPUT_ARROW_UP:
        case 'k': {
            editor_execute_action(editor, wb, EDITOR_ACTION_MOVE_UP, 1);
        } break;

        case WIN_INPUT_ARROW_RIGHT:
        case 'l': {
            editor_execute_action(editor, wb, EDITOR_ACTION_MOVE_RIGHT, 1);
        } break;

        case 'c': {
            editor_execute_action(editor, wb, EDITOR_ACTION_WIN_CLOSE, 1);
        } break;
    }
}

