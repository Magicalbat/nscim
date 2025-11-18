
void _editor_cursor_up(workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->cursor_pos.row, n);
    win->cursor_pos.row -= move_size;

    u32 ref_row = win->cursor_pos.row < 2 ? 0 : win->cursor_pos.row - 2;
    if (ref_row < win->scroll_pos.row) {
        win->scroll_pos.row = ref_row;
    }
}

void _editor_cursor_down(workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN((SHEET_MAX_ROWS - 1) - win->cursor_pos.row, n);
    win->cursor_pos.row += move_size;

    u32 ref_row = win->cursor_pos.row >= SHEET_MAX_ROWS - 2 ?
        SHEET_MAX_ROWS : win->cursor_pos.row + 2;

    if (ref_row >= win->scroll_pos.row + win->num_rows) {
        sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

        i64 height_diff = 0;
        for (
            u32 row = win->scroll_pos.row + win->num_rows;
            row <= ref_row; row++
        ) {
            height_diff += (i64)sheet_get_row_height(sheet, row);
        }

        while (height_diff > 0) {
            height_diff -= (i64)sheet_get_row_height(sheet, win->scroll_pos.row);
            win->scroll_pos.row++;
        }
    }
}

void _editor_cursor_left(workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->cursor_pos.col, n);
    win->cursor_pos.col -= move_size;

    u32 ref_col = win->cursor_pos.col < 1 ? 0 : win->cursor_pos.col - 1;
    if (ref_col < win->scroll_pos.col) {
        win->scroll_pos.col = ref_col;
    }
}

void _editor_cursor_right(workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN((SHEET_MAX_COLS - 1) - win->cursor_pos.col, n);
    win->cursor_pos.col += move_size;

    u32 ref_col = win->cursor_pos.col >= SHEET_MAX_COLS - 1 ?
        SHEET_MAX_COLS : win->cursor_pos.col + 1;

    if (ref_col >= win->scroll_pos.col + win->num_cols) {
        sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

        i64 width_diff = 0;
        for (
            u32 col = win->scroll_pos.col + win->num_cols;
            col <= ref_col; col++
        ) {
            width_diff += (i64)sheet_get_col_width(sheet, col);
        }

        while (width_diff > 0) {
            width_diff -= (i64)sheet_get_col_width(sheet, win->scroll_pos.col);
            win->scroll_pos.col++;
        }
    }
}


