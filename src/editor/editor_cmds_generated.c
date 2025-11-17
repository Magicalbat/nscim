
editor_cmd_res editor_cmd_execute_args(
   editor_cmd_id cmd_id,
   string8* args,
   u32 num_args
) {
    u32 cmd_index = EDITOR_CMD_GET_INDEX(cmd_id);
    if (cmd_index <= EDITOR_NUM_CMDS) {
        return (editor_cmd_res){
            EDITOR_CMD_STATUS_ERROR,
            STR8_LIT("Command does not exist")
        };
    }
    
    switch (cmd_index) {
        // null
        case 0x00000000: {
            b32 parse_ok = true;
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_null();
        } break;

        // move_up
        case 0x00000001: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_move_up(count);
        } break;

        // move_down
        case 0x00000002: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_move_down(count);
        } break;

        // move_left
        case 0x00000003: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_move_left(count);
        } break;

        // move_right
        case 0x00000004: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_move_right(count);
        } break;

        // clear
        case 0x00000005: {
            b32 parse_ok = true;
            sheet_range range;
            parse_ok &= num_args > 0 && _editor_cmd_parse_range(
                args[0], &range
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_clear(range);
        } break;

        // sort
        case 0x00000006: {
            b32 parse_ok = true;
            sheet_range range;
            parse_ok &= num_args > 0 && _editor_cmd_parse_range(
                args[0], &range
            );
            i64 direction;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 1 ? STR8_LIT("1") : args[1],
                &direction
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_sort(range, direction);
        } break;

        // scroll_up
        case 0x00000007: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_scroll_up(count);
        } break;

        // scroll_down
        case 0x00000008: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_scroll_down(count);
        } break;

        // scroll_left
        case 0x00000009: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_scroll_left(count);
        } break;

        // scroll_right
        case 0x0000000a: {
            b32 parse_ok = true;
            i64 count;
            parse_ok &= _editor_cmd_parse_i64(
                num_args > 0 ? STR8_LIT("1") : args[0],
                &count
            );
            if (!parse_ok) { goto parse_error; }
            return editor_cmd_scroll_right(count);
        } break;

    }

parse_error:
    return (editor_cmd_res){
        EDITOR_CMD_STATUS_ERROR,
        STR8_LIT("Invalid command arguments"),
    };
}

const editor_cmd_info editor_cmd_infos[EDITOR_NUM_CMDS] = {
    // null
    {
        { (u8*)"null", 4 },
        { (u8*)"Does nothing", 12},
        0x0, 0, { 0 }, { 0 },
    },

    // move_up
    {
        { (u8*)"move_up", 7 },
        { (u8*)"Moves the cursor [count] rows up", 32},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_down
    {
        { (u8*)"move_down", 9 },
        { (u8*)"Moves the cursor [count] rows down", 34},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_left
    {
        { (u8*)"move_left", 9 },
        { (u8*)"Moves the cursor [count] columns left", 37},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_right
    {
        { (u8*)"move_right", 10 },
        { (u8*)"Moves the cursor [count] columns right", 38},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // clear
    {
        { (u8*)"clear", 5 },
        { (u8*)"Clears cells in [range]", 23},
        0x1, 1, {
            { (u8*)"range", 5 },
        }, {
            { (u8*)"Range of cells to execute on", 28 },
        }
    },

    // sort
    {
        { (u8*)"sort", 4 },
        { (u8*)"Sorts a given range", 19},
        0x2, 2, {
            { (u8*)"range", 5 },
            { (u8*)"direction", 9 },
        }, {
            { (u8*)"Range of cells to sort", 22 },
            { (u8*)"+1 for ascending order (default), -1 for descending", 51 },
        }
    },

    // scroll_up
    {
        { (u8*)"scroll_up", 9 },
        { (u8*)"Scrolls the screen [count] rows up", 34},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_down
    {
        { (u8*)"scroll_down", 11 },
        { (u8*)"Scrolls the screen [count] rows down", 36},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_left
    {
        { (u8*)"scroll_left", 11 },
        { (u8*)"Scrolls the screen [count] columns left", 39},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_right
    {
        { (u8*)"scroll_right", 12 },
        { (u8*)"Scrolls the screen [count] columns right", 40},
        0x0, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

};

