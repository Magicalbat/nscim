
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

// These functions perform bounds checking and make sure that
// the cursor remains within the window's scroll range
void _editor_cursor_up(workbook* wb, u32 n);
void _editor_cursor_down(workbook* wb, u32 n);
void _editor_cursor_left(workbook* wb, u32 n);
void _editor_cursor_right(workbook* wb, u32 n);

#include "editor_init.c"
#include "editor_update.c"
#include "editor_draw.c"
#include "editor_input.c"

#include "modes/editor_modes_common.c"
#include "modes/editor_normal.c"

