
typedef enum {
    EDITOR_WIN_MODE_EMPTY = 0,

    EDITOR_WIN_MODE_SHEET = 1,
    EDITOR_WIN_MODE_TEXT_VIEW = 2,
} editor_window_mode;

typedef enum {
    EDITOR_WIN_SPLIT_VERT = 0b0,
    EDITOR_WIN_SPLIT_HORZ = 0b1,
} editor_window_split;

// Stores information about how the user is editing a sheet
// Akin to vim windows
typedef struct editor_window {
    editor_window_mode mode;

    struct editor_window* parent;

    // For free-list
    struct editor_window* next;

    // Union to make operations on both children easier
    union {
        struct {
            // Left or top child
            struct editor_window* child0;
            // Right or bottom child
            struct editor_window* child1;
        };

        struct editor_window* children[2];
    };

    // Fraction of parent's size the window takes up
    // along the axis perpendicular to the split
    // Only child0's parent_fraction is actually used
    // during calculations
    f64 parent_fraction;

    // Top left of window, in characters
    u32 start_x;
    u32 start_y;
    // Total characters taken up by the entire view of the window
    u32 width;
    u32 height;

    f32 anim_start_x;
    f32 anim_start_y;
    f32 anim_width;
    f32 anim_height;

    // Total rows and columns the window currently displays
    u32 num_rows;
    u32 num_cols;

    // Amount of width cutoff of the last column on screen
    u32 cutoff_width;
    // Amount of height cutoff of the last row on screen
    u32 cutoff_height;

    editor_window_split split_dir;

    // If the window is internal, it only exists to hold its 
    // children and should not reference a buffer 
    b32 internal;

    // Everything below is only for non-internal windows

    // Buffer should not be accessed directly
    // Use `wb_win_get_sheet`
    sheet_buffer* _sheet;

    // Top-left scroll pos
    sheet_pos scroll_pos;
    // Current position of cursor
    sheet_pos cursor_pos;
    // Position where selection started
    sheet_pos select_start;
    // Previously edited cell
    sheet_pos prev_edit_pos;
} editor_window;

sheet_buffer* editor_win_get_sheet(
    editor_context* editor, workbook* wb,
    editor_window* win, b32 create_if_empty
);

// Operates on the active win
// If `open_in_both` is true, both children windows will have the same
// buffer open. Otherwise, only the first child will
void editor_win_split(
    editor_context* editor, editor_window_split split, b32 open_in_both
);

// Operates on the active win
void editor_win_close(editor_context* editor);

void editor_win_compute_sizes(
    editor_context* editor, u32 total_width, u32 total_height
);

void editor_win_update_anims(editor_context* editor, f32 delta);

// Given the current height of the window,
// this function updates the number of visible rows
// Called automatically by wb_win_compute_sizes
void editor_win_update_num_rows(editor_window* win);

// Given the current width of the window,
// this function updates the number of visible columns
// Called automatically by wb_win_compute_sizes
void editor_win_update_num_cols(editor_window* win);

// Operates on the active win
// Will attempt to increment the current win's width
// Must call after calling `wb_win_compute_sizes`
void editor_win_inc_width(editor_context* editor, i32 amount);

// Operates on the active win
// Will attempt to increment the current win's height
// Must call after calling `wb_win_compute_sizes`
void editor_win_inc_height(editor_context* editor, i32 amount);

// Moves cursor to the window to the left (-1) or right (+1)
void editor_win_change_active_horz(editor_context* editor, i32 dir);

// Moves cursor to the window above (-1) or below (+1)
void editor_win_change_active_vert(editor_context* editor, i32 dir);

