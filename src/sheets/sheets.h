
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

typedef enum {
    SHEET_WIN_SPILT_VERT,
    SHEET_WIN_SPILT_HORZ,
} sheet_window_split_enum;

// Used to ensure the enum size
typedef struct {
    u32 s;
} sheet_window_split;

// Stores information about how the user is editing a sheet
// Akin to vim windows
typedef struct sheet_window {
    struct sheet_window* parent;

    // For free-list
    struct sheet_window* next;

    // Union to make operations on both children easier
    union {
        struct {
            // Left or top child
            struct sheet_window* child0;
            // Right or bottom child
            struct sheet_window* child1;
        };

        struct sheet_window* children[2];
    };

    // Fraction of parent's size the window takes up
    // along the axis perpendicular to the split
    f64 parent_fraction;

    // Top left of window, in characters
    u32 start_x;
    u32 start_y;
    // Total characters taken up by the entire view of the window
    u32 width;
    u32 height;

    sheet_window_split split_dir;

    // If the window is internal, it only exists to hold its 
    // children and should not reference a buffer 
    b32 internal;

    // Everything below is only for non-internal windows

    // Buffer should not be accessed directly
    // Use `wb_win_get_sheet`
    sheet_buffer* _sheet;

    // Top-left scroll pos
    sheet_cell_pos scroll_pos;
    // Current position of cursor
    sheet_cell_pos cursor_pos;
    // Position where selection started
    sheet_cell_pos select_pos;

    // Input line info (for entering cell data)
    u32 input_size;
    u32 input_cursor;
    u32 input_select_start;
    u8 input_buf[SHEET_MAX_STRLEN];
} sheet_window;

#define WORKBOOK_RESERVE_SIZE MiB(256)
#define WORKBOOK_COMMIT_SIZE MiB(4)

// Workbooks store the state of all open sheets and windows
// All allocations for sheets (besides buffers themselves)
// are done on the workbook arena, so free-list can be shared
// between open sheets and windows
typedef struct workbook {
    // This arena stores everything but the buffer allocations
    // and the scratch chunks array (the empty buffer is stored
    // on this arena)
    mem_arena* arena;

    // Used when opening a new window or empty workbook
    // Cannot actually store any data
    sheet_buffer* empty_buffer;

    // Stores the number of total window nodes
    // Some may be internal windows
    u32 num_windows;
    sheet_window* root_win;
    // Active win must not be internal
    sheet_window* active_win;

    sheet_window* first_free_win;
    sheet_window* last_free_win;

    u32 num_sheets;
    sheet_buffer* first_sheet;
    sheet_buffer* last_sheet;

    sheet_buffer* first_free_sheet;
    sheet_buffer* last_free_sheet;

    // Used when sheet_buffers need to rehash
    // Allocated on its own
    // Can only be used by one buffer at a time
    sheet_chunk** scratch_chunks;
    u64 scratch_chunks_reserve;
    u64 scratch_chunks_commit;

    // TODO: copy/paste and undo/redo buffers

    sheet_chunk* first_free_chunk;
    sheet_chunk* last_free_chunk;

    sheet_string_list free_strings[SHEET_NUM_STRLENS];

    u32 cmd_size;
    u32 cmd_cursor;
    u32 cmd_select_start;
    u8 cmd_buf[SHEET_MAX_STRLEN];
} workbook;

workbook* wb_create(void);
void wb_destroy(workbook* wb);

sheet_buffer* wb_create_sheet_buffer(workbook* wb);
sheet_buffer* wb_get_sheet_buffer(workbook* wb, string8 name);
void wb_free_sheet_buffer(workbook* wb, sheet_buffer* sheet);

sheet_chunk* wb_create_chunk(workbook* wb);
void wb_free_chunk(workbook* wb, sheet_chunk* chunk);

// Size must be a power of two between SHEET_MIN_STRLEN
// and SHEET_MAX_STRLEN (inclusive)
// Invalid sizes will get rounded up and capped at SHEET_MAX_STRLEN
sheet_string* wb_create_string(workbook* wb, u32 capacity);
void wb_free_string(workbook* wb, sheet_string* str);

sheet_buffer* wb_win_get_sheet(workbook* wb, sheet_window* win, b32 create_if_empty);
// All of these below operate on the active window
// If `open_in_both` is true, both children windows will have the same
// buffer open. Otherwise, only the first child will
void wb_win_split(workbook* wb, sheet_window_split split, b32 open_in_both);
void wb_win_close(workbook* wb);

void wb_win_compute_sizes(workbook* wb, u32 total_width, u32 total_height);

// Will attempt to increment the current win's width or height
// Must call after calling `wb_win_compute_sizes`
void wb_win_inc_width(workbook* wb, u32 amount);
void wb_win_inc_height(workbook* wb, u32 amount);

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

