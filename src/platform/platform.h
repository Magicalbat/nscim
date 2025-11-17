
#if defined(PLATFORM_WIN32)

#define UNICODE
#include <windows.h>
#include <bcrypt.h>

#elif defined(PLATFORM_LINUX)

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#endif

void plat_init(void);

void plat_exit(i32 code);
void plat_fatal_error(const char* msg, i32 code);

void plat_sleep_ms(u32 ms);

u64 plat_now_usec(void);

u32 plat_page_size(void);

void* plat_mem_reserve(u64 size);
b32 plat_mem_commit(void* ptr, u64 size);
b32 plat_mem_decommit(void* ptr, u64 size);
b32 plat_mem_release(void* ptr, u64 size);

void plat_get_entropy(void* data, u64 size);

