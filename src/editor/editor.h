
// Maximum command langth (bytes)
#define EDITOR_CMD_MAX 1024
// Maximum keyboard input combination (`win_input`s)
#define EDITOR_INPUT_MAX 16

typedef enum {
    EDITOR_MODE_NONE = 0,

    EDITOR_MODE_NORMAL,
    EDITOR_MODE_INSERT,
    EDITOR_MODE_VISUAL,
    EDITOR_MODE_CMD,

    EDITOR_MODE_COUNT
} editor_mode;

typedef struct {
    editor_mode mode;

    u32 cmd_size;
    u32 cmd_cursor;
    u32 cmd_select_start;

    u32 input_size;

    u8 cmd_buf[EDITOR_CMD_MAX];
    win_input input_buf[EDITOR_INPUT_MAX];
} editor_context;

editor_context* editor_init(mem_arena* arena);

void editor_update(window* win, editor_context* editor, workbook* wb);

void editor_draw(window* win, editor_context* editor, workbook* wb);

