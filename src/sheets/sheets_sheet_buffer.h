
#define SHEET_CHUNK_COLS 16
#define SHEET_CHUNK_ROWS 128
#define SHEET_CHUNK_SIZE (SHEET_CHUNK_COLS * SHEET_CHUNK_ROWS)

#define SHEET_MAX_COLS KiB(32)
#define SHEET_MAX_ROWS MiB(2)

#define SHEET_CHUNKS_X (SHEET_MAX_COLS / SHEET_CHUNK_COLS)
#define SHEET_CHUNKS_Y (SHEET_MAX_ROWS / SHEET_CHUNK_ROWS)
#define SHEET_MAX_CHUNKS (SHEET_CHUNKS_X * SHEET_CHUNKS_Y)

#define SHEET_DEF_COL_WIDTH 10
#define SHEET_DEF_ROW_HEIGHT 1

// Minimum allocation for strings
// This applies to strings and formulas
#define SHEET_MIN_STRLEN_EXP 4
#define SHEET_MAX_STRLEN_EXP 11
#define SHEET_MIN_STRLEN (1 << SHEET_MIN_STRLEN_EXP)
#define SHEET_MAX_STRLEN (1 << SHEET_MAX_STRLEN_EXP)
// Valid strlens are at each power of two between min and max (inclusive)
#define SHEET_NUM_STRLENS (SHEET_MAX_STRLEN_EXP - SHEET_MIN_STRLEN_EXP + 1)

// To convert from cell to chunk pos, simply divide each element
// by the corresponding chunk size
typedef struct {
    // Zero-based
    u32 row, col;
} sheet_cell_pos;

typedef struct {
    // Zero-based
    u32 row, col;
} sheet_chunk_pos;

typedef struct {
    sheet_cell_pos start;
    sheet_cell_pos end;
} sheet_cell_range;

typedef enum {
    SHEET_CELL_TYPE_NONE = 0,

    SHEET_CELL_TYPE_NUM,
    SHEET_CELL_TYPE_STRING,

    SHEET_CELL_TYPE_COUNT
} sheet_cell_type_enum;

// Used to ensure the size of the enum
typedef struct {
    u8 t;
} sheet_cell_type;

STATIC_ASSERT(SHEET_CELL_TYPE_COUNT < 255, cell_type_count);

typedef struct sheet_string {
    u32 size;
    u32 capacity;

    // For free list
    struct sheet_string* next;

    // Array of length capacity
    // Dynamically allocated with the struct
    u8 str[];
} sheet_string;

typedef struct {
    sheet_string* first;
    sheet_string* last;

    // Capcity of each string stored in the list
    u32 str_capacity;
} sheet_string_list;

typedef struct {
    sheet_cell_pos pos;

    sheet_cell_type* type;
    f64* num;
    sheet_string* str;
} sheet_cell_ref;

typedef struct sheet_chunk {
    sheet_chunk_pos pos;
    // Computed exclusively from chunk_row and chunk_col;
    u64 hash;

    // Used for hash collisions and free lists
    struct sheet_chunk* next;

    // Cell data, all stored column major
    sheet_cell_type cell_types[SHEET_CHUNK_SIZE];
    f64 cell_nums[SHEET_CHUNK_SIZE];
    sheet_string* cell_strings[SHEET_CHUNK_SIZE];
} sheet_chunk;

typedef struct {
    sheet_chunk** chunks;
    u32 num_chunks;
} sheet_chunk_arr;

// Stores the actual data inside each sheet
// Akin to vim buffers
typedef struct sheet_buffer {
    string8 name;

    // Used for global buffer list and buffer free list
    struct sheet_buffer* next;
    struct sheet_buffer* prev;

    u32 map_capacity;
    u32 num_chunks;

    // Mapping chunk positions -> chunk pointers
    sheet_chunk** chunk_map;

    u32 num_column_widths;
    u32 num_row_heights;

    // These two should not be accessed directly
    // Currently stored as numbers of characcters
    u16* _column_widths;
    // Currently stored as numbers of characcters
    u8* _row_heights;
} sheet_buffer;

sheet_chunk* sheet_get_chunk(
    workbook* wb, sheet_buffer* sheet, b32 create_if_empty
);

// Chunks will be returned column-major in the array,
// but there may be less than expected if `create_if_empty` is false
sheet_chunk_arr sheet_get_chunks_range(
    workbook* wb, sheet_buffer* sheet, sheet_cell_range range, b32 create_if_empty
);

sheet_cell_ref sheet_get_cell(
    workbook* wb, sheet_buffer* sheet, sheet_cell_pos pos, b32 create_if_empty
);

u16 sheet_get_col_width(sheet_buffer* sheet, u32 col);

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u16 width);

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row);

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height);


