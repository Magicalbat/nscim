
sheet_buffer* wb_win_get_sheet(workbook* wb, sheet_window* win, b32 create_if_empty) {
    if (win->internal) {
        return wb->empty_buffer;
    }

    if (create_if_empty && win->_sheet == wb->empty_buffer) {
        win->_sheet = wb_create_sheet_buffer(wb);
    }

    return win->_sheet;
}

void wb_win_split(workbook* wb, sheet_window_split split, b32 open_in_both) {
    sheet_window* win = wb->active_win;

    for (u32 i = 0; i < 2; i++) {
        win->children[i] = _wb_create_win(wb);

        win->children[i]->parent = win;
        win->children[i]->parent_fraction = 0.5;

        if (open_in_both || i == 0) {
            win->children[i]->_sheet = win->_sheet;
            win->children[i]->scroll_pos = win->scroll_pos;
            win->children[i]->cursor_pos = win->cursor_pos;
        }
    }

    win->split_dir = split;
    win->internal = true;
    win->_sheet = wb->empty_buffer;

    wb->active_win = win->child1;
}

void wb_win_close(workbook* wb) {
    if (wb->active_win == wb->root_win) {
        // TODO: error handling of some sort
        return;
    }

    sheet_window* win = wb->active_win;

    if (win->internal) {
        // TODO: error handling
        return;
    }

    sheet_window* parent = win->parent;
    sheet_window* other_child = win->parent->child0 == win ?
        win->parent->child1 : win->parent->child0;

    sheet_window* grandparent = parent->parent;

    if (grandparent == NULL) {
        wb->root_win = other_child;
        other_child->parent = NULL;
    } else {
        other_child->parent = grandparent;

        if (grandparent->child0 == parent) {
            grandparent->child0 = other_child;
        } else {
            grandparent->child1 = other_child;
        }
    }

    wb->active_win = other_child;
    while (wb->active_win->internal && wb->active_win->child0 != NULL) {
        wb->active_win = wb->active_win->child0;
    }

    _wb_free_win(wb, parent);
    _wb_free_win(wb, win);
}

void wb_win_compute_sizes(workbook* wb, u32 total_width, u32 total_height) {
    sheet_window* root = wb->root_win;

    root->width = total_width;
    root->height = total_height;
    root->start_x = 0;
    root->start_y = 0;

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);
    u32 stack_size = 0;
    sheet_window** stack = PUSH_ARRAY(scratch.arena, sheet_window*, wb->num_windows);

    // Ensure that child0 is processed first
    if (root->child1 != NULL) { stack[stack_size++] = root->child1; }
    if (root->child0 != NULL) { stack[stack_size++] = root->child0; }

    while (stack_size != 0) {
        sheet_window* cur = stack[--stack_size];
        sheet_window* parent = cur->parent;
        b32 is_child1 = parent->child1 == cur;

        u32 child_case = (parent->split_dir.s << 1) | (u32)is_child1;

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

        // Ensure that child0 is processed first
        if (cur->child1 != NULL) { stack[stack_size++] = cur->child1; }
        if (cur->child0 != NULL) { stack[stack_size++] = cur->child0; }
    }

    arena_scratch_release(scratch);
}

// Expands in the direction opposite the split
// e.g. Expands width when split.s == SHEET_WIN_SPLIT_VERT
void _wb_win_expand(workbook* wb, i32 amount, sheet_window_split split) {
    sheet_window* cur = wb->active_win;

    while (
        cur->parent != NULL &&
        cur->parent->split_dir.s != split.s
    ) {
        cur = cur->parent;
    }

    if (cur->parent == NULL) {
        return;
    }

    sheet_window* parent = cur->parent;

    u32 parent_dim = 1;
    u32 child_dim = 1;

    if (split.s == SHEETS_WIN_SPLIT_VERT) {
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

    sheet_window* other = parent->child0 == cur ?
        parent->child1 : parent->child0;

    other->parent_fraction = 1.0 - cur->parent_fraction;
}

void wb_win_inc_width(workbook* wb, i32 amount) {
    _wb_win_expand(wb, amount, (sheet_window_split){ SHEETS_WIN_SPLIT_VERT });
}

void wb_win_inc_height(workbook* wb, i32 amount) {
    _wb_win_expand(wb, amount, (sheet_window_split){ SHEETS_WIN_SPLIT_HORZ });
}

// Moves in the direction opposite the split
// e.g. moves up/down when split.s == SHEET_WIN_SPLIT_HORZ
// TODO: make 4 quadrant windows work better (i.e. go from child1 -> child1)
void _wb_win_change_active(workbook* wb, sheet_window* cur, i32 dir, sheet_window_split split) {
    if (cur == NULL) {
        return;
    }

    while (
        cur->parent != NULL && 
        cur->parent->split_dir.s != split.s
    ) {
        cur = cur->parent;
    }

    if (cur->parent == NULL) {
        return;
    }

    i32 cur_child = cur->parent->child0 == cur ? 0 : 1;
    i32 new_child = cur_child + dir;

    if (new_child < 0 || new_child > 1) {
        _wb_win_change_active(wb, cur->parent, dir, split);
        return;
    }

    cur = cur->parent->children[new_child];
    while (cur->internal && cur->child0 != NULL) {
        cur = cur->child0;
    }

    if (cur->internal) {
        return;
    }

    wb->active_win = cur;
}

void wb_win_change_active_horz(workbook* wb, i32 dir) {
    _wb_win_change_active(
        wb, wb->active_win, dir,
        (sheet_window_split){ SHEETS_WIN_SPLIT_VERT }
    );
}

void wb_win_change_active_vert(workbook* wb, i32 dir) {
    _wb_win_change_active(
        wb, wb->active_win, dir,
        (sheet_window_split){ SHEETS_WIN_SPLIT_HORZ }
    );
}



