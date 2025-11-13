
void editor_update(window* win, editor_context* editor, workbook* wb) {
    win_input input = win_next_input(win);

    if (input == 0) {
        editor->should_draw = false;
        return;
    } else {
        editor->should_draw = true;
    }

    if (input == 'q') {
        editor->should_quit = true;
        return;
    }
}

