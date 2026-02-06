
#define EDITOR_STATUS_ROWS_TOP 0
#define EDITOR_STATUS_ROWS_BOTTOM 1
#define EDITOR_STATUS_ROWS (EDITOR_STATUS_ROWS_TOP + EDITOR_STATUS_ROWS_BOTTOM)

#define EDITOR_STATUS_PAD 2

#define EDITOR_SHEET_NAME_PAD 2

void _editor_draw_sheet_win(
    window* user_win, editor_context* editor,
    workbook* wb, editor_window* win
) {
    win_buffer* buf = &user_win->buffer;

    u32 start_x = MIN((u32)win->anim_start_x, buf->width - 1);
    u32 start_y = MIN((u32)win->anim_start_y, buf->height - 1);
    u32 win_width = MIN((u32)win->anim_width, buf->width - start_x);
    u32 win_height = MIN((u32)win->anim_height, buf->height - start_y);

    u32 y = EDITOR_STATUS_ROWS_TOP + start_y;
    u32 max_y = y + win_height - 1;
    u32 max_x = start_x + win_width - 1;

    sheet_buffer* sheet = editor_win_get_sheet(editor, wb, win, false);
    b32 cur_active =
        win == editor->active_win &&
        editor->mode != EDITOR_MODE_CMD;

    if (win_height == 0 || win_width == 0) { return; }

    sheet_cell_view cur_cell = sheet_get_cell_view(wb, sheet, win->cursor_pos);

    // First status row
    {
        for (u32 i = 0; i < win_width; i++) {
            u32 index = i + start_x + y * buf->width;

            buf->fg_cols[index] = editor->colors.status_fg;
            buf->bg_cols[index] = editor->colors.status_bg;
        }

        string8 name = sheet->name.size == 0 ?
            STR8_LIT("[No Name]") : sheet->name;

        for (u64 i = 0; i < name.size; i++) {
            u64 x = i + EDITOR_SHEET_NAME_PAD;

            if (x >= win_width) { break; }

            x += start_x;
            buf->chars[x + y * buf->width] = name.str[i];
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

            if (x >= win_width) { break; }

            x += start_x;
            buf->chars[x + y * buf->width] = cell_pos_chars[i];
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

            if (x >= win_width) { break; }

            x += start_x;
            buf->chars[x + y * buf->width] = type_str.str[i];
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

            user_win->cursor_row = start_y + 1;
            user_win->cursor_col = start_x + EDITOR_SHEET_NAME_PAD +
                editor->cell_input_cursor;
        } else {
            cell_chars_size = sheets_cell_to_chars(
                cur_cell, cell_chars, sizeof(cell_chars)
            );
        }

        for (u32 i = 0; i < win_width; i++) {
            u32 index = i + start_x + y * buf->width;

            buf->fg_cols[index] = editor->colors.status_fg;
            buf->bg_cols[index] = editor->colors.status_bg;

            if (
                i >= EDITOR_SHEET_NAME_PAD &&
                i < cell_chars_size + EDITOR_SHEET_NAME_PAD
            ) {
                buf->chars[index] = cell_chars[i-EDITOR_SHEET_NAME_PAD];
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
        u32 x = SHEET_MAX_ROW_CHARS + start_x;

        u32 num_col_chars = 0;
        u8 col_chars[SHEET_MAX_COL_CHARS] = { 0 };

        for (u32 col_off = 0; col_off < win->num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;
            u32 width = sheet_get_col_width(sheet, col);

            num_col_chars = sheets_col_to_chars(
                col, col_chars, sizeof(col_chars)
            );

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
                u32 index = x + i + y * buf->width;

                buf->fg_cols[index] = fg;
                buf->bg_cols[index] = bg;

                if (i >= draw_start && i < draw_start + num_col_chars) {
                    buf->chars[index] = col_chars[i - draw_start];
                }
            }

            x += width;
        }

        for (; x <= max_x; x++) {
            u32 index = x + y * buf->width;

            buf->fg_cols[index] = editor->colors.status_fg;
            buf->bg_cols[index] = editor->colors.status_bg;
        }

        y++;
    }

    if (y > max_y) { return; }

    // Drawing row numbers
    {
        u32 y_tmp = y;

        u32 num_row_chars = 0;
        u8 row_chars[SHEET_MAX_ROW_CHARS] = { 0 };

        u32 max_row_chars = MIN(SHEET_MAX_ROW_CHARS, win_width);

        for (
            u32 row_off = 0;
            row_off < win->num_rows && y_tmp <= max_y;
            row_off++
        ) {
            u32 row = row_off + win->scroll_pos.row;
            u32 height = sheet_get_row_height(sheet, row);

            num_row_chars = chars_from_u32(row, row_chars, SHEET_MAX_ROW_CHARS);

            u32 draw_start = num_row_chars < max_row_chars ?
                max_row_chars - num_row_chars : 0;

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
                    u32 index = j + start_x + y_tmp * buf->width;

                    buf->fg_cols[index] = fg;
                    buf->bg_cols[index] = bg;

                    if (i == 0 && j >= draw_start) {
                        buf->chars[index] = row_chars[j - draw_start];
                    }
                }
            }
        }

        for (; y_tmp <= max_y; y_tmp++) {
            for (u32 x = 0; x < max_row_chars; x++) {
                u32 index = x + start_x + y_tmp * buf->width;

                buf->fg_cols[index] = editor->colors.status_fg;
                buf->bg_cols[index] = editor->colors.status_bg;
            }
        }
    }

    for (u32 row_off = 0; row_off < win->num_rows && y <= max_y; row_off++) {
        u32 row = row_off + win->scroll_pos.row;
        u32 height = sheet_get_row_height(sheet, row);

        u32 x = SHEET_MAX_ROW_CHARS + start_x;

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

                    u32 index = x + j + (y + i) * buf->width;

                    buf->fg_cols[index] = fg;
                    buf->bg_cols[index] = bg;
                    buf->chars[index] = c;
                }
            }

            x += width;
        }

        y += height;
    }
}

