
#define ARENA_ALIGN sizeof(void*)
#define ARENA_NUM_SCRATCH 2

#define ARENA_SCRATCH_RESERVE MiB(64)
#define ARENA_SCRATCH_COMMIT KiB(64)

typedef struct arena {
    struct arena* current;
    struct arena* prev;

    u64 reserve_size;
    u64 commit_size;
    b32 growable;

    u64 base_pos;
    u64 pos;
    u64 commit_pos;
} mem_arena;

typedef struct {
    mem_arena* arena;
    u64 start_pos;
} mem_arena_temp;

#define PUSH_STRUCT(arena, T) (T*)arena_push((arena), sizeof(T), false)
#define PUSH_STRUCT_NZ(arena, T) (T*)arena_push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T*)arena_push((arena), sizeof(T) * (n), false)
#define PUSH_ARRAY_NZ(arena, T, n) (T*)arena_push((arena), sizeof(T) * (n), true)

mem_arena* arena_create(u64 reserve_size, u64 commit_size, b32 growable);
void arena_destroy(mem_arena* arena);
u64 arena_get_pos(mem_arena* arena);
void* arena_push(mem_arena* arena, u64 size, b32 non_zero);
void arena_pop(mem_arena* arena, u64 size);
void arena_pop_to(mem_arena* arena, u64 pos);

mem_arena_temp arena_temp_begin(mem_arena* arena);
void arena_temp_end(mem_arena_temp temp);

mem_arena_temp arena_scratch_get(mem_arena** conflicts, u32 num_conflicts);
void arena_scratch_release(mem_arena_temp scratch);

