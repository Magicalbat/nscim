b32 _editor_do_visual(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    b32 enter_normal = false;

    sheet_range select_range = (sheet_range){ {
        wb->active_win->select_start, wb->active_win->cursor_pos 
    }};

    if (editor->cur_inputs_size == 0) {
        switch (input) {
            case '\x1b': { enter_normal = true; } break;

            case WIN_INPUT_ARROW_LEFT:
            case 'h': { _editor_cursor_left(editor, wb, count); } break;
            case WIN_INPUT_ARROW_DOWN:
            case 'j': { _editor_cursor_down(editor, wb, count); } break;
            case WIN_INPUT_ARROW_UP:
            case 'k': { _editor_cursor_up(editor, wb, count); } break;
            case WIN_INPUT_ARROW_RIGHT:
            case 'l': { _editor_cursor_right(editor, wb, count); } break;

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
                    editor, wb, wb->active_win->cursor_pos.col
                );
            } break;
            
            case WIN_INPUT_CTRL('e'): {
                _editor_scroll_down(editor, wb, count); 
            } break;
            case WIN_INPUT_CTRL('y'): {
                _editor_scroll_up(editor, wb, count); 
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
                sheet_buffer* sheet = wb_get_active_sheet(wb, false);
                wb_copy_range(wb, sheet, select_range);

                enter_normal = true;
            } break;

            case 'x': 
            case 'd': {
                // TODO: x vs d behavior for entire row/column

                sheet_buffer* sheet = wb_get_active_sheet(wb, false);
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
                _editor_continue_series(wb, select_range, _EDITOR_SERIES_INFER);
                enter_normal = true;
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
                    case 'h': { _editor_scroll_left(editor, wb, count); } break;
                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': { _editor_scroll_down(editor, wb, count); } break;
                    case WIN_INPUT_ARROW_UP:
                    case 'k': { _editor_scroll_up(editor, wb, count); } break;
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': { _editor_scroll_right(editor, wb, count); } break;

                    case 'z': {
                        _editor_scroll_center(wb);
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
                            _editor_resize_col_width(wb, col, -(i32)count);
                        }
                    } break;

                    case WIN_INPUT_ARROW_DOWN:
                    case 'j': {
                        for (
                            u32 row = fixed_range.start.row;
                            row <= fixed_range.end.row; row++
                        ) {
                            _editor_resize_row_height(wb, row, (i32)count);
                        }
                    } break;

                    case WIN_INPUT_ARROW_UP:
                    case 'k': {
                        for (
                            u32 row = fixed_range.start.row;
                            row <= fixed_range.end.row; row++
                        ) {
                            _editor_resize_row_height(wb, row, -(i32)count);
                        }
                    } break;
                        
                    case WIN_INPUT_ARROW_RIGHT:
                    case 'l': {
                        for (
                            u32 col = fixed_range.start.col;
                            col <= fixed_range.end.col; col++
                        ) {
                            _editor_resize_col_width(wb, col, (i32)count);
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
                            wb, select_range, _EDITOR_SERIES_INFER
                        );
                        enter_normal = true;
                    } break;

                    case 'l': {
                        _editor_continue_series(
                            wb, select_range, _EDITOR_SERIES_LINEAR
                        );
                        enter_normal = true;
                    } break;

                    case 'e': {
                        _editor_continue_series(
                            wb, select_range, _EDITOR_SERIES_EXPONENTIAL
                        );
                        enter_normal = true;
                    } break;
                }
            } break;

            // Window Commands 
            case WIN_INPUT_CTRL('w'): {
                enter_normal = true;

                switch (input) {
                    case 'h': {
                        while (count--) { wb_win_change_active_horz(wb, -1); }
                    } break;
                    case 'j': {
                        while (count--) { wb_win_change_active_vert(wb, +1); }
                    } break;
                    case 'k': {
                        while (count--) { wb_win_change_active_vert(wb, -1); }
                    } break;
                    case 'l': {
                        while (count--) { wb_win_change_active_horz(wb, +1); }
                    } break;

                    case 'c': { wb_win_close(wb); } break;

                    // TODO: deal with count for these
                    case 'v': {
                        wb_win_split(wb, SHEETS_WIN_SPLIT_VERT, true);
                    } break;

                    case 's': {
                        wb_win_split(wb, SHEETS_WIN_SPLIT_HORZ, true);
                    } break;

                    case 'n': {
                        wb_win_split(wb, SHEETS_WIN_SPLIT_HORZ, false);
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
