//See LICENSE file for copyright and license details.
// Help info for using gred, the Grid-based text Editor
#include "gred.h"

// Variables from gred.c:
extern struct line document[MAX_LINES];
extern int cursor_x;
extern int cursor_y;
extern int top_line_of_screen;
extern int screen_height;
extern char* menu_alert;
extern int in_escape_sequence;
extern struct line cur_escape_sequence;
extern int show_line_numbers;
extern int quit;
extern int debug;
extern int colorize;
extern int redraw_full_screen;

// help page stuff
#define NUM_HELP_PAGES 4
#define MAX_HELP_PAGE_LEN 2048
char help_pages[NUM_HELP_PAGES][MAX_HELP_PAGE_LEN]; // set at the bottom of this file
char macro_msg[] = "Press <escape> twice to end the macro.";


/////////////////////////////////////////////////////////////////////////////////
// Commands! (enum is in gred.h)
//

// Key binds for commands.
#define MAX_BINDING_LEN 16
struct binding {
    char input[MAX_BINDING_LEN];
    int command_id;
};
static struct binding bindings[] = {
    {"u", UNDO}, // undo
    {"r", REDO}, // redo
    {"i", SWITCH_TO_INSERT_MODE}, // insert mode
    {"I", SWITCH_TO_INSERT_MODE_AT_START_OF_LINE}, // goto start, insert
    {"a", SWITCH_TO_INSERT_MODE_AT_END_OF_LINE}, // goto end, insert
    {"q", QUIT}, // quit
    {"x", QUIT}, // quit
    {"s", SAVE}, // save file
    {"/", SEARCH}, // search for word
    {"o", SWITCH_TO_INSERT_MODE_IN_NEW_LINE_BELOW}, // insert new line below
    {"O", SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE}, // insert new line above
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
    {"n", LINE_NUMBERS}, // toggle show_line_numbers
    {"D", DEBUG}, // debug input by printing the ascii value of your inputs
    {"m", MACRO}, // begin recording a macro
    {"?", HELP}, // show help info
    //
    // Escape codes start with '['
    //
    {"[A",      UP}, // arrow keys
    {"[B",      DOWN,},
    {"[D",      LEFT,},
    {"[C",      RIGHT,},
    {"[3~",     DELETE}, // delete key
    {"[H",      GOTO_DOCUMENT_TOP, }, // home key
    {"[F",      GOTO_LINE_END,     }, // end key
    {"[5~",     SCROLL_PAGE_UP,    }, // page up key
    {"[6~",     SCROLL_PAGE_DOWN,  }, // page down key
    {"[secret", SECRET}, // secret
    {"[colors", COLORIZE}, // keyword color-coding
    {0, 0} // Null terminator, ends the list.
};
// run a command from the commands enum in gred.h (can be from a keybind or an escape code)
void run_command(int command_id) {
    switch(command_id) {
        case NO_COMMAND: // In an escape sequence, or got an invalid input.
            break;
        //
        // NAVIGATION:
        //
        case LEFT:
            cursor_x -= 1; // Moves the cursor left.
            break;
        case RIGHT:
            cursor_x += 1;
            break;
        case UP:
            cursor_y -= 1;
            break;
        case DOWN:
            cursor_y += 1;
            break;
        case GOTO_LINE_START: // Jump cursor to x=0.
            cursor_x = 0;
            break;
        case GOTO_LINE_END:
            cursor_x = document[cursor_y].len;
            break;
        case GOTO_DOCUMENT_TOP: // Jump cursor to y=0.
            cursor_y = 0;
            break;
        case GOTO_DOCUMENT_BOTTOM:
            cursor_y = MAX_LINES-1;
            while (document[cursor_y].len == 0)
                cursor_y -= 1;
            break;
        // Scroll the screen.
        case SCROLL_UP:
            if (top_line_of_screen > 0) {
                cursor_y -= 1;
                top_line_of_screen -= 1;
            }
            break;
        case SCROLL_DOWN:
            cursor_y += 1;
            top_line_of_screen += 1;
            break;
        case SCROLL_LEFT:
            if (text_display_x_start > 0) {
                cursor_x -= 1;
                text_display_x_start -= 1;
            }
            break;
        case SCROLL_RIGHT:
            cursor_x += 1;
            text_display_x_start += 1;
            break;
        case SCROLL_PAGE_UP: // Move screen up by (screen_height-1).
            top_line_of_screen -= screen_height-1;
            cursor_y -= screen_height-1;
            break;
        case SCROLL_PAGE_DOWN:
            top_line_of_screen += screen_height-1;
            cursor_y += screen_height-1;
            break;
        case SEARCH:
            open_menu(&menu_search);
            break;
        //
        // EDITING:
        //
        case UNDO:
            undo();
            break;
        case REDO:
            redo();
            break;
        case DELETE:
            record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
            line_backspace(cursor_x+1, &document[cursor_y]);
            record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            break;
        case INSERT:
            insert(cur_char); // Handle the INSERT_MODE character.
            break;
        case SWITCH_TO_INSERT_MODE:
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE); // user explicitly chose this mode
            break;
        case SWITCH_TO_INSERT_MODE_AT_START_OF_LINE:
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            cursor_x = 0;
            break;
        case SWITCH_TO_INSERT_MODE_AT_END_OF_LINE:
            switch_mode(INSERT_MODE);                                                                                                            
            remember_mode(INSERT_MODE);
            cursor_x = document[cursor_y].len + 1;
            break;
        case SWITCH_TO_INSERT_MODE_IN_NEW_LINE_BELOW:
            // ugly hack: Cursor movement treated as an edit to preserve cursor position on undo.
            record_before_edit(cursor_x, cursor_y, FIRST_OF_MULTIPLE_EDITS);
            cursor_y += 1;
            record_after_edit(cursor_x, cursor_y, EDIT_MOVE_CURSOR);
            record_before_edit(cursor_x, cursor_y, LAST_OF_MULTIPLE_EDITS);
            insert_new_empty_line(cursor_y);
            record_after_edit(cursor_x, cursor_y, EDIT_INSERT_LINE);
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            break;
        case SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE:
            record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
            insert_new_empty_line(cursor_y);
            record_after_edit(cursor_x, cursor_y, EDIT_INSERT_LINE);
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            break;
        case MACRO:
            recording_macro = 1;
            menu_alert = macro_msg; //TODO this <------------ TODO
            break;
        //
        // SETTINGS:
        //
        case LINE_NUMBERS:
            show_line_numbers = !show_line_numbers;
            break;
        case SECRET:
            show_line_numbers = 0;
            menu_alert = secret_message;
            break;
        case COLORIZE:
            colorize = !colorize;
            redraw_full_screen = 1;
            break;
        default:
            break;
        //
        // SAVE and QUIT:
        //
        case SAVE:
            open_menu(&menu_save_file);
            break;
        case QUIT:
            quit = 1; // Exit the program.
            break;
        // Help menu
        case HELP:
            system("clear");
            int cur_help_page = 0;
            char help_input = '?';
            while (help_input == '?' && cur_help_page < NUM_HELP_PAGES) {
                printf("%s", help_pages[cur_help_page]);
                help_input = getch();
                cur_help_page += 1;
                printf("\n-------------------------------\n");
            }
            system("clear");
            redraw_full_screen = 1; // redraw the screen
            break;
        case DEBUG:
            system("clear");
            debug = ~debug;
            break;
    }
    clip_cursor_to_grid();
}

