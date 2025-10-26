
// Both of these operate on the active window
void wb_win_split(workbook* wb, sheet_window_split split);
void wb_win_close(workbook* wb);
// Will attempt to increment the current win's width or height
void wb_win_inc_width(workbook* wb, u32 amount);
void wb_win_inc_height(workbook* wb, u32 amount);

void wb_win_update_sizes(workbook* wb, u32 total_width, u32 total_height);


