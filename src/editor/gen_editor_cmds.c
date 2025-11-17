
// This program runs on its own to generate
// code for editor commands
#ifdef GEN_CMDS

#include "base/base.h"
#include "platform/platform.h"
#include "sheets/sheets.h"
#include "win/win.h"
#include "editor/editor.h"

#include "base/base.c"
#include "platform/platform.c"

#undef EDITOR_CMD_MOTION_BIT
#undef EDITOR_CMD_RANGE_BIT
#undef EDITOR_CMD_MODIFY_BIT
#undef EDITOR_CMD_JUST_COUNT_BIT
#undef EDITOR_CMD_NUM_FLAG_BITS

// Only using 31 bits (as to fit in a signed 32 bit int)
#define EDITOR_CMD_MOTION_BIT (1 << 30)
#define EDITOR_CMD_RANGE_BIT (1 << 29)
#define EDITOR_CMD_MODIFY_BIT (1 << 28)
#define EDITOR_CMD_JUST_COUNT_BIT (1 << 27)

#define EDITOR_CMD_NUM_FLAG_BITS 4

typedef enum {
    TYPE_NONE = 0,

    TYPE_I64,
    TYPE_F64,
    TYPE_CELL,
    TYPE_RANGE,
    TYPE_STR,
} cmd_arg_type;

const char* type_strs[] = {
    "[None]",

    "i64", "f64",
    "sheet_cell_pos",
    "sheet_range",
    "string8"
};

const char* type_parse_strs[] = {
    "[None]",

    "_editor_cmd_parse_i64",
    "_editor_cmd_parse_f64",
    "_editor_cmd_parse_cell",
    "_editor_cmd_parse_range",
};

typedef struct {
    cmd_arg_type t;
    string8 name;
    b32 required;
    string8 def_value;
    string8 desc;
} cmd_arg;

typedef struct {
    string8 name;
    u32 flags;
    string8 desc;

    cmd_arg args[EDITOR_CMD_MAX_ARGS];
} cmd_def;

#define CMD(name, flags, desc, ...) \
(cmd_def){ STR8_LIT(name), flags, STR8_LIT(desc), .args = {__VA_ARGS__} }

