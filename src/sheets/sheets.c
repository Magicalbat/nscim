
// These are internal functions, do not call directly
sheet_buffer* _sheet_buffer_create(void);
// Clears and decommits memory so the buffer can be reused
void _sheet_buffer_reset(sheet_buffer* sheet);
void _sheet_buffer_destroy(sheet_buffer* sheet);

// Only one buffer can the scratch chunks at a time
sheet_chunk** _wb_get_scratch_chunks(workbook* wb, u64 ensure_size);

#include "sheets_workbook.c"
#include "sheets_window.c"
#include "sheets_sheet_buffer.c"

