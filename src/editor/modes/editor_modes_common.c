
void _editor_cursor_up(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->cursor_pos.row, n);
    win->cursor_pos.row -= move_size;

    u32 pad = editor->settings.cursor_row_pad;
    u32 ref_row = win->cursor_pos.row < pad ? 0 : win->cursor_pos.row - pad;

    if (ref_row < win->scroll_pos.row) {
        win->scroll_pos.row = ref_row;
    }
}

void _editor_cursor_down(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN((SHEET_MAX_ROWS - 1) - win->cursor_pos.row, n);
    win->cursor_pos.row += move_size;

    u32 pad = editor->settings.cursor_row_pad;
    u32 ref_row = win->cursor_pos.row >= SHEET_MAX_ROWS - pad ?
        SHEET_MAX_ROWS - 1 : win->cursor_pos.row + pad;

    if (ref_row >= win->scroll_pos.row + win->num_rows || win->cutoff_height) {
        sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

        i64 height_diff = 0;
        for (
            u32 row = win->scroll_pos.row + win->num_rows;
            row <= ref_row; row++
        ) {
            height_diff += (i64)sheet_get_row_height(sheet, row);
        }

        if (win->cursor_pos.row == SHEET_MAX_ROWS - 1) {
            height_diff += win->cutoff_height;
        }

        while (height_diff > 0) {
            height_diff -= (i64)sheet_get_row_height(sheet, win->scroll_pos.row);
            win->scroll_pos.row++;
        }
    }
}

void _editor_cursor_left(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->cursor_pos.col, n);
    win->cursor_pos.col -= move_size;

    u32 pad = editor->settings.cursor_col_pad;
    u32 ref_col = win->cursor_pos.col < pad ? 0 : win->cursor_pos.col - pad;

    if (ref_col < win->scroll_pos.col) {
        win->scroll_pos.col = ref_col;
    }
}

void _editor_cursor_right(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN((SHEET_MAX_COLS - 1) - win->cursor_pos.col, n);
    win->cursor_pos.col += move_size;

    u32 pad = editor->settings.cursor_col_pad;
    u32 ref_col = win->cursor_pos.col >= SHEET_MAX_COLS - pad ?
        SHEET_MAX_COLS - 1 : win->cursor_pos.col + pad;

    if (ref_col >= win->scroll_pos.col + win->num_cols || win->cutoff_width) {
        sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

        i64 width_diff = 0;
        for (
            u32 col = win->scroll_pos.col + win->num_cols;
            col <= ref_col; col++
        ) {
            width_diff += (i64)sheet_get_col_width(sheet, col);
        }

        if (win->cursor_pos.col >= SHEET_MAX_COLS - 1) {
            width_diff += win->cutoff_width;
        }

        while (width_diff > 0) {
            width_diff -= (i64)sheet_get_col_width(sheet, win->scroll_pos.col);
            win->scroll_pos.col++;
        }
    }
}

void _editor_cursor_block_move_vert(editor_context* editor, workbook* wb, i32 n) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_get_active_sheet(wb, false);

    i32 row = (i32)win->cursor_pos.row;
    u32 col = win->cursor_pos.col;

    i32 dir = 1; u32 count = (u32)n;

    if (n < 0) {
        dir = -1;
        count = (u32)(-n);
    }

    while (count--) {
        if (row == (dir < 0 ? 0 : (i32)SHEET_MAX_ROWS - 1)) {
            break;
        }

        u32 is_empty_bitfield = 0;

        sheet_chunk_pos chunk_pos = {
            (u32)row / SHEET_CHUNK_ROWS,
            col / SHEET_CHUNK_COLS 
        };

        sheet_chunk* chunk = sheet_get_chunk(wb, sheet, chunk_pos, false);

        u32 col_offset = (col % SHEET_CHUNK_COLS) * SHEET_CHUNK_ROWS;

        for (u32 i = 0; i < 2; i++) {
            if (chunk == NULL) {
                is_empty_bitfield |= 1 << i;
            } else {
                u32 index = (u32)row % SHEET_CHUNK_ROWS + col_offset;

                if (chunk->types[index] == SHEET_CELL_TYPE_NONE) {
                    is_empty_bitfield |= 1 << i;
                }
            }

            if (i == 0 && row + dir > 0 && row + dir < (i32)SHEET_MAX_ROWS - 1) {
                row += dir; 

                if (((u32)row / SHEET_CHUNK_ROWS) != chunk_pos.row) {
                    chunk_pos.row = (u32)((i32)chunk_pos.row + dir);
                    chunk = sheet_get_chunk(wb, sheet, chunk_pos, false);
                }
            }
        }

        if ((is_empty_bitfield & 1) && !(is_empty_bitfield >> 1)) {
            continue;
        }

        b8 looking_for_empty = !(is_empty_bitfield >> 1);

        b8 searching = true;
        while (searching && row > 0 && row < (i32)SHEET_MAX_ROWS - 1) {
            if (chunk == NULL) {
                if (looking_for_empty) {
                    break;
                } else if (dir < 0 && chunk_pos.row == 0) {
                    row = 0;
                    break;
                } else if (dir > 0 && chunk_pos.row >= SHEET_CHUNKS_Y - 1) {
                    row = SHEET_MAX_ROWS - 1;
                    break;
                } else {
                    row = dir < 0 ? 
                        (i32)chunk_pos.row * SHEET_CHUNK_ROWS - 1:
                        (i32)(chunk_pos.row + 1) * SHEET_CHUNK_ROWS;

                    chunk_pos.row = (u32)((i32)chunk_pos.row + dir);
                    chunk = sheet_get_chunk(wb, sheet, chunk_pos, false);
                }
            } else {
                i32 end_row = dir < 0 ?
                    (i32)(chunk_pos.row * SHEET_CHUNK_ROWS) - 1 :
                    (i32)((chunk_pos.row + 1) * SHEET_CHUNK_ROWS);
                end_row = CLAMP(end_row, 0, (i32)SHEET_MAX_ROWS - 1);

                for (;row != end_row; row += dir) {
                    u32 index = (u32)(row % SHEET_CHUNK_ROWS) + col_offset;

                    if (
                        !((chunk->types[index] ==
                        SHEET_CELL_TYPE_NONE) ^ looking_for_empty)
                    ) {
                        searching = false;
                        break;
                    }
                }

                if (
                    searching && (i32)chunk_pos.row + dir >= 0 &&
                    (i32)chunk_pos.row + dir <= (i32)SHEET_MAX_ROWS - 1
                ) {
                    chunk_pos.row = (u32)((i32)chunk_pos.row + dir);
                    chunk = sheet_get_chunk(wb, sheet, chunk_pos, false);
                }
            }
        }

        if (row != 0 && row != SHEET_MAX_ROWS - 1 && looking_for_empty) {
            row -= dir;
        }
    }

    u32 final_row = (u32)CLAMP(row, 0, (i32)SHEET_MAX_ROWS - 1);

    if (final_row != win->cursor_pos.row) {
        if (dir < 0) {
            _editor_cursor_up(editor, wb, win->cursor_pos.row - final_row);
        } else {
            _editor_cursor_down(editor, wb, final_row - win->cursor_pos.row);
        }
    }
}

