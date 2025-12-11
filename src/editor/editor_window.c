
sheet_buffer* editor_win_get_sheet(
    editor_context* editor, workbook* wb,
    editor_window* win, b32 create_if_empty
) {
    if (win->internal) {
        return editor->empty_sheet;
    }

    if (create_if_empty && win->_sheet == editor->empty_sheet) {
        win->_sheet = wb_create_sheet_buffer(wb);
    }

    return win->_sheet;
}

void _editor_win_compute_size(editor_window* cur, editor_window* parent) {
    b32 is_child1 = parent->child1 == cur;

    u32 child_case = ((u32)parent->split_dir << 1) | (u32)is_child1;

    switch (child_case) {
        // Vertical, child0
        case 0b00: {
            cur->start_x = parent->start_x;
            cur->start_y = parent->start_y;
            cur->width = (u32)ceil((f64)parent->width * cur->parent_fraction);
            cur->height = parent->height;
        } break;

        // Vertical, child1
        case 0b01: {
            cur->start_x = parent->start_x + parent->child0->width;
            cur->start_y = parent->start_y;
            cur->width = parent->width - parent->child0->width;
            cur->height = parent->height;
        } break;

        // Horizontal, child0
        case 0b10: {
            cur->start_x = parent->start_x;
            cur->start_y = parent->start_y;
            cur->width = parent->width;
            cur->height = (u32)ceil((f64)parent->height * cur->parent_fraction);
        } break;

        // Horizontal, child1
        case 0b11: {
            cur->start_x = parent->start_x;
            cur->start_y = parent->start_y + parent->child0->height;
            cur->width = parent->width;
            cur->height = parent->height - parent->child0->height;
        } break;
    }
}

void editor_win_split(
    editor_context* editor, editor_window_split split, b32 open_in_both
) {
    editor_window* parent = editor->active_win;

    parent->split_dir = split;

    for (u32 i = 0; i < 2; i++) {
        parent->children[i] = _editor_create_win(editor);

        editor_window* cur = parent->children[i];

        cur->parent = parent;
        cur->parent_fraction = 0.5;

        if (open_in_both || i == 0) {
            cur->_sheet = parent->_sheet;
            cur->scroll_pos = parent->scroll_pos;
            cur->cursor_pos = parent->cursor_pos;
        }

        _editor_win_compute_size(cur, parent);

        u32 child_case = ((u32)split << 1) | (u32)(i == 1);
        switch (child_case) {
            // Vertical, child0
            case 0b00: {
                cur->anim_start_x = (f32)parent->start_x;
                cur->anim_start_y = (f32)parent->start_y;
                cur->anim_width = (f32)parent->width;
                cur->anim_height = (f32)parent->height;
            } break;

            // Vertical, child1
            case 0b01: {
                cur->anim_start_x = (f32)(parent->start_x + parent->width);
                cur->anim_start_y = (f32)parent->start_y;
                cur->anim_width = 0.0f;
                cur->anim_height = (f32)parent->height;
            } break;

            // Horizontal, child0
            case 0b10: {
                cur->anim_start_x = (f32)parent->start_x;
                cur->anim_start_y = (f32)parent->start_y;
                cur->anim_width = (f32)parent->width;
                cur->anim_height = (f32)parent->height;
            } break;

            // Horizontal, child1
            case 0b11: {
                cur->anim_start_x = (f32)parent->start_x;
                cur->anim_start_y = (f32)(parent->start_y + parent->height);
                cur->anim_width = (f32)parent->width;
                cur->anim_height = 0.0f;
            } break;
        }
    }

    parent->internal = true;
    parent->_sheet = editor->empty_sheet;

    editor->active_win = parent->child1;
}

