
// Returns true if an input sequence is complete,
// false when it requires more input to finish
typedef b32 (_editor_do_mode_func)(editor_context*, workbook*, win_input, u32);

b32 _editor_do_normal(editor_context*, workbook*, win_input, u32);
b32 _editor_do_visual(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_edit(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_visual(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_insert(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cmd(editor_context*, workbook*, win_input, u32);

// These functions perform bounds checking and make sure that
// the cursor remains within the window's scroll range
void _editor_cursor_up(editor_context* editor, workbook* wb, u32 n);
void _editor_cursor_down(editor_context* editor, workbook* wb, u32 n);
void _editor_cursor_left(editor_context* editor, workbook* wb, u32 n);
void _editor_cursor_right(editor_context* editor, workbook* wb, u32 n);

// These functions act like CTRL+Arrow in Excel
// i.e. When in a cell with contents, it will move
// until just before an empty cell. When in an empty
// cell, it will move until a non-empty cell
void _editor_move_block_vert(editor_context* editor, workbook* wb, i32 diff);
void _editor_move_block_horz(editor_context* editor, workbook* wb, i32 diff);

// Moves the cursor until it has cleared some multiple of
// the window's width or height
void _editor_move_win_multiple_vert(
    editor_context* editor, workbook* wb, f32 multiple
);
void _editor_move_win_multiple_horz(
    editor_context* editor, workbook* wb, f32 multiple
);

// These functions perform bounds checking and make sure that
// the cursor remains on screen when scrolling
void _editor_scroll_up(editor_context* editor, workbook* wb, u32 n);
void _editor_scroll_down(editor_context* editor, workbook* wb, u32 n);
void _editor_scroll_left(editor_context* editor, workbook* wb, u32 n);
void _editor_scroll_right(editor_context* editor, workbook* wb, u32 n);

// Centers the scroll at the cursor
void _editor_scroll_center(workbook* wb);

void _editor_resize_width(workbook* wb, i32 change);
void _editor_resize_height(workbook* wb, i32 change);


