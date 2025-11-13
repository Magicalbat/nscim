
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

    editor_action action = EDITOR_ACTION_NONE;

    // NOTE: Temporary input handling for testing purposes
    switch (input) {
        case WIN_INPUT_ARROW_LEFT:
        case 'h': {
            action = EDITOR_ACTION_MOVE_LEFT;
        } break;
            
        case WIN_INPUT_ARROW_DOWN:
        case 'j': {
            action = EDITOR_ACTION_MOVE_DOWN;
        } break;

        case WIN_INPUT_ARROW_UP:
        case 'k': {
            action = EDITOR_ACTION_MOVE_UP;
        } break;

        case WIN_INPUT_ARROW_RIGHT:
        case 'l': {
            action = EDITOR_ACTION_MOVE_RIGHT;
        } break;

        case 'w': {
            action = EDITOR_ACTION_SCROLL_UP;
        } break;
        case 'a': {
            action = EDITOR_ACTION_SCROLL_LEFT;
        } break;
        case 's': {
            action = EDITOR_ACTION_SCROLL_DOWN;
        } break;
        case 'd': {
            action = EDITOR_ACTION_SCROLL_RIGHT;
        } break;

        case 'c': {
            action = EDITOR_ACTION_WIN_CLOSE;
        } break;
    }

    editor_execute_action(editor, wb, action, 5);
}

