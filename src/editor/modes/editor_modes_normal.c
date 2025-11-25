
b32 _editor_do_normal(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    if (editor->raw_input_seq_size == 0) {
        switch (input) {
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

            // All of these begin multi-input actions
            case 'z':
            case 'r':
            case WIN_INPUT_CTRL('w'): {
                return false;
            } break;
        }
    } else if (editor->raw_input_seq_size == 1) {
        switch (editor->raw_input_seq[0]) {
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

            case 'r': {
                switch (input) {
                    case 'h': {
                        _editor_resize_width(wb, (i32)(-count));
                    } break;
                    case 'j': {
                        _editor_resize_height(wb, (i32)count);
                    } break;
                    case 'k': {
                        _editor_resize_height(wb, (i32)(-count));
                    } break;
                    case 'l': {
                        _editor_resize_width(wb, (i32)count);
                    } break;
                }
            } break;



            // Window Commands 
            case WIN_INPUT_CTRL('w'): {
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
                }
            } break;
        }
    }

    return true;
}

