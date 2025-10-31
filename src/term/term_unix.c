
#include <termios.h>
#include <sys/ioctl.h>

typedef struct _term_backend {
    struct termios orig_termios;
} _term_backend;

b32 _term_init_backend(mem_arena* arena, term_context* context) {
    struct termios orig = { 0 };
    struct termios raw = { 0 };

    if (tcgetattr(STDIN_FILENO, &orig) != 0) {
        return false;
    }
    if (tcgetattr(STDIN_FILENO, &raw) != 0) {
        return false;
    }

    raw.c_iflag &= (u32)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (u32)~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= (u32)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return false;
    }

    context->backend = PUSH_STRUCT(arena, _term_backend);
    context->backend->orig_termios = orig;

    return true;
}

void term_quit(term_context* context) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &context->backend->orig_termios);
}

void term_get_size(term_context* context, u32* width, u32* height) {
    struct winsize w = { 0 };
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    *width = w.ws_col;
    *height = w.ws_row;
}

u32 term_read(term_context* context, u8* chars, u32 capacity) {
    UNUSED(context);

    return (u32)read(STDIN_FILENO, chars, capacity);
}

void term_flush(term_context* context) {
    i64 written = write(STDOUT_FILENO, context->draw_buf, (u64)context->draw_buf_pos);

    if (written != (i64)context->draw_buf_pos) {
        plat_fatal_error("Failed to write to terminal", 1);
    }

    context->draw_buf_pos = 0;
}

