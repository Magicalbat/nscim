
typedef struct editor_context editor_context;

#include "editor_window.h"
#include "editor_register.h"
#include "editor_context.h"

editor_context* editor_create(void);
void editor_destroy(editor_context* editor);

sheet_buffer* editor_get_active_sheet(
    editor_context* editor, workbook* wb, b32 create_if_empty
);

void editor_push_inputs(
    editor_context* editor, workbook* wb,
    win_input* inputs, u32 num_inputs
);

void editor_update(
    window* win, editor_context* editor,
    workbook* wb, f32 delta
);

void editor_draw(window* win, editor_context* editor, workbook* wb);

