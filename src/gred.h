//See LICENSE file for copyright and license details.
// Simple Grid-based text-Editor 
#ifndef GRED_H

#include "stdio.h"
#include <sys/ioctl.h> // get terminal xy in draw_screen()
#include <unistd.h>    // get terminal xy in draw_screen()
#include <stdlib.h>    // system("clear") in draw_screen()
#include <termios.h>   // terminal input in getch()
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
enum line_flags { // properties a line can have
    CHANGED=  0b1,
    WRAPPED= 0b10,
};

// Information used to undo/redo changes to a document.
struct edit {
    // what operation was performed
    int type;
    // location of cursor before edit
    int before_x, before_y;
    // location of cursor after edit
    int after_x, after_y;
    // whether this is the start of a sequence of edits
    int start_of_sequence;
    // whether this is the end of a sequence of edits
    int end_of_sequence;
    // copy of the line before and after editing
    struct line before; // document[before_y] before edit
    struct line after;  // document[before_y] after edit
};
enum edit_types { // used by the undo/redo system
    BEFORE_EDIT,
    EDIT_CHANGE_LINE,
    EDIT_DELETE_LINE,
    EDIT_INSERT_LINE,
    EDIT_MOVE_CURSOR,
    NUM_EDIT_TYPES
};
enum edit_sequence_types { // for linking multiple edits to be undone/redone at once
    SINGLE_EDIT,
    FIRST_OF_MULTIPLE_EDITS,
    MIDDLE_OF_MULTIPLE_EDITS,
    LAST_OF_MULTIPLE_EDITS
};

// The editor has 2 modes, insert mode for typing, and escape mode for everything else.
enum modes { // for escape codes
    INSERT_MODE,
    COMMAND_MODE,
    NUM_MODES
};

// Document being edited:
#define MAX_LINES 4096
extern struct line document[MAX_LINES];

// Editor mode:
extern int mode; // INSERT_MODE or COMMAND_MODE
extern int previous_mode; // last mode used (used to revert the mode after an escape sequence)
extern int recording_macro;

// Cursor position in document:
extern int cursor_x; // where we are in the document
extern int cursor_y;

// Current command and input character:
extern int command;
extern struct line input; // Invocation of the command.
extern char cur_char;
extern char prev_char;

// Screen data:
extern struct line file_name; // Name of current file.
extern int top_line_of_screen; // index of the top line displayed on the screen
extern int old_top_line_of_screen; // Previous top line of document displayed.
extern int text_display_x_start; // index of leftmost char displayed on the screen
extern int text_display_x_end; // index of rightmost char displayed on the screen
extern int total_screen_height; // terminal height (# of lines)
extern int screen_height; // number of document lines that are displayed at once
extern int screen_width; // number of document characters that fit on screen
extern int redraw_full_screen; // flag for redrawing the full screen
extern int total_line_number_width; // full width of the line numbers section, includes padding
extern int menu_height; // how many lines tall the menu is

// Menu:
extern void (*menu)(); // pointer to a menu function
extern int cursor_in_menu; // If the cursor is in the menu.
extern int menu_cursor_x;
extern int reading_menu_input;
extern int menu_was_just_opened;
extern struct line menu_input; // Text input to the current menu.
extern char* menu_prompt;// Text displayed by the current menu.
extern char* menu_alert; // Message that can be set anywhere, disapears next time a key is pressed.
#define MAX_MENU_PROMPT_WIDTH 16
extern char insert_mode_help[]; // Menu text when in insert mode.
extern char escape_mode_help[]; // Menu text when in escape mode.

// Editor settings:
extern int show_line_numbers; // whether to display line numbers or not
extern int colorize; // change color of keywords such as "int" and "return"
extern int use_tabulators;
extern int num_tab_spaces;
extern int mode_specific_cursors_enabled; // whether the terminal supports bar/box cursor swapping

// Escape sequences:
extern int in_escape_sequence; // whether of not an escape sequence is being processed
extern struct line cur_escape_sequence; // current escape code (omitting the initial "<escape>[")

// Special state flags:
extern int quit; // if exiting the editor or not
extern int debug; // if displaying input debug information or not (disables normal text)





#define BACKSPACE 127
#define ESCAPE 27
#define CTRL_S 19

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
// Handle a character input in INSERT_MODE
void insert(char c);
// copy line a's text and length into b
void copy_line(struct line* a, struct line* b);
// backspace on line l, return new cursor_x position
int line_backspace(int cursor_x_pos, struct line* l);
// remove an empty line from the document
void delete_empty_line(int row);
// insert on line l, return new cursor_x position
int line_insert(char c, int cursor_x_pos, struct line* l);
// move the current line into the previous line
void merge_line_upwards(int row);
// Insert a new empty line at the given row.
void insert_new_empty_line(int row);
// Get the # of lines after the last non-empty line.
int find_num_empty_lines();
// Copy given region of line b into given region of line a
void line_copy_range(struct line* a, int a_first, int a_last,
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
int build_escape_sequence(char c);

// track edits for undo/redo
void undo();
void redo();
void record_before_edit(int x, int y, int is_this_the_first_last_or_only_edit_in_this_sequence);
void record_after_edit(int cursor_x, int cursor_y, int type_of_edit);
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
    UNDO,
    REDO,
    DELETE,
    INSERT, //TODO rethink this <--------------------------------- TODO
    SWITCH_TO_INSERT_MODE,
    SWITCH_TO_INSERT_MODE_AT_START_OF_LINE,
    SWITCH_TO_INSERT_MODE_AT_END_OF_LINE,
    SWITCH_TO_INSERT_MODE_IN_NEW_LINE_BELOW,
    SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE,
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
