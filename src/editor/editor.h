
// Maximum langth of command string (bytes)
#define EDITOR_MAX_CMD_STRLEN 4096

#define EDITOR_INPUT_QUEUE_MAX 1024
#define EDITOR_INPUT_SEQ_MAX 32

#define EDITOR_WIN_STATUS_ROWS_TOP 2
#define EDITOR_WIN_STATUS_ROWS_BOTTOM 0
#define EDITOR_WIN_STATUS_ROWS \
    (EDITOR_WIN_STATUS_ROWS_TOP + EDITOR_WIN_STATUS_ROWS_BOTTOM)

typedef enum {
    EDITOR_MODE_NULL = 0,

    EDITOR_MODE_NORMAL,
    EDITOR_MODE_VISUAL,

    EDITOR_MODE_CELL_EDIT,
    EDITOR_MODE_CELL_VISUAL,
    EDITOR_MODE_CELL_INSERT,

    EDITOR_MODE_CMD,

    _EDITOR_MODE_COUNT
} editor_mode;

typedef enum {
    EDITOR_FLAG_NONE = 0,

     EDITOR_FLAG_SHOULD_QUIT = (1 << 0),
     EDITOR_FLAG_SHOULD_DRAW = (1 << 1),
    _EDITOR_FLAG_READING_NUM = (1 << 2),
} editor_flags;

typedef struct {
    win_col status_fg;
    win_col status_bg;

    win_col cell_fg;
    win_col cell_bg;

    win_col inactive_cursor_fg;
    win_col inactive_cursor_bg;

    win_col rc_fg;
    win_col rc_bg;
} editor_colors;

typedef struct {
    u32 cursor_row_pad;
    u32 cursor_col_pad;
} editor_settings;

typedef struct {
    editor_mode mode;

    editor_colors colors;
    editor_settings settings;

    sheet_pos motion_start;
    u32 count;

    u32 cmd_size;
    u32 cmd_cursor;
    u32 cmd_select_start;

    u32 cell_input_size;
    u32 cell_input_cursor;
    u32 cell_input_select_start;

    u32 input_queue_start;
    u32 input_queue_end;
    u32 input_queue_size;

    u32 cur_inputs_size;

    u32 flags;

    win_input cur_inputs[EDITOR_INPUT_SEQ_MAX];

    u8 cmd_buf[EDITOR_MAX_CMD_STRLEN];
    u8 cell_input_buf[SHEET_MAX_STRLEN];

    win_input input_queue[EDITOR_INPUT_QUEUE_MAX];
} editor_context;

editor_context* editor_init(mem_arena* arena);

void editor_push_inputs(
    editor_context* editor, workbook* wb,
    win_input* inputs, u32 num_inputs
);

void editor_update(window* win, editor_context* editor, workbook* wb);

void editor_draw(window* win, editor_context* editor, workbook* wb);

