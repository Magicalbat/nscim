
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

u32 sheets_range_to_chars(sheet_range range, u8* chars, u32 max_chars);

b32 sheets_range_from_str(string8 str, sheet_range* out_range);

u32 sheets_cell_to_str(sheet_cell_ref cell, u8* chars, u32 max_chars);

