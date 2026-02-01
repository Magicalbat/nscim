
#define EDITOR_ARENA_RESERVE_SIZE MiB(1)
#define EDITOR_ARENA_COMMIT_SIZE KiB(16)

// Maximum langth of command string (bytes)
#define EDITOR_CMD_MAX_STRLEN 2048

#define EDITOR_INPUT_QUEUE_MAX 1024
#define EDITOR_INPUT_SEQ_MAX 32

#define EDITOR_WIN_STATUS_ROWS_TOP 2
#define EDITOR_WIN_STATUS_ROWS_BOTTOM 0
#define EDITOR_WIN_STATUS_ROWS \
    (EDITOR_WIN_STATUS_ROWS_TOP + EDITOR_WIN_STATUS_ROWS_BOTTOM)

#define EDITOR_REGISTER_FIRST ((u8)'"')
#define EDITOR_REGISTER_LAST ((u8)'_')
#define EDITOR_REGISTER_COUNT (EDITOR_REGISTER_LAST - EDITOR_REGISTER_FIRST + 1)
#define EDITOR_REGISTER_DEFAULT '"'

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

     EDITOR_FLAG_SHOULD_QUIT     = (1 << 0),
     EDITOR_FLAG_SHOULD_DRAW     = (1 << 1),
    _EDITOR_FLAG_READING_NUM     = (1 << 2),
    _EDITOR_FLAG_CONTINUE_ACTION = (1 << 3),
    _EDITOR_FLAG_PENDING_MOTION  = (1 << 4),
} editor_flags;

typedef struct {
    win_col status_fg;
    win_col status_bg;

    win_col cell_fg;
    win_col cell_bg;

    win_col inactive_cursor_fg;
    win_col inactive_cursor_bg;
    
    win_col selection_fg;
    win_col selection_bg;

    win_col rc_fg;
    win_col rc_bg;
} editor_colors;

typedef struct {
    u32 cursor_row_pad;
    u32 cursor_col_pad;

    f32 anim_speed;
} editor_settings;

typedef struct editor_context {
    mem_arena* arena;

    editor_mode mode;

    editor_colors colors;
    editor_settings settings;

    sheet_buffer* empty_sheet;

    u32 num_windows;
    editor_window* root_win;
    editor_window* active_win;

    editor_window* first_free_win;
    editor_window* last_free_win;

    sheet_pos motion_start;

    f32 last_action_time_us;

    // Index into input queue, where the action began
    u32 action_start_input;
    u32 pending_action_count;
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
    u32 pending_action_inputs_size;

    u32 flags;

    u8 selected_register;
    b8 append_to_register;
    editor_register* registers[EDITOR_REGISTER_COUNT];

    win_input cur_inputs[EDITOR_INPUT_SEQ_MAX];
    win_input pending_action_inputs[EDITOR_INPUT_SEQ_MAX];

    u8 cmd_buf[EDITOR_CMD_MAX_STRLEN];
    u8 cell_input_buf[SHEET_MAX_STRLEN];

    win_input input_queue[EDITOR_INPUT_QUEUE_MAX];
} editor_context;

editor_context* editor_create(void);
void editor_destroy(editor_context* editor);

void editor_push_inputs(
    editor_context* editor, workbook* wb,
    win_input* inputs, u32 num_inputs
);

void editor_update(
    window* win, editor_context* editor,
    workbook* wb, f32 delta
);

void editor_draw(window* win, editor_context* editor, workbook* wb);

sheet_buffer* editor_get_active_sheet(
    editor_context* editor, workbook* wb, b32 create_if_empty
);

editor_register* editor_get_reg(editor_context* editor, u8 reg);

