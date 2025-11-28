

void _editor_push_input_raw(
    editor_context* editor, workbook* wb, win_input input
);

void _editor_process_inputs_raw(editor_context* editor, workbook* wb);

b32 _editor_do_visual(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cell_visual(editor_context*, workbook*, win_input, u32) {
    return false;
}
b32 _editor_do_cmd(editor_context*, workbook*, win_input, u32) {
    return false;
}

#include "modes/editor_modes_internal.h"

#include "editor_init.c"
#include "editor_update.c"
#include "editor_draw.c"
#include "editor_input.c"

#include "modes/editor_modes_sheets_common.c"
#include "modes/editor_modes_cell_common.c"
#include "modes/editor_modes_normal.c"
#include "modes/editor_modes_cell_insert.c"
#include "modes/editor_modes_cell_edit.c"

