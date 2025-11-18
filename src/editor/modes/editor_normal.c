
b32 _editor_do_normal(
    editor_context* editor, workbook* wb,
    win_input input, u32 count
) {
    if (editor->raw_input_seq_size == 0) {
        switch (input) {
            case WIN_INPUT_ARROW_LEFT:
            case 'h': {
                _editor_cursor_left(wb, count);
            } break;

            case WIN_INPUT_ARROW_DOWN:
            case 'j': {
                _editor_cursor_down(wb, count);
            } break;

            case WIN_INPUT_ARROW_UP:
            case 'k': {
                _editor_cursor_up(wb, count);
            } break;

            case WIN_INPUT_ARROW_RIGHT:
            case 'l': {
                _editor_cursor_right(wb, count);
            } break;

            case WIN_INPUT_CTRL('w'): {
                return false;
            } break;
        }
    } else if (editor->raw_input_seq_size == 1) {
        if (editor->raw_input_seq[0] == WIN_INPUT_CTRL('w')) {
            switch (input) {
                case 'h': {
                    while (count--) {
                        wb_win_change_active_horz(wb, -1);
                    }
                } break;

                case 'j': {
                    while (count--) {
                        wb_win_change_active_vert(wb, +1);
                    }
                } break;

                case 'k': {
                    while (count--) {
                        wb_win_change_active_vert(wb, -1);
                    }
                } break;

                case 'l': {
                    while (count--) {
                        wb_win_change_active_horz(wb, +1);
                    }
                } break;

                case 'c': {
                    wb_win_close(wb);
                } break;

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

            return true;
        }
    }

    return true;
}

