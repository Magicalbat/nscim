
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

u64 plat_file_size(string8 file_name) {
    struct stat file_stats = { 0 };

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u8* name_cstr = str8_to_cstr(scratch.arena, file_name);
    i32 ret = stat((char*)name_cstr, &file_stats);

    arena_scratch_release(scratch);

    if (ret == -1) {
        //error_emitf("Failed to get size of file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return 0;
    }

    return (u64)file_stats.st_size;
}

string8 plat_file_read(mem_arena* arena, string8 file_name) {
    i32 fd = -1;

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u8* name_cstr = str8_to_cstr(scratch.arena, file_name);
    fd = open((char*)name_cstr, O_RDONLY);

    arena_scratch_release(scratch);

    if (fd == -1) {
        //error_emitf("Failed to open file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return (string8){ 0 };
    }

    string8 out = { 0 };

    struct stat file_stats = { 0 };
    if (fstat(fd, &file_stats) == -1) {
        //error_emitf("Failed to open file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        goto end;
    }

    if (!S_ISREG(file_stats.st_mode)) {
        //error_emitf("Incorrect mode for reading of file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        goto end;
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    out.size = (u64)file_stats.st_size;
    out.str = PUSH_ARRAY(maybe_temp.arena, u8, out.size);

    u64 str_pos = 0;

    while (str_pos < out.size) {
        i64 bytes_read = read(fd, out.str + str_pos, out.size - str_pos);

        if (bytes_read < 0) {
            //error_emitf("Failed to read from file \"%.*s\"", (int)file_name.size, (char*)file_name.str);

            arena_temp_end(maybe_temp);
            out = (string8){ 0 };
            goto end;
        }

        str_pos += (u64)bytes_read;
    }

end:
    close(fd);
    return out;
}

b32 plat_file_write(string8 file_name, const string8_list* list, b32 append) {
    i32 fd = -1;

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u8* name_cstr = str8_to_cstr(scratch.arena, file_name);
        i32 append_flag = append ? O_APPEND : O_TRUNC;
        fd = open((char*)name_cstr, O_CREAT | append_flag | O_WRONLY, S_IRUSR | S_IWUSR);

        arena_scratch_release(scratch);
    }

    if (fd == -1) {
        //error_emitf("Failed to open file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
        return false;
    }

    b32 out = true;

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 full_file = str8_concat_simple(scratch.arena, list);

        u64 total_written = 0;
        while (total_written < full_file.size) {
            i64 written = write(fd, full_file.str + total_written, full_file.size - total_written);

            if (written < 0) {
                //error_emitf("Failed to write to file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
                out = false;
                break;
            }

            total_written += (u64)written;
        }

        arena_scratch_release(scratch);
    }

    close(fd);

    return out;
}

b32 plat_file_delete(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u8* name_cstr = str8_to_cstr(scratch.arena, file_name);
    i32 ret = remove((char*)name_cstr);

    arena_scratch_release(scratch);

    if (ret == -1) {
        //error_emitf("Failed to delete file \"%.*s\"", (int)file_name.size, (char*)file_name.str);
    }

    return ret == 0;
}

