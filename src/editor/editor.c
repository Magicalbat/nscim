
editor_context* editor_init(mem_arena* arena) {
    editor_context* editor = PUSH_STRUCT(arena, editor_context);

    editor->mode = EDITOR_MODE_NORMAL;
    editor->colors = (editor_colors) {
        .win_status_fg = { { 255, 255, 255 } },
        .win_status_bg = { {  25,  30,  40 } },
        .cell_fg       = { { 255, 255, 255 } },
        .cell_bg       = { {  15,  18,  20 } },
        .rc_fg         = { { 131,  27,  88 } },
        .rc_bg         = { { 214, 212, 243 } },
    };

    return editor;
}

#define EDITOR_STATUS_ROWS_TOP 0
#define EDITOR_STATUS_ROWS_BOTTOM 1
#define EDITOR_STATUS_ROWS (EDITOR_STATUS_ROWS_TOP + EDITOR_STATUS_ROWS_BOTTOM)

#define EDITOR_SHEET_NAME_PAD 2

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

    sheet_buffer* sheet = wb_win_get_sheet(wb, win, false);

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

    // Second status row
    {
        for (u32 i = 0; i < win->width; i++) {
            buf->tiles[i + win->start_x + y * buf->width] = status_tile;
        }

        y++;
    }

    if (y > max_y) { return; }

    u32 num_cols = 0;
    u32 num_rows = 0;

    u32 max_col_width = SHEET_DEF_COL_WIDTH;
    u32 max_row_height = SHEET_DEF_ROW_HEIGHT;

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

    {
        u32 x = SHEET_MAX_ROW_CHARS;

        u32 num_col_chars = 0;
        u8 col_chars[SHEET_MAX_COL_CHARS] = { 0 };

        for (u32 col_off = 0; col_off < num_cols; col_off++) {
            u32 col = col_off + win->scroll_pos.col;

            u32 width = sheet_get_col_width(sheet, col);

            num_col_chars = _editor_col_name(col, col_chars);

            u32 draw_start = num_col_chars < width ?
                width / 2 - num_col_chars / 2 : 0;

            for (u32 i = 0; i < width && x + i < buf->width; i++) {
                u32 index = x + i + y * buf->width;

                buf->tiles[index] = (win_tile){
                    .fg = editor->colors.rc_fg,
                    .bg = editor->colors.rc_bg,
                    .c = i >= draw_start && i < draw_start + num_col_chars ?
                        col_chars[i - draw_start] : ' '
                };
            }

            x += width;
        }
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

