
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

b32 str8_start_equals(string8 a, string8 b) {
    u64 size = MIN(a.size, b.size);

    for (u64 i = 0; i < size; i++) {
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

u64 str8_find_first(string8 str, u8 c) {
    u64 i = 0;

    for (; i < str.size; i++) {
        if (str.str[i] == c) { break; }
    }

    return i;
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

void str8_memcpy(string8* dest, const string8* src, u64 offset) {
    if (offset > dest->size) { return; }

    u64 size = MIN(src->size, dest->size - offset);
    memcpy(dest->str + offset, src->str, size);
}

string8 str8_concat_simple(mem_arena* arena, const string8_list* list) {
    if (list->count == 0) {
        return (string8){ 0 };
    }

    string8 out = {
        .str = PUSH_ARRAY_NZ(arena, u8, list->total_size),
        .size = list->total_size
    };

    u64 pos = 0;
    string8_node* node = list->first;
    for (u32 i = 0; i < list->count && node != NULL; i++, node = node->next) {
        str8_memcpy(&out, &node->str, pos);
        pos += node->str.size;
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

    u64 pos = 0;

    str8_memcpy(&out, &desc->begin, pos);

    pos += desc->begin.size;

    string8_node* node = list->first;
    for (u32 i = 0; i < list->count && node != NULL; i++, node = node->next) {
        str8_memcpy(&out, &node->str, pos);
        pos += node->str.size;

        if (i < list->count - 1 && node->next != NULL) {
            str8_memcpy(&out, &desc->delim, pos);
            pos += desc->delim.size;
        }
    }

    str8_memcpy(&out, &desc->end, pos);

    return out;
}

string8 str8_pushfv(mem_arena* arena, const char* fmt, va_list args) {
    string8 out = { 0 };

    va_list args2;
    va_copy(args2, args);

    i32 size = vsnprintf(NULL, 0, fmt, args);

    if (size > 0) {
        mem_arena_temp maybe_temp = arena_temp_begin(arena);

        out.size = (u64)size;
        out.str = PUSH_ARRAY_NZ(maybe_temp.arena, u8, out.size + 1);

        size = vsnprintf((char*)out.str, out.size + 1, fmt, args2);

        if (size <= 0) {
            out = (string8){ 0 };
            arena_temp_end(maybe_temp);
        }
    }

    va_end(args2);

    return out;
}

string8 str8_pushf(mem_arena* arena, const char* fmt, ...) {
    va_list args;
    
    va_start(args, fmt);

    string8 out = str8_pushfv(arena, fmt, args);

    va_end(args);

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

// Based off of the decoders from Chris Wellons and Mr 4th
// https://nullprogram.com/blog/2017/10/06/
// https://git.mr4th.com/mr4th-public/mr4th/src/branch/main/src/base/base_big_functions.c#L696
string_decode utf8_decode(string8 str, u64 offset) {
    static const u8 lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };
    static const u8 first_byte_masks[] = {
        0, 0x7f, 0x1f, 0x0f, 0x07
    };
    static const u8 final_shifts[] = {
        0, 18, 12, 6, 0
    };

    string_decode out = {
        // Replacement Character '�'
        .codepoint = 0xfffd,
        .len = 1
    };

    if (offset >= str.size) {
        return out;
    }

    u32 first_byte = str.str[offset];
    u32 len = lengths[first_byte >> 3];

    if (len <= 0 || offset + len > str.size) {
        return out;
    }

    u32 codepoint = (first_byte & first_byte_masks[len]) << 18;
    switch (len) {
        case 4: codepoint |= ((u32)str.str[offset+3] & 0x3f);
        // fallthrough
        case 3: codepoint |= (((u32)str.str[offset+2] & 0x3f) << 6);
        // fallthrough
        case 2: codepoint |= (((u32)str.str[offset+1] & 0x3f) << 12);
        default: break;
    }

    codepoint >>= final_shifts[len];

    out.codepoint = codepoint;
    out.len = len;

    return out;
}

string_decode utf16_decode(string16 str, u64 offset) {
    string_decode out = {
        // Replacement Character '�'
        .codepoint = 0xfffd,
        .len = 1
    };

    if (offset >= str.size) {
        return out;
    }

    u32 first_char = str.str[offset];

    if (first_char < 0xd800 || first_char > 0xdbff) {
        out.codepoint = first_char;
    } else if (offset + 1 < str.size) {
        u32 second_char = str.str[offset + 1];

        // First char was already checked
        if (second_char >= 0xdc00 && second_char <= 0xdfff) {
            out.codepoint = ((first_char - 0xd800) << 10) |
                (second_char - 0xdc00) + 0x10000;
            out.len = 2;
        }
    }

    return out;
}

u32 utf8_encode(u32 codepoint, u8* out) {
    u32 size = 0;

    if (codepoint <= 0x7f) {
        out[0] = (u8)(codepoint & 0xff);

        size = 1;
    } else if (codepoint <= 0x7ff) {
        out[0] = (u8)(0xc0 | (codepoint >> 6));
        out[1] = (u8)(0x80 | (codepoint & 0x3f));

        size = 2;
    } else if (codepoint <= 0xffff) {
        out[0] = (u8)(0xe0 | (codepoint >> 12));
        out[1] = (u8)(0x80 | ((codepoint >> 6) & 0x3f));
        out[2] = (u8)(0x80 | (codepoint & 0x3f));

        size = 3;
    } else if (codepoint <= 0x10fff) {
        out[0] = (u8)(0xf0 | (codepoint >> 18));
        out[1] = (u8)(0x80 | ((codepoint >> 12) & 0x3f));
        out[2] = (u8)(0x80 | ((codepoint >> 6) & 0x3f));
        out[3] = (u8)(0x80 | (codepoint & 0x3f));

        size = 4;
    } else {
        // Replacement Character '�'
        size = utf8_encode(0xfffd, out);
    }

    return size;
}

u32 utf16_encode(u32 codepoint, u16* out) {
    u32 size = 0;

    if (codepoint < 0x10000) {
        out[0] = (u16)codepoint;

        size = 1;
    } else {
        codepoint -= 0x10000;
        
        out[0] = (u16)((codepoint >> 10) + 0xd800);
        out[1] = (u16)((codepoint & 0x3ff) + 0xdc00);

        size = 1;
    }

    return size;
}

string8 str8_from_str16(mem_arena* arena, string16 str, b32 null_terminate) {
    u64 max_size = str.size * 3 + (null_terminate ? 1 : 0);
    u8* out = PUSH_ARRAY(arena, u8, max_size);

    u64 out_size = 0;
    u64 offset = 0;

    while (offset < str.size) {
        string_decode decode = utf16_decode(str, offset);

        offset += decode.len;
        
        out_size += utf8_encode(decode.codepoint, out);
    }

    u64 required_chars = out_size + (null_terminate ? 1 : 0);
    u64 unused_chars = max_size - required_chars;
    arena_pop(arena, unused_chars * sizeof(u8));

    return (string8) {
        .str = out,
        .size = out_size
    };
}

string16 str16_from_str8(mem_arena* arena, string8 str, b32 null_terminate) {
    u64 max_size = str.size + (null_terminate ? 1 : 0);
    u16* out = PUSH_ARRAY(arena, u16, max_size);

    u64 out_size = 0;
    u64 offset = 0;

    while (offset < str.size) {
        string_decode decode = utf8_decode(str, offset);

        offset += decode.len;

        out_size += utf16_encode(decode.codepoint, out + out_size);
    }

    u64 required_chars = out_size + (null_terminate ? 1 : 0);
    u64 unused_chars = max_size - required_chars;
    arena_pop(arena, unused_chars * sizeof(u16));

    return (string16) {
        .str = out,
        .size = out_size
    };
}

