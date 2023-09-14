//See LICENSE file for copyright and license details.
// Help info for using gred, the Grid-based text Editor
#include "gred.h"

int selecting = 0;
int sel_cursor_x = 0;
int sel_cursor_y = 0;
#define COPY_BUFFER_SIZE 4096
int copy_len = 0;
char copy_buffer[COPY_BUFFER_SIZE];

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

// run a command from the commands enum in gred.h (can be from a keybind or an escape code)
void run_command(int command_id) {
    int start_x = cursor_x; // Used for remembering the cursor's initial position.
    int start_y = cursor_y;
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
    case SCROLL_UP_AUTO:
        auto_scroll_dir = UP;
        auto_scroll_cursor_only = 1;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_DOWN_AUTO:
        auto_scroll_dir = DOWN;
        auto_scroll_cursor_only = 1;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_LEFT_AUTO:
        auto_scroll_dir = LEFT;
        auto_scroll_cursor_only = 1;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_RIGHT_AUTO:
        auto_scroll_dir = RIGHT;
        auto_scroll_cursor_only = 1;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_UP_AUTO_CAP:
        auto_scroll_dir = UP;
        auto_scroll_cursor_only = 0;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_DOWN_AUTO_CAP:
        auto_scroll_dir = DOWN;
        auto_scroll_cursor_only = 0;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_LEFT_AUTO_CAP:
        auto_scroll_dir = LEFT;
        auto_scroll_cursor_only = 0;
        open_menu(menu_auto_scroll);
        break;
    case SCROLL_RIGHT_AUTO_CAP:
        auto_scroll_dir = RIGHT;
        auto_scroll_cursor_only = 0;
        open_menu(menu_auto_scroll);
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
    case SCROLL_CENTER:
        alert("Screen centered.");
        display_text_top = cursor_y - (display_full_height/2);
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
    case FIND_CHAR_NEXT: // Search for char left/right.
        open_menu(menu_find_char_next);
        break;
    case FIND_CHAR_PREV:
        open_menu(menu_find_char_prev);
        break;
    case ELEVATOR_UP: // Search for char up/down.
        open_menu(menu_elevator_up);
        break;
    case ELEVATOR_DOWN:
        open_menu(menu_elevator_down);
        break;
    //
    // Editing:
    //
    case UNDO:
        undo();
        break;
    case REDO:
        redo();
        break;
    case DELETE:
        chain_start(cursor_x, cursor_y);
        delete_char(cursor_x, cursor_y);
        chain_end(cursor_x, cursor_y);
        break;
    case DELETE_WORD: // TODO this <-------------------------------------- TODO
        break;
    case DELETE_LINE:
        chain_start(cursor_x, cursor_y);
        while(document[cursor_y].len > 0) {
            delete_char(0, cursor_y);
        }
        delete_empty_line(cursor_x, cursor_y);
        cursor_y += 1;
        chain_end(start_x, start_y);
        break;
    case DELETE_TRAILING:
        chain_start(cursor_x, cursor_y);
        while(document[cursor_y].len > cursor_x) {
            delete_char(cursor_x, cursor_y);
        }
        chain_end(cursor_x, cursor_y);
        break;
    case INSERT:
        insert(cur_char); // Handle the INSERT_MODE character. See input.c for details.
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
        chain_start(cursor_x, cursor_y);
        insert_empty_line(cursor_x, cursor_y+1);
        cursor_move(cursor_x, cursor_y);
        chain_end(cursor_x, cursor_y);
        switch_mode(INSERT_MODE);
        remember_mode(INSERT_MODE);
        break;
    case SWITCH_TO_INSERT_MODE_IN_NEW_LINE_ABOVE:
        chain_start(cursor_x, cursor_y);
        insert_empty_line(cursor_x, cursor_y);
        cursor_move(cursor_x, cursor_y);
        chain_end(cursor_x, cursor_y);
        switch_mode(INSERT_MODE);
        remember_mode(INSERT_MODE);
        break;
    case SELECT: // Set a selection.
        display_redraw_all = 1;
        if (selecting) {
            selecting = 0;
            alert("Stopped selecting.");
        }
        else {
            selecting = 1;
            sel_cursor_x = cursor_x;
            sel_cursor_y = cursor_y;
            alert("Started selecting.");
        }
        break;
    case COPY:
        copy_len = 0;
        if (!selecting && (cursor_x < document[cursor_y].len)) {
            copy_buffer[0] = document[cursor_y].text[cursor_x];
            copy_len = 1;
        }
        if (selecting) {
            cursor_x = sel_left; //TODO confirm sel bounds
            cursor_y = sel_top;
            for (int i=0; i<COPY_BUFFER_SIZE; i++) {
                if (cursor_x > sel_right)
                    break;
                else if (cursor_x < document[cursor_y].len)
                    copy_buffer[i] = document[cursor_y].text[cursor_x];
                copy_len += 1;
                cursor_x += 1;
            }
            cursor_x = start_x;
            cursor_y = start_y;
        }
        if (copy_len < COPY_BUFFER_SIZE-1) // null terminate, so it prints properly
            copy_buffer[copy_len] = 0;
        alert(copy_buffer);
        break;
    case PASTE:
        alert("Paste.");
        if (copy_len < 1) // If nothing copied, don't paste.
            break;
        chain_start(cursor_x, cursor_y);
        if (cursor_x > document[cursor_y].len)
            cursor_x = document[cursor_y].len;
        for (int i=0; i<copy_len; i++) {
            if (copy_buffer[i] == '\n')
                insert_empty_line(cursor_x, cursor_y);
            else
                insert_char(copy_buffer[i], cursor_x, cursor_y);
        }
        chain_end(cursor_x, cursor_y);
        break;
    case MACRO:
        in_macro = 1;
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
    case SAVE_AS:
        alert("Opening save menu...");
        open_menu(&menu_save_file);
        break;
    case SAVE:
        alert("File saved.");
        save_file(file_name.text);
        break;
    case QUIT:
        if (in_tutorial) {
            command = HELP; // Close the tutorial.
        }
        else {
            quit = 1; // Exit the program.
            // Delete any temp files.
            pclose(popen("rm /tmp/cur_document.txt", "r"));
            break;
        }
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
                        "/.dotfiles/gred/tutorial.txt");
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
    case SAVE_AND_QUIT:
        save_file(file_name.text);
        quit = 1;
        break;
    case DEBUG:
        system("clear");
        debug = ~debug;
        display_redraw_all = 1;
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
        if (mode == COMMAND_MODE)
            remember_mode(mode);
        else
            switch_mode(COMMAND_MODE);
        line_clear(&input);
        return NO_COMMAND;
    }
    else if (mode == INSERT_MODE) {
        return INSERT; // Allow insertion of the character.
    }
    // Add the c to the input buffer.
    line_insert_char(c, input.len, &input);
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
            line_clear(&input);
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
        line_clear(&input);
    }
    // No matches at all.
    return NO_COMMAND;
}


char secret_message[] = "You found a secret!"; // Hi there! :D
