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

#define LINE_WIDTH 256
#define MAX_LINES 4096

enum line_flags { // properties a line can have
    CHANGED=0b1,
    WRAPPED=0b10,
};
// Lines of text that make up a document.
struct line {
    int len; // keep track of the line length for every line
    int flags; // info about the line
    char text[LINE_WIDTH];
    // If you add another member, you must update the copy_line() function in gred.c!
};

// The document.
extern struct line document[MAX_LINES];

//
// State of the editor:
//
extern int cursor_x; // where we are in the document
extern int cursor_y;
extern int mode; // Whether in ESCAPE_MODE or INSERT_MODE (running commands or inserting text)
extern struct line file_name; // Name of current file.
extern int top_line_of_screen; // index of the top line displayed on the screen
extern int text_display_x_start; // index of leftmost char displayed.
extern int text_display_x_start; // index of first column displayed on the screen
extern int screen_height; // number of document lines that are displayed at once
extern int in_escape_sequence; // whether of not an escape sequence is being processed
extern struct line cur_escape_sequence; // current escape code (omitting the initial "<escape>[")
extern int show_line_numbers; // whether to display line numbers or not
extern int quit; // if exiting the editor or not
extern int debug; // if displaying input debug information or not (disables normal text)
// Menu state:
extern int in_menu; // Currently in a menu?
extern int menu_cursor_x;
extern struct line menu_input; // Text input to the current menu.
extern char* menu_prompt;// Text displayed by the current menu.
extern char* menu_alert; // Message that can be set anywhere, disapears next time a key is pressed.
#define MAX_MENU_PROMPT_WIDTH 16
extern char insert_mode_help[]; // Menu text when in insert mode.
extern char escape_mode_help[]; // Menu text when in escape mode.
// Editor settings:
extern int keyword_coloring;

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
    NUM_EDIT_TYPES
};
enum edit_sequence_types { // for linking multiple edits to be undone/redone at once
    SINGLE_EDIT,
    FIRST_OF_MULTIPLE_EDITS,
    MIDDLE_OF_MULTIPLE_EDITS,
    LAST_OF_MULTIPLE_EDITS
};

#define BACKSPACE 127
#define ESCAPE 27
#define CTRL_S 19

// The editor has 2 modes, insert mode for typing, and escape mode for everything else.
enum modes { // for escape codes
    INSERT_MODE,
    ESCAPE_MODE,
    NUM_MODES
};
#define MAX_HELP_LINE_WIDTH 256

//
// Document display functions:
//
// show the document and the menu
void draw_screen();
// keep the cursor from leaving the document
void clip_cursor_to_grid();
void print_highlighted(struct line* l, int left_index, int right_index);

//
// Document editing functions:
//
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

//
// User input functions:
//
// grab the next character the user types
char getch();
// run a command from escape mode
void run_command(char c);
// perform an operation for a given escape code
void run_escape_code(int escape_code_index);
// open the save_file menu
void menu_save_file();
// search for word
void menu_search();

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

// misc utilites
int bound_value(int v, int min, int max); // clips value to [min, max]

extern char secret_message[];

#endif
