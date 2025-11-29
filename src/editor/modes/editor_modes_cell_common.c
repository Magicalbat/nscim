
void _editor_load_cell_to_input(
    editor_context* editor, workbook* wb, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_get_active_sheet(wb, false);

    sheet_cell_ref cell = sheet_get_cell(wb, sheet, win->cursor_pos, false);

    u32 size = sheets_cell_to_chars(
        cell, editor->cell_input_buf,
        sizeof(editor->cell_input_buf)
    );

    editor->cell_input_size = size;

    if (editor->cell_input_size <= max_cursor_off) {
        editor->cell_input_cursor = 0;
    } else {
        editor->cell_input_cursor = editor->cell_input_size - max_cursor_off;
    }
}

void _editor_store_cell_from_input(editor_context* editor, workbook* wb) {
    sheet_window* win = wb->active_win;

    string8 base_str = { editor->cell_input_buf, (u64)editor->cell_input_size };
    string8 str = base_str;

    if (str.size == 0) {
        // Fetching the sheet separately in case it is empty
        // It is not necessary to create the sheet just to clear a
        // cell that must already be empty
        sheet_buffer* tmp_sheet = wb_get_active_sheet(wb, false);
        sheet_clear_cell(wb, tmp_sheet, win->cursor_pos);
        return;
    }

    sheet_buffer* sheet = wb_get_active_sheet(wb, true);

    b8 is_num = true;
    b8 negate = false;
    b8 past_decimal = false;
    b8 past_sci_exp_decimal = false;
    b8 reading_sci_exp = false;
    b8 negate_sci_exp = false;
    u32 decimal_count = 0;
    u32 sci_exp_decimal_count = 0;
    f64 num = 0.0;
    f64 sci_exp = 0.0;

    if (str.str[0] == '-' || str.str[0] == '+') {
        negate = str.str[0] == '-';

        str.str++;
        str.size--;
    } 

    for (u64 i = 0; is_num && i < str.size; i++) {
        u8 c = str.str[i];

        switch (c) {
            case '.': {
                if (reading_sci_exp) {
                    if (past_sci_exp_decimal) {
                        is_num = false;
                    } else {
                        past_sci_exp_decimal = true;
                    }
                } else {
                    if (past_decimal) {
                        is_num = false;
                    } else {
                        past_decimal = true;
                    }
                }
            } break;

            case 'e':
            case 'E': {
                if (reading_sci_exp) {
                    is_num = false;
                } else {
                    reading_sci_exp = true;

                    if (i + 1 >= str.size) {
                        is_num = false;
                    } else {
                        if (str.str[i + 1] == '-' || str.str[i + 1] == '+') {
                            negate_sci_exp = str.str[i + 1] == '-';
                            i++;
                        }
                    }
                }
            } break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                if (reading_sci_exp) {
                    if (past_sci_exp_decimal) {
                        sci_exp_decimal_count++;
                    }

                    sci_exp *= 10.0;
                    sci_exp += c - '0';
                } else {
                    if (past_decimal) {
                        decimal_count++;
                    }

                    num *= 10.0;
                    num += c - '0';
                }
            } break;

            default: {
                is_num = false;
            } break;
        }
    }

    if (is_num) {
        while (decimal_count--) {
            num *= 0.1;
        }
        
        if (reading_sci_exp) {
            while (sci_exp_decimal_count--) {
                sci_exp *= 0.1;
            }

            if (negate_sci_exp) {
                sci_exp *= -1.0;
            }

            num *= pow(10.0, sci_exp);
        }

        if (negate) {
            num *= -1.0;
        }

        sheet_set_cell_num(wb, sheet, win->cursor_pos, num);
    } else {
        sheet_set_cell_str(wb, sheet, win->cursor_pos, base_str);
    }
}

void _editor_input_cursor_left(editor_context* editor, u32 n) {
    u32 move_size = MIN(n, editor->cell_input_cursor);
    editor->cell_input_cursor -= move_size;
}

void _editor_input_cursor_right(
    editor_context* editor, u32 n, u32 max_cursor_off
) {
    u32 max_cursor = editor->cell_input_size > max_cursor_off ? 
        editor->cell_input_size - max_cursor_off : 0;

    u32 move_size = MIN(n, max_cursor - editor->cell_input_cursor);
    editor->cell_input_cursor += move_size;
}

void _editor_input_delete(editor_context* editor, u32 start, u32 n) {
    start = MIN(start, editor->cell_input_size - 1);
    n = MIN(n, editor->cell_input_size - start);

    if (n == 0) { return; }

    for (u32 i = start; i < editor->cell_input_size - n; i++) {
        editor->cell_input_buf[i] = editor->cell_input_buf[i + n];
    }

    editor->cell_input_size -= n;
}

void _editor_input_active_up(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.row == 0) { return; }

    u32 move_size = MIN(n, win->cursor_pos.row);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.row -= move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_down(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.row == SHEET_MAX_ROWS - 1) { return; }

    u32 move_size = MIN(n, SHEET_MAX_ROWS - 1 - win->cursor_pos.row);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.row += move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_left(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.col == 0) { return; }

    u32 move_size = MIN(n, win->cursor_pos.col);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.col -= move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

void _editor_input_active_right(
    editor_context* editor, workbook* wb, u32 n, u32 max_cursor_off
) {
    sheet_window* win = wb->active_win;

    if (win->cursor_pos.col == SHEET_MAX_COLS - 1) { return; }

    u32 move_size = MIN(n, SHEET_MAX_COLS - 1 - win->cursor_pos.col);

    _editor_store_cell_from_input(editor, wb);
    win->cursor_pos.col += move_size;
    _editor_load_cell_to_input(editor, wb, max_cursor_off);
}

