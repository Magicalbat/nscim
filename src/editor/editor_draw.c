#define EDITOR_STATUS_ROWS_TOP 0
#define EDITOR_STATUS_ROWS_BOTTOM 1
#define EDITOR_STATUS_ROWS (EDITOR_STATUS_ROWS_TOP + EDITOR_STATUS_ROWS_BOTTOM)

#define EDITOR_SHEET_NAME_PAD 2

u32 _editor_get_cell_str(workbook* wb, sheet_buffer* sheet, sheet_cell_pos cell_pos, u8 chars[SHEET_MAX_STRLEN]) {
    sheet_cell_ref cell = sheet_get_cell(wb, sheet, cell_pos, false);

    switch (*cell.type) {
        case SHEET_CELL_TYPE_NUM: {
        } break;
        case SHEET_CELL_TYPE_STRING: {
            sheet_string* str = *cell.str;
            memcpy(chars, str->str, str->size);
            return str->size;
        } break;

        default: {
            return 0;
        } break;
    }

    return 0;
}

u32 _editor_col_name(u32 col, u8 chars[SHEET_MAX_COL_CHARS]) {
    u32 size = 0;

    for (u32 i = 0; i < SHEET_MAX_COL_CHARS; i++) {
        chars[size++] = (col % 26) + 'A';
        col /= 26;

        if (col == 0) {
            break;
        } else {
            col--;
        }
    }

    for (u32 i = 0; i < size/2; i++) {
        u8 tmp = chars[size-i-1];
        chars[size-i-1] = chars[i];
        chars[i] = tmp;
    }

    return size;
}

