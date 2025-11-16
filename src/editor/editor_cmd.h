
typedef enum {
    EDITOR_CMD_STATUS_OKAY,
    EDITOR_CMD_STATUS_WARNING,
    EDITOR_CMD_STATUS_ERROR,
} editor_cmd_status;

typedef struct {
    editor_cmd_status status;
    string8 message;
} editor_cmd_res;

typedef struct {
    string8 name;
    string8 desc;
    string8 arg_names[EDITOR_CMD_MAX_ARGS];
    string8 arg_descs[EDITOR_CMD_MAX_ARGS];
    u32 arg_required_bitmap;
    u32 num_args;
} editor_cmd_info;

editor_cmd_res editor_cmd_execute_str(string8 cmd_str);

