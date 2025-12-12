
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

