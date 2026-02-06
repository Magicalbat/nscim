
typedef struct {
    u8* str;
    u64 size;
} string8;

typedef struct {
    u16* str;
    u64 size;
} string16;

typedef struct string8_node {
    string8 str;
    struct string8_node* next;
} string8_node;

typedef struct {
    string8_node* first;
    string8_node* last;

    u64 total_size;
    u64 count;
} string8_list;

typedef struct {
    string8 begin;
    string8 delim;
    string8 end;
} string8_concat_desc;

typedef struct {
    u32 codepoint;
    // In characters of the string
    u32 len;
} string_decode;

// Used when a const string8 is being declared outside a function
// MSVC gets mad when you include the (string8) in front of the 
// intializer when it is outside a function  
#define STR8_CONST_LIT(s) { (u8*)(s), sizeof(s) - 1 }

#define STR8_LIT(s) (string8){ (u8*)(s), sizeof(s) - 1 }

// This is used for printing string8s
// e.g. printf("My String: %.*s\n", STR8_FMT(my_str));
#define STR8_FMT(s) (int)(s).size, (char*)(s).str

string8 str8_from_cstr(u8* cstr);
u8* str8_to_cstr(mem_arena* arena, string8 str);
string8 str8_copy(mem_arena* arena, string8 src);

// Copies as much of src to dest.str + offset as possible
void str8_memcpy(string8* dest, const string8* src, u64 offset);

b32 str8_equals(string8 a, string8 b);

// Compares as many chars as they share (MIN(a.size, b.size))
b32 str8_start_equals(string8 a, string8 b);

string8 str8_substr(string8 base, u64 start, u64 end);
string8 str8_substr_size(string8 base, u64 start, u64 size);

// Finds the first occurance of some character `c`
// Returns `str.size` if `c` if not in the string
u64 str8_find_first(string8 str, u8 c);

// Will fill as much of out as it can
void str8_to_upper_ip(string8 in, string8* out);
// Will fill as much of out as it can
void str8_to_lower_ip(string8 in, string8* out);

string8 str8_to_upper(mem_arena* arena, string8 str);
string8 str8_to_lower(mem_arena* arena, string8 str);

string8 str8_concat_simple(mem_arena* arena, const string8_list* list);
string8 str8_concat(
    mem_arena* arena,
    const string8_list* list,
    const string8_concat_desc* desc
);

string8 str8_pushfv(mem_arena* arena, const char* fmt, va_list args);
string8 str8_pushf(mem_arena* arena, const char* fmt, ...);

void str8_list_add_existing(string8_list* list, string8_node* node);
void str8_list_add(mem_arena* arena, string8_list* list, string8 str);

// Returns the decode output of the first unicode codepoint
// after offset bytes in the utf-8 string
string_decode utf8_decode(string8 str, u64 offset);
// Returns the decode output of the first unicode codepoint
// after offset 16-bit characters in the utf-16 string
string_decode utf16_decode(string16 str, u64 offset);
// Encodes the codepoint to out and returns the length in characters
// out must contain at least four characters
u32 utf8_encode(u32 codepoint, u8* out);
// Encodes the codepoint to out and returns the length in characters
// out must contain at least two characters
u32 utf16_encode(u32 codepoint, u16* out);

// Converts a utf-16 string16 to a utf-8 string8
// Null termination is not counted toward the size of the string
string8 str8_from_str16(mem_arena* arena, string16 str, b32 null_terminate);
// Converts a utf-8 string8 to a utf-16 string16
// Null termination is not counted toward the size of the string
string16 str16_from_str8(mem_arena* arena, string8 str, b32 null_terminate);

