
term_context* term_init(mem_arena* arena, u32 draw_buf_capacity) {
    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    term_context* context = PUSH_STRUCT(maybe_temp.arena, term_context);

    if (!_term_init_backend(arena, context)) {
        arena_temp_end(maybe_temp);
        return NULL;
    }

    context->draw_buf_capacity = draw_buf_capacity;
    context->draw_buf_pos = 0;
    context->draw_buf = PUSH_ARRAY(arena, u8, draw_buf_capacity);

    return context;
}

void term_write(term_context* context, string8 str) {
    if (context == NULL) { return; }

    while (str.size > 0) {
        u32 buf_left = context->draw_buf_capacity - context->draw_buf_pos;
        b32 need_flush = false;
        u32 cur_write = (u32)MIN(str.size, ~((u32)0));

        if (cur_write > buf_left) {
            cur_write = buf_left;
            need_flush = true;
        }

        memcpy(context->draw_buf + context->draw_buf_pos, str.str, cur_write);

        context->draw_buf_pos += cur_write;
        str = str8_substr(str, cur_write, str.size);

        if (need_flush) {
            term_flush(context);
        }
    }
}

void term_write_c(term_context* context, u8 c) {
    if (context == NULL) { return; }

    context->draw_buf[context->draw_buf_pos++] = c;

    if (context->draw_buf_pos >= context->draw_buf_capacity) {
        term_flush(context);
    }
}

