
typedef union {
    struct {
        u8 r, g, b;
    };

    u8 c[3];
} win_col;

typedef struct {
    win_col fg;
    win_col bg;

    // Character to be drawn
    u8 c;
} win_tile;

#define WIN_EMPTY_TILE (win_tile){ \
    .fg.r = 0xff, .fg.g = 0xff, .fg.b = 0xff, \
    .bg.r = 0x00, .bg.g = 0x00, .bg.b = 0x00, \
    .c = ' ' \
}

typedef enum {
    WIN_CUSOR_MODE_DEFAULT = 0,
    WIN_CURSOR_MODE_BLOCK_BLINK,
    WIN_CURSOR_MODE_BLOCK_STEADY,
    WIN_CURSOR_MODE_UNDERLINE_BLINK,
    WIN_CURSOR_MODE_UNDERLINE_STEADY,
    WIN_CURSOR_MODE_BAR_BLINK,
    WIN_CURSOR_MODE_BAR_STEADY,

    WIN_CURSOR_MODE_HIDDEN,

    _WIN_CURSOR_MODE_COUNT,
} win_cursor_mode_enum;

typedef u8 win_cursor_mode;

STATIC_ASSERT(
    _WIN_CURSOR_MODE_COUNT <= (1 << (sizeof(win_cursor_mode) * 8)),
    win_ursor_mode_size
);

typedef struct {
    u32 width, height;
    // Stored row-major
    win_tile* tiles;
} win_buffer;

typedef struct {
    struct _win_backend* _backend;

    win_cursor_mode cursor_mode;
    win_cursor_mode _prev_cursor_mode;
    b8 _first_draw;

    // Zero-based
    u32 cursor_row;
    // Zero-based
    u32 cursor_col;

    // These are allocated each frame on frame areans
    win_buffer front_buf;

    // All active drawing should be done to the back buffer
    win_buffer back_buf;
} window;

typedef u8 win_input;

// Only works for letter keys
#define WIN_INPUT_CTRL(k) ((k) & 0x1f)

#define WIN_INPUT_BACKSPACE 127

#define WIN_INPUT_ARROW_UP 128
#define WIN_INPUT_ARROW_DOWN 129
#define WIN_INPUT_ARROW_LEFT 130
#define WIN_INPUT_ARROW_RIGHT 131

#define WIN_INPUT_CTRL_ARROW_UP 132
#define WIN_INPUT_CTRL_ARROW_DOWN 133
#define WIN_INPUT_CTRL_ARROW_LEFT 134
#define WIN_INPUT_CTRL_ARROW_RIGHT 135

// Maximum number of win_inputs you can represent
#define WIN_INPUT_MAX (1 << (sizeof(win_input) * 8))

b32 win_col_eq(win_col a, win_col b);
b32 win_tile_eq(win_tile a, win_tile b);

window* win_create(mem_arena* arena);
void win_destroy(window* win);

// Any ascii inputs will get returned as their ascii values,
// but certain multi-byte inputs like arrow keys will get
// translated
// Will return 0 if there is no new input
win_input win_next_input(window* win);

b32 win_needs_resize(window* win);

void win_begin_frame(window* win, mem_arena* frame_arena);
void win_update(window* win);