static string8 _editor_mode_names[_EDITOR_MODE_COUNT] = {
    [EDITOR_MODE_NULL] = STR8_CONST_LIT("Null"),

    [EDITOR_MODE_NORMAL] = STR8_CONST_LIT("Normal"),
    [EDITOR_MODE_VISUAL] = STR8_CONST_LIT("Visual"),

    [EDITOR_MODE_CELL_EDIT] = STR8_CONST_LIT("Cell Edit"),
    [EDITOR_MODE_CELL_VISUAL] = STR8_CONST_LIT("Cell Visual"),
    [EDITOR_MODE_CELL_INSERT] = STR8_CONST_LIT("Cell Insert"),

    [EDITOR_MODE_CMD] = STR8_CONST_LIT("Command"),
};

win_col _editor_output_line_color(editor_context* editor, string8 line) {
    win_col col = editor->colors.status_fg;

    string8 err_str = STR8_LIT("Error");
    string8 warn_str = STR8_LIT("Warning");
    string8 info_str = STR8_LIT("Info");

    if (str8_start_equals(line, err_str)) {
        col = editor->colors.error_fg;
    } else if (str8_start_equals(line, warn_str)) {
        col = editor->colors.warning_fg;
    } else if (str8_start_equals(line, info_str)) {
        col = editor->colors.info_fg;
    }

    return col;
}

// Draws status when `editor->output` has only one line
void _editor_draw_typical_status(window* user_win, editor_context* editor) {
    win_buffer* buf = &user_win->buffer;

    u32 status_offset = (buf->height - 1) * buf->width;
    for (u32 x = 0; x < buf->width; x++) {
        buf->fg_cols[x + status_offset] = editor->colors.status_fg;
        buf->bg_cols[x + status_offset] = editor->colors.status_bg;
    }

    if (editor->mode == EDITOR_MODE_CMD) {
        u8 draw_chars[EDITOR_CMD_MAX_STRLEN + 1] = { ':' };
        u32 draw_size = editor->cmd_size + 1;
        memcpy(draw_chars + 1, editor->cmd_buf, editor->cmd_size);

        for ( u32 i = 0; i < buf->width && i < draw_size; i++) {
            buf->chars[i + status_offset] = draw_chars[i];
        }

        user_win->cursor_row = buf->height - 1;
        user_win->cursor_col = MIN(buf->width - 1, editor->cmd_cursor + 1);
    } else {
        string8 mode_str = _editor_mode_names[editor->mode];

        u32 mode_draw_offset = EDITOR_STATUS_PAD + (u32)mode_str.size;
        u32 mode_draw_start = buf->width > mode_draw_offset ? 
            buf->width - mode_draw_offset : 0;

        for (
            u32 i = 0; i + mode_draw_start < buf->width &&
            i < mode_str.size; i++
        ) {
            buf->chars[i + mode_draw_start + status_offset] = mode_str.str[i];
        }

        u32 input_draw_offset = EDITOR_INPUT_SEQ_MAX + EDITOR_STATUS_PAD;
        u32 input_draw_start = buf->width > input_draw_offset ?
            buf->width - input_draw_offset : 0;

        u32 input_idx = editor->action_start_input;

        for (
            u32 x = input_draw_start; x < mode_draw_start &&
            input_idx != editor->input_queue_end; x++
        ) {
            win_input input = editor->input_queue[input_idx];

            if (input < '\x1b') {
                buf->chars[x + status_offset] = '^';

                if (++x < buf->width) {
                    buf->chars[x + status_offset] = 'A' - 1 + input;
                }
            } else {
                buf->chars[x + status_offset] = input;
            }

            input_idx++;
            input_idx %= EDITOR_INPUT_QUEUE_MAX;
        }

        win_col output_fg = _editor_output_line_color(editor, editor->output);
        u32 output_start = EDITOR_STATUS_PAD;
        for (
            u32 i = 0;
            i + output_start < input_draw_start &&
            i < editor->output.size; i++
        ) {
            u32 index = i + output_start + status_offset;
            buf->fg_cols[index] = output_fg;
            buf->chars[index] = editor->output.str[i];
        }
    }
}

