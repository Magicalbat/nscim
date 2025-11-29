
editor_context* editor_init(mem_arena* arena) {
    editor_context* editor = PUSH_STRUCT(arena, editor_context);

    editor->mode = EDITOR_MODE_NORMAL;

    editor->count = 1;

    editor->colors = (editor_colors) {
        .status_fg          = { { 255, 255, 255 } },
        .status_bg          = { {  25,  30,  40 } },
        .cell_fg            = { { 255, 255, 255 } },
        .cell_bg            = { {  15,  18,  20 } },
        .inactive_cursor_fg = { {  15,  18,  20 } },
        .inactive_cursor_bg = { { 150, 150, 150 } },
        .rc_fg              = { { 131,  27,  88 } },
        .rc_bg              = { { 214, 212, 243 } },
    };

    editor->settings = (editor_settings) {
        .cursor_row_pad = 2,
        .cursor_col_pad = 1
    };

    return editor;
}
