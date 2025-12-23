b32 _editor_do_visual(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    b32 enter_normal = false;

    editor_window* active_win = editor->active_win;

    sheet_range select_range = (sheet_range){ {
        active_win->select_start, active_win->cursor_pos 
    }};

    if (editor->cur_inputs_size == 0) {
        switch (input) {
            case '\x1b': { enter_normal = true; } break;

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

            case 'y': {
                sheet_buffer* sheet = editor_get_active_sheet(
                    editor, wb, false
                );
                editor_register* reg = editor_get_reg(
                    editor, editor->selected_register
                );

                editor_reg_set_cells(NULL, reg, sheet, select_range);

                enter_normal = true;
            } break;

            case 'x': 
            case 'd': {
                // TODO: x vs d behavior for entire row/column

                sheet_buffer* sheet = editor_get_active_sheet(
                    editor, wb, false
                );
                // Clear cell will do nothing on an empty sheet
                sheet_clear_range(wb, sheet, select_range);

                enter_normal = true;
            } break;

            case 'i': {
                #warning TODO
            } break;

            case 'e': {
                #warning TODO
            } break;

            case 'm': {
                _editor_continue_series(
                    editor, wb, select_range, _EDITOR_SERIES_INFER
                );
                enter_normal = true;
            } break;

            // All of these begin multi-input actions
            case 'z':
            case 'f':
            case 'a':
            case 'M':
            case '"':
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
                sheet_range fixed_range = sheets_fix_range(select_range);

                switch (input) {
                    case WIN_INPUT_ARROW_LEFT:
                    case 'h': {
                        for (
                            u32 col = fixed_range.start.col;
                            col <= fixed_range.end.col; col++
                        ) {
                            _editor_resize_col_width(
                                editor, wb, col, -(i32)count
                            );
                        }
                    } break;

                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': {
                        for (
                            u32 row = fixed_range.start.row;
                            row <= fixed_range.end.row; row++
                        ) {
                            _editor_resize_row_height(
                                editor, wb, row, (i32)count
                            );
                        }
                    } break;

                    case WIN_INPUT_ARROW_UP:
                    case 'k': {
                        for (
                            u32 row = fixed_range.start.row;
                            row <= fixed_range.end.row; row++
                        ) {
                            _editor_resize_row_height(
                                editor, wb, row, -(i32)count
                            );
                        }
                    } break;
                        
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': {
                        for (
                            u32 col = fixed_range.start.col;
                            col <= fixed_range.end.col; col++
                        ) {
                            _editor_resize_col_width(
                                editor, wb, col, (i32)count
                            );
                        }
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
                    case 'i': {
                        _editor_continue_series(
                            editor, wb, select_range, _EDITOR_SERIES_INFER
                        );
                        enter_normal = true;
                    } break;

                    case 'l': {
                        _editor_continue_series(
                            editor, wb, select_range, _EDITOR_SERIES_LINEAR
                        );
                        enter_normal = true;
                    } break;

                    case 'e': {
                        _editor_continue_series(
                            editor, wb, select_range,
                            _EDITOR_SERIES_EXPONENTIAL
                        );
                        enter_normal = true;
                    } break;
                }
            } break;

            case '"': {
                _editor_try_select_register(editor, input);
            } break;

            // Window Commands 
            case WIN_INPUT_CTRL('w'): {
                enter_normal = true;

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

                    default: {
                        enter_normal = false;
                    } break;
                }
            } break;
        }
    }

    if (enter_normal) {
        editor->mode = EDITOR_MODE_NORMAL;
    }

    return true;
}
