
// Maximum langth of command string (bytes)
#define EDITOR_MAX_CMD_STRLEN 1024
#define EDITOR_MAX_CMD_ARGS 16

typedef enum {
    EDITOR_MODE_NONE = 0,

    EDITOR_MODE_NORMAL,
    EDITOR_MODE_VISUAL,

    EDITOR_MODE_CELL_EDIT,
    EDITOR_MODE_CELL_INSERT,
    EDITOR_MODE_CELL_VISUAL,

    EDITOR_MODE_CMD,

    EDITOR_MODE_COUNT
} editor_mode;

typedef struct {
    win_col win_status_fg;
    win_col win_status_bg;

    win_col cell_fg;
    win_col cell_bg;

    win_col inactive_cursor_fg;
    win_col inactive_cursor_bg;

    win_col rc_fg;
    win_col rc_bg;
} editor_colors;

typedef struct {
    editor_mode mode;

    editor_colors colors;

    u32 cmd_size;
    u32 cmd_cursor;
    u32 cmd_select_start;

    u32 cell_input_size;
    u32 cell_input_cursor;
    u32 cell_input_select_start;

    u32 key_input_size;

    b32 should_quit;
    b32 should_draw;

    u8 cmd_buf[EDITOR_MAX_CMD_STRLEN];
    u8 cell_input_buf[SHEET_MAX_STRLEN];
} editor_context;

editor_context* editor_init(mem_arena* arena);

void editor_update(window* win, editor_context* editor, workbook* wb);

void editor_draw(window* win, editor_context* editor, workbook* wb);

