
void plat_exit(i32 code) {
    exit(code);
}

void plat_fatal_error(const char* msg, i32 code) {
    fprintf(stderr, "\x1b[31m%s\n\x1b[39m", msg);
    exit(code);
}

#define _POSIX_C_SOURCE 200809L 
#include <time.h>

void plat_sleep_ms(u32 ms) {
    struct timespec dur = { .tv_nsec = (i64)ms * 1000000 };
    struct timespec remaining = { 0 };
    nanosleep(&dur, &remaining);
}

u32 plat_page_size(void) {
    return (u32)sysconf(_SC_PAGESIZE);
}

void* plat_mem_reserve(u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (out == MAP_FAILED) {
        return NULL;
    }
    return out;
}

b32 plat_mem_commit(void* ptr, u64 size) {
    i32 ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return ret == 0;
}

b32 plat_mem_decommit(void* ptr, u64 size) {
    i32 ret = mprotect(ptr, size, PROT_NONE);
    if (ret != 0) return false;
    ret = madvise(ptr, size, MADV_DONTNEED);
    return ret == 0;
}

b32 plat_mem_release(void* ptr, u64 size) {
    i32 ret = munmap(ptr, size);
    return ret == 0;
}

void plat_get_entropy(void* data, u64 size) {
    getentropy(data, size);
}

