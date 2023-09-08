//See LICENSE file for copyright and license details.
// Key binds for commands.
#include "gred.h"

#define MAX_BINDING_LEN 16
/* Shell script to print these bindings for the tutorial:
sed -n '/Start of bindings./,/End of bindings./p' keybinds.c | # Get range of lines.
    sed 's/{"/</; s/",/>/; s/},//' # Format the lines.
*/
struct binding bindings[] = {
    // Start of bindings:
    
    // Editing:
    {"u", UNDO}, // undo
    {"r", REDO}, // redo
    {"c", COPY}, // copy selected text TODO copy current word if no selection
    {"v", PASTE}, // paste from clipboard
    {"d", DELETE_WORD}, // delete current word TODO this
    {"D", DELETE_TRAILING}, // delete the current line
    {"i", SWITCH_TO_INSERT_MODE}, // insert mode
    {"I", SWITCH_TO_INSERT_MODE_AT_START_OF_LINE}, // goto start, insert
    {"a", SWITCH_TO_INSERT_MODE_AT_END_OF_LINE}, // goto end, insert
    {"o", SWITCH_TO_INSERT_MODE_IN_NEW_LINE_BELOW}, // insert new line below
    {"O", SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE}, // insert new line above
    {"q", QUIT}, // quit
    {"x", QUIT}, // quit
    {"s", SAVE}, // save file
    
    // Navigation:
    {"/", SEARCH}, // search for word
    {"f", FIND_CHAR_NEXT}, // Find char horizontally.
    {"F", FIND_CHAR_PREV},
    {"e", ELEVATOR_DOWN}, // Find char vertically.
    {"E", ELEVATOR_UP},
    {"k", UP}, // cursor movement
    {"j", DOWN},
    {"h", LEFT},
    {"l", RIGHT},
    {"K", SCROLL_UP}, // scrolling the screen
    {"J", SCROLL_DOWN},
    {"H", SCROLL_LEFT},
    {"L", SCROLL_RIGHT},
    {"0", GOTO_LINE_START}, // goto start of line
    {"$", GOTO_LINE_END}, // goto end of line
    {"g", GOTO_DOCUMENT_TOP}, // goto top of document
    {"G", GOTO_DOCUMENT_BOTTOM}, // goto bottom of document
    
    // Settings:
    {"n", LINE_NUMBERS}, // toggle show_line_numbers
    {"D", DEBUG}, // debug input by printing the ascii value of your inputs
    {"m", MACRO}, // begin recording a macro
    {"?", HELP}, // show help info

    // Arrow keys send multi-character "escape sequences" to the terminal.
    {"[A",      UP},
    {"[B",      DOWN,},
    {"[D",      LEFT,},
    {"[C",      RIGHT,},
    
    // More keys that send multi-character "escape sequences" instead of a single byte.
    {"[3~",     DELETE}, // delete key
    {"[H",      GOTO_DOCUMENT_TOP, }, // home key
    {"[F",      GOTO_LINE_END,     }, // end key
    {"[5~",     SCROLL_PAGE_UP,    }, // page up key
    {"[6~",     SCROLL_PAGE_DOWN,  }, // page down key
    
    // Custom "escape sequences" only accessible by typing them.
    {"[secret", SECRET}, // secret
    {"[colors", COLORIZE}, // keyword color-coding
    
    // End of bindings.
    {0, 0} // Null terminator, ends the list.
};
