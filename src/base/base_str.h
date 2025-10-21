
typedef struct {
    u8* str;
    u64 size;
} string8;

typedef struct string8_node {
    string8 str;
    struct string8_node* next;
} string8_node;

typedef struct {
    string8_node* first;
    string8_node* last;

    u32 count;
    u64 total_size;
} string8_list;

typedef struct {
    string8 begin;
    string8 delim;
    string8 end;
} string8_concat_desc;

#define STR8_LIT(s) (string8){ (u8*)(s), sizeof(s) - 1 }

// This is used for printing string8s
// e.g. printf("My String: %.*s\n", STR8_FMT(my_str));
#define STR8_FMT(s) (int)(s).size, (char*)(s).str

string8 str8_from_cstr(u8* cstr);
u8* str8_to_cstr(mem_arena* arena, string8 str);
string8 str8_copy(mem_arena* arena, string8 src);

b32 str8_equals(string8 a, string8 b);

string8 str8_substr(string8 base, u64 start, u64 end);
string8 str8_substr_size(string8 base, u64 start, u64 size);

string8 str8_concat(
    mem_arena* arena,
    const string8_list* list,
    const string8_concat_desc* desc
);

void str8_list_add_existing(string8_list* list, string8_node* node);
void str8_list_add(mem_arena* arena, string8_list* list, string8 str);

