
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

u64 plat_file_size(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    WIN32_FILE_ATTRIBUTE_DATA attribs = { 0 };
    b32 ret = GetFileAttributesExW(file_name16.str, GetFileExInfoStandard, &attribs);

    arena_scratch_release(scratch);

    if (ret == false) {
        //error_emitf("Failed to get size of file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return 0;
    }

    return ((u64)attribs.nFileSizeHigh << 32) | (u64)attribs.nFileSizeLow;
}

string8 plat_file_read(mem_arena* arena, string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);

    HANDLE file_handle = CreateFileW(
        (LPCWSTR)file_name16.str, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );

    arena_scratch_release(scratch);

    if (file_handle == INVALID_HANDLE_VALUE) {
        //error_emitf("Failed to open file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return (string8){ 0 };
    }

    LARGE_INTEGER file_size = { 0 };
    if (!GetFileSizeEx(file_handle, &file_size)) {
        //error_emitf("Failed to get size of file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return (string8){ 0 };
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    string8 out = { 
        .size = (u64)file_size.QuadPart,
        .str = PUSH_ARRAY(maybe_temp.arena, u8, (u64)file_size.QuadPart)
    };

    u64 total_read = 0;
    while (total_read < out.size) {
        u64 to_read = out.size - total_read;
        DWORD to_read_capped = (DWORD)MIN(to_read, (u64)MAX_U32);

        DWORD cur_read = 0;
        if (!ReadFile(file_handle, out.str + total_read, to_read_capped, &cur_read, NULL)) {
            //error_emitf("Failed to read from file \"%.*s\"", (int)file_name.size, (char*)file_name.str);

            arena_temp_end(maybe_temp);
            out = (string8){ 0 };

            break;
        }

        total_read += cur_read;
    }

    CloseHandle(file_handle);

    return out;
}

b32 plat_file_write(string8 file_name, const string8_list* list, b32 append) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    HANDLE file_handle = CreateFileW(
        (LPCWSTR)file_name16.str,
        append ? FILE_APPEND_DATA : GENERIC_WRITE,
        0, NULL,
        append ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    arena_scratch_release(scratch);

    if (file_handle == INVALID_HANDLE_VALUE) {
        //error_emitf("Failed to open file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return false;
    }

    b32 out = true;

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 full_file = str8_concat_simple(scratch.arena, list);

        u64 total_written = 0;
        while (total_written < full_file.size) {
            u64 to_write = full_file.size - total_written;
            DWORD to_write_capped = (DWORD)MIN(to_write, (u64)MAX_U32);

            DWORD written = 0;
            if (!WriteFile(file_handle, full_file.str + total_written, to_write_capped, &written, NULL)) {
                //error_emitf("Failed to write to file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
                out = false;

                break;
            }

            total_written += written;
        }

        arena_scratch_release(scratch);
    }

    CloseHandle(file_handle);

    return out;
}

b32 plat_file_delete(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    b32 ret = DeleteFileW((LPCWSTR)file_name16.str);

    arena_scratch_release(scratch);

    if (!ret) {
        //error_emitf("Failed to delete file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
    }

    return ret;
}

