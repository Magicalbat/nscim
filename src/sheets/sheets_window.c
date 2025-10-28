
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

    sheet_window* parent = win;
    sheet_window* other_child = parent->child0 == win ?
        parent->child1 : parent->child0;

    parent->child0 = NULL;
    parent->child1 = NULL;
    parent->internal = false;
    parent->_sheet = other_child->_sheet;
    parent->scroll_pos = other_child->scroll_pos;
    parent->cursor_pos = other_child->cursor_pos;

    _wb_free_win(wb, win);
    _wb_free_win(wb, other_child);
}

void wb_win_compute_sizes(workbook* wb, u32 total_width, u32 total_height) {
}

void wb_win_inc_width(workbook* wb, u32 amount);

void wb_win_inc_height(workbook* wb, u32 amount);