void editor_win_close(editor_context* editor) {
    if (editor->active_win == editor->root_win) {
        // TODO: error handling of some sort
        return;
    }

    editor_window* win = editor->active_win;

    if (win->internal) {
        // TODO: error handling
        return;
    }

    editor_window* parent = win->parent;
    editor_window* other_child = win->parent->child0 == win ?
        win->parent->child1 : win->parent->child0;

    editor_window* grandparent = parent->parent;

    if (grandparent == NULL) {
        editor->root_win = other_child;
        other_child->parent = NULL;
    } else {
        other_child->parent = grandparent;

        if (grandparent->child0 == parent) {
            grandparent->child0 = other_child;
        } else {
            grandparent->child1 = other_child;
        }
    }

    editor->active_win = other_child;
    while (editor->active_win->internal && editor->active_win->child0 != NULL) {
        editor->active_win = editor->active_win->child0;
    }

    _editor_free_win(editor, parent);
    _editor_free_win(editor, win);
}

void editor_win_compute_sizes(
    editor_context* editor, u32 total_width, u32 total_height
) {
    editor_window* root = editor->root_win;

    root->width = total_width;
    root->height = total_height;
    root->start_x = 0;
    root->start_y = 0;

    if (!root->internal) {
        editor_win_update_num_rows(root);
        editor_win_update_num_cols(root);
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);
    u32 stack_size = 0;
    editor_window** stack = PUSH_ARRAY(
        scratch.arena, editor_window*, editor->num_windows
    );

    // Ensure that child0 is processed first
    if (root->child1 != NULL) { stack[stack_size++] = root->child1; }
    if (root->child0 != NULL) { stack[stack_size++] = root->child0; }

    while (stack_size != 0) {
        editor_window* cur = stack[--stack_size];
        editor_window* parent = cur->parent;

        _editor_win_compute_size(cur, parent);

        if (!cur->internal) {
            editor_win_update_num_rows(cur);
            editor_win_update_num_cols(cur);
        }

        // Ensure that child0 is processed first
        if (cur->child1 != NULL) { stack[stack_size++] = cur->child1; }
        if (cur->child0 != NULL) { stack[stack_size++] = cur->child0; }
    }

    arena_scratch_release(scratch);
}

void editor_win_update_num_rows(editor_window* win) {
    win->num_rows = 0;

    sheet_buffer* sheet = win->_sheet;
    sheet_pos scroll = win->scroll_pos;

    u32 win_height = (u32)win->anim_height;

    u32 cur_row_height = SHEET_DEF_ROW_HEIGHT;
    u32 y = EDITOR_WIN_STATUS_ROWS_TOP + 1;
    for (;
        y < win_height && scroll.row + win->num_rows < SHEET_MAX_ROWS;
        y += cur_row_height, win->num_rows++
    ) {
        cur_row_height = sheet_get_row_height(
            sheet, scroll.row + win->num_rows
        );
    }

    win->cutoff_height = win_height > y ? 0 : y - win_height;
}

void editor_win_update_num_cols(editor_window* win) {
    win->num_cols = 0;

    sheet_buffer* sheet = win->_sheet;
    sheet_pos scroll = win->scroll_pos;

    u32 win_width = (u32)win->anim_width;

    u32 cur_col_width = SHEET_DEF_COL_WIDTH;
    u32 x = SHEET_MAX_ROW_CHARS;
    for (;
        x < win_width && scroll.col + win->num_cols < SHEET_MAX_COLS;
        x += cur_col_width, win->num_cols++
    ) {
        cur_col_width = sheet_get_col_width(sheet, scroll.col + win->num_cols);
    }

    win->cutoff_width = win_width > x ? 0 : x - win_width;
}

