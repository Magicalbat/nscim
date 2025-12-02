
u32 sheets_col_to_chars(u32 col, u8* chars, u32 max_chars) {
    u32 size = 0;

    for (u32 i = 0; i < max_chars; i++) {
        chars[size++] = (col % 26) + 'A';
        col /= 26;

        if (col == 0) {
            break;
        } else {
            col--;
        }
    }

    for (u32 i = 0; i < size/2; i++) {
        u8 tmp = chars[size-i-1];
        chars[size-i-1] = chars[i];
        chars[i] = tmp;
    }

    return size;
}

b32 sheets_col_from_str(string8 str, u32* out_col) {
    u32 col = 0;

    for (u64 i = 0; i < str.size; i++) {
        u8 c = str.str[i];

        if (c >= 'a' && c <= 'z') {
            c -= 'a' - 'A';
        }

        if (c < 'A' || c > 'Z') {
            return false;
        }

        u32 n = c - 'A';
        col *= 26;
        col += n + 1;
    }
    col -= 1;

    *out_col = col;

    return true;
}

u32 sheets_pos_to_chars(sheet_pos pos, u8* chars, u32 max_chars) {
    u32 size = 0;

    size += sheets_col_to_chars(pos.col, chars, max_chars);
    size += chars_from_u32(pos.row, chars + size, max_chars - size);

    return size;
}

b32 sheets_pos_from_str(string8 str, sheet_pos* out_pos) {
    sheet_pos pos = { 0 };

    u64 i = 0;

    for (; i < str.size; i++) {
        if (i >= str.size || !isalpha(str.str[i])) {
            return false;
        }

        u8 c = str.str[i];

        if (c >= 'a' && c <= 'z') {
            c -= 'a' - 'A';
        }

        if (c < 'A' || c > 'Z') {
            break;
        }

        u32 n = c - 'A';
        pos.col *= 26;
        pos.col += n + 1;
    }
    pos.col--;

    for (; i < str.size; i++) {
        u8 c = str.str[i];

        if (c < '0' || c > '9') {
            return false;
        }

        pos.row *= 10;
        pos.row += c - '0';
    }

    *out_pos = pos;

    return true;
}

u32 sheets_range_to_chars(sheet_range range, u8* chars, u32 max_chars) {
    u32 size = 0;

    size += sheets_pos_to_chars(range.start, chars, max_chars);
    if (size < max_chars) {
        chars[size++] = ':';
    }
    size += sheets_pos_to_chars(range.end, chars + size, max_chars - size);

    return size;
}

b32 sheets_range_from_str(string8 str, sheet_range* out_range) {
    sheet_pos positions[2] = { 0 };

    u64 i = 0;

    for (u32 j = 0; j < 2; j++) {
        if (i >= str.size || !isalpha(str.str[i])) {
            return false;
        }

        for (; i < str.size; i++) {
            u8 c = str.str[i];

            if (c >= 'a' && c <= 'z') {
                c -= 'a' - 'A';
            }

            if (c < 'A' || c > 'Z') {
                break;
            }

            u32 n = c - 'A';
            positions[j].col *= 26;
            positions[j].col += n + 1;
        }
        positions[j].col--;

        if (i >= str.size || !isdigit(str.str[i])) {
            return false;
        }

        for (; i < str.size; i++) {
            u8 c = str.str[i];

            if (c < '0' || c > '9') {
                break;
            }

            positions[j].row *= 10;
            positions[j].row += c - '0';
        }

        if (j == 0 && (i >= str.size || str.str[i] != ':')) {
            return false;
        }

        i++;
    }

    *out_range = (sheet_range){ { positions[0], positions[1] } };

    return true;
}

u32 sheets_cell_to_chars(sheet_cell_view cell, u8* chars, u32 max_chars) {
    switch ((sheet_cell_type_enum)cell.type) {
        case _SHEET_CELL_TYPE_COUNT:
        case SHEET_CELL_TYPE_EMPTY_CHUNK:
        case SHEET_CELL_TYPE_EMPTY: {
            return 0;
        } break;

        case SHEET_CELL_TYPE_NUM: {
            u64 max_write = max_chars > 0 ? max_chars - 1 : 0;

            i32 written = 0;

            if (cell.num < (f64)INT64_MAX && cell.num == floor(cell.num)) {
                written = snprintf(
                    (char*)chars, max_write, "%" PRIi64, (i64)cell.num
                );
            } else {
                written = snprintf((char*)chars, max_write, "%g", cell.num);
            }

            if (written < 0) {
                return 0;
            }

            return (u32)written;
        } break;

        case SHEET_CELL_TYPE_STRING: {
            u32 size = MIN(cell.str->size, max_chars);
            memcpy(chars, cell.str->str, size);

            return size;
        } break;
    }
}

sheet_range sheets_fix_range(sheet_range in_range) {
    sheet_range range = {
        { {
            MIN(in_range.start.row, in_range.end.row),
            MIN(in_range.start.col, in_range.end.col),
        }, {
            MAX(in_range.start.row, in_range.end.row),
            MAX(in_range.start.col, in_range.end.col),
        } },
    };

    range.start.row = MIN(SHEET_MAX_ROWS - 1, range.start.row);
    range.start.col = MIN(SHEET_MAX_COLS - 1, range.start.col);
    range.end.row = MIN(SHEET_MAX_ROWS - 1, range.end.row);
    range.end.col = MIN(SHEET_MAX_COLS - 1, range.end.col);

    return range;
}

