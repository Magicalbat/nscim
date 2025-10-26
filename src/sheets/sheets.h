
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
#define SHEET_MIN_STRLEN (1 << 4)
// This applies to strings and formulas
#define SHEET_MAX_STRLEN (1 << 11)
// Valid strlens are at each power of two between min and max (inclusive)
#define SHEET_NUM_STRLENS 8

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
    u8* str;

    u32 size;
    u32 capacity;
} sheet_string;

#define SHEET_STR_BUCKET_SIZE 16

// Used for string free lists
typedef struct sheet_string_bucket {
    struct sheet_string_bucket* next;

    u32 num_strings;

    sheet_string strings[SHEET_STR_BUCKET_SIZE];
} sheet_string_bucket;

// Used for string free lists
typedef struct {
    sheet_string_bucket* first;
    sheet_string_bucket* last;

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
    sheet_string cell_strings[SHEET_CHUNK_SIZE];
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
    // Currently stored as numbers of characcters
    u16* column_widths;
    // Currently stored as numbers of characcters
    u8* row_heights;
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

    // Left or top child
    struct sheet_window* child0;
    // Right or bottom child
    struct sheet_window* child1;

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

    sheet_buffer* buffer;

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

    // Windows
    sheet_window* root_win;
    sheet_window* active_win;

    sheet_window* first_free_win;
    sheet_window* last_free_win;

    // Buffers
    u32 num_buffers;
    sheet_buffer* first_buffer;
    sheet_buffer* last_buffer;

    sheet_buffer* first_free_buffer;
    sheet_buffer* last_free_buffer;

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

sheet_buffer* wb_create_buffer(workbook* wb);
sheet_buffer* wb_get_buffer(workbook* wb, string8 name);
void wb_free_buffer(workbook* wb, sheet_buffer* buffer);

sheet_chunk* wb_create_chunk(workbook* wb);
void wb_free_chunk(workbook* wb, sheet_chunk* chunk);

// Size must be a power of two between SHEET_MIN_STRLEN
// and SHEET_MAX_STRLEN (inclusive)
// Invalid sizes will get rounded up and capped at SHEET_MAX_STRLEN
sheet_string wb_create_string(workbook* wb, u32 size);
void wb_free_string(workbook* wb, sheet_string str);

// Both of these operate on the active window
void wb_win_split(workbook* wb, sheet_window_split split);
void wb_win_close(workbook* wb);
// Will attempt to increment the current win's width or height
void wb_win_inc_width(workbook* wb, u32 amount);
void wb_win_inc_height(workbook* wb, u32 amount);

void wb_win_update_sizes(workbook* wb, u32 total_width, u32 total_height);

sheet_chunk* sheet_get_chunk(sheet_buffer* sheet, b32 create_if_empty);
sheet_chunk_arr sheet_get_chunks_range(sheet_buffer* sheet, sheet_cell_range range, b32 create_if_empty);
sheet_cell_ref sheet_get_cell(sheet_buffer* sheet, sheet_cell_pos pos, b32 create_if_empty);

u32 sheet_get_col_width(sheet_buffer* sheet, u32 col);
void sheet_set_col_width(sheet_buffer* sheet, u32 col, u32 width);
u32 sheet_get_row_height(sheet_buffer* sheet, u32 row);
void sheet_set_row_height(sheet_buffer* sheet, u32 row, u32 height);

