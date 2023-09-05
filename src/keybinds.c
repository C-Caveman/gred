//See LICENSE file for copyright and license details.
// Help info for using gred, the Grid-based text Editor
#include "gred.h"

// Tutorial information (used for switching to and from the tutorial).
int in_tutorial = 0;
struct line pre_tutorial_file_name;
int pre_tutorial_cursor_x;
int pre_tutorial_cursor_y;
int tutorial_cursor_x = 0;
int tutorial_cursor_y = 0;
int pre_tutorial_display_text_top; // Screen position.
int tutorial_display_text_top = 0;

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
/* Shell script to print these bindings in tutorial format:
sed -n '/Start of bindings./,/End of bindings./p' keybinds.c | 
    sed 's/{"/</; s/",/>/; s/},//'
*/
static struct binding bindings[] = {
    
    // Editing:
    {"u", UNDO}, // undo
    {"r", REDO}, // redo
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
        if (display_text_top > 0) {
            cursor_y -= 1;
            display_text_top -= 1;
        }
        break;
    case SCROLL_DOWN:
        cursor_y += 1;
        display_text_top += 1;
        break;
    case SCROLL_LEFT:
        if (display_text_x_start > 0) {
            cursor_x -= 1;
            display_text_x_start -= 1;
        }
        break;
    case SCROLL_RIGHT:
        cursor_x += 1;
        display_text_x_start += 1;
        break;
    case SCROLL_PAGE_UP: // Move screen up by (display_text_height-1).
        display_text_top -= display_text_height-1;
        cursor_y -= display_text_height-1;
        break;
    case SCROLL_PAGE_DOWN:
        display_text_top += display_text_height-1;
        cursor_y += display_text_height-1;
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
        display_redraw_all = 1;
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
        // Delete any temp files.
        pclose(popen("rm /tmp/cur_document.txt", "r"));
        break;
    // Help menu
    case HELP:
        if (!in_tutorial) {
            in_tutorial = 1;
            // Remember the pre_tutorial cursor position.
            pre_tutorial_cursor_x = cursor_x;
            pre_tutorial_cursor_y = cursor_y;
            pre_tutorial_display_text_top = display_text_top;
            // Reset the cursor position to where it was in the tutorial last time.
            cursor_x = tutorial_cursor_x;
            cursor_y = tutorial_cursor_y;
            display_text_top = tutorial_display_text_top;
            // Store the document in a temporary file.
            save_file("/tmp/cur_document.txt");
            // Store the file_name so we can restore it later.
            strncpy(pre_tutorial_file_name.text, file_name.text, LINE_WIDTH);
            pre_tutorial_file_name.len = strlen(file_name.text);
            // Open the tutorial
            #define TUT_FNAME_BUFFER_SIZE 64
            char tut_fname_buffer[TUT_FNAME_BUFFER_SIZE];
            snprintf(tut_fname_buffer, 
                        TUT_FNAME_BUFFER_SIZE, 
                        "%s%s", 
                        getenv("HOME"),
                        "/.dotfiles/gred/gred_tutorial.txt");
            load_file(tut_fname_buffer);
            // Change the file_name to prevent overwriting the tutorial or the document.
            strncpy(file_name.text, "tutorial.txt", LINE_WIDTH);
            file_name.len = strlen("tutorial.txt");
        }
        else {
            in_tutorial = 0;
            // Remember the tutorial cursor position.
            tutorial_cursor_x = cursor_x;
            tutorial_cursor_y = cursor_y;
            tutorial_display_text_top = display_text_top;
            // Restore the pre_tutorial cursor position.
            cursor_x = pre_tutorial_cursor_x;
            cursor_y = pre_tutorial_cursor_y;
            display_text_top = pre_tutorial_display_text_top;
            // Open the original document.
            load_file("/tmp/cur_document.txt");
            // Delete the temp file.
            pclose(popen("rm /tmp/cur_document.txt", "r"));
            // Restore the original file_name.
            strncpy(file_name.text, pre_tutorial_file_name.text, LINE_WIDTH);
            file_name.len = strlen(pre_tutorial_file_name.text);
        }
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


char secret_message[] = "You found a secret!"; // Hi there! :D
