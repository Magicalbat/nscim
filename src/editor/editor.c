
editor_window* _editor_create_win(editor_context* editor);
void _editor_free_win(editor_context* editor, editor_window* win);

void _editor_push_input_raw(
    editor_context* editor, workbook* wb, win_input input
);

void _editor_process_inputs_raw(editor_context* editor, workbook* wb);

b32 _editor_do_cell_visual(editor_context*, workbook*, win_input, u32) {
    return false;
}

#include "modes/editor_modes_internal.h"

#include "editor_window.c"
#include "editor_register.c"

#include "editor_update.c"
#include "editor_draw.c"
#include "editor_input.c"

#include "modes/editor_modes_common.c"
#include "modes/editor_modes_sheets_common.c"
#include "modes/editor_modes_cell_common.c"

#include "modes/editor_modes_normal.c"
#include "modes/editor_modes_visual.c"
#include "modes/editor_modes_cell_insert.c"
#include "modes/editor_modes_cell_edit.c"
#include "modes/editor_modes_cmd.c"

editor_context* editor_create(void) {
    mem_arena* arena = arena_create(
        EDITOR_ARENA_RESERVE_SIZE,
        EDITOR_ARENA_COMMIT_SIZE,
        0
    );

    editor_context* editor = PUSH_STRUCT(arena, editor_context);
    editor->arena = arena;

    editor->mode = EDITOR_MODE_NORMAL;

    editor->colors = (editor_colors) {
        .status_fg          = { { 255, 255, 255 } },
        .status_bg          = { {  25,  30,  40 } },
        .cell_fg            = { { 255, 255, 255 } },
        .cell_bg            = { {  15,  18,  20 } },
        .inactive_cursor_fg = { {  15,  18,  20 } },
        .inactive_cursor_bg = { { 150, 150, 150 } },
        .selection_fg       = { {  15,  18,  20 } },
        .selection_bg       = { { 200, 200, 200 } },
        .rc_fg              = { { 131,  27,  88 } },
        .rc_bg              = { { 214, 212, 243 } },
    };

    editor->settings = (editor_settings) {
        .cursor_row_pad = 2,
        .cursor_col_pad = 1,
        .anim_speed = 25.0f,
    };

    editor->empty_sheet = PUSH_STRUCT(arena, sheet_buffer);

    editor_window* root_win = PUSH_STRUCT(arena, editor_window);
    root_win->_sheet = editor->empty_sheet;

    editor->root_win = root_win;
    editor->active_win = root_win;

    editor->count = 1;

    editor->test_buf_a = PUSH_ARRAY(arena, f64, SHEET_MAX_ROWS);
    editor->test_buf_b = PUSH_ARRAY(arena, f64, SHEET_MAX_ROWS);

    for (u32 i = 0; i < SHEET_MAX_ROWS; i++) {
        editor->test_buf_a[i] = (f64)i;
    }

    editor->selected_register = EDITOR_REGISTER_DEFAULT;
    for (u32 i = 0; i < EDITOR_REGISTER_COUNT; i++) {
        u8 c = (u8)(i + EDITOR_REGISTER_FIRST); 

        editor_register_type reg_type = EDITOR_REGISTER_TYPE_INVALID;
        
        if (
            (c == '"') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <+ '9')
        ) {
            reg_type = EDITOR_REGISTER_TYPE_EMPTY;
        }

        if (c == '_') {
            reg_type = EDITOR_REGISTER_TYPE_BLACKHOLE;
        }

        editor_register* reg = NULL;

        if (reg_type != EDITOR_REGISTER_TYPE_INVALID) {
            reg = PUSH_STRUCT(arena, editor_register);
            editor_reg_create(reg, reg_type, true);
        }

        editor->registers[i] = reg;
    }

    return editor;
}

void editor_destroy(editor_context* editor) {
    for (u32 i = 0; i < EDITOR_REGISTER_COUNT; i++) {
        if (editor->registers[i] != NULL) {
            editor_reg_destroy(editor->registers[i]);
        }
    }

    arena_destroy(editor->arena);
}

sheet_buffer* editor_get_active_sheet(
    editor_context* editor, workbook* wb, b32 create_if_empty
) {
    return editor_win_get_sheet(
        editor, wb, editor->active_win, create_if_empty
    );
}

editor_register* editor_get_reg(editor_context* editor, u8 reg) {
    if (reg < EDITOR_REGISTER_FIRST || reg > EDITOR_REGISTER_LAST) {
        return NULL;
    }

    return editor->registers[reg - EDITOR_REGISTER_FIRST];
}

editor_window* _editor_create_win(editor_context* editor) {
    editor->num_windows++;

    if (editor->first_free_win != NULL) {
        editor_window* win = editor->first_free_win;
        SLL_POP_FRONT(editor->first_free_win, editor->last_free_win);

        return win;
    }

    editor_window* win = PUSH_STRUCT(editor->arena, editor_window);
    win->_sheet = editor->empty_sheet;

    return win;
}

void _editor_free_win(editor_context* editor, editor_window* win) {
    if (editor->num_windows) { editor->num_windows--; }

    MEM_ZERO(win, sizeof(editor_window));
    win->_sheet = editor->empty_sheet;

    SLL_PUSH_BACK(editor->first_free_win, editor->last_free_win, win);
}


