
#include <termios.h>

typedef struct _term_backend {
    struct termios orig_termios;
} _term_backend;

term_context* term_init(mem_arena* arena) {
    struct termios orig = { 0 };
    struct termios raw = { 0 };

    if (tcgetattr(STDIN_FILENO, &orig) != 0) {
        return NULL;
    }
    if (tcgetattr(STDIN_FILENO, &raw) != 0) {
        return NULL;
    }

    raw.c_iflag &= (u32)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (u32)~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (u32)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return NULL;
    }

    term_context* context = PUSH_STRUCT(arena, term_context);
    context->backend = PUSH_STRUCT(arena, _term_backend);
    context->backend->orig_termios = orig;

    return context;
}

void term_quit(term_context* context) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &context->backend->orig_termios);
}

u32 term_read(term_context* context, u8* chars, u32 capacity) {
    UNUSED(context);

    return (u32)read(STDIN_FILENO, chars, capacity);
}

