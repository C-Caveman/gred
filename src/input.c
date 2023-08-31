//See LICENSE file for copyright and license details.
// Handle input to the text editor.
#include "gred.h"

// Get a character immediately after it is pressed.
// from: https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
char getch() {
        char c = '?';
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        c = fgetc(stdin);
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (c);
}

// swap between editing and running commands
void switch_mode(int next_mode) {
    mode = next_mode;
    if (mode == ESCAPE_MODE) {
        menu_prompt = escape_mode_help;
    }
    if (mode == INSERT_MODE) {
        menu_prompt = insert_mode_help;
    }
}
// For when the user manually switches mode. Allows returning to insert mode after escape sequences.
void remember_mode(int next_mode) {
    previous_mode = mode;
}

void handle_insert_mode_newline(int column, int row) {
    int num_empty_lines = find_num_empty_lines();
    // Only insert if there is space to do so.
    if (num_empty_lines <= 0 || row >= MAX_LINES-1) {
        return;
    }
    int extra_line_len = document[row].len - (column);
    // make room for the new line
    record_before_edit(column, row, FIRST_OF_MULTIPLE_EDITS);
    insert_new_empty_line(row);
    record_after_edit(0, row+1, EDIT_INSERT_LINE);
    // put the end of the current line into the new line
    record_before_edit(column, row, MIDDLE_OF_MULTIPLE_EDITS);
    line_copy_range(&document[row],   0, LINE_WIDTH, 
                    &document[row+1],     0, column);
    document[row].len = column; // reduce the current line length
    record_after_edit(column, row, EDIT_CHANGE_LINE);
    // remove the start of the current line
    record_before_edit(0, row+1, LAST_OF_MULTIPLE_EDITS);
    line_copy_range(&document[row+1],     0,      LINE_WIDTH, 
                    &document[row+1],     column, LINE_WIDTH);
    document[row+1].len = extra_line_len;;
    record_after_edit(0, row+1, EDIT_CHANGE_LINE);
    cursor_y += 1;
    cursor_x = 0;
}

// Process a keystroke from the user.
void handle_input(char c) {
    int line_len = document[cursor_y].len;
    // handle escape code
    if (in_escape_sequence) {
        run_escape_code(build_escape_sequence(c));
        return;
    }
    // run escape codes or user commands
    if (mode == ESCAPE_MODE) {
        // enter a terminal escape sequence (used for arrow keys)
        if (c == '[') {
            in_escape_sequence = 1;
            switch_mode(previous_mode); // return to the mode the user chose explicitly
        }
        // run an escape mode command
        else {
            // prevent reverting to INSERT_MODE on next '[' char
            switch_mode(ESCAPE_MODE);
            remember_mode(ESCAPE_MODE); // user explicitly chose this mode
            run_command(c);
        }
        return;
    }
    // INSERT_MODE
    if (mode == INSERT_MODE && c != 0 && !in_escape_sequence) {
        if (c == ESCAPE) {
            switch_mode(ESCAPE_MODE);
            cur_escape_sequence.len = 0;
            return;
        }
        if (c == BACKSPACE) {
            if (cursor_x == 0 && cursor_y > 0) { // merge current and previous lines
                merge_line_upwards(cursor_y);
                cursor_y -= 1;
            }
            else if (line_len > 0) {             // delete the character next to the cursor
                record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
                cursor_x = line_backspace(cursor_x, &document[cursor_y]);
                record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            }
            else if (line_len == 0 && cursor_y > 0) { // delete an empty line
                record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
                delete_empty_line(cursor_y);
                cursor_y -= 1;
                record_after_edit(cursor_x, cursor_y, EDIT_DELETE_LINE);
            }
            return;
        }
        if (document[cursor_y].len < LINE_WIDTH) { // insert the current character
            if (c == '\n') {
                handle_insert_mode_newline(cursor_x, cursor_y);
            }
            else if (isprint(c) && c != '\t') { // only insert printable characters!
                record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
                cursor_x = line_insert(c, cursor_x, &document[cursor_y]);
                record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            }
            else if (c == '\t') {
                record_before_edit(cursor_x, cursor_y, SINGLE_EDIT);
                if (!use_tabulators) {
                    for (int i=0; i<num_tab_spaces; i++)
                        cursor_x = line_insert(' ', cursor_x, &document[cursor_y]);
                }
                else {
                    cursor_x = line_insert('\t', cursor_x, &document[cursor_y]);
                }         
                record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
            }
        }
    }
}
