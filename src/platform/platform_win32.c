
static u64 _w32_perf_freq = 0;

void plat_init(void) {
    LARGE_INTEGER perf_freq = { 0 };

    if (!QueryPerformanceFrequency(&perf_freq)) {
        fprintf(stderr, "Failed to query performance frequency\n");
    } else {
        _w32_perf_freq = (u64)perf_freq.QuadPart;
    }
}

void plat_exit(i32 code) {
    ExitProcess((u32)code);
}

void plat_fatal_error(const char* msg, i32 code) {
    MessageBoxA(NULL, msg, "Fatal Error", MB_OK | MB_ICONERROR);
    ExitProcess((u32)code);
}

void plat_sleep_ms(u32 ms) {
    Sleep(ms);
}

u64 plat_now_usec(void) {
    LARGE_INTEGER ticks = { 0 };

    if (!QueryPerformanceFrequency(&ticks)) {
        return 0;
    }

    return (u64)ticks.QuadPart * 1000000 / _w32_perf_freq;
}

u32 plat_page_size(void) {
    SYSTEM_INFO sysinfo = { 0 };
    GetSystemInfo(&sysinfo);

    return sysinfo.dwPageSize;
}

void* plat_mem_reserve(u64 size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

b32 plat_mem_commit(void* ptr, u64 size) {
    void* ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return ret != NULL;
}

b32 plat_mem_decommit(void* ptr, u64 size) {
    return VirtualFree(ptr, size, MEM_DECOMMIT);
}

b32 plat_mem_release(void* ptr, u64 size) {
    return VirtualFree(ptr, size, MEM_RELEASE);
}

void plat_get_entropy(void* data, u64 size) {
    BCryptGenRandom(
        NULL, data, (u32)(size & (~(u32)0)),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
}

