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
void run_command(char c) {
    switch(c) {
        case 't': // Update file type based on file name suffix.
            update_settings_from_file_type(get_file_type(&file_name));
            break;
        case 'u': // undo
            undo();
            break;
        case 'r': // redo
            redo();
            break;
        case 'i': // insert mode
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE); // user explicitly chose this mode
            break;
        case 'I': // insert mode, goto start of line
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            cursor_x = 0;
            break;
        case 'a': // insert mode, goto end of line
            switch_mode(INSERT_MODE);                                                                                                            
            remember_mode(INSERT_MODE);
            cursor_x = document[cursor_y].len + 1;
            break;
        case 'q': // quit
            quit = 1;
            break;
        case 'x': // quit
            quit = 1;
            break;
        case 's': // save file
            menu_save_file();
            break;
        case '/': // search for word
            menu_search();
            break;
        case CTRL_S: // save file
            menu_save_file();
            break;
        case 'o': // insert new line below
            // ugly hack: Cursor movement treated as an edit to preserve cursor position on undo.
            record_before_edit(cursor_x, cursor_y, FIRST_OF_MULTIPLE_EDITS);
            cursor_y += 1;
            record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            record_before_edit(cursor_x, cursor_y, LAST_OF_MULTIPLE_EDITS);
            insert_new_empty_line(cursor_y);
            record_after_edit(cursor_x, cursor_y, EDIT_INSERT_LINE);
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            break;
        case 'O': // insert new line above
            record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
            insert_new_empty_line(cursor_y);
            record_after_edit(cursor_x, cursor_y, EDIT_INSERT_LINE);
            switch_mode(INSERT_MODE);
            remember_mode(INSERT_MODE);
            break;
        //                vi arrow keys
        case 'k': // up
            cursor_y -= 1;
            break;
        case 'j': // down
            cursor_y += 1;
            break;
        case 'h': // left
            cursor_x -= 1;
            break;
        case 'l': // right
            cursor_x += 1;
            break;
        //                SCROLLING
        case 'K': // up
            if (top_line_of_screen > 0) {
                cursor_y -= 1;
                top_line_of_screen -= 1;
            }
            break;
        case 'J': // down
            cursor_y += 1;
            top_line_of_screen += 1;
            break;
        case 'H': // left
            if (text_display_x_start > 0) {
                cursor_x -= 1;
                text_display_x_start -= 1;
            }
            break;
        case 'L': // right
            cursor_x += 1;
            text_display_x_start += 1;
            break;
        case '0': // goto start of line
            cursor_x = 0;
            break;
        case '$': // goto end of line
            cursor_x = document[cursor_y].len;
            break;
        case 'g': // goto top of document
            cursor_y = 0;
            break;
        case 'G': // goto bottom of document
            cursor_y = MAX_LINES-1;
            while (document[cursor_y].len == 0)
                cursor_y -= 1;
            break;
        case 'n': // toggle show_line_numbers
            show_line_numbers = !show_line_numbers;
            break;
        case 'D': // debug input by printing the ascii value of your inputs
            debug = !debug;
            system("clear");
            break;
        case '?': // show help info
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
        default:
    }
    clip_cursor_to_grid();
}

/////////////////////////////////////////////////////////////////////////////////
// Escape Sequences!
//

enum escape_sequences_enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    DELETE,
    HOME,
    END,
    PAGE_UP,
    PAGE_DOWN,
    SECRET,
    COLORS,
    
    NUM_ESCAPE_SEQUENCES, // <--- important, used by for loop
    INCOMPLETE_ESCAPE_SEQUENCE,
    INVALID_ESCAPE_SEQUENCE
};
#define MAX_ESCAPE_SEQUENCE_LEN 8
// These are the escape sequences which will be checked for (the "<escape>[" is omitted here).
static char escape_sequences[NUM_ESCAPE_SEQUENCES][MAX_ESCAPE_SEQUENCE_LEN] = {
    {'A'}, // up
    {'B'}, // down
    {'D'}, // left
    {'C'}, // right
    {'3', '~'}, // delete
    {'H'}, // home
    {'F'}, // end
    {'5', '~'}, // page up
    {'6', '~'}, // page down
    {"secret"}, // secret
    {"colors"}, // keyword color-coding
};

// Add your own escape codes to the enum in gred.h!
void run_escape_code(int escape_code_index) {
    switch(escape_code_index) {
        case INVALID_ESCAPE_SEQUENCE: // ignore the sequence
            break;
        case INCOMPLETE_ESCAPE_SEQUENCE: // incomplete escape sequence
            return;
        // These are the escape codes for the arrow keys!
        case LEFT: // left
            cursor_x -= 1;
            break;
        case RIGHT: // right
            cursor_x += 1;
            break;
        case UP: // up
            cursor_y -= 1;
            break;
        case DOWN: // down
            cursor_y += 1;
            break;
        // more navigation keys
        case HOME: // go to begginning of line
            cursor_x = 0;
            break;
        case END: // go to end of line
            clip_cursor_to_grid(); // paranoia
            cursor_x = document[cursor_y].len;
            break;
        case PAGE_UP: // scroll the screen up
            top_line_of_screen -= screen_height-1;
            cursor_y -= screen_height-1;
            break;
        case PAGE_DOWN: // scroll the screen down
            top_line_of_screen += screen_height-1;
            cursor_y += screen_height-1;
            break;
        // non-navigation keys
        case DELETE:
            record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
            line_backspace(cursor_x+1, &document[cursor_y]);
            record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            break;
        case SECRET:
            show_line_numbers = 0;
            menu_alert = secret_message;
            break;
        case COLORS:
            colorize = !colorize;
            redraw_full_screen = 1;
            break;
        default:
            break;
    }
    in_escape_sequence = 0;
    cur_escape_sequence.len = 0;
    clip_cursor_to_grid();
}

// add to the current escape code, return an index number for it if a match is found
int build_escape_sequence(char c) {
    line_insert(c, cur_escape_sequence.len, &cur_escape_sequence);
    if (cur_escape_sequence.len > MAX_ESCAPE_SEQUENCE_LEN)
        return INVALID_ESCAPE_SEQUENCE;
    int checked_code_len = 0;
    int partial_match = 0;
    int mismatch = 0;
    for (int i=0; i<NUM_ESCAPE_SEQUENCES; i++) {
        mismatch = 0;
        partial_match = 0;
        checked_code_len = strnlen(escape_sequences[i], MAX_ESCAPE_SEQUENCE_LEN);
        for (int j=0; j<checked_code_len; j++) {
            if (cur_escape_sequence.text[j] != escape_sequences[i][j]) {
                mismatch = 1;
                break;
            }
            else {
                partial_match = 1;
            }
        }
        if (!mismatch) {
            return i;
        }
        if (partial_match) {
            // show the escape sequence in the menu
            menu_alert = &cur_escape_sequence.text[0];
            return INCOMPLETE_ESCAPE_SEQUENCE;
        }
    }
    // no partial or complete matches
    return INVALID_ESCAPE_SEQUENCE;
}























// Info to show in the help menu:
char help_pages[NUM_HELP_PAGES][MAX_HELP_PAGE_LEN] = {
    {
    "Start adding text by entering INSERT_MODE\n\n"
    "Press i to enter INSERT_MODE\n"
    "You can then type in text as you would expect.\n"
    "\n"
    "Press <escape> to enter ESCAPE_MODE\n"
    "In ESCAPE_MODE you can:\n"
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
    "ESCAPE_MODE is for running various commands.\n"
    "You can save, undo/redo, ect. in this mode.\n"
    "Press <escape> at any time to enter ESCAPE_MODE.\n"
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
