
void editor_reg_create(
    editor_register* reg, editor_register_type reg_type, b32 create_arena
) {
    reg->reg_type = reg_type;

    if (
        reg->reg_type == EDITOR_REGISTER_TYPE_INVALID ||
        reg->reg_type == EDITOR_REGISTER_TYPE_BLACKHOLE
    ) {
        return;
    }

    if (create_arena) {
        reg->arena = arena_create(
            EDITOR_REGISTER_ARENA_RESERVE,
            EDITOR_REGISTER_ARENA_COMMIT,
            ARENA_FLAG_GROWABLE | ARENA_FLAG_DECOMMIT
        );
    }
}

void editor_reg_destroy(editor_register* reg) {
    if (reg->arena != NULL) {
        arena_destroy(reg->arena);
    }
}

void editor_reg_set_cells(
    mem_arena* arena, editor_register* reg,
    sheet_buffer* sheet, sheet_range in_range
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

    if (arena == NULL) { arena_clear(reg->arena); }

    reg->orig_pos = range.start;

    reg->num_rows = range.end.row - range.start.row + 1;
    reg->num_cols = range.end.col - range.start.col + 1;

    // TODO: column width and row heights

    _editor_register_cells* cells = &reg->content.cells;

    u32 num_cells = reg->num_rows * reg->num_cols;

    mem_arena* selected_arena = arena == NULL ? reg->arena : arena;

    cells->types = PUSH_ARRAY_NZ(selected_arena, sheet_cell_type, num_cells);
    void* values = arena_push(selected_arena, sizeof(f64) * num_cells, false);

    cells->nums = (f64*)values;
    cells->strings = (string8**)values;

    // TODO: make this faster by accessing chunks directly
    for (u32 col = range.start.col; col <= range.end.col; col++) {
        for (u32 row = range.start.row; row <= range.end.row; row++) {
            sheet_cell_view cell_view = sheet_get_cell_view(
                NULL, sheet, (sheet_pos){ row, col }
            );

            u32 index = (row - range.start.row) +
                (col - range.start.col) * reg->num_rows;

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

                    string8* new_str = PUSH_STRUCT_NZ(selected_arena, string8);
                    new_str->size = str->size;
                    new_str->str = PUSH_ARRAY_NZ(selected_arena, u8, str->size);

                    memcpy(new_str->str, str->str, str->size);

                    cells->strings[index] = new_str;
                } break;
            }
        }
    }
}

void editor_reg_put_cells(
    editor_register* reg, workbook* wb,
    sheet_buffer* sheet, sheet_pos start_pos
) {
    if (reg == NULL || reg->reg_type != EDITOR_REGISTER_TYPE_CELLS) {
        return;
    }

    _editor_register_cells* cells = &reg->content.cells;

    for (u32 col = 0; col < reg->num_cols; col++) {
        for (u32 row = 0; row < reg->num_rows; row++) {
            u32 index = row + col * reg->num_rows;

            sheet_pos pos = {
                start_pos.row + row,
                start_pos.col + col
            };

            switch (cells->types[index]) {
                case SHEET_CELL_TYPE_EMPTY:
                case SHEET_CELL_TYPE_EMPTY_CHUNK: {
                    sheet_clear_cell(wb, sheet, pos);
                } break;

                case SHEET_CELL_TYPE_NUM: {
                    sheet_set_cell_num(wb, sheet, pos, cells->nums[index]);
                } break;

                case SHEET_CELL_TYPE_STRING: {
                    sheet_set_cell_str(wb, sheet, pos, *cells->strings[index]);
                } break;
            }
        }
    }
}

