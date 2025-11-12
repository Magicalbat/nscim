
void editor_execute_action(
    editor_context* editor, workbook* wb,
    editor_action action, u32 repeat
) {
    // Converting back to enum to get warnings about missed
    // cases in the switch statement
    editor_action_enum action_e = action;

    sheet_window* win = wb->active_win;

    if (EDITOR_ACTION_IS_MOTION(action)) {
        editor->motion_range.start = win->cursor_pos;
    }

    switch (action_e) {
        case EDITOR_ACTION_MOVE_UP: {
            u32 move_size = MIN(win->cursor_pos.row, repeat);
            win->cursor_pos.row -= move_size;
        } break;

        case EDITOR_ACTION_MOVE_DOWN: {
            win->cursor_pos.row += repeat;
            if (win->cursor_pos.row >= SHEET_MAX_ROWS) {
                win->cursor_pos.row = SHEET_MAX_ROWS-1;
            }
        } break;

        case EDITOR_ACTION_MOVE_LEFT: {
            u32 move_size = MIN(win->cursor_pos.col, repeat);
            win->cursor_pos.col -= move_size;
        } break;

        case EDITOR_ACTION_MOVE_RIGHT: {
            win->cursor_pos.col += repeat;
            if (win->cursor_pos.col >= SHEET_MAX_COLS) {
                win->cursor_pos.col = SHEET_MAX_COLS-1;
            }
        } break;


        case EDITOR_ACTION_WIN_CLOSE: {
            wb_win_close(wb);
        } break;

        case EDITOR_ACTION_NONE:
        case _EDITOR_ACTION_MOTION_END:
        case _EDITOR_ACTION_MODIFY_END:
        case _EDITOR_ACTION_COUNT:
            break;
    }

    if (EDITOR_ACTION_IS_MOTION(action)) {
        editor->motion_range.end = win->cursor_pos;
    }
}

