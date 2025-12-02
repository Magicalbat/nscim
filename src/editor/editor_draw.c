
#define EDITOR_STATUS_ROWS_TOP 0
#define EDITOR_STATUS_ROWS_BOTTOM 1
#define EDITOR_STATUS_ROWS (EDITOR_STATUS_ROWS_TOP + EDITOR_STATUS_ROWS_BOTTOM)

#define EDITOR_STATUS_PAD 2

#define EDITOR_SHEET_NAME_PAD 2

void _editor_draw_sheet_win(
    window* user_win, editor_context* editor,
    workbook* wb, sheet_window* win
) {
    win_buffer* buf = &user_win->back_buf;

    u32 y = EDITOR_STATUS_ROWS_TOP + win->start_y;
    u32 max_y = y + win->height - 1;
    u32 max_x = win->start_x + win->width - 1;

    sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);
    b32 cur_active = win == wb->active_win && editor->mode != EDITOR_MODE_CMD;

    if (win->height == 0) {
        return;
    }

    win_tile status_tile = {
        .fg = editor->colors.status_fg,
        .bg = editor->colors.status_bg,
        .c = ' '
    };

    sheet_cell_view cur_cell = sheet_get_cell_view(wb, sheet, win->cursor_pos);

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

        u32 cell_pos_size = 3;
        u8 cell_pos_chars[3 + SHEET_MAX_COL_CHARS + SHEET_MAX_ROW_CHARS] =
            { ' ', '-', ' ' };

        cell_pos_size += sheets_pos_to_chars(
            win->cursor_pos, cell_pos_chars + cell_pos_size,
            sizeof(cell_pos_chars) - cell_pos_size
        );

        for (u32 i = 0; i < cell_pos_size; i++) {
            u64 x = (u64)i + EDITOR_SHEET_NAME_PAD + name.size;

            if (x >= win->width) { break; }

            x += win->start_x;
            buf->tiles[x + y * buf->width].c = cell_pos_chars[i];
        }

        string8 type_str = { 0 };

        switch ((sheet_cell_type_enum)cur_cell.type) {
            case SHEET_CELL_TYPE_EMPTY_CHUNK:
            case SHEET_CELL_TYPE_EMPTY: {
                type_str = STR8_LIT(" (Empty)");
            } break;

            case SHEET_CELL_TYPE_NUM: {
                type_str = STR8_LIT(" (Number)");
            } break;

            case SHEET_CELL_TYPE_STRING: {
                type_str = STR8_LIT(" (String)");
            } break;

            case _SHEET_CELL_TYPE_COUNT: break;
        }

        for (u32 i = 0; i < type_str.size; i++) {
            u64 x = (u64)i + EDITOR_SHEET_NAME_PAD + name.size + cell_pos_size;

            if (x >= win->width) { break; }

            x += win->start_x;
            buf->tiles[x + y * buf->width].c = type_str.str[i];
        }

        y++;
    }

    if (y > max_y) { return; }

    u32 cell_chars_size = 0;
    u8 cell_chars[SHEET_MAX_STRLEN] = { 0 };

    b32 cell_mode = (
        editor->mode == EDITOR_MODE_CELL_EDIT || 
        editor->mode == EDITOR_MODE_CELL_VISUAL || 
        editor->mode == EDITOR_MODE_CELL_INSERT
    );

    // Second status row
    {
        if (cur_active && cell_mode) {
            cell_chars_size = MIN(editor->cell_input_size, sizeof(cell_chars));
            memcpy(cell_chars, editor->cell_input_buf, cell_chars_size);

            user_win->cursor_row = win->start_y + 1;
            user_win->cursor_col = win->start_x + EDITOR_SHEET_NAME_PAD +
                editor->cell_input_cursor;
        } else {
            cell_chars_size = sheets_cell_to_chars(
                cur_cell, cell_chars, sizeof(cell_chars)
            );
        }

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

    u32 max_col_width = SHEET_DEF_COL_WIDTH;
    u32 max_row_height = SHEET_DEF_ROW_HEIGHT;

    for (u32 col = 0; col < win->num_cols; col++) {
        u32 cur_width = sheet_get_col_width(sheet, win->scroll_pos.col + col);

        if (cur_width > max_col_width) {
            max_col_width = cur_width;
        }
    }

    for (u32 row = 0; row < win->num_rows; row++) {
        u32 cur_height = sheet_get_row_height(sheet, win->scroll_pos.row + row);

        if (cur_height > max_row_height) {
            max_row_height = cur_height;
        }
    }

    u32 draw_select = cur_active && editor->mode == EDITOR_MODE_VISUAL;
    sheet_range select_range = sheets_fix_range(
        (sheet_range){ { win->select_start, win->cursor_pos } }
    );

    // Drawing column titles
    {
        u32 x = SHEET_MAX_ROW_CHARS + win->start_x;

        u32 num_col_chars = 0;
        u8 col_chars[SHEET_MAX_COL_CHARS] = { 0 };

        for (u32 col_off = 0; col_off < win->num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;
            u32 width = sheet_get_col_width(sheet, col);

            num_col_chars = sheets_col_to_chars(col, col_chars, sizeof(col_chars));

            u32 draw_start = num_col_chars < width ?
                width / 2 - num_col_chars / 2 : 0;

            win_col fg, bg;
            if (
                col == win->cursor_pos.col || (
                    draw_select && 
                    col >= select_range.start.col &&
                    col <= select_range.end.col
                )
            ) {
                fg = editor->colors.rc_bg;
                bg = editor->colors.rc_fg;
            } else {
                fg = editor->colors.rc_fg;
                bg = editor->colors.rc_bg;
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

        for (; x <= max_x; x++) {
            buf->tiles[x + y * buf->width] = status_tile;
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

        for (
            u32 row_off = 0;
            row_off < win->num_rows && y_tmp <= max_y;
            row_off++
        ) {
            u32 row = row_off + win->scroll_pos.row;
            u32 height = sheet_get_row_height(sheet, row);

            num_row_chars = chars_from_u32(row, row_chars, SHEET_MAX_ROW_CHARS);

            u32 draw_start = num_row_chars < max_row_chars ? max_row_chars - num_row_chars : 0;

            win_col fg, bg;
            if (
                row == win->cursor_pos.row || (
                    draw_select &&
                    row >= select_range.start.row &&
                    row <= select_range.end.row
                )
            ) {
                fg = editor->colors.rc_bg;
                bg = editor->colors.rc_fg;
            } else {
                fg = editor->colors.rc_fg;
                bg = editor->colors.rc_bg;
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

        for (; y_tmp <= max_y; y_tmp++) {
            for (u32 x = 0; x < max_row_chars; x++) {
                u32 idx = x + win->start_x + y_tmp * buf->width;
                buf->tiles[idx] = status_tile;
            }
        }
    }

    for (u32 row_off = 0; row_off < win->num_rows && y <= max_y; row_off++) {
        u32 row = row_off + win->scroll_pos.row;
        u32 height = sheet_get_row_height(sheet, row);

        u32 x = SHEET_MAX_ROW_CHARS + win->start_x;

        for (u32 col_off = 0; col_off < win->num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;
            u32 width = sheet_get_col_width(sheet, col);

            b32 in_cursor = col == win->cursor_pos.col &&
                row == win->cursor_pos.row;

            win_col fg, bg;

            switch ((cur_active << 1) | in_cursor) {
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

            if (
                !in_cursor && draw_select && 
                row >= select_range.start.row && 
                row <= select_range.end.row && 
                col >= select_range.start.col && 
                col <= select_range.end.col
            ) {
                fg = editor->colors.selection_fg;
                bg = editor->colors.selection_bg;
            }

            sheet_cell_view cell = sheet_get_cell_view(
                wb, sheet, (sheet_pos){ row, col }
            );
            cell_chars_size = sheets_cell_to_chars(
                cell, cell_chars, sizeof(cell_chars)
            );

            b32 too_big = cell_chars_size > width;

            for (u32 i = 0; i < height && i + y <= max_y; i++) {
                for (u32 j = 0; j < width && x + j <= max_x; j++) {
                    u8 c = i == 0 && j < cell_chars_size ? cell_chars[j] : ' ';
                    if (too_big) { c = '#'; }

                    buf->tiles[x + j + (y + i) * buf->width] = (win_tile) {
                        .fg = fg, .bg = bg, .c = c
                    };
                }
            }

            x += width;
        }

        y += height;
    }
}

static string8 _editor_mode_names[_EDITOR_MODE_COUNT] = {
    [EDITOR_MODE_NULL] = STR8_LIT("Null"),

    [EDITOR_MODE_NORMAL] = STR8_LIT("Normal"),
    [EDITOR_MODE_VISUAL] = STR8_LIT("Visual"),

    [EDITOR_MODE_CELL_EDIT] = STR8_LIT("Cell Edit"),
    [EDITOR_MODE_CELL_VISUAL] = STR8_LIT("Cell Visual"),
    [EDITOR_MODE_CELL_INSERT] = STR8_LIT("Cell Insert"),

    [EDITOR_MODE_CMD] = STR8_LIT("Command"),
};

void _editor_draw_status(window* user_win, editor_context* editor) {
    win_buffer* buf = &user_win->back_buf;

    win_tile status_tile = {
        .fg = editor->colors.status_fg,
        .bg = editor->colors.status_bg,
        .c = ' '
    };

    u32 status_offset = (buf->height - 1) * buf->width;
    for (u32 x = 0; x < buf->width; x++) {
        buf->tiles[x + status_offset] = status_tile;
    }

    if (editor->mode == EDITOR_MODE_CMD) {
    } else {
        string8 mode_str = _editor_mode_names[editor->mode];

        for (
            u32 i = 0; i + EDITOR_STATUS_PAD < buf->width &&
            i < mode_str.size; i++
        ) {
            buf->tiles[i + EDITOR_STATUS_PAD + status_offset].c = mode_str.str[i];
        }

        u32 input_draw_offset = EDITOR_INPUT_SEQ_MAX + EDITOR_STATUS_PAD;
        u32 input_draw_start = buf->width > input_draw_offset ?
            buf->width - input_draw_offset : 0;

        u32 input_idx = editor->action_start_input;

        for (
            u32 x = input_draw_start; x < buf->width &&
            input_idx != editor->input_queue_end; x++
        ) {
            win_input input = editor->input_queue[input_idx];

            if (input < '\x1b') {
                buf->tiles[x + status_offset].c = '^';
                if (++x < buf->width) {
                    buf->tiles[x + status_offset].c = 'A' - 1 + input;
                }
            } else {
                buf->tiles[x + status_offset].c = input;
            }

            input_idx++;
            input_idx %= EDITOR_INPUT_QUEUE_MAX;
        }
    }
}

void editor_draw(window* user_win, editor_context* editor, workbook* wb) {
    win_buffer* buf = &user_win->back_buf;

    if (buf->height < EDITOR_STATUS_ROWS) {
        return;
    }
    
    switch (editor->mode) {
        case EDITOR_MODE_NORMAL:
        case EDITOR_MODE_VISUAL: {
            user_win->cursor_mode = WIN_CURSOR_MODE_HIDDEN;
        } break;

        case EDITOR_MODE_CELL_EDIT:
        case EDITOR_MODE_CELL_VISUAL: {
            user_win->cursor_mode = WIN_CURSOR_MODE_BLOCK_STEADY;
        } break;

        case EDITOR_MODE_CELL_INSERT:
        case EDITOR_MODE_CMD: {
            user_win->cursor_mode = WIN_CURSOR_MODE_BAR_STEADY;
        } break;

        default: {
            user_win->cursor_mode = WIN_CURSOR_MODE_HIDDEN;
        } break;
    }

    wb_win_compute_sizes(wb, buf->width, buf->height - EDITOR_STATUS_ROWS);

    _editor_draw_status(user_win, editor);
    
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 stack_size = 0;
    sheet_window** stack = PUSH_ARRAY(scratch.arena, sheet_window*, wb->num_windows);
    stack[stack_size++] = wb->root_win;

    while (stack_size) {
        sheet_window* cur = stack[--stack_size];

        if (cur->internal) {
            if (cur->child0 != NULL) stack[stack_size++] = cur->child0;
            if (cur->child1 != NULL) stack[stack_size++] = cur->child1;
        } else {
            _editor_draw_sheet_win(user_win, editor, wb, cur);
        }
    }

    arena_scratch_release(scratch);
}

