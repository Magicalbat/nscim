
#define EDITOR_REGISTER_ARENA_RESERVE KiB(256)
#define EDITOR_REGISTER_ARENA_COMMIT KiB(16)

typedef enum {
    EDITOR_REGISTER_TYPE_INVALID = 0,

    EDITOR_REGISTER_TYPE_BLACKHOLE,

    EDITOR_REGISTER_TYPE_EMPTY,
    EDITOR_REGISTER_TYPE_STRING,
    EDITOR_REGISTER_TYPE_CELLS,
    EDITOR_REGISTER_TYPE_CHUNKS
} editor_register_type;

typedef string8 _editor_register_str;

typedef struct {
    sheet_cell_type* types;

    // These two pointers point to the same memory
    f64* nums;
    string8* strings;
} _editor_register_cells;

typedef struct {
    u32 num_chunk_rows;
    u32 num_chunk_cols;

    u32 num_chunks;
    u32 chunk_map_capacity;

    sheet_chunk** chunk_map;
} _editor_register_chunks;

typedef struct editor_register {
    mem_arena* arena;

    editor_register_type reg_type;

    sheet_pos orig_pos;

    u32 num_rows;
    u32 num_cols;

    u32 num_column_widths;
    u32 num_row_heights;

    u8* column_widths;
    u8* row_heights;

    union {
        _editor_register_str string;
        _editor_register_cells cells;
        _editor_register_chunks chunks;
    } contents;
} editor_register;

void editor_reg_create(editor_register* reg, editor_register_type reg_type);

void editor_reg_destroy(editor_register* reg);

