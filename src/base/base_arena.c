
mem_arena* arena_create(u64 reserve_size, u64 commit_size, u32 flags) {
    u32 page_size = plat_page_size();

    reserve_size = ALIGN_UP_POW2(reserve_size, page_size);
    commit_size = ALIGN_UP_POW2(commit_size, page_size);

    mem_arena* arena = plat_mem_reserve(reserve_size);

    if (plat_mem_commit(arena, commit_size) == false) {
        arena = NULL;
    }

    if (arena == NULL) {
        plat_fatal_error("Fatal error: unable to commit memory for arena", 1);
    }

    // TODO: ASAN stuff for memory

    arena->current = arena;
    arena->prev = NULL;

    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;

    arena->flags = flags;

    arena->base_pos = 0;
    arena->pos = ARENA_HEADER_SIZE;
    arena->commit_pos = commit_size;

    return arena;
}

void arena_destroy(mem_arena* arena) {
    mem_arena* current = arena->current;

    while (current != NULL) {
        mem_arena* prev = current->prev;
        plat_mem_release(current, current->reserve_size);

        current = prev;
    }
}

u64 arena_get_pos(mem_arena* arena) {
    return arena->current->base_pos + arena->current->pos;
}

void* arena_push(mem_arena* arena, u64 size, b32 non_zero) {
    void* out = NULL;

    mem_arena* current = arena->current;

    u64 pos_aligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
    out = (u8*)current + pos_aligned;
    u64 new_pos = pos_aligned + size;

    if (new_pos > current->reserve_size) {
        out = NULL;

        if (arena->flags & ARENA_FLAG_GROWABLE) {
            u64 reserve_size = arena->reserve_size;
            u64 commit_size = arena->commit_size;

            if (size + ARENA_HEADER_SIZE > reserve_size) {
                reserve_size = ALIGN_UP_POW2(
                    size + ARENA_HEADER_SIZE, ARENA_ALIGN
                );
            }

            mem_arena* new_arena = arena_create(
                reserve_size, commit_size, arena->flags
            );
            new_arena->base_pos = current->base_pos + current->reserve_size;

            mem_arena* prev_cur = current;
            current = new_arena;
            current->prev = prev_cur;
            arena->current = current;

            pos_aligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
            out = (u8*)current + pos_aligned;
            new_pos = pos_aligned + size;
        }
    }

    if (new_pos > current->commit_pos) {
        u64 new_commit_pos = new_pos;
        new_commit_pos += current->commit_size - 1;
        new_commit_pos -= new_commit_pos % current->commit_size;
        new_commit_pos = MIN(new_commit_pos, current->reserve_size);

        u64 commit_size = new_commit_pos - current->commit_pos;

        u8* commit_pointer = (u8*)current + current->commit_pos;

        if (!plat_mem_commit(commit_pointer, commit_size)) {
            out = NULL;
        } else {
            current->commit_pos = new_commit_pos;
        }
    }

    if (out == NULL) {
        plat_fatal_error("Fatal error: failed to allocate memory on arena", 1);
    }

    current->pos = new_pos;

    if (!non_zero) {
        MEM_ZERO(out, size);
    }

    return out;
}

void arena_pop(mem_arena* arena, u64 size) {
    size = MIN(size, arena_get_pos(arena));

    mem_arena* current = arena->current;
    while (current != NULL && size > current->pos) {
        mem_arena* prev = current->prev;

        size -= current->pos;
        plat_mem_release(current, current->reserve_size);

        current = prev;
    }

    arena->current = current;

    size = MIN(current->pos - ARENA_HEADER_SIZE, size);
    current->pos -= size;

    if (arena->flags & ARENA_FLAG_DECOMMIT) {
        u64 required_commit_pos = current->pos + current->commit_size - 1;
        required_commit_pos -= required_commit_pos % current->commit_size;

        if (required_commit_pos < arena->commit_pos) {
            u8* commit_pointer = (u8*)current + required_commit_pos;

            if (!plat_mem_decommit(
                commit_pointer, arena->commit_pos - required_commit_pos
            )) {
                plat_fatal_error(
                    "Fatal error: failed to decommit arena memory", 1
                );
            }

            arena->commit_pos = required_commit_pos;
        }
    }
}

void arena_pop_to(mem_arena* arena, u64 pos) {
    u64 cur_pos = arena_get_pos(arena);
    
    pos = MIN(pos, cur_pos);

    arena_pop(arena, cur_pos - pos);
}

void arena_clear(mem_arena* arena) {
    arena_pop_to(arena, ARENA_HEADER_SIZE);
}

mem_arena_temp arena_temp_begin(mem_arena* arena) {
    return (mem_arena_temp) {
        .arena = arena,
        .start_pos = arena_get_pos(arena)
    };
}

void arena_temp_end(mem_arena_temp temp) {
    arena_pop_to(temp.arena, temp.start_pos);
}

static THREAD_LOCAL mem_arena* scratch_arenas[2] = { 0 };

mem_arena_temp arena_scratch_get(mem_arena** conflicts, u32 num_conflicts) {
    i32 scratch_index = -1;

    for (i32 i = 0; i < 2; i++) {
        b32 conflict_found = false;

        for (u32 j = 0; j < num_conflicts; j++) {
            if (scratch_arenas[i] == conflicts[j]) {
                conflict_found = true;
                break;
            }
        }

        if (!conflict_found) {
            scratch_index = i;
            break;
        }
    }

    if (scratch_index == -1) {
        return (mem_arena_temp){ 0 };
    }

    if (scratch_arenas[scratch_index] == NULL) {
        scratch_arenas[scratch_index] = arena_create(
            ARENA_SCRATCH_RESERVE,
            ARENA_SCRATCH_COMMIT,
            0
        );
    }

    return arena_temp_begin(scratch_arenas[scratch_index]);
}

void arena_scratch_release(mem_arena_temp scratch) {
    arena_temp_end(scratch);
}

