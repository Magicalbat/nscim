
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
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_down
    {
        { (u8*)"move_down", 9 },
        { (u8*)"Moves the cursor [count] rows down", 34},
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_left
    {
        { (u8*)"move_left", 9 },
        { (u8*)"Moves the cursor [count] columns left", 37},
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // move_right
    {
        { (u8*)"move_right", 10 },
        { (u8*)"Moves the cursor [count] columns right", 38},
        0x1, 1, {
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
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_down
    {
        { (u8*)"scroll_down", 11 },
        { (u8*)"Scrolls the screen [count] rows down", 36},
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_left
    {
        { (u8*)"scroll_left", 11 },
        { (u8*)"Scrolls the screen [count] columns left", 39},
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

    // scroll_right
    {
        { (u8*)"scroll_right", 12 },
        { (u8*)"Scrolls the screen [count] columns right", 40},
        0x1, 1, {
            { (u8*)"count", 5 },
        }, {
            { (u8*)"Number of times to execute command", 34 },
        }
    },

};

