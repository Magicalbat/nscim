
// TODO: clz versions of applicable functions for clang and gcc

// https://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers
const u32 _log2_tab32[32] = {
     0,  9,  1, 10, 13, 21,  2, 29,
    11, 14, 16, 18, 22, 25,  3, 30,
     8, 12, 20, 28, 15, 17, 24,  7,
    19, 27, 23,  6, 26,  5,  4, 31
};

u32 log2_u32 (u32 value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;

    return _log2_tab32[(u32)(value*0x07C4ACDD) >> 27];
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

