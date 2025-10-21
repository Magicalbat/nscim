#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"

#include "base/base.c"
#include "platform/platform.c"

int main(void) {
    mem_arena* perm_arena = arena_create(MiB(64), MiB(1), true);

    arena_destroy(perm_arena);

    return 0;
}

