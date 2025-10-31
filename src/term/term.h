
// These structures and functions are for dealing
// with the terminal in raw mode

typedef struct _term_backend _term_backend;

typedef struct {
    _term_backend* backend;

    u32 draw_buf_capacity;
    u32 draw_buf_pos;
    u8* draw_buf;
} term_context;

// Initializes the context and enters raw mode
term_context* term_init(mem_arena* arena, u32 draw_buf_capacity);
// Exits raw mode
void term_quit(term_context* context);

void term_get_size(term_context* context, u32* width, u32* height);

// Returns the number of characters written
u32 term_read(term_context* context, u8* chars, u32 capacity);

// Will not always write the output immediately,
// use term_flush if you need immediate writes
void term_write(term_context* context, string8 str);

void term_flush(term_context* context);

