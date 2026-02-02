
#define LOG_CONCAT_CHAR '\n'

typedef enum {
    LOG_NONE  = 0b000,
    LOG_INFO  = 0b001,
    LOG_WARN  = 0b010,
    LOG_ERROR = 0b100,
    LOG_ALL   = 0b111,
} log_level;

typedef enum {
    LOG_RES_NONE = 0,

    LOG_RES_FIRST,
    LOG_RES_LAST,
    LOG_RES_CONCAT
} log_res_type;

typedef struct log_msg {
    struct log_msg* next;
    string8 msg;
    log_level level;
} log_msg;

typedef struct log_frame {
    struct log_frame* next;

    u64 arena_start_pos;

    u32 num_logs;
    log_msg* first;
    log_msg* last;
} log_frame;

typedef struct {
    mem_arena* arena;
    log_frame* stack;
} log_context;

#define info_emit(msg) log_emit(LOG_INFO, (msg))
#define info_emitf(fmt, ...) log_emitf(LOG_INFO, (fmt), __VA_ARGS__)

#define warn_emit(msg) log_emit(LOG_WARN, (msg))
#define warn_emitf(fmt, ...) log_emitf(LOG_WARN, (fmt), __VA_ARGS__)

#define error_emit(msg) log_emit(LOG_ERROR, (msg))
#define error_emitf(fmt, ...) log_emitf(LOG_ERROR, (fmt), __VA_ARGS__)

void log_frame_begin(void);
string8 log_frame_peek(
    mem_arena* arena, i32 level_mask,
    log_res_type res_type, b32 prefix_level
);
string8 log_frame_end(
    mem_arena* arena, i32 level_mask,
    log_res_type res_type, b32 prefix_level
);

void log_emit(log_level level, string8 msg);
void log_emitf(log_level level, const char* fmt, ...);

