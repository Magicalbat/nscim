
string8 str8_from_cstr(u8* cstr) {
    u8* start = cstr;

    while (*(cstr++));

    return (string8) {
        .str = start,
        .size = (u64)(cstr - start)
    };
}

u8* str8_to_cstr(mem_arena* arena, string8 str) {
    u8* out = PUSH_ARRAY_NZ(arena, u8, str.size);

    memcpy(out, str.str, str.size);
    out[str.size] = '\0';

    return out;
}

string8 str8_copy(mem_arena* arena, string8 src) {
    string8 out = {
        .str = PUSH_ARRAY_NZ(arena, u8, src.size),
        .size = src.size
    };

    memcpy(out.str, src.str, src.size);

    return out;
}

b32 str8_equals(string8 a, string8 b) {
    if (a.size != b.size) {
        return false;
    }

    for (u64 i = 0; i < a.size; i++) {
        if (a.str[i] != b.str[i]) {
            return false;
        }
    }

    return true;
}

string8 str8_substr(string8 base, u64 start, u64 end) {
    end = MIN(base.size, end);
    start = MIN(end, start);

    return (string8) {
        .str = base.str + start,
        .size = end - start
    };
}

string8 str8_substr_size(string8 base, u64 start, u64 size) {
    start = MIN(base.size, start);
    size = MIN(size, base.size - start);

    return (string8) {
        .str = base.str + start,
        .size = size
    };
}

void str8_to_upper_ip(string8 in, string8* out) {
    u64 size = MIN(in.size, out->size);
    out->size = size;

    for (u64 i = 0; i < size; i++) {
        u8 c = in.str[i];

        if (c >= 'a' && c <= 'z') {
            c -= 'a' - 'A';
        }

        out->str[i] = c;
    }
}

void str8_to_lower_ip(string8 in, string8* out) {
    u64 size = MIN(in.size, out->size);
    out->size = size;

    for (u64 i = 0; i < size; i++) {
        u8 c = in.str[i];

        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }

        out->str[i] = c;
    }
}

string8 str8_to_upper(mem_arena* arena, string8 str) {
    string8 out = str8_copy(arena, str);

    for (u64 i = 0; i < out.size; i++) {
        if (out.str[i] >= 'a' && out.str[i] <= 'z') {
            out.str[i] -= 'a' - 'A';
        }
    }

    return out;
}

string8 str8_to_lower(mem_arena* arena, string8 str) {
    string8 out = str8_copy(arena, str);

    for (u64 i = 0; i < out.size; i++) {
        if (out.str[i] >= 'A' && out.str[i] <= 'Z') {
            out.str[i] += 'a' - 'A';
        }
    }

    return out;

}

string8 str8_concat(
    mem_arena* arena,
    const string8_list* list,
    const string8_concat_desc* desc
) {
    if (list->count == 0) {
        return (string8) { 0 };
    }

    u64 total_size = list->total_size +
        desc->begin.size + desc->delim.size * (list->count - 1) + desc->end.size;

    string8 out = (string8){
        .str = PUSH_ARRAY_NZ(arena, u8, total_size),
        .size = total_size
    };

    memcpy(out.str, desc->begin.str, desc->begin.size);

    u64 pos = desc->begin.size;

    string8_node* node = list->first;
    for (u32 i = 0; i < list->count && node != NULL; i++, node = node->next) {
        memcpy(out.str + pos, node->str.str, node->str.size);
        pos += node->str.size;

        if (i < list->count - 1 && node->next != NULL) {
            memcpy(out.str + pos, desc->delim.str, desc->delim.size);
            pos += desc->delim.size;
        }
    }

    memcpy(out.str + pos, desc->end.str, desc->end.size);

    return out;
}

void str8_list_add_existing(string8_list* list, string8_node* node) {
    list->count++;
    list->total_size += node->str.size;
    
    SLL_PUSH_BACK(list->first, list->last, node);
}

void str8_list_add(mem_arena* arena, string8_list* list, string8 str) {
    string8_node* node = PUSH_STRUCT(arena, string8_node);
    node->str = str;

    str8_list_add_existing(list, node);
}

