
// Maximum command langth (bytes)
#define EDITOR_CMD_MAX 1024

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

typedef enum {
    EDITOR_ACTION_NONE = 0,

    EDITOR_ACTION_MOVE_UP,
    EDITOR_ACTION_MOVE_DOWN,
    EDITOR_ACTION_MOVE_LEFT,
    EDITOR_ACTION_MOVE_RIGHT,

    _EDITOR_ACTION_MOTION_END,

    _EDITOR_ACTION_MODIFY_END,

    EDITOR_ACTION_SCROLL_UP,
    EDITOR_ACTION_SCROLL_DOWN,
    EDITOR_ACTION_SCROLL_LEFT,
    EDITOR_ACTION_SCROLL_RIGHT,

    EDITOR_ACTION_WIN_SPLIT_VERT,
    EDITOR_ACTION_WIN_SPLIT_HORZ,

    EDITOR_ACTION_WIN_CLOSE,

    EDITOR_ACTION_WIN_SELECT_UP,
    EDITOR_ACTION_WIN_SELECT_DOWN,
    EDITOR_ACTION_WIN_SELECT_LEFT,
    EDITOR_ACTION_WIN_SELECT_RIGHT,

    _EDITOR_ACTION_COUNT
} editor_action_enum;

typedef u16 editor_action;

STATIC_ASSERT(
    _EDITOR_ACTION_COUNT < (1 << (sizeof(editor_action) * 8)),
    editor_action_count
);

#define EDITOR_ACTION_IS_MOTION(action) \
    ((action) < _EDITOR_ACTION_MOTION_END && (action) != EDITOR_ACTION_NONE)

#define EDITOR_ACTION_IS_MODIFY(action) ( \
    (action) > _EDITOR_ACTION_MOTION_END && \
    (action) < _EDITOR_ACTION_MODIFY_END \
)

typedef struct {
    editor_mode mode;

    editor_colors colors;

    u32 cmd_size;
    u32 cmd_cursor;
    u32 cmd_select_start;

    // Set by motion actions and used by modify actions
    sheet_cell_range motion_range;

    u32 cell_input_size;
    u32 cell_input_cursor;
    u32 cell_input_select_start;

    u32 key_input_size;

    b32 should_quit;
    b32 should_draw;

    u8 cmd_buf[EDITOR_CMD_MAX];
    u8 cell_input_buf[SHEET_MAX_STRLEN];
} editor_context;

editor_context* editor_init(mem_arena* arena);

void editor_execute_action(
    editor_context* editor, workbook* wb,
    editor_action action, u32 repeat
);

void editor_update(window* win, editor_context* editor, workbook* wb);

void editor_draw(window* win, editor_context* editor, workbook* wb);

