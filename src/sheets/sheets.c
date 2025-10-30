
// These are internal functions, do not call directly
sheet_buffer* _sheet_buffer_create(void);
// Clears and decommits memory so the buffer can be reused
void _sheet_buffer_reset(workbook* wb, sheet_buffer* sheet);
void _sheet_buffer_destroy(workbook* wb, sheet_buffer* sheet);

// These functions do not modify the window tree in the workbook
// These are internal only and all window interactions should
// use the wb_win_* functions
sheet_window* _wb_create_win(workbook* wb);
void _wb_free_win(workbook* wb, sheet_window* win);

// Only one buffer can the scratch chunks at a time
// `ensure_size` should be in bytes
sheet_chunk** _wb_get_scratch_chunks(workbook* wb, u64 ensure_size);

#include "sheets_workbook.c"
#include "sheets_window.c"
#include "sheets_sheet_buffer.c"

