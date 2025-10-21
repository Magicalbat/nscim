
// These structures and functions are for dealing
// with the terminal in raw mode

typedef struct _term_backend _term_backend;

typedef struct {
    _term_backend* backend;
} term_context;

// Initializes the context and enters raw mode
term_context* term_init(mem_arena* arena);
// Exits raw mode
void term_quit(term_context* context);

// Returns the number of characters written
u32 term_read(term_context* context, u8* chars, u32 capacity);

