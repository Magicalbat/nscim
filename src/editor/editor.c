
// Returns true if an input sequence is complete,
// false when it requires more input to finish
typedef b32 (_editor_do_mode_func)(editor_context*, workbook*, win_input, u32);

b32 _editor_do_normal(editor_context*, workbook*, win_input, u32);
b32 _editor_do_visual(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_edit(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_visual(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cell_insert(editor_context*, workbook*, win_input, u32);
b32 _editor_do_cmd(editor_context*, workbook*, win_input, u32);

void _editor_push_input_raw(
    editor_context* editor, workbook* wb, win_input input
);

void _editor_process_inputs_raw(editor_context* editor, workbook* wb);

b32 _editor_do_visual(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cell_edit(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cell_visual(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cell_insert(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cmd(editor_context*, workbook*, win_input, u32) {
    return false;
}

#include "editor_update.c"
#include "editor_draw.c"
#include "editor_input.c"

#include "modes/editor_normal.c"

editor_context* editor_init(mem_arena* arena) {
    editor_context* editor = PUSH_STRUCT(arena, editor_context);

    editor->mode = EDITOR_MODE_NORMAL;
    editor->colors = (editor_colors) {
        .win_status_fg      = { { 255, 255, 255 } },
        .win_status_bg      = { {  25,  30,  40 } },
        .cell_fg            = { { 255, 255, 255 } },
        .cell_bg            = { {  15,  18,  20 } },
        .inactive_cursor_fg = { {  15,  18,  20 } },
        .inactive_cursor_bg = { { 150, 150, 150 } },
        .rc_fg              = { { 131,  27,  88 } },
        .rc_bg              = { { 214, 212, 243 } },
    };

    return editor;
}

