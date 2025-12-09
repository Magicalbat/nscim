
sheet_range_copy* sheet_range_copy_create(
    mem_arena* arena, sheet_buffer* sheet, sheet_range in_range
) {
    sheet_range_copy* range_copy = PUSH_STRUCT(arena, sheet_range_copy);

    sheet_range range = sheets_fix_range(in_range);

    range_copy->orig_pos = range.start;

    range_copy->num_rows = range.end.row - range.start.row + 1;
    range_copy->num_cols = range.end.col - range.start.col + 1;

    u32 last_resized_col = range.end.col;
    while (last_resized_col >= range.start.col) {
        if (
            sheet_get_col_width(sheet, last_resized_col) !=
            SHEET_DEF_COL_WIDTH
        ) {
            break;
        }

        last_resized_col--;
    }

    range_copy->num_column_widths = last_resized_col < range.start.col ?
        0 : last_resized_col - range.start.col + 1;

    for (u32 i = 0; i < range_copy->num_column_widths; i++) {
        range_copy->column_widths[i] = sheet_get_col_width(
            sheet, range.start.col + i
        );
    }

    u32 last_resized_row = range.end.row;
    while (last_resized_row >= range.start.row) {
        if (
            sheet_get_row_height(sheet, last_resized_row) !=
            SHEET_DEF_ROW_HEIGHT
        ) {
            break;
        }

        last_resized_row--;
    }

    range_copy->num_row_heights = last_resized_row < range.start.row ?
        0 : last_resized_row - range.start.row + 1;

    for (u32 i = 0; i < range_copy->num_row_heights; i++) {
        range_copy->row_heights[i] = sheet_get_row_height(
            sheet, range.start.row + i
        );
    }

    u64 arrays_size = (u64)(sizeof(sheet_cell_type) + sizeof(f64)) *
        (u64)range_copy->num_rows * (u64)range_copy->num_cols;
    u64 chunks_size = ~(u64)0;

    sheet_chunk_pos start_chunk = {
        range.start.row / SHEET_CHUNK_ROWS,
        range.start.col / SHEET_CHUNK_COLS,
    };

    sheet_chunk_pos end_chunk = {
        range.end.row / SHEET_CHUNK_ROWS,
        range.end.col / SHEET_CHUNK_COLS,
    };
    
    if (arrays_size < chunks_size) {
        // Creating array mode range copy

        range_copy->mode = SHEET_RANGE_COPY_MODE_ARRAYS;

        u64 num_cells = (u64)range_copy->num_rows * (u64)range_copy->num_cols;

        range_copy->arrays.types = PUSH_ARRAY(arena, sheet_cell_type, num_cells);
        range_copy->arrays.nums = PUSH_ARRAY(arena, f64, num_cells);

        for (u32 c_col = start_chunk.col; c_col <= end_chunk.col; c_col++) {
            for (u32 c_row = start_chunk.row; c_row <= end_chunk.row; c_row++) {
                sheet_chunk* chunk = sheet_get_chunk(
                    NULL, sheet, (sheet_chunk_pos){ c_row, c_col }, false
                );

                if (chunk == NULL) { continue; }

                u32 l_start_col = 0;
                u32 l_start_row = 0;
                u32 l_end_col = SHEET_CHUNK_COLS - 1;
                u32 l_end_row = SHEET_CHUNK_ROWS - 1;

                if (c_col == start_chunk.col) {
                    l_start_col = range.start.col % SHEET_CHUNK_COLS;
                }
                if (c_col == end_chunk.col) {
                    l_end_col = range.end.col % SHEET_CHUNK_COLS;
                }
                if (c_row == start_chunk.row) {
                    l_start_row = range.start.row % SHEET_CHUNK_ROWS;
                }
                if (c_row == end_chunk.row) {
                    l_end_row = range.end.row % SHEET_CHUNK_ROWS;
                }

                for (u32 l_col = l_start_col; l_col <= l_end_col; l_col++) {
                    for (u32 l_row = l_start_row; l_row <= l_end_row; l_row++) {
                        u64 arrays_index =
                            (u64)c_row * SHEET_CHUNK_ROWS + l_row - range.start.row + 
                            (u64)(c_col * SHEET_CHUNK_COLS + l_col -range.start.col) *
                            range_copy->num_rows;

                        u32 chunk_index = l_row + l_col * SHEET_CHUNK_ROWS;

                        sheet_cell_type cell_type = chunk->types[chunk_index];
                        range_copy->arrays.types[arrays_index] = cell_type;

                        if (cell_type == SHEET_CELL_TYPE_NUM) {
                            range_copy->arrays.nums[arrays_index] = chunk->nums[chunk_index];
                        } else if (cell_type == SHEET_CELL_TYPE_STRING) {
                            sheet_string* chunk_str = chunk->strings[chunk_index];

                            string8* new_str = PUSH_STRUCT(arena, string8);
                            new_str->size = chunk_str->size;
                            new_str->str = PUSH_ARRAY_NZ(arena, u8, chunk_str->size);
                            memcpy(new_str->str, chunk_str->str, chunk_str->size);

                            range_copy->arrays.strings[arrays_index] = new_str;
                        }
                    }
                }
            }
        }
    } else {
        // Creating chunk mode range copy
        #warning TODO: chunk mode range copy
    }

    return range_copy;
}

void sheet_range_copy_restore(
    sheet_range_copy* range_copy, workbook* wb,
    sheet_buffer* sheet, sheet_pos pos
) {
    if (range_copy->mode == SHEET_RANGE_COPY_MODE_EMPTY) {
        sheet_clear_range(
            wb, sheet, (sheet_range){ {
                pos, (sheet_pos){
                    pos.row + range_copy->num_rows,
                    pos.col + range_copy->num_cols,
                }
            } }
        );
    } else if (range_copy->mode == SHEET_RANGE_COPY_MODE_ARRAYS) {
        for (u32 col = 0; col < range_copy->num_cols; col++) {
            for (u32 row = 0; row < range_copy->num_rows; row++) {
                u64 arrays_index = row + col * range_copy->num_rows;
                sheet_pos dest_pos = { pos.row + row, pos.col + col };

                switch (range_copy->arrays.types[arrays_index]) {
                    case SHEET_CELL_TYPE_EMPTY: {
                        sheet_clear_cell(wb, sheet, dest_pos);
                    } break;

                    case SHEET_CELL_TYPE_NUM: {
                        sheet_set_cell_num(
                            wb, sheet, dest_pos,
                            range_copy->arrays.nums[arrays_index]
                         );
                    } break;

                    case SHEET_CELL_TYPE_STRING: {
                        sheet_set_cell_str(
                            wb, sheet, dest_pos,
                            *range_copy->arrays.strings[arrays_index]
                        );
                    } break;
                }
            }
        }
    }
}

