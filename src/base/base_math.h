
u32 log2_u32(u32 n);

u32 round_up_pow2_u32(u32 n);

// Returns the number of chars written
u32 chars_from_u32(u32 n, u8* chars, u32 max_chars);

// k0 and k1 are the keys for the hashing
u64 siphash(u8* data, u64 size, u64 k0, u64 k1);

