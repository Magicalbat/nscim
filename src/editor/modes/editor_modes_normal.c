
b32 _editor_do_normal(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    editor_window* active_win = editor->active_win;

    u32 act_on_motion = false;
    sheet_pos init_cursor = active_win->cursor_pos;

    // Checking if the same inputs were pressed twice, execute action if so
    // This allows for dd to delete the current cell or
    // yy to yank the current cell, etc.
    u32 next_inputs_size = editor->cur_inputs_size;
    if (next_inputs_size < EDITOR_INPUT_SEQ_MAX) { next_inputs_size++; };

    if (
        editor->flags & _EDITOR_FLAG_PENDING_MOTION && 
        next_inputs_size == editor->pending_action_inputs_size
    ) {
        b32 same_input = true;

        win_input next_inputs[EDITOR_INPUT_SEQ_MAX] = { };
        memcpy(
            next_inputs, editor->cur_inputs,
            sizeof(win_input) * editor->cur_inputs_size
        );

        if (next_inputs_size <= EDITOR_INPUT_SEQ_MAX) {
            next_inputs[next_inputs_size - 1] = input;
        }

        for (u32 i = 0; i < next_inputs_size; i++) {
            if (next_inputs[i] != editor->pending_action_inputs[i]) {
                same_input = false;
                break;
            }
        }

        if (same_input) {
            act_on_motion = true;
            goto execute_motion_action;
        }
    }

    if (editor->flags & _EDITOR_FLAG_PENDING_MOTION) {
        count *= editor->pending_action_count;
    }
    
    if (editor->cur_inputs_size == 0) {
        switch (input) {
            case 'v': {
                editor_window* win = active_win;

                win->select_start = win->cursor_pos;
                editor->mode = EDITOR_MODE_VISUAL;
            } break;

            case 'i': {
                _editor_load_cell_to_input(editor, wb, 0);
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;

            case 'I': {
                sheet_pos prev_pos = active_win->prev_edit_pos;
                _editor_cursor_set_row(editor, wb, prev_pos.row);
                _editor_cursor_set_col(editor, wb, prev_pos.col);

                _editor_load_cell_to_input(editor, wb, 0);
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;

            case 'e': {
                _editor_load_cell_to_input(editor, wb, 1);
                editor->mode = EDITOR_MODE_CELL_EDIT;
            } break;

            case 'E': {
                sheet_pos prev_pos = active_win->prev_edit_pos;
                _editor_cursor_set_row(editor, wb, prev_pos.row);
                _editor_cursor_set_col(editor, wb, prev_pos.col);

                _editor_load_cell_to_input(editor, wb, 1);
                editor->mode = EDITOR_MODE_CELL_EDIT;
            } break;

            case 's': {
                sheet_buffer* sheet = editor_get_active_sheet(
                    editor, wb, false
                );
                sheet_clear_cell(wb, sheet, active_win->cursor_pos);

                _editor_load_cell_to_input(editor, wb, 0);
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;

            case 'S': {
                sheet_pos prev_pos = active_win->prev_edit_pos;
                _editor_cursor_set_row(editor, wb, prev_pos.row);
                _editor_cursor_set_col(editor, wb, prev_pos.col);

                sheet_buffer* sheet = editor_get_active_sheet(
                    editor, wb, false
                );
                sheet_clear_cell(wb, sheet, active_win->cursor_pos);

                _editor_load_cell_to_input(editor, wb, 0);
                editor->mode = EDITOR_MODE_CELL_INSERT;
            } break;

            case ':': {
                editor->mode = EDITOR_MODE_CMD;
                editor->cmd_size = 0;
                editor->cmd_cursor = 0;
            } break;

            case WIN_INPUT_ARROW_LEFT:
            case 'h': { _editor_cursor_left(editor, count, true); } break;
            case WIN_INPUT_ARROW_DOWN:
            case 'j': { _editor_cursor_down(editor, wb, count, true); } break;
            case WIN_INPUT_ARROW_UP:
            case 'k': { _editor_cursor_up(editor, count, true); } break;
            case WIN_INPUT_ARROW_RIGHT:
            case 'l': { _editor_cursor_right(editor, wb, count, true); } break;

            case WIN_INPUT_CTRL_ARROW_LEFT:
            case 'H': {
                _editor_move_block_horz(editor, wb, -(i32)count); 
            } break;
            case WIN_INPUT_CTRL_ARROW_DOWN:
            case 'J': {
                _editor_move_block_vert(editor, wb, (i32)count); 
            } break;
            case WIN_INPUT_CTRL_ARROW_UP:
            case 'K': {
                _editor_move_block_vert(editor, wb, -(i32)count); 
            } break;
            case WIN_INPUT_CTRL_ARROW_RIGHT:
            case 'L': {
                _editor_move_block_horz(editor, wb, (i32)count); 
            } break;

            case '0': {
                _editor_cursor_left(
                    editor, active_win->cursor_pos.col, true
                );
            } break;
            
            case WIN_INPUT_CTRL('e'): {
                _editor_scroll_down(editor, count); 
            } break;
            case WIN_INPUT_CTRL('y'): {
                _editor_scroll_up(editor, count); 
            } break;

            case WIN_INPUT_CTRL('b'): {
                _editor_move_win_multiple_vert(editor, wb, -(f32)count);
            } break;
            case WIN_INPUT_CTRL('f'): {
                _editor_move_win_multiple_vert(editor, wb, (f32)count);
            } break;
            case WIN_INPUT_CTRL('u'): {
                _editor_move_win_multiple_vert(editor, wb, -(f32)count * 0.5f);
            } break;
            case WIN_INPUT_CTRL('d'): {
                _editor_move_win_multiple_vert(editor, wb, (f32)count * 0.5f);
            } break;

            case WIN_INPUT_CTRL('g'): {
                _editor_move_win_multiple_horz(editor, wb, (f32)count * 0.5f);
            } break;
            case WIN_INPUT_CTRL('s'): {
                _editor_move_win_multiple_horz(editor, wb, -(f32)count * 0.5f);
            } break;

            case 'x': {
                sheet_buffer* sheet = editor_get_active_sheet(editor, wb, false);
                // Clear cell will do nothing on an empty sheet
                sheet_clear_cell(wb, sheet, active_win->cursor_pos);
            } break;
            
            case 'X':
            case 'd':
            case 'm': {
                _editor_await_motion(editor, input);
                return true;
            } break;

            // All of these begin multi-input actions
            case 'z':
            case 'f':
            case 'a':
            case 'M':
            case WIN_INPUT_CTRL('w'): {
                return false;
            } break;
        }
    } else if (editor->cur_inputs_size == 1) {
        switch (editor->cur_inputs[0]) {
            case 'z': {
                switch (input) {
                    case WIN_INPUT_ARROW_LEFT:
                    case 'h': { _editor_scroll_left(editor, count); } break;
                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': { _editor_scroll_down(editor, count); } break;
                    case WIN_INPUT_ARROW_UP:
                    case 'k': { _editor_scroll_up(editor, count); } break;
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': { _editor_scroll_right(editor, count); } break;

                    case 'z': {
                        _editor_scroll_center(editor);
                    } break;
                }
            } break;

            case 'f': {
                switch (input) {
                    case WIN_INPUT_ARROW_LEFT:
                    case 'h': {
                        _editor_resize_col_width(
                            editor, wb, active_win->cursor_pos.col, -(i32)count
                        );
                    } break;

                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': {
                        _editor_resize_row_height(
                            editor, wb, active_win->cursor_pos.row, (i32)count
                        );
                    } break;

                    case WIN_INPUT_ARROW_UP:
                    case 'k': {
                        _editor_resize_row_height(
                            editor, wb, active_win->cursor_pos.row, -(i32)count
                        );
                    } break;
                        
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': {
                        _editor_resize_col_width(
                            editor, wb, active_win->cursor_pos.col, (i32)count
                        );
                    } break;
                }
            } break;

            case 'a': {
                switch (input) {
                    case WIN_INPUT_ARROW_LEFT:
                    case 'h': {
                        _editor_move_along_horz(editor, wb, -1);
                    } break;

                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': {
                        _editor_move_along_vert(editor, wb, +1);
                    } break;

                    case WIN_INPUT_ARROW_UP:
                    case 'k': {
                        _editor_move_along_vert(editor, wb, -1);
                    } break;
                        
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': {
                        _editor_move_along_horz(editor, wb, +1);
                    } break;
                }
            } break;

            case 'M': {
                switch (input) {
                    case 'i':
                    case 'e':
                    case 'l': {
                        _editor_await_motion(editor, input);
                        return true;
                    } break;
                }
            } break;

            // Window Commands 
            case WIN_INPUT_CTRL('w'): {
                switch (input) {
                    case 'h': {
                        while (count--) {
                            editor_win_change_active_horz(editor, -1); 
                        }
                    } break;
                    case 'j': {
                        while (count--) {
                            editor_win_change_active_vert(editor, +1); 
                        }
                    } break;
                    case 'k': {
                        while (count--) {
                            editor_win_change_active_vert(editor, -1); 
                        }
                    } break;
                    case 'l': {
                        while (count--) {
                            editor_win_change_active_horz(editor, +1); 
                        }
                    } break;

                    case 'c': { editor_win_close(editor); } break;

                    // TODO: deal with count for these
                    case 'v': {
                        editor_win_split(editor, EDITOR_WIN_SPLIT_VERT, true);
                    } break;

                    case 's': {
                        editor_win_split(editor, EDITOR_WIN_SPLIT_HORZ, true);
                    } break;

                    case 'n': {
                        editor_win_split(editor, EDITOR_WIN_SPLIT_HORZ, false);
                    } break;
                }
            } break;
        }
    }

    if (
        (editor->flags & _EDITOR_FLAG_PENDING_MOTION) !=
        _EDITOR_FLAG_PENDING_MOTION
    ) {
        return true;
    }

