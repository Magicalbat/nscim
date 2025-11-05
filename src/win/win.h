
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

typedef struct {
    u32 width, height;
    // Stored row-major
    win_tile* tiles;
} win_buffer;

typedef struct {
    struct _win_backend* backend;

    b32 first_draw;

    // These are allocated each frame on frame areans
    win_buffer front_buf;

    // All active drawing should be done to the back buffer
    win_buffer back_buf;
} window;

typedef u16 win_input;

// Only works for letter keys
#define WIN_INPUT_CTRL(k) ((k) & 0x1f)

#define WIN_INPUT_BACKSPACE 127

#define WIN_INPUT_UP_ARROW 257
#define WIN_INPUT_DOWN_ARROW 258
#define WIN_INPUT_LEFT_ARROW 259
#define WIN_INPUT_RIGHT_ARROW 260

b32 win_col_eq(win_col a, win_col b);
b32 win_tile_eq(win_tile a, win_tile b);

window* win_create(mem_arena* arena);
void win_destroy(window* win);

// Any ascii inputs will get returned as their ascii values,
// but certain multi-byte inputs like arrow keys will get
// translated
// Will return 0 if there is no new input
win_input win_next_input(window* win);

void win_begin_frame(window* win, mem_arena* frame_arena);
void win_update(window* win);


