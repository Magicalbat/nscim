
void editor_reg_create(editor_register* reg, editor_register_type reg_type) {
    reg->reg_type = reg_type;

    if (
        reg->reg_type == EDITOR_REGISTER_TYPE_INVALID ||
        reg->reg_type == EDITOR_REGISTER_TYPE_BLACKHOLE
    ) {
        return;
    }

    reg->arena = arena_create(
        EDITOR_REGISTER_ARENA_RESERVE,
        EDITOR_REGISTER_ARENA_COMMIT,
        ARENA_FLAG_GROWABLE | ARENA_FLAG_DECOMMIT
    );
}

void editor_reg_destroy(editor_register* reg) {
    if (reg->arena != NULL) {
        arena_destroy(reg->arena);
    }
}

void editor_reg_set_cells(
    editor_register* reg, sheet_buffer* sheet, sheet_range in_range
) {
    if (
        reg == NULL ||
        reg->reg_type == EDITOR_REGISTER_TYPE_INVALID ||
        reg->reg_type == EDITOR_REGISTER_TYPE_BLACKHOLE
    ) {
        return;
    }

    sheet_range range = sheets_fix_range(in_range);

    reg->reg_type = EDITOR_REGISTER_TYPE_CELLS;
    arena_clear(reg->arena);

    reg->orig_pos = range.start;

    reg->num_rows = range.end.row - range.start.row + 1;
    reg->num_cols = range.end.col - range.start.col + 1;

    // TODO: column width and row heights


    _editor_register_cells* cells = &reg->content.cells;

    u32 num_cells = reg->num_rows * reg->num_cols;

    cells->types = PUSH_ARRAY_NZ(reg->arena, sheet_cell_type, num_cells);

    void* values = arena_push(reg->arena, sizeof(f64) * num_cells, false);

    cells->nums = (f64*)values;
    cells->strings = (string8**)values;

    // TODO: make this faster by accessing chunks directly
    for (u32 col = range.start.col; col <= range.end.col; col++) {
        for (u32 row = range.start.row; row <= range.end.row; row++) {
            sheet_cell_view cell_view = sheet_get_cell_view(
                NULL, sheet, (sheet_pos){ row, col }
            );

            u32 index = row + col * reg->num_rows;

            switch (cell_view.type) {
                case SHEET_CELL_TYPE_EMPTY:
                case SHEET_CELL_TYPE_EMPTY_CHUNK: {
                    cells->types[index] = SHEET_CELL_TYPE_EMPTY;
                } break;

                case SHEET_CELL_TYPE_NUM: {
                    cells->types[index] = SHEET_CELL_TYPE_NUM;
                    cells->nums[index] = cell_view.num;
                } break;

                case SHEET_CELL_TYPE_STRING: {
                    cells->types[index] = SHEET_CELL_TYPE_STRING;

                    const sheet_string* str = cell_view.str;

                    string8* new_str = PUSH_STRUCT_NZ(reg->arena, string8);
                    new_str->size = str->size;
                    new_str->str = PUSH_ARRAY_NZ(reg->arena, u8, str->size);

                    memcpy(new_str->str, str->str, str->size);

                    cells->strings[index] = new_str;
                } break;
            }
        }
    }
}