void _editor_draw_output_status(window* user_win, editor_context* editor) {
    win_buffer* buf = &user_win->buffer;

    string8 output = editor->output;

    // One line for the initial (before any new line)
    // One line for the "Enter any key yada yada" message at the bottom
    u32 num_lines = 2;
    for (u64 i = 0; i < output.size; i++) {
        if (output.str[i] == '\n') {
            num_lines++;
        }
    }

    u32 y_start = num_lines < buf->height ? buf->height - num_lines : 0;

    for (u32 y = y_start; y < buf->height; y++) {
        for (u32 x = 0; x < buf->width; x++) {
            u32 index = x + y * buf->width;

            buf->bg_cols[index] = editor->colors.status_bg;
            buf->fg_cols[index] = editor->colors.status_fg;
            buf->chars[index] = ' ';
        }
    }

    u32 num_draw_lines = buf->height - y_start;
    u32 lines_offset = num_lines - num_draw_lines;

    for (u32 i = 0; output.size > 0 && i < lines_offset; i++) {
        u64 newline_index = str8_find_first(output, '\n');
        output = str8_substr(output, newline_index + 1, output.size);
    }

    for (
        u32 i = lines_offset;
        output.size > 0 && (i32)i < (i32)num_lines - 1;
        i++
    ) {
        u64 newline_index = str8_find_first(output, '\n');
        string8 line = str8_substr(output, 0, newline_index);
        output = str8_substr(output, newline_index + 1, output.size);

        win_col line_col = _editor_output_line_color(editor, line);
        u32 to_draw = MIN((u32)line.size, buf->width - EDITOR_STATUS_PAD);

        for (u32 j = 0; j < to_draw; j++) {
            u32 index = j + EDITOR_STATUS_PAD +
                (y_start + i - lines_offset) * buf->width;

            buf->fg_cols[index] = line_col;
            buf->chars[index] = line.str[j];
        }
    }

    if (num_lines > 0) {
        string8 continue_msg = STR8_LIT("Press any key to continue");

        u32 to_draw = MIN(
            (u32)continue_msg.size,
            buf->width - EDITOR_STATUS_PAD
        );

        for (u32 i = 0; i < to_draw; i++) {
            u32 index = (i + EDITOR_STATUS_PAD) + 
                (y_start + num_lines - 1) * buf->width;

            buf->chars[index] = continue_msg.str[i];
        }
    }
}

void _editor_draw_status(window* user_win, editor_context* editor) {
    string8 output = editor->output;

    if (str8_find_first(output, '\n') == output.size) {
        // There is only one line of the output
        _editor_draw_typical_status(user_win, editor);
    } else {
        // There are multiple output lines
        _editor_draw_output_status(user_win, editor);
    }

}

void editor_draw(window* user_win, editor_context* editor, workbook* wb) {
    win_buffer* buf = &user_win->buffer;

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

    editor_win_compute_sizes(
        editor, buf->width, buf->height - EDITOR_STATUS_ROWS
    );
    
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 stack_size = 0;
    editor_window** stack = PUSH_ARRAY(
        scratch.arena, editor_window*, editor->num_windows
    );
    stack[stack_size++] = editor->root_win;

    while (stack_size) {
        editor_window* cur = stack[--stack_size];

        if (cur->internal) {
            if (cur->child0 != NULL) stack[stack_size++] = cur->child0;
            if (cur->child1 != NULL) stack[stack_size++] = cur->child1;
        } else {
            _editor_draw_sheet_win(user_win, editor, wb, cur);
        }
    }

    _editor_draw_status(user_win, editor);

    arena_scratch_release(scratch);
}

