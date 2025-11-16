
#define EDITOR_CMD_MAX_ARGS 8

#define EDITOR_CMD_MOTION_BIT     0x80000000
#define EDITOR_CMD_RANGE_BIT      0x40000000
#define EDITOR_CMD_MODIFY_BIT     0x20000000
#define EDITOR_CMD_JUST_COUNT_BIT 0x10000000

#define EDITOR_CMD_NUM_FLAG_BITS 4
#define EDITOR_CMD_FLAG_MASK 0xf0000000

#define EDITOR_NUM_CMDS 11

#define EDITOR_CMD_GET_INDEX(cmd) ((cmd) & ~(EDITOR_CMD_FLAG_MASK))

// Index into this with the cmd index, NOT the id
extern const editor_cmd_info editor_cmd_infos[EDITOR_NUM_CMDS];

typedef enum {
   EDITOR_CMD_NULL         = 0x00000000,
   EDITOR_CMD_MOVE_UP      = 0x90000001,
   EDITOR_CMD_MOVE_DOWN    = 0x90000002,
   EDITOR_CMD_MOVE_LEFT    = 0x90000003,
   EDITOR_CMD_MOVE_RIGHT   = 0x90000004,
   EDITOR_CMD_CLEAR        = 0x60000005,
   EDITOR_CMD_SORT         = 0x60000006,
   EDITOR_CMD_SCROLL_UP    = 0x10000007,
   EDITOR_CMD_SCROLL_DOWN  = 0x10000008,
   EDITOR_CMD_SCROLL_LEFT  = 0x10000009,
   EDITOR_CMD_SCROLL_RIGHT = 0x1000000a,
} editor_cmd_id_enum;

typedef u32 editor_cmd_id;

editor_cmd_res editor_cmd_execute_args(
   editor_cmd_id cmd_id,
   string8* args,
   u32 num_args
);

editor_cmd_res editor_cmd_null(void);
editor_cmd_res editor_cmd_move_up(i64 count);
editor_cmd_res editor_cmd_move_down(i64 count);
editor_cmd_res editor_cmd_move_left(i64 count);
editor_cmd_res editor_cmd_move_right(i64 count);
editor_cmd_res editor_cmd_clear(sheet_range range);
editor_cmd_res editor_cmd_sort(sheet_range range, i64 direction);
editor_cmd_res editor_cmd_scroll_up(i64 count);
editor_cmd_res editor_cmd_scroll_down(i64 count);
editor_cmd_res editor_cmd_scroll_left(i64 count);
editor_cmd_res editor_cmd_scroll_right(i64 count);


