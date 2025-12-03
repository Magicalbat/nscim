
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

void sheets_parse_store_str(
    workbook* wb, sheet_buffer* sheet, string8 base_str, sheet_pos pos
) {
    string8 str = base_str;

    if (str.size == 0) {
        sheet_clear_cell(wb, sheet, pos);
        return;
    }

    b8 is_num = true;
    b8 negate = false;
    b8 past_decimal = false;
    b8 past_sci_exp_decimal = false;
    b8 reading_sci_exp = false;
    b8 negate_sci_exp = false;
    u32 decimal_count = 0;
    u32 sci_exp_decimal_count = 0;
    f64 num = 0.0;
    f64 sci_exp = 0.0;

    if (str.str[0] == '-' || str.str[0] == '+') {
        negate = str.str[0] == '-';

        str.str++;
        str.size--;
    } 

    for (u64 i = 0; is_num && i < str.size; i++) {
        u8 c = str.str[i];

        switch (c) {
            case '.': {
                if (reading_sci_exp) {
                    if (past_sci_exp_decimal) {
                        is_num = false;
                    } else {
                        past_sci_exp_decimal = true;
                    }
                } else {
                    if (past_decimal) {
                        is_num = false;
                    } else {
                        past_decimal = true;
                    }
                }
            } break;

            case 'e':
            case 'E': {
                if (reading_sci_exp) {
                    is_num = false;
                } else {
                    reading_sci_exp = true;

                    if (i + 1 >= str.size) {
                        is_num = false;
                    } else {
                        if (str.str[i + 1] == '-' || str.str[i + 1] == '+') {
                            negate_sci_exp = str.str[i + 1] == '-';
                            i++;
                        }
                    }
                }
            } break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                if (reading_sci_exp) {
                    if (past_sci_exp_decimal) {
                        sci_exp_decimal_count++;
                    }

                    sci_exp *= 10.0;
                    sci_exp += c - '0';
                } else {
                    if (past_decimal) {
                        decimal_count++;
                    }

                    num *= 10.0;
                    num += c - '0';
                }
            } break;

            default: {
                is_num = false;
            } break;
        }
    }

    if (is_num) {
        while (decimal_count--) {
            num *= 0.1;
        }
        
        if (reading_sci_exp) {
            while (sci_exp_decimal_count--) {
                sci_exp *= 0.1;
            }

            if (negate_sci_exp) {
                sci_exp *= -1.0;
            }

            num *= pow(10.0, sci_exp);
        }

        if (negate) {
            num *= -1.0;
        }

        sheet_set_cell_num(wb, sheet, pos, num);
    } else {
        sheet_set_cell_str(wb, sheet, pos, base_str);
    }

}

