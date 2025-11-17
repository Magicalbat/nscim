
b32 _editor_cmd_parse_i64(string8 str, i64* out) {
    return false;
}

b32 _editor_cmd_parse_f64(string8 str, f64* out) {
    return false;
}

b32 _editor_cmd_parse_cell(string8 str, sheet_cell_pos* out) {
    return false;
}

b32 _editor_cmd_parse_range(string8 str, sheet_range* out) {
    return false;
}

editor_cmd_res editor_cmd_null(void) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_move_up(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_move_down(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_move_left(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_move_right(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_clear(sheet_range range) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_sort(sheet_range range, i64 direction) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_scroll_up(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_scroll_down(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_scroll_left(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}

editor_cmd_res editor_cmd_scroll_right(i64 count) {
    return (editor_cmd_res){ .status = EDITOR_CMD_STATUS_OKAY };
}