void _editor_cursor_block_move_horz(editor_context* editor, workbook* wb, i32 n) {
}

void _editor_scroll_up(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->scroll_pos.row, n);
    win->scroll_pos.row -= move_size;

    wb_win_update_num_rows(win);

    u32 pad = editor->settings.cursor_row_pad;
    u32 bottom_row = win->scroll_pos.row + win->num_rows - 1;
    u32 max_cursor_row = bottom_row >= pad ? bottom_row - pad : bottom_row;

    if (
        // This prevents unintended behavior when scrolling
        // past the bottom of the sheet bounds
        (win->cutoff_height || max_cursor_row < SHEET_MAX_ROWS - pad - 1) &&
        win->cursor_pos.row > max_cursor_row
    ) {
        win->cursor_pos.row = max_cursor_row;
    }
}

void _editor_scroll_down(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(SHEET_MAX_ROWS - 1 - win->scroll_pos.row, n);
    win->scroll_pos.row += move_size;

    u32 pad = editor->settings.cursor_row_pad;
    u32 min_cursor_row = win->scroll_pos.row < SHEET_MAX_ROWS - pad ?
        win->scroll_pos.row + pad : win->scroll_pos.row;

    if (win->cursor_pos.row < min_cursor_row) {
        win->cursor_pos.row = min_cursor_row;
    }
}

void _editor_scroll_left(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(win->scroll_pos.col, n);
    win->scroll_pos.col -= move_size;

    wb_win_update_num_cols(win);

    u32 pad = editor->settings.cursor_col_pad;
    u32 rightmost_col = win->scroll_pos.col + win->num_cols - 1;
    u32 max_cursor_col = rightmost_col >= pad ?
        rightmost_col - pad : rightmost_col;

    if (
        // This prevents unintended behavior when scrolling
        // past the right of the sheet bounds
        (win->cutoff_width || max_cursor_col < SHEET_MAX_COLS - pad - 1) &&
        win->cursor_pos.col > max_cursor_col
    ) {
        win->cursor_pos.col = max_cursor_col;
    }
}

void _editor_scroll_right(editor_context* editor, workbook* wb, u32 n) {
    sheet_window* win = wb->active_win;

    u32 move_size = MIN(SHEET_MAX_COLS - 1 - win->scroll_pos.col, n);
    win->scroll_pos.col += move_size;

    u32 pad = editor->settings.cursor_col_pad;
    u32 min_cursor_col = win->scroll_pos.col < SHEET_MAX_COLS - pad ? 
        win->scroll_pos.col + pad : win->scroll_pos.col;

    if (win->cursor_pos.col < min_cursor_col) {
        win->cursor_pos.col = min_cursor_col;
    }
}

void _editor_scroll_center(workbook* wb) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

    win->scroll_pos = win->cursor_pos;

    u32 cur_half_width = sheet_get_col_width(sheet, win->scroll_pos.col) / 2;
    u32 cur_half_height = sheet_get_row_height(sheet, win->scroll_pos.row) / 2;

    while (win->scroll_pos.col > 0 && cur_half_width < win->width / 2) {
        win->scroll_pos.col--;
        cur_half_width += sheet_get_col_width(sheet, win->scroll_pos.col);
    }

    while (win->scroll_pos.row > 0 && cur_half_height < win->height / 2) {
        win->scroll_pos.row--;
        cur_half_height += sheet_get_row_height(sheet, win->scroll_pos.row);
    }
}

void _editor_resize_width(workbook* wb, i32 change) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_win_get_sheet(wb, win, true);
    i32 width = (i32)sheet_get_col_width(sheet, win->cursor_pos.col);

    width += change;
    width = CLAMP(width, 1, SHEET_MAX_COL_WIDTH);

    sheet_set_col_width(sheet, win->cursor_pos.col, (u8)width);
}

void _editor_resize_height(workbook* wb, i32 change) {
    sheet_window* win = wb->active_win;
    sheet_buffer* sheet = wb_win_get_sheet(wb, win, true);
    i32 height = (i32)sheet_get_row_height(sheet, win->cursor_pos.row);

    height += change;
    height = CLAMP(height, 1, SHEET_MAX_ROW_HEIGHT);

    sheet_set_row_height(sheet, win->cursor_pos.row, (u8)height);
}

