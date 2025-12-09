
typedef enum {
    SHEETS_WIN_SPLIT_VERT = 0b0,
    SHEETS_WIN_SPLIT_HORZ = 0b1,
} sheet_window_split;

// Stores information about how the user is editing a sheet
// Akin to vim windows
typedef struct sheet_window {
    struct sheet_window* parent;

    // For free-list
    struct sheet_window* next;

    // Union to make operations on both children easier
    union {
        struct {
            // Left or top child
            struct sheet_window* child0;
            // Right or bottom child
            struct sheet_window* child1;
        };

        struct sheet_window* children[2];
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

    sheet_window_split split_dir;

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
} sheet_window;

sheet_buffer* wb_win_get_sheet(workbook* wb, sheet_window* win, b32 create_if_empty);

// Operates on the active win
// If `open_in_both` is true, both children windows will have the same
// buffer open. Otherwise, only the first child will
void wb_win_split(workbook* wb, sheet_window_split split, b32 open_in_both);

// Operates on the active win
void wb_win_close(workbook* wb);

void wb_win_compute_sizes(workbook* wb, u32 total_width, u32 total_height);

void wb_win_update_anims(workbook* wb, f32 anim_speed, f32 delta);

// Given the current height of the window,
// this function updates the number of visible rows
// Called automatically by wb_win_compute_sizes
void wb_win_update_num_rows(sheet_window* win);

// Given the current width of the window,
// this function updates the number of visible columns
// Called automatically by wb_win_compute_sizes
void wb_win_update_num_cols(sheet_window* win);

// Operates on the active win
// Will attempt to increment the current win's width
// Must call after calling `wb_win_compute_sizes`
void wb_win_inc_width(workbook* wb, i32 amount);

// Operates on the active win
// Will attempt to increment the current win's height
// Must call after calling `wb_win_compute_sizes`
void wb_win_inc_height(workbook* wb, i32 amount);

// Moves cursor to the window to the left (-1) or right (+1)
void wb_win_change_active_horz(workbook* wb, i32 dir);

// Moves cursor to the window above (-1) or below (+1)
void wb_win_change_active_vert(workbook* wb, i32 dir);

