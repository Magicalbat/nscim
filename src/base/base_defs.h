
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(__linux__)
#   define PLATFORM_LINUX
#elif defined(_WIN32)
#   define PLATFORM_WIN32
#endif

#if defined(__clang__)
#   define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#   define COMPILER_GCC
#elif defined(_MSC_VER)
#   define COMPILER_MSVC
#else
#   define COMPILER_UNKNOWN
#endif


#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#    define THREAD_LOCAL __thread
#elif defined(COMPILER_MSVC)
#    define THREAD_LOCAL __declspec(thread)
#elif (__STDC_VERSION__ >= 201112L)
#    define THREAD_LOCAL _Thread_local
#else
#    error "Invalid compiler/version for thead variable; Use Clang, GCC, or MSVC, or use C11 or greater"
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#   define TRAP() __builtin_trap()
#elif defined(COMPILER_MSVC)
#   define TRAP() __debugbreak()
#else
#   error "Unknown trap for this compiler"
#endif

#define ASSERT_ALWAYS(x) do{ if(!(x)) { TRAP(); } } while (0);

#ifndef NDEBUG
#   define ASSERT(x) ASSERT_ALWAYS(x)
#else
#   define ASSERT(x) (void)(x)
#endif

#define STATIC_ASSERT(c, id) static u8 CONCAT(id, __LINE__)[(c) ? 1 : -1]

#define CONCAT_NX(a, b) a##b
#define CONCAT(a, b) CONCAT_NX(a, b)

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

typedef float f32;
typedef double f64;

STATIC_ASSERT(sizeof(f32) == 4, f32_size);
STATIC_ASSERT(sizeof(f64) == 8, f64_size);

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))
#define ALIGN_DOWN_POW2(n, p) (((u64)(n)) & (~((u64)(p) - 1)))

#define UNUSED(x) (void)(x)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, a, b) (MIN((b), MAX((x), (a))))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) < 0 ? -1 : 1)

#define MEM_ZERO(p, size) memset((p), 0, (size))

#define SLL_PUSH_FRONT(f, l, n) ((f) == NULL ? \
    ((f) = (l) = (n)) :                        \
    ((n)->next = (f), (f) = (n)))              \

#define SLL_PUSH_BACK(f, l, n) ((f) == NULL ? \
    ((f) = (l) = (n)) :                       \
    ((l)->next = (n), (l) = (n)),             \
    ((n)->next = NULL))                       \

#define SLL_POP_FRONT(f, l) ((f) == (l) ? \
    ((f) = (l) = NULL) :                  \
    ((f) = (f)->next))                    \

#define DLL_PUSH_BACK(f, l, n) ((f) == 0 ? \
    ((f) = (l) = (n), (n)->next = (n)->prev = 0) : \
    ((n)->prev = (l), (l)->next = (n), (l) = (n), (n)->next = 0))

#define DLL_PUSH_FRONT(f, l, n) DLL_PUSH_BACK(l, f, n)

#define DLL_REMOVE(f, l, n) ( \
    (f) == (n) ? \
        ((f) == (l) ? \
            ((f) = (l) = (0)) : \
            ((f) = (f)->next, (f)->prev = 0)) : \
        (l) == (n) ? \
            ((l) = (l)->prev, (l)->next = 0) : \
            ((n)->next->prev = (n)->prev, \
            (n)->prev->next = (n)->next))