#define ARG(t, name, required, def_value, desc) \
    { t, STR8_LIT(name), required, STR8_LIT(#def_value), STR8_LIT(desc) }

#define MOTION (u32)EDITOR_CMD_MOTION_BIT
#define RANGE (u32)EDITOR_CMD_RANGE_BIT
#define MODIFY (u32)EDITOR_CMD_MODIFY_BIT
#define JUST_COUNT (u32)EDITOR_CMD_JUST_COUNT_BIT

cmd_def cmds[] = {
    CMD("null", 0, "Does nothing", 0),

    CMD("move_up", MOTION | JUST_COUNT,
        "Moves the cursor [count] rows up", 0),
    CMD("move_down", MOTION | JUST_COUNT,
        "Moves the cursor [count] rows down", 0),
    CMD("move_left", MOTION | JUST_COUNT,
        "Moves the cursor [count] columns left", 0),
    CMD("move_right", MOTION | JUST_COUNT,
        "Moves the cursor [count] columns right", 0),

    CMD("clear", RANGE | MODIFY, "Clears cells in [range]", 0),

    CMD(
        "sort", RANGE | MODIFY, "Sorts a given range",
        ARG(TYPE_RANGE, "range", true, 0, "Range of cells to sort"),
        ARG(TYPE_I64, "direction", false, 1, 
            "+1 for ascending order (default), -1 for descending")
    ),

    CMD("scroll_up", JUST_COUNT,
        "Scrolls the screen [count] rows up", 0),
    CMD("scroll_down", JUST_COUNT,
        "Scrolls the screen [count] rows down", 0),
    CMD("scroll_left", JUST_COUNT,
        "Scrolls the screen [count] columns left", 0),
    CMD("scroll_right", JUST_COUNT,
        "Scrolls the screen [count] columns right", 0),
};

int main(void) {
    mem_arena* perm_arena = arena_create(MiB(64), KiB(256), false);

    u32 num_cmds = sizeof(cmds) / sizeof(cmds[0]);

    cmd_arg just_count_args[EDITOR_CMD_MAX_ARGS] = {
        ARG(TYPE_I64, "count", false, 1, "Number of times to execute command")
    };
    cmd_arg range_args[EDITOR_CMD_MAX_ARGS] = {
        ARG(TYPE_RANGE, "range", true, A0:A0, "Range of cells to execute on")
    };

    for (u32 i = 0; i < num_cmds; i++) {
        if (cmds[i].args[0].t != TYPE_NONE) { continue; }

        if (cmds[i].flags & JUST_COUNT) {
            memcpy(cmds[i].args, just_count_args, sizeof(just_count_args));
        }  else if (cmds[i].flags & RANGE) {
            memcpy(cmds[i].args, range_args, sizeof(range_args));
        }
    }

    // TODO: get rid of stdio once I write file IO in platform layer?
    FILE* h_file = fopen("src/editor/editor_cmds_generated.h", "w");

    u32 flag_mask = 0;
    for (u32 i = 0; i < EDITOR_CMD_NUM_FLAG_BITS; i++) {
        flag_mask |= 1;
        flag_mask <<= 1;
    }
    flag_mask <<= (30 - EDITOR_CMD_NUM_FLAG_BITS);

    fprintf(
        h_file,
        "\n"
        "#define EDITOR_CMD_MOTION_BIT     0x%x\n"
        "#define EDITOR_CMD_RANGE_BIT      0x%x\n"
        "#define EDITOR_CMD_MODIFY_BIT     0x%x\n"
        "#define EDITOR_CMD_JUST_COUNT_BIT 0x%x\n"
        "\n"
        "#define EDITOR_CMD_NUM_FLAG_BITS %d\n"
        "#define EDITOR_CMD_FLAG_MASK 0x%x\n"
        "\n"
        "#define EDITOR_NUM_CMDS %d\n\n",
        EDITOR_CMD_MOTION_BIT,
        EDITOR_CMD_RANGE_BIT,
        EDITOR_CMD_MODIFY_BIT,
        EDITOR_CMD_JUST_COUNT_BIT,
        EDITOR_CMD_NUM_FLAG_BITS,
        flag_mask,
        num_cmds
    );

    fprintf(
        h_file,
        "#define EDITOR_CMD_GET_INDEX(cmd) ((cmd) & ~(u32)EDITOR_CMD_FLAG_MASK)\n"
        "\n"
        "// Index into this with the cmd index, NOT the id\n"
        "extern const editor_cmd_info editor_cmd_infos[EDITOR_NUM_CMDS];\n\n"
    );

    u64 max_cmd_name_size = 0;
    for (u32 i = 0; i < num_cmds; i++) {
        if (cmds[i].name.size > max_cmd_name_size) {
            max_cmd_name_size = cmds[i].name.size;
        }
    }

    u8* spaces = PUSH_ARRAY_NZ(perm_arena, u8, max_cmd_name_size);
    for (u64 i = 0; i < max_cmd_name_size; i++) {
        spaces[i] = ' ';
    }

    u8* upper_data = PUSH_ARRAY(perm_arena, u8, max_cmd_name_size);
    string8 upper_str = { .str = upper_data };

    fprintf(h_file, "typedef enum {\n");
    for (u32 i = 0; i < num_cmds; i++) {
        upper_str.size = max_cmd_name_size;
        str8_to_upper_ip(cmds[i].name, &upper_str);

        fprintf(
            h_file,
            "   EDITOR_CMD_%.*s %.*s= 0x%08x,\n",
            STR8_FMT(upper_str),
            (i32)(max_cmd_name_size - cmds[i].name.size),
            spaces, i | cmds[i].flags
        );
    }
    fprintf(
        h_file,
        "} editor_cmd_id_enum;\n\n"
        "typedef u32 editor_cmd_id;\n\n"
    );

    fprintf(
        h_file,
        "editor_cmd_res editor_cmd_execute_args(\n"
        "   editor_cmd_id cmd_id,\n"
        "   string8* args,\n"
        "   u32 num_args\n"
        ");\n\n"
    );

    for (u32 i = 0; i < num_cmds; i++) {
        fprintf(
            h_file, "editor_cmd_res editor_cmd_%.*s(",
            STR8_FMT(cmds[i].name)
        );

        if (cmds[i].args[0].t == TYPE_NONE) {
            fprintf(h_file, "void");
        }

        for (
            u32 j = 0;
            j < EDITOR_CMD_MAX_ARGS && cmds[i].args[j].t != TYPE_NONE;
            j++
        ) {
            if (j != 0) {
                fprintf(h_file, ", ");
            }

            fprintf(
                h_file, "%s %.*s",
                type_strs[cmds[i].args[j].t],
                STR8_FMT(cmds[i].args[j].name)
            );
        }

        fprintf(h_file, ");\n");
    }

    fprintf(h_file, "\n");
    fclose(h_file);

    FILE* c_file = fopen("src/editor/editor_cmds_generated.c", "w");

    fprintf(
        c_file,
        "\neditor_cmd_res editor_cmd_execute_args(\n"
        "   editor_cmd_id cmd_id,\n"
        "   string8* args,\n"
        "   u32 num_args\n"
        ") {\n"
        "    u32 cmd_index = EDITOR_CMD_GET_INDEX(cmd_id);\n"
        "    if (cmd_index <= EDITOR_NUM_CMDS) {\n"
        "        return (editor_cmd_res){\n"
        "            EDITOR_CMD_STATUS_ERROR,\n"
        "            STR8_LIT(\"Command does not exist\")\n"
        "        };\n"
        "    }\n"
        "    \n"
        "    switch (cmd_index) {\n"
    );

    for (u32 i = 0; i < num_cmds; i++) {
        fprintf(
            c_file,
            "        // %.*s\n"
            "        case 0x%08x: {\n",
            STR8_FMT(cmds[i].name), i
        );

        fprintf(c_file, "            b32 parse_ok = true;\n");

        for (
            u32 j = 0;
            j < EDITOR_CMD_MAX_ARGS && cmds[i].args[j].t != TYPE_NONE;
            j++
        ) {
            if (cmds[i].args[j].required) {
                fprintf(
                    c_file,
                    "            %s %.*s;\n"
                    "            parse_ok &= num_args > %u && %s(\n"
                    "                args[%u], &%.*s\n"
                    "            );\n",
                    type_strs[cmds[i].args[j].t],
                    STR8_FMT(cmds[i].args[j].name),
                    j, type_parse_strs[cmds[i].args[j].t],
                    j, STR8_FMT(cmds[i].args[j].name)
                );
            } else {
                fprintf(
                    c_file,
                    "            %s %.*s;\n"
                    "            parse_ok &= %s(\n"
                    "                num_args > %u ? "
                    "STR8_LIT(\"%.*s\") : args[%u],\n"
                    "                &%.*s\n"
                    "            );\n",
                    type_strs[cmds[i].args[j].t],
                    STR8_FMT(cmds[i].args[j].name),
                    type_parse_strs[cmds[i].args[j].t],
                    j, STR8_FMT(cmds[i].args[j].def_value), j,
                    STR8_FMT(cmds[i].args[j].name)
                );
            }
        }

        fprintf(
            c_file,
            "            if (!parse_ok) { goto parse_error; }\n"
            "            return editor_cmd_%.*s(",
            STR8_FMT(cmds[i].name)
        );

        for (
            u32 j = 0;
            j < EDITOR_CMD_MAX_ARGS && cmds[i].args[j].t != TYPE_NONE;
            j++
        ) {
            if (j != 0) {
                fprintf(c_file, ", ");
            }
            fprintf(c_file, "%.*s", STR8_FMT(cmds[i].args[j].name));
        }

        fprintf(c_file, ");\n        } break;\n\n");
    }

    fprintf(
        c_file,
        "    }\n\n"
        "parse_error:\n"
        "    return (editor_cmd_res){\n"
        "        EDITOR_CMD_STATUS_ERROR,\n"
        "        STR8_LIT(\"Invalid command arguments\"),\n"
        "    };\n"
        "}\n"
    );

    fprintf(
        c_file,
        "\nconst editor_cmd_info editor_cmd_infos[EDITOR_NUM_CMDS] = {\n"
    );

    for (u32 i = 0; i < num_cmds; i++) {
        u32 arg_required_bitmap = 0;

        u32 num_args = 0;
        for (
            u32 j = 0;
            j < EDITOR_CMD_MAX_ARGS && cmds[i].args[j].t != TYPE_NONE;
            j++, num_args++
        ) {
            arg_required_bitmap <<= 1;
            arg_required_bitmap |= (u32)cmds[i].args[j].required;
        }

        fprintf(c_file, "    // %.*s\n", STR8_FMT(cmds[i].name));
        
        fprintf(
            c_file,
            "    {\n"
            "        { (u8*)\"%.*s\", %" PRIu64 " },\n"
            "        { (u8*)\"%.*s\", %" PRIu64 "},\n"
            "        0x%x, %d, ",
            STR8_FMT(cmds[i].name), cmds[i].name.size,
            STR8_FMT(cmds[i].desc), cmds[i].desc.size,
            arg_required_bitmap, num_args
        );

        if (num_args == 0) {
            fprintf(c_file, "{ 0 }, { 0 },\n");
        } else {
            fprintf(c_file, "{\n");
            for (u32 j = 0; j < num_args; j++) {
                fprintf(
                    c_file,
                    "            { (u8*)\"%.*s\", %" PRIu64 " },\n",
                    STR8_FMT(cmds[i].args[j].name),
                    cmds[i].args[j].name.size
                );
            }

            fprintf(c_file, "        }, {\n");

            for (u32 j = 0; j < num_args; j++) {
                fprintf(
                    c_file,
                    "            { (u8*)\"%.*s\", %" PRIu64 " },\n",
                    STR8_FMT(cmds[i].args[j].desc),
                    cmds[i].args[j].desc.size
                );
            }

            fprintf(c_file, "        }\n");
        }

        fprintf(c_file, "    },\n\n");
    }

    fprintf(c_file, "};\n");

    fprintf(c_file, "\n");
    fclose(c_file);

    arena_destroy(perm_arena);

    return 0;
}

#endif