// Add c to input, 
// check if input matches a bound input (keybind or escape sequence).
// Return the id number of the match's command, otherwise return NO_COMMAND.
struct line input;
int command_len = 0;
int get_command(char c) {
    // Handle the ESCAPE character first.
    if (c == ESCAPE || input.len >= MAX_BINDING_LEN-2) {
        switch_mode(COMMAND_MODE);
        input.len = 0;
        input.text[0] = 0; // null terminate
        return NO_COMMAND;
    }
    else if (mode == INSERT_MODE) {
        return INSERT; // Allow insertion of the character.
    }
    // Add the c to the input buffer.
    //line_insert(c, input.len, &input);
    input.text[input.len] = c;
    input.len += 1;
    input.text[input.len] = 0; // null terminate
    int cur_bind_len = 0;
    int matched_chars = 0;
    int partial_match_found = 0;
    int i=0;
    // Loop through all the bindings (stop at the null terminator).
    while (bindings[i].input[0] != 0) {
        matched_chars = 0;
        cur_bind_len = strnlen(bindings[i].input, MAX_BINDING_LEN);
        // Check current binding.
        for (int j=0; j<cur_bind_len; j++) {
            if (input.text[j] == bindings[i].input[j])
                matched_chars += 1;
            else
                break;
        }
        // Found a match!
        if (matched_chars == cur_bind_len && input.len == cur_bind_len) {
            menu_alert = input.text;
            // If this was an escape code, return to the previous mode.
            if (input.text[0] == '[')
                switch_mode(previous_mode);
            else
                remember_mode(mode);
            input.len = 0;
            input.text[0] = 0; // null terminate
            clip_cursor_to_grid();
            return bindings[i].command_id;
        }
        else if (matched_chars == input.len && input.len < cur_bind_len) {
            partial_match_found = 1;
        }
        i += 1; // Check the next binding.
    }
    // Reset command len if no partial matches.
    if (!partial_match_found) {
        input.len = 0;
        input.text[0] = 0; // null terminate
    }
    // No matches at all.
    return NO_COMMAND;
}





















