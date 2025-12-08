
typedef enum {
    SHEET_RANGE_COPY_MODE_EMPTY = 0,
    SHEET_RANGE_COPY_MODE_ARRAYS,
    SHEET_RANGE_COPY_MODE_CHUNKS
} sheet_range_copy_mode;

typedef struct {
    sheet_range_copy_mode mode;

    sheet_pos orig_pos;

    u32 rows;
    u32 cols;

    union {
        struct {
            sheet_cell_type* types;

            union {
                f64* nums;
                sheet_string** strings;
            };
        } arrays;

        struct {
            u32 map_capacity;
            u32 num_chunks;

            sheet_chunk** chunk_map;
        } chunks;
    };
} sheet_range_copy;


