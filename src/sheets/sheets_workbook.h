
#define SHEETS_WB_RESERVE_SIZE MiB(256)
#define SHEETS_WB_COMMIT_SIZE MiB(16)

// Workbooks store the state of all open sheets and windows
// All allocations for sheets (besides buffers themselves)
// are done on the workbook arena, so free-list can be shared
// between open sheets and windows
typedef struct workbook {
    // This is the main arena for most things in the workbook
    // Certain items like sheet buffers and the clipboard
    // are allocated individually
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
    
    sheet_chunk* first_free_chunk;
    sheet_chunk* last_free_chunk;

    sheet_string_list free_strings[SHEET_NUM_STRLENS];

    // Used when sheet_buffers need to rehash
    // Allocated on its own
    // Can only be used by one buffer at a time
    sheet_chunk** scratch_chunks;
    u64 scratch_chunks_reserve;
    u64 scratch_chunks_commit;
} workbook;

workbook* wb_create(void);
void wb_destroy(workbook* wb);

sheet_buffer* wb_get_active_sheet(workbook* wb, b32 create_if_empty);

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

