
#define SHEET_MAX_COLS KiB(32)
#define SHEET_MAX_ROWS MiB(2)

// Maximum number of characters is takes to represent the max row/col
#define SHEET_MAX_COL_CHARS 4
#define SHEET_MAX_ROW_CHARS 7

#define SHEET_CHUNK_COLS 16
#define SHEET_CHUNK_ROWS 128
#define SHEET_CHUNK_SIZE (SHEET_CHUNK_COLS * SHEET_CHUNK_ROWS)
#define SHEET_CHUNKS_X (SHEET_MAX_COLS / SHEET_CHUNK_COLS)
#define SHEET_CHUNKS_Y (SHEET_MAX_ROWS / SHEET_CHUNK_ROWS)
#define SHEET_MAX_CHUNKS (SHEET_CHUNKS_X * SHEET_CHUNKS_Y)

#define SHEET_META_CHUNK_COLS 4
#define SHEET_META_CHUNK_ROWS 4
#define SHEET_META_CHUNK_SIZE (SHEET_META_CHUNK_ROWS * SHEET_META_CHUNK_COLS)
#define SHEET_META_CHUNKS_X (SHEET_CHUNKS_X / SHEET_META_CHUNK_COLS)
#define SHEET_META_CHUNKS_Y (SHEET_CHUNKS_Y / SHEET_META_CHUNK_ROWS)

#define SHEET_DEF_COL_WIDTH 10
#define SHEET_DEF_ROW_HEIGHT 1

#define SHEET_MAX_COL_WIDTH 255
#define SHEET_MAX_ROW_HEIGHT 255

// Minimum allocation for strings
// This applies to strings
#define SHEET_MIN_STRLEN_EXP 4
#define SHEET_MAX_STRLEN_EXP 16
#define SHEET_MIN_STRLEN (1 << SHEET_MIN_STRLEN_EXP)
#define SHEET_MAX_STRLEN (1 << SHEET_MAX_STRLEN_EXP)
// Valid strlens are at each power of two between min and max (inclusive)
#define SHEET_NUM_STRLENS (SHEET_MAX_STRLEN_EXP - SHEET_MIN_STRLEN_EXP + 1)

// To convert from cell to chunk pos, simply divide each element
// by the corresponding chunk size
typedef struct {
    // Zero-based
    u32 row, col;
} sheet_pos;

typedef struct {
    // Zero-based
    u32 row, col;
} sheet_chunk_pos;

typedef union {
    struct {
        sheet_pos start;
        sheet_pos end;
    };
    sheet_pos cells[2];
} sheet_range;

typedef enum {
    SHEET_CELL_TYPE_EMPTY = 0,

    SHEET_CELL_TYPE_NUM,
    SHEET_CELL_TYPE_STRING,

    SHEET_CELL_TYPE_EMPTY_CHUNK,

    _SHEET_CELL_TYPE_COUNT
} sheet_cell_type_enum;

typedef u8 sheet_cell_type;

STATIC_ASSERT(_SHEET_CELL_TYPE_COUNT < (1 << (sizeof(sheet_cell_type) * 8)), cell_type_count);

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
    sheet_cell_type type;

    union {
        f64 num;
        const sheet_string* str;
    };
} sheet_cell_view;

typedef struct sheet_chunk {
    sheet_chunk_pos pos;
    // Computed exclusively from chunk_row and chunk_col;
    u64 hash;

    // Used for hash collisions and free lists
    struct sheet_chunk* next;

    // Number of non-empty cells
    u32 set_cell_count;

    // Cell data, all stored column major

    sheet_cell_type types[SHEET_CHUNK_SIZE];

    union {
        f64 nums[SHEET_CHUNK_SIZE];
        sheet_string* strings[SHEET_CHUNK_SIZE];
    };
} sheet_chunk;

typedef struct {
    sheet_chunk** chunks;
    u32 size;
} sheet_chunk_arr;

// Stores the actual data inside each sheet
// Akin to vim buffers
typedef struct sheet_buffer {
    string8 name;

    // Used for global buffer list and buffer free list
    struct sheet_buffer* next;
    struct sheet_buffer* prev;

    // Bitfield of committed pages
    // 0 - reserved, 1 - committed
    u64* page_bitfield;

    // Start of dynamic pages (specified by page_bitfield)
    // As of now, equal to chunks pointer
    u8* dynamic_mem;

    // Mapping chunk positions -> chunk pointers
    sheet_chunk** chunks;

    // Currently stored as numbers of characcters
    u8* column_widths;
    // Currently stored as numbers of characcters
    u8* row_heights;
} sheet_buffer;

// `wb` can be NULL if create_if_empty is false
sheet_chunk* sheet_get_chunk(
    workbook* wb, sheet_buffer* sheet,
    sheet_chunk_pos chunk_pos, b32 create_if_empty
);

// Returns the chunk that the cell is in 
sheet_chunk* sheet_get_cells_chunk(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, b32 create_if_empty
);

// Chunks will be returned column-major in the array,
// but there may be less than expected if `create_if_empty` is false
sheet_chunk_arr sheet_get_range(
    mem_arena* arena, workbook* wb, sheet_buffer* sheet,
    sheet_range cell_range, b32 create_if_empty
);

sheet_cell_view sheet_get_cell_view(
    workbook* wb, sheet_buffer* sheet, sheet_pos pos
);

b32 sheet_is_cell_empty(sheet_buffer* sheet, sheet_pos pos);

void sheet_set_cell_num(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, f64 num
);

void sheet_set_cell_str(
    workbook* wb, sheet_buffer* sheet,
    sheet_pos pos, string8 str
);

void sheet_clear_cell(workbook* wb, sheet_buffer* sheet, sheet_pos pos);

void sheet_clear_range(workbook* wb, sheet_buffer* sheet, sheet_range range);

u8 sheet_get_col_width(sheet_buffer* sheet, u32 col);

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u8 width);

u8 sheet_get_row_height(sheet_buffer* sheet, u32 row);

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u8 height);


