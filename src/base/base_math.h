
#define MAX_U32 (~(u32)(0))

u32 clz_u64(u64 n);
u32 clz_u32(u32 n);

u32 log2_u32(u32 n);

u32 round_up_pow2_u32(u32 n);

// Returns the number of chars written
u32 chars_from_u32(u32 n, u8* chars, u32 max_chars);

// k0 and k1 are the keys for the hashing
u64 siphash(u8* data, u64 size, u64 k0, u64 k1);

f32 exp_decay(f32 a, f32 b, f32 decay, f32 delta);
f32 exp_anim(f32 a, f32 b, f32 decay, f32 delta, f32 thresh);