// Expands in the direction opposite the split
// e.g. Expands width when split == SHEET_WIN_SPLIT_VERT
void _editor_win_expand(
    editor_context* editor, i32 amount, editor_window_split split
) {
    editor_window* cur = editor->active_win;

    while (
        cur->parent != NULL &&
        cur->parent->split_dir != split
    ) {
        cur = cur->parent;
    }

    if (cur->parent == NULL) {
        return;
    }

    editor_window* parent = cur->parent;

    u32 parent_dim = 1;
    u32 child_dim = 1;

    if (split == EDITOR_WIN_SPLIT_VERT) {
        parent_dim = cur->parent->width;
        child_dim = cur->width;
    } else {
        parent_dim = cur->parent->height;
        child_dim = cur->height;
    }

    amount = CLAMP(amount, -((i32)child_dim), (i32)(parent_dim - child_dim));

    child_dim = (u32)((i32)child_dim + amount);
    f64 new_fraction = (f64)parent_dim / (f64)child_dim;

    cur->parent_fraction = new_fraction;

    editor_window* other = parent->child0 == cur ?
        parent->child1 : parent->child0;

    other->parent_fraction = 1.0 - cur->parent_fraction;
}

void editor_win_inc_width(editor_context* editor, i32 amount) {
    _editor_win_expand(
        editor, amount, (editor_window_split){ EDITOR_WIN_SPLIT_VERT }
    );
}

void editor_win_inc_height(editor_context* editor, i32 amount) {
    _editor_win_expand(
        editor, amount, (editor_window_split){ EDITOR_WIN_SPLIT_HORZ }
    );
}

// Moves in the direction opposite the split
// e.g. moves up/down when split == SHEET_WIN_SPLIT_HORZ
// TODO: make 4 quadrant windows work better (i.e. go from child1 -> child1)
void _editor_win_change_active(
    editor_context* editor, editor_window* cur,
    i32 dir, editor_window_split split
) {
    if (cur == NULL) {
        return;
    }

    while (
        cur->parent != NULL && 
        cur->parent->split_dir != split
    ) {
        cur = cur->parent;
    }

    if (cur->parent == NULL) {
        return;
    }

    i32 cur_child = cur->parent->child0 == cur ? 0 : 1;
    i32 new_child = cur_child + dir;

    if (new_child < 0 || new_child > 1) {
        _editor_win_change_active(editor, cur->parent, dir, split);
        return;
    }

    cur = cur->parent->children[new_child];
    while (cur->internal && cur->child0 != NULL) {
        cur = cur->child0;
    }

    if (cur->internal) {
        return;
    }

    editor->active_win = cur;
}

void editor_win_update_anims(editor_context* editor, f32 delta) {
    f32 anim_speed = editor->settings.anim_speed;

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);
    u32 stack_size = 0;
    editor_window** stack = PUSH_ARRAY(
        scratch.arena, editor_window*, editor->num_windows
    );

    stack[stack_size++] = editor->root_win;

    while (stack_size != 0) {
        editor_window* cur = stack[--stack_size];

        cur->anim_start_x = exp_anim(
            cur->anim_start_x, (f32)cur->start_x, anim_speed, delta, 1.0f
        );
        cur->anim_start_y = exp_anim(
            cur->anim_start_y, (f32)cur->start_y, anim_speed, delta, 1.0f
        );
        cur->anim_width = exp_anim(
            cur->anim_width, (f32)cur->width, anim_speed, delta, 1.0f
        );
        cur->anim_height = exp_anim(
            cur->anim_height, (f32)cur->height, anim_speed, delta, 1.0f
        );

        // Ensure that child0 is processed first
        if (cur->child1 != NULL) { stack[stack_size++] = cur->child1; }
        if (cur->child0 != NULL) { stack[stack_size++] = cur->child0; }
    }

    arena_scratch_release(scratch);
}

void editor_win_change_active_horz(editor_context* editor, i32 dir) {
    _editor_win_change_active(
        editor, editor->active_win, dir,
        (editor_window_split){ EDITOR_WIN_SPLIT_VERT }
    );
}

void editor_win_change_active_vert(editor_context* editor, i32 dir) {
    _editor_win_change_active(
        editor, editor->active_win, dir,
        (editor_window_split){ EDITOR_WIN_SPLIT_HORZ }
    );
}



