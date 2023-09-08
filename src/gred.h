//See LICENSE file for copyright and license details.
// Simple Grid-based text-Editor 
#ifndef GRED_H

#include "stdio.h"
#include <sys/ioctl.h> // get terminal xy in draw_screen()
#include <unistd.h>    // get terminal xy in draw_screen()
#include <stdlib.h>    // system("clear") in draw_screen()
#include <termios.h>   // instant terminal input in getch()
#include "string.h"    // strnlen() in draw_screen()
#include <ctype.h>     // isprint() in handle_input()

// Lines of text that make up a document.
#define LINE_WIDTH 256
struct line {
    int len; // keep track of the line length for every line
    int flags; // info about the line
    char text[LINE_WIDTH];
    // If you add another member, you must update the copy_line() function in gred.c!
};
enum line_flags { // properties a line of text can have
    CHANGED=  0b1,
    WRAPPED= 0b10,
};

//
// Changes to the document are made using the functions in "editing.c"   ::
//
enum edit_types {
    INSERT_CHAR, // Add a character at x,y.
    DELETE_CHAR, // Remove a character at x,y.
    INSERT_EMPTY_LINE, // Add a line at y.
    DELETE_EMPTY_LINE, // Remove a line at y.
    CURSOR_MOVE, // Move cursor to x,y. (used for tracking unusual cursor movement)
    CHAIN_START, // Bookend a chain of multiple edits.
    CHAIN_END,   //  (bookended edits are undone/redone as one)
    MACRO_START,
    MACRO_END,
};
enum edit_flags { // Used to mark edits as part of a CHAIN/MACRO.
    IN_CHAIN =  0b1,
    IN_MACRO =  0b01, // use this <---------- TODO
};

// The editor has 2 modes, insert mode for typing, and escape mode for everything else.
enum modes { // for escape codes
    INSERT_MODE,
    COMMAND_MODE,
    NUM_MODES
};

#define MAX_LINES 4096 // Maximum document length allowed.

// Document being edited:
extern struct line document[MAX_LINES];
extern struct line file_name; // Name of current document.
extern int in_tutorial;

// Editor mode:
extern int mode; // INSERT_MODE or COMMAND_MODE
extern int previous_mode; // last mode used (used to revert the mode after an escape sequence)
extern int in_chain; // Whether recording a series of edits or not. Used by undo_redo.c
extern int in_macro; // Whether recording a macro or not. Used by undo_redo.c
extern struct line macro_buffer; // Characters recorded for a macro.

// Cursor position in document:
extern int cursor_x; // where we are in the document
extern int cursor_y;

// Current command and input character:
extern int command;
extern struct line input; // Character(s) used to call the current command.
extern char cur_char;
extern char prev_char;

// Keybindings:
#define MAX_BINDING_LEN 16
struct binding {
    char input[MAX_BINDING_LEN];
    int command_id;
};
extern struct binding bindings[];

// Screen data:
extern int display_text_top; // index of the top line displayed on the screen
extern int display_text_top_old; // Previous top line of document displayed.
extern int display_text_x_start; // index of leftmost char displayed on the screen
extern int display_text_x_end; // index of rightmost char displayed on the screen
extern int display_text_height; // number of document lines that are displayed at once
extern int display_full_height; // terminal height (# of lines)
extern int display_full_width; // number of document characters that fit on screen
extern int display_redraw_all; // flag for redrawing the full screen
extern int display_line_number_width; // full width of the line numbers section, includes padding
extern int menu_height; // how many lines tall the menu is

// Menu:
extern void (*menu)(); // Pointer to a menu. Set this with open_menu().
extern int cursor_in_menu; // If the cursor is in the menu.
extern int menu_cursor_x; // Position of cursor when in the menu_input text box.
extern int reading_menu_input; // If currently typing into menu_input.
extern int menu_was_just_opened; // Set to 1 when open_menu() is called.
extern struct line menu_input; // Text input to the current menu.
extern char* menu_prompt; // Prompt for the current menu. Displays info about the current mode when not in a menu.
extern char* menu_alert;  // Temporary message. Dissapears upon the next screen refresh.
#define MAX_MENU_PROMPT_WIDTH 16
extern char insert_mode_help[]; // Menu text when in insert mode.
extern char escape_mode_help[]; // Menu text when in escape mode.

// Editor settings:
extern int show_line_numbers; // whether to display line numbers or not
extern int colorize; // change color of keywords such as "int" and "return"
extern int use_tabulators;
extern int num_tab_spaces;
extern int mode_specific_cursors_enabled; // whether the terminal supports bar/box cursor swapping

// Special state flags:
extern int quit; // if exiting the editor or not
extern int debug; // if displaying input debug information or not (disables normal text)


