
u32 clz_u64(u64 n) {
    if (n == 0) { return 64; }

#if defined(COMPILER_CLANG) || defined (COMPILER_GCC)
    return (u32)__builtin_clzll(n);
#elif defined(COMPILER_MSVC)
    return __lzcnt64(n);
#else

    // Based on this article and stackoverflow post:
    // https://andrewlock.net/counting-the-leading-zeroes-in-a-binary-number/
    // https://stackoverflow.com/questions/10439242/count-leading-zeroes-in-an-int32/10439333#10439333
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    n -= n >> 1 & 0x5555555555555555ULL;
    n = (n >> 2 & 0x3333333333333333ULL) + (n & 0x3333333333333333ULL);
    n = (n >> 4) + n & 0x0f0f0f0f0f0f0f0fULL;
    n += n >> 8;
    n += n >> 16;
    n += n >> 32;

    return 64 - (n & 0x7f);
#endif
}

u32 clz_u32(u32 n) {
    if (n == 0) { return 32; }

#if defined(COMPILER_CLANG) || defined (COMPILER_GCC)
    return (u32)__builtin_clz(n);
#elif defined(COMPILER_MSVC)
    return __lzcnt(n);
#else

    // Based on this article and stackoverflow post:
    // https://andrewlock.net/counting-the-leading-zeroes-in-a-binary-number/
    // https://stackoverflow.com/questions/10439242/count-leading-zeroes-in-an-int32/10439333#10439333
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    n -= n >> 1 & 0x55555555;
    n = (n >> 2 & 0x33333333) + (n & 0x33333333);
    n = (n >> 4) + n & 0x0f0f0f0f;
    n += n >> 8;
    n += n >> 16;

    return 32 - (n & 0x3f);
#endif
}

u32 log2_u32(u32 value) {
    return 31 - clz_u32(value);
}

u32 round_up_pow2_u32(u32 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

u32 chars_from_u32(u32 n, u8* chars, u32 max_chars) {
    u32 size = 0;

    for (u32 i = 0; i < max_chars; i++) {
        chars[size++] = (n % 10) + '0';
        n /= 10;

        if (n == 0) {
            break;
        }
    }

    for (u32 i = 0; i < size/2; i++) {
        u8 tmp = chars[size-i-1];
        chars[size-i-1] = chars[i];
        chars[i] = tmp;
    }

    return size;
}

// Siphash adapted from 
// https://github.com/veorq/SipHash/blob/master/siphash.c

#define _SH_cROUNDS 2
#define _SH_dROUNDS 4

#define _SH_ROTL(x, b) (u64)(((x) << (b)) | ((x) >> (64 - (b))))

#define _SH_U32TO8_LE(p, v)                                               \
    (p)[0] = (u8)((v));                                                   \
    (p)[1] = (u8)((v) >> 8);                                              \
    (p)[2] = (u8)((v) >> 16);                                             \
    (p)[3] = (u8)((v) >> 24);

#define _SH_U64TO8_LE(p, v)                                               \
    U32TO8_LE((p), (u32)((v)));                                           \
    U32TO8_LE((p) + 4, (u32)((v) >> 32));

#define _SH_U8TO64_LE(p)                                             \
    (((u64)((p)[0])) | ((u64)((p)[1]) << 8) |                        \
     ((u64)((p)[2]) << 16) | ((u64)((p)[3]) << 24) |                 \
     ((u64)((p)[4]) << 32) | ((u64)((p)[5]) << 40) |                 \
     ((u64)((p)[6]) << 48) | ((u64)((p)[7]) << 56))

#define _SH_ROUND                                                              \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = _SH_ROTL(v1, 13);                                                 \
        v1 ^= v0;                                                              \
        v0 = _SH_ROTL(v0, 32);                                                 \
        v2 += v3;                                                              \
        v3 = _SH_ROTL(v3, 16);                                                 \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = _SH_ROTL(v3, 21);                                                 \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = _SH_ROTL(v1, 17);                                                 \
        v1 ^= v2;                                                              \
        v2 = _SH_ROTL(v2, 32);                                                 \
    } while (0)

u64 siphash(u8* data, u64 size, u64 k0, u64 k1) {
    u64 v0 = UINT64_C(0x736f6d6570736575);
    u64 v1 = UINT64_C(0x646f72616e646f6d);
    u64 v2 = UINT64_C(0x6c7967656e657261);
    u64 v3 = UINT64_C(0x7465646279746573);

    u64 m;
    i32 i;

    const u8 *end = data + size - (size % sizeof(u64));
    const int left = size & 7;
    u64 b = ((u64)size) << 56;

    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; data != end; data += 8) {
        m = _SH_U8TO64_LE(data);
        v3 ^= m;

        for (i = 0; i < _SH_cROUNDS; ++i)
            _SH_ROUND;

        v0 ^= m;
    }

    switch (left) {
        case 7:
            b |= ((u64)data[6]) << 48;
            /* FALLTHRU */
        case 6:
            b |= ((u64)data[5]) << 40;
            /* FALLTHRU */
        case 5:
            b |= ((u64)data[4]) << 32;
            /* FALLTHRU */
        case 4:
            b |= ((u64)data[3]) << 24;
            /* FALLTHRU */
        case 3:
            b |= ((u64)data[2]) << 16;
            /* FALLTHRU */
        case 2:
            b |= ((u64)data[1]) << 8;
            /* FALLTHRU */
        case 1:
            b |= ((u64)data[0]);
            break;
        case 0:
            break;
    }

    v3 ^= b;

    for (i = 0; i < _SH_cROUNDS; ++i)
        _SH_ROUND;

    v0 ^= b;

    v2 ^= 0xff;

    for (i = 0; i < _SH_dROUNDS; ++i)
        _SH_ROUND;

    b = v0 ^ v1 ^ v2 ^ v3;

    return b;
}

f32 exp_decay(f32 a, f32 b, f32 decay, f32 delta) {
    return b + (a - b) * expf(-decay * delta);
}

f32 exp_anim(f32 a, f32 b, f32 decay, f32 delta, f32 thresh) {
    f32 out = b + (a - b) * expf(-decay * delta);
    if (fabsf(b - out) < thresh) {
        out = b;
    }

    return out;
}

