
typedef enum {
    SHEET_RANGE_COPY_MODE_EMPTY = 0,
    SHEET_RANGE_COPY_MODE_ARRAYS,
    SHEET_RANGE_COPY_MODE_CHUNKS
} sheet_range_copy_mode;

typedef struct {
    sheet_range_copy_mode mode;

    sheet_pos orig_pos;

    u32 num_rows;
    u32 num_cols;

    u32 num_column_widths;
    u32 num_row_heights;

    u8* column_widths;
    u8* row_heights;

    union {
        struct {
            sheet_cell_type* types;

            union {
                f64* nums;
                string8** strings;
            };
        } arrays;

        struct {
            u32 offset_rows;
            u32 offset_cols;

            u32 map_capacity;
            u32 num_chunks;

            sheet_chunk** chunk_map;
        } chunks;
    };
} sheet_range_copy;

sheet_range_copy* sheet_range_copy_create(
    mem_arena* arena, sheet_buffer* sheet, sheet_range range
);

void sheet_range_copy_restore(
    sheet_range_copy* range_copy, workbook* wb,
    sheet_buffer* sheet, sheet_pos pos
);

