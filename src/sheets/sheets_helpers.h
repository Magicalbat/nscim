
// Use an array that is at least SHEET_MAX_COL_CHARS
u32 sheets_col_to_chars(u32 col, u8* chars, u32 max_chars);

// Returns true if the string is a valid column
b32 sheets_col_from_str(string8 str, u32* col);

// Use an array that is at least
// SHEET_MAX_COL_CHARS + SHEET_MAX_ROW_CHARS
u32 sheets_pos_to_chars(sheet_pos pos, u8* chars, u32 max_chars);

// Returns true if the string is a valid pos
b32 sheets_pos_from_str(string8 str, sheet_pos* pos);

// Use an array that is at least
// 1 + 2 * (SHEET_MAX_COL_CHARS + SHEET_MAX_ROW_CHARS)
u32 sheets_range_to_chars(sheet_range range, u8* chars, u32 max_chars);

// Returns true if the string is a valid range
b32 sheets_range_from_str(string8 str, sheet_range* range);

// Use an array that is at least SHEET_MAX_STRLEN
u32 sheets_cell_to_str(sheet_cell_ref cell, u8* chars, u32 max_chars);

