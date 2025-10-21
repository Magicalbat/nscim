
#if defined(PLATFORM_WIN32)

#define UNICODE
#include <windows.h>
#include <bcrypt.h>

#elif defined(PLATFORM_LINUX)


#endif



void plat_exit(i32 code);

u32 plat_page_size(void);

void* plat_mem_reserve(u64 size);
b32 plat_mem_commit(void* ptr, u64 size);
b32 plat_mem_decommit(void* ptr, u64 size);
b32 plat_mem_release(void* ptr);

void plat_get_entropy(void* data, u64 size);