void _editor_draw_sheet_win(
    win_buffer* buf, editor_context* editor,
    workbook* wb, sheet_window* win
) {
    u32 y = EDITOR_STATUS_ROWS_TOP + win->start_y;
    u32 max_y = y + win->height - 1;
    u32 max_x = win->start_x + win->width - 1;

    sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);
    b32 active = win == wb->active_win;

    if (win->height == 0) {
        return;
    }

    win_tile status_tile = {
        .fg = editor->colors.win_status_fg,
        .bg = editor->colors.win_status_bg,
        .c = ' '
    };

    // First status row
    {
        for (u32 i = 0; i < win->width; i++) {
            buf->tiles[i + win->start_x + y * buf->width] = status_tile;
        }

        string8 name = sheet->name.size == 0 ?
            STR8_LIT("[No Name]") : sheet->name;

        for (u64 i = 0; i < name.size; i++) {
            u64 x = i + EDITOR_SHEET_NAME_PAD;

            if (x >= win->width) { break; }

            x += win->start_x;
            buf->tiles[x + y * buf->width].c = name.str[i];
        }

        #define _CELL_POS_STR_SIZE (3 + SHEET_MAX_COL_CHARS + SHEET_MAX_ROW_CHARS)

        u32 cell_pos_size = 3;
        u8 cell_pos_chars[_CELL_POS_STR_SIZE] = { ' ', '-', ' ' };

        cell_pos_size += _editor_col_name(
            win->cursor_pos.col, cell_pos_chars + cell_pos_size
        );
        cell_pos_size += chars_from_u32(
            win->cursor_pos.row, cell_pos_chars + cell_pos_size,
            _CELL_POS_STR_SIZE - cell_pos_size
        );

        for (u32 i = 0; i < cell_pos_size; i++) {
            u64 x = (u64)i + EDITOR_SHEET_NAME_PAD + name.size;

            if (x >= win->width) { break; }

            x += win->start_x;
            buf->tiles[x + y * buf->width].c = cell_pos_chars[i];
        }

        y++;
    }

    if (y > max_y) { return; }

    u32 cell_chars_size = 0;
    u8 cell_chars[SHEET_MAX_STRLEN] = { 0 };

    // Second status row
    {
        cell_chars_size = _editor_get_cell_str(
            wb, sheet, win->cursor_pos, cell_chars
        );

        for (u32 i = 0; i < win->width; i++) {
            u32 index = i + win->start_x + y * buf->width;
            buf->tiles[index] = status_tile;

            if (
                i >= EDITOR_SHEET_NAME_PAD &&
                i < cell_chars_size + EDITOR_SHEET_NAME_PAD
            ) {
                buf->tiles[index].c = cell_chars[i-EDITOR_SHEET_NAME_PAD];
            }
        }

        y++;
    }

    if (y > max_y) { return; }

    u32 num_cols = 0;
    u32 num_rows = 0;

    u32 max_col_width = SHEET_DEF_COL_WIDTH;
    u32 max_row_height = SHEET_DEF_ROW_HEIGHT;

    // Getting row/column bounds
    {
        u32 cur_col_width = SHEET_DEF_COL_WIDTH;
        u32 cur_row_height = SHEET_DEF_ROW_HEIGHT;

        for (
            u32 x = SHEET_MAX_ROW_CHARS;
            x < win->width;
            x += cur_col_width, num_cols++
        ) {
            cur_col_width = sheet_get_col_width(
                sheet, win->scroll_pos.col + num_cols
            );

            if (cur_col_width > max_col_width) {
                max_col_width = cur_col_width;
            }
        }

        for (
            u32 yp = y + 1;
            yp <= max_y;
            yp += cur_row_height, num_rows++
        ) {
            cur_row_height = sheet_get_row_height(
                sheet, win->scroll_pos.row + num_rows
            );

            if (cur_row_height > max_row_height) {
                max_row_height = cur_row_height;
            }
        }
    }

    // Drawing column titles
    {
        u32 x = SHEET_MAX_ROW_CHARS + win->start_x;

        u32 num_col_chars = 0;
        u8 col_chars[SHEET_MAX_COL_CHARS] = { 0 };

        for (u32 col_off = 0; col_off < num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;
            u32 width = sheet_get_col_width(sheet, col);

            num_col_chars = _editor_col_name(col, col_chars);

            u32 draw_start = num_col_chars < width ?
                width / 2 - num_col_chars / 2 : 0;

            win_col fg, bg;
            if (col != win->cursor_pos.col) {
                fg = editor->colors.rc_fg;
                bg = editor->colors.rc_bg;
            } else {
                fg = editor->colors.rc_bg;
                bg = editor->colors.rc_fg;
            }

            for (u32 i = 0; i < width && x + i <= max_x; i++) {
                buf->tiles[x + i + y * buf->width] = (win_tile){
                    .fg = fg, .bg = bg,
                    .c = i >= draw_start && i < draw_start + num_col_chars ?
                        col_chars[i - draw_start] : ' '
                };
            }

            x += width;
        }

        y++;
    }

    if (y > max_y) { return; }

    // Drawing row numbers
    {
        u32 y_tmp = y;

        u32 num_row_chars = 0;
        u8 row_chars[SHEET_MAX_ROW_CHARS] = { 0 };

        u32 max_row_chars = MIN(SHEET_MAX_ROW_CHARS, win->width);

        for (u32 row_off = 0; row_off < num_rows && y_tmp <= max_y; row_off++) {
            u32 row = row_off + win->scroll_pos.row;
            u32 height = sheet_get_row_height(sheet, row);

            num_row_chars = chars_from_u32(row, row_chars, SHEET_MAX_ROW_CHARS);

            u32 draw_start = num_row_chars < max_row_chars ? max_row_chars - num_row_chars : 0;

            win_col fg, bg;
            if (row != win->cursor_pos.row) {
                fg = editor->colors.rc_fg;
                bg = editor->colors.rc_bg;
            } else {
                fg = editor->colors.rc_bg;
                bg = editor->colors.rc_fg;
            }

            for (u32 i = 0; i < height && y_tmp <= max_y; i++, y_tmp++) {
                for (u32 j = 0; j < max_row_chars; j++) {
                    buf->tiles[j + win->start_x + y_tmp * buf->width] =
                    (win_tile){
                        .fg = fg, .bg = bg,
                        .c = i == 0 && j >= draw_start ?
                            row_chars[j - draw_start] : ' '
                    };
                }
            }
        }
    }

    for (u32 row_off = 0; row_off < num_rows && y <= max_y; row_off++) {
        u32 row = row_off + win->scroll_pos.row;
        u32 height = sheet_get_row_height(sheet, row);

        u32 x = SHEET_MAX_ROW_CHARS + win->start_x;

        for (u32 col_off = 0; col_off < num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;
            u32 width = sheet_get_col_width(sheet, col);


            b32 in_cursor = col == win->cursor_pos.col &&
                row == win->cursor_pos.row;

            win_col fg, bg;

            switch ((active << 1) | in_cursor) {
                case 0b00: 
                case 0b10: {
                    fg = editor->colors.cell_fg;
                    bg = editor->colors.cell_bg;
                } break;

                case 0b11: {
                    fg = editor->colors.cell_bg;
                    bg = editor->colors.cell_fg;
                } break;

                case 0b01: {
                    fg = editor->colors.inactive_cursor_fg;
                    bg = editor->colors.inactive_cursor_bg;
                } break;
            }

            cell_chars_size = _editor_get_cell_str(
                wb, sheet, (sheet_cell_pos){ row, col }, cell_chars
            );

            for (u32 i = 0; i < height && i + y <= max_y; i++) {
                for (u32 j = 0; j < width && x + j <= max_x; j++) {
                    buf->tiles[x + j + (y + i) * buf->width] = (win_tile) {
                        .fg = fg, .bg = bg,
                        .c = i == 0 && j < cell_chars_size ? cell_chars[j] : ' '
                    };
                }
            }

            x += width;
        }

        y += height;
    }
}

void editor_draw(window* win, editor_context* editor, workbook* wb) {
    win_buffer* buf = &win->back_buf;

    if (buf->height < EDITOR_STATUS_ROWS) {
        return;
    }

    // TODO: draw status rows

    wb_win_compute_sizes(wb, buf->width, buf->height - EDITOR_STATUS_ROWS);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 stack_size = 0;
    sheet_window** stack = PUSH_ARRAY(scratch.arena, sheet_window*, wb->num_windows);
    stack[stack_size++] = wb->root_win;

    while (stack_size) {
        sheet_window* cur = stack[--stack_size];

        if (cur->internal) {
            if (cur->child0 != NULL)
                stack[stack_size++] = cur->child0;

            if (cur->child1 != NULL)
                stack[stack_size++] = cur->child1;
        } else {
            _editor_draw_sheet_win(buf, editor, wb, cur);
        }
    }

    arena_scratch_release(scratch);
}

