
/*

Sheet Buffer Memory Allocation Scheme

- Each sheet buffer reserves its own section of virtual address space
- In this region, the sheet buffer structure itself is stored,
    as well as three dynamic arrays:
    1) `chunk_map`, which is the storage for the chunk hash map
    2) `column_widths`, which stores the column widths
    3) `row_heights`, which stores the row heights
- Each of these three arrays will dynamically commit virtual
    address space as needed so as not to use too much memory
    on small sheets
- The maximum size of each array is a power of two, such that they
    should fit nicely on page boundaries
- The first page of the buffer only contains the sheet_buffer struct
    itself followed by page_size - sizeof(sheet_buffer) padding bytes
- All of the offsets are only computed once and stored in a static struct

*/


sheet_buffer* _sheet_buffer_create(void) {
}

void _sheet_buffer_reset(sheet_buffer* sheet);

void _sheet_buffer_destroy(sheet_buffer* sheet) {
}

sheet_chunk* sheet_get_chunk(sheet_buffer* sheet, b32 create_if_empty);

sheet_chunk_arr sheet_get_chunks_range(sheet_buffer* sheet, sheet_cell_range range, b32 create_if_empty);

sheet_cell_ref sheet_get_cell(sheet_buffer* sheet, sheet_cell_pos pos, b32 create_if_empty);

u32 sheet_get_col_width(sheet_buffer* sheet, u32 col);

void sheet_set_col_width(sheet_buffer* sheet, u32 col, u32 width);

u32 sheet_get_row_height(sheet_buffer* sheet, u32 row);

void sheet_set_row_height(sheet_buffer* sheet, u32 row, u32 height);