// ASCII values for various characters.   ::
#define BACKSPACE 127
#define ESCAPE 27
#define CTRL_S 19


//
// Functions:        ::
//

//
// Document display functions:
//
// show the document and the menu
void draw_screen();
// redraw the menu
void draw_menu();
// keep the cursor from leaving the document
void clip_cursor_to_grid();
void display_line_highlighted(struct line* l, int left_index, int right_index);
// set cursor position in terminal (where cursor is shown, and text from printf goes)
void move_cursor(int x, int y);
// print a line of the document
void display_line(struct line* l, int start_index, int stop_index);
// Check if the terminal supports having a vertical bar for the cursor.
int check_vertical_bar_cursor_supported();

//
// Document editing functions:
//
// Load a text file.
void load_file(char* fname);
// Handle a character input in INSERT_MODE
void insert(char c);
//
// Make an edit to the document.    ::
//
void insert_char(char c, int x, int y);
void delete_char(int x, int y);
void insert_empty_line(int x, int y);
void delete_empty_line(int x, int y);
void cursor_move(int x, int y);
void chain_start(int x, int y);
void chain_end(int x, int y);
void macro_start(int x, int y);
void macro_end(int x, int y);
//                          WARNING:
// Edit a NON-DOCUMENT LINE (no undo/redo support for these)
//
int line_insert_char(char c, int cursor_x_pos, struct line* l);
int line_delete_char(int cursor_x_pos, struct line* l);
void line_clear(struct line* l);
int add_line(int row);
int delete_line(int row);
void copy_line(struct line* a, struct line* b); // copy line a's text and length into b
int line_insert(char c, int cursor_x_pos, struct line* l); // insert on line l, return new cursor_x position
void merge_line_upwards(int row); // move the current line into the previous line
int find_num_empty_lines(); // Get the # of lines after the last non-empty line.
void line_copy_range(struct line* a, int a_first, int a_last, // Copy given region of line b into given region of line a
                     struct line* b, int b_first, int b_last);

//
// User input functions:
//
// grab the next character the user types
char getch();
// Process a keystroke from the user.
void handle_input(char c);
// Add user's input to "input", return the command that it matches.
int get_command(char c);
// run a command from escape mode
void run_command(int command_id);
// perform an operation for a given escape code
void run_escape_code(int escape_code_index);

//
// Menus:
//
// open a menu using a function pointer (uses "cur_char" and "command" for input)
void open_menu(void (*cur_menu)());
// Exit current menu if escape was double-tapped.
void double_escape_menu_exit();
// save_file menu
void menu_save_file();
// search menu
void menu_search();
// Set the file type of the document (.txt, .c, Makefile, ect.)
int get_file_type(struct line* fname);
// Update settings such as use_tabulators for a given file type
void update_settings_from_file_type(int file_type);

//
// Escape code processing:
//

// track edits for undo/redo
void undo();
void redo();
void chain_start(int x, int y);
void chain_end(int cursor_x, int cursor_y);
void save_file(char* fname);

// change state of editor
void switch_mode(int next_mode);
void remember_mode(int next_mode);

// utilites for debugging
void debug_input(char c);
void print_debug_input(char c);
void print_debug_mode(int state);
void test_highlighting();
void doi(); // Print "DOI!!!" and quit.
// misc utilites
int bound_value(int v, int min, int max); // clips value to [min, max]

//
// Commands:
//
enum COMMANDS_ENUM {
    NO_COMMAND,

    // Navigation
    LEFT,
    RIGHT,
    UP,
    DOWN,
    GOTO_LINE_START,
    GOTO_LINE_END,
    GOTO_DOCUMENT_TOP,
    GOTO_DOCUMENT_BOTTOM,
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_LEFT,
    SCROLL_RIGHT,
    SCROLL_PAGE_UP,
    SCROLL_PAGE_DOWN,
    SEARCH,
    FIND_CHAR_NEXT,
    FIND_CHAR_PREV,

    //Editing
    UNDO,
    REDO,
    DELETE,
    DELETE_WORD,
    DELETE_TRAILING,
    INSERT,
    SWITCH_TO_INSERT_MODE,
    SWITCH_TO_INSERT_MODE_AT_START_OF_LINE,
    SWITCH_TO_INSERT_MODE_AT_END_OF_LINE,
    SWITCH_TO_INSERT_MODE_IN_NEW_LINE_BELOW,
    SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE,
    SELECT, // Begin selecting some text.
    COPY, // Copy selected text. TODO add default behavior that copies the currend word.
    PASTE,

    //Settings
    LINE_NUMBERS,
    SECRET,
    COLORIZE,
    DEBUG,
    MACRO,
    SAVE,
    QUIT,
    HELP,
};
extern char secret_message[];

#endif