// Info to show in the help menu:
char help_pages[NUM_HELP_PAGES][MAX_HELP_PAGE_LEN] = {
    {
    "Start adding text by entering INSERT_MODE\n\n"
    "Press i to enter INSERT_MODE\n"
    "You can then type in text as you would expect.\n"
    "\n"
    "Press <escape> to enter COMMAND_MODE\n"
    "In COMMAND_MODE you can:\n"
    "    Save:         s CTRL_S\n"
    "    Quit:         q x\n"
    "    Undo:         u\n"
    "    Redo:         r\n"
    "    Move cursor:  h j k l\n"
    "    Goto top:     g\n"
    "    Goto bottom:  G\n"
    "    Show line #s: n\n"
    "    Help:         ?\n"
    "\n"
    "Press ? again for more help."
},
{
    "COMMAND_MODE is for running various commands.\n"
    "You can save, undo/redo, ect. in this mode.\n"
    "Press <escape> at any time to enter COMMAND_MODE.\n"
    "\n"
    "INSERT_MODE is for normal text editing.\n"
    "Type some text, and it goes where "
    "your cursor is.\n"
    "Press i at any time to enter INSERT_MODE.\n"
    "\n"
    "Press ? again for advanced info (and secrets)."
},
{
    "This program is meant to be tinkered with.\n"
    "You can add your own commands very easily!\n"
    "In gred_keybinds.c, put a new command in\n"
    "    run_command()\n"
    "\n"
    "You can also add your own escape sequences!\n"
    "Escape sequences are codes which start with\n"
    "\"<escape>[\", which tells the editor to\n"
    "start reading the code.\n"
    "\n"
    "In gred.h, there is a list of codes\n"
    "the editor looks for called escape_sequences[][].\n"
    "Add your own escape code there AND in\n"
    "escape_sequences_enum{} to add it.\n"
    "You can now check for that code in\n"
    "    run_escape_code()\n"
    "and have something happen when it finds it.\n"
    "\nPress ? again for secrets..."
},
{
    "When you leave here, try this secret code:\n"
    "    First, press <escape>, then the \'[\' key.\n"
    "\n"
    "This makes the editor start listening for\n"
    "an escape sequence.\n"
    "\n"
    "    Now, type the word \"secret\"\n"
    "\n"
    "Try adding your own escape codes!\n"
    "\n"
    "Press any key to return to editing."
}
};

char secret_message[] = "You found a secret!"; // Hi there! :D
