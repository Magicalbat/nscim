
static THREAD_LOCAL log_context _log_context = { 0 };

void log_frame_begin(void) {
    if (_log_context.arena == NULL) {
        _log_context.arena = arena_create(KiB(64), KiB(16), ARENA_FLAG_GROWABLE);
    }

    log_frame* frame = PUSH_STRUCT(_log_context.arena, log_frame);
    frame->arena_start_pos = arena_get_pos(_log_context.arena);
    SLL_STACK_PUSH(_log_context.stack, frame);
}

u32 log_frame_peek_count(u32 level_mask) {
    if (_log_context.arena == NULL) { return 0; }

    log_frame* frame = _log_context.stack;

    if (level_mask == LOG_ALL) { return frame->num_logs; }

    u32 count = 0;

    for (
        log_msg* msg = frame->first; msg != NULL; msg = msg->next
    ) {
        if (msg->level & level_mask) {
            count++;
        }
    }

    return count;
}

const string8 _level_prefixes[] = {
    [LOG_INFO] = STR8_CONST_LIT("Info: "),
    [LOG_WARN] = STR8_CONST_LIT("Warning: "),
    [LOG_ERROR] = STR8_CONST_LIT("Error: "),
    STR8_CONST_LIT("Unknown Log: "),
};

#define _LOG_PREFIX_INDEX(level) \
    MIN(sizeof(_level_prefixes) / sizeof(_level_prefixes[0]) - 1, (u32)(level))

string8 log_frame_peek(
    mem_arena* arena, u32 level_mask,
    log_res_type res_type, b32 prefix_level
) {
    if (_log_context.arena == NULL) { return (string8){ 0 }; }

    log_frame* frame = _log_context.stack;

    string8 out = { 0 };

    if (res_type == LOG_RES_CONCAT) {
        u32 num_logs = 0;
        u64 output_size = 0;

        for (
            log_msg* cur_log = frame->first;
            cur_log != NULL; cur_log = cur_log->next
        ) {
            if ((cur_log->level & level_mask) == 0) continue;

            num_logs++;

            u32 prefix_index = _LOG_PREFIX_INDEX(cur_log->level);
            string8 prefix = prefix_level ?
                _level_prefixes[prefix_index] : (string8){ 0 };

            output_size += prefix.size;
            output_size += cur_log->msg.size;
        }

        if (num_logs > 0) {
            output_size += num_logs - 1;

            out.size = output_size;
            out.str = PUSH_ARRAY(arena, u8, out.size);

            u64 out_pos = 0;

            for (
                log_msg* cur_log = frame->first;
                cur_log != NULL; cur_log = cur_log->next
            ) {
                if ((cur_log->level & level_mask) == 0) continue;

                num_logs++;

                u32 prefix_index = _LOG_PREFIX_INDEX(cur_log->level);
                string8 prefix = prefix_level ?
                    _level_prefixes[prefix_index] : (string8){ 0 };

                str8_memcpy(&out, &prefix, out_pos);
                out_pos += prefix.size;

                str8_memcpy(&out, &cur_log->msg, out_pos);
                out_pos += cur_log->msg.size;

                if (out_pos < out.size) {
                    out.str[out_pos++] = LOG_CONCAT_CHAR;
                }
            }
        }
    } else {
        log_msg* selected_log = NULL;

        if (res_type == LOG_RES_FIRST) {
            log_msg* cur_log = frame->first;

            while (cur_log != NULL && (cur_log->level & level_mask) == 0) {
                cur_log = cur_log->next;
            }

            selected_log = cur_log;
        } else if (res_type == LOG_RES_LAST) {
            log_msg* cur_log = frame->first;
            log_msg* last_valid = NULL;

            while (cur_log != NULL) {
                if (cur_log->level & level_mask) {
                    last_valid = cur_log;
                }

                cur_log = cur_log->next;
            }

            selected_log = last_valid;
        }

        if (selected_log != NULL) {
            string8 msg = selected_log->msg;
            u32 prefix_index = _LOG_PREFIX_INDEX(selected_log->level);
            string8 prefix = prefix_level ?
                _level_prefixes[prefix_index] : (string8){ 0 };

            out.size = msg.size + prefix.size;
            out.str = PUSH_ARRAY_NZ(arena, u8, out.size);

            memcpy(out.str, prefix.str, prefix.size);
            memcpy(out.str + prefix.size, msg.str, msg.size);
        }
    }

    return out;
}

string8 log_frame_end(
    mem_arena* arena, u32 level_mask,
    log_res_type res_type, b32 prefix_level
) {
    if (_log_context.arena == NULL) { return (string8){ 0 }; }

    string8 out = log_frame_peek(arena, level_mask, res_type, prefix_level);

    arena_pop_to(_log_context.arena, _log_context.stack->arena_start_pos);
    SLL_STACK_POP(_log_context.stack);

    return out;
}

// `msg` must already be allocated on the log_context arena
void _log_emit_impl(log_level level, string8 msg) {
    log_frame* frame = _log_context.stack;

    log_msg* cur_log = PUSH_STRUCT(_log_context.arena, log_msg);
    cur_log->msg = msg;
    cur_log->level = level;

    frame->num_logs++;
    SLL_PUSH_BACK(frame->first, frame->last, cur_log);
}

void log_emit(log_level level, string8 orig_msg) {
    if (_log_context.arena == NULL) { return; }

    string8 msg = str8_copy(_log_context.arena, orig_msg);
    _log_emit_impl(level, msg);
}

void log_emitf(log_level level, const char* fmt, ...) {
    if (_log_context.arena == NULL) { return; }

    va_list args;
    va_start(args, fmt);

    string8 msg = str8_pushfv(_log_context.arena, fmt, args);

    va_end(args);

    _log_emit_impl(level, msg);
}