execute_motion_action:

    sheet_pos cur_cursor = active_win->cursor_pos;

    if (
        init_cursor.row != cur_cursor.row ||
        init_cursor.col != cur_cursor.col
    ) {
        act_on_motion = true;
    }

    if (!act_on_motion) { goto consume_motion; }

    if (editor->pending_action_inputs_size == 0) {
        goto consume_motion;
    }

    sheet_range motion_range = { {init_cursor, cur_cursor} };

    switch (editor->pending_action_inputs[0]) {
        case 'X':
        case 'd': {
            // TODO: x vs d behavior for entire row/column

            sheet_clear_range(
                wb, editor_get_active_sheet(editor, wb, false),
                motion_range
            );
        } break;

        case 'M': 
        case 'm': {
            _editor_series_mode series_mode = _EDITOR_SERIES_INFER;

            if (editor->pending_action_inputs_size > 1) {
                switch (editor->pending_action_inputs[1]) {
                    case 'l': {
                        series_mode = _EDITOR_SERIES_LINEAR;
                    } break;
                    case 'e': {
                        series_mode = _EDITOR_SERIES_EXPONENTIAL;
                    } break;
                }
            }

            _editor_continue_series(editor, wb, motion_range, series_mode);
        } break;
    }

consume_motion:
    _editor_consume_motion(editor);
    return true;
}

