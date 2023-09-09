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
    int unicode_chars = 0;
    if (mode == COMMAND_MODE) {
        menu_prompt = escape_mode_help;
        for (int i=0; i<document[cursor_y].len; i++) {
            if (document[cursor_y].text[i] & 0b10000000)
                unicode_chars += 1;
        }
        cursor_x -= (unicode_chars > 0) ? unicode_chars-1 : 0;
    }
    if (mode == INSERT_MODE) {
        menu_prompt = insert_mode_help;
        // Adjust the cursor to deal with multi-byte unicode characters.
        for (int i=0; i<document[cursor_y].len; i++) {
            if (document[cursor_y].text[i] & 0b10000000)
                unicode_chars += 1;
        }
        cursor_x += (unicode_chars > 0) ? unicode_chars-1 : 0;
    }
}
// For when the user manually switches mode. Allows returning to insert mode after escape sequences.
void remember_mode(int next_mode) {
    previous_mode = next_mode;
}

void handle_insert_mode_newline(int column, int row) {
    int num_empty_lines = find_num_empty_lines();
    // Only insert if there is space to do so.
    if (num_empty_lines <= 0 || row >= MAX_LINES-1) {
        return;
    }
    chain_start(column, row);
    // make room for the new line
    insert_empty_line(cursor_x, row+1);
    
    // Move end of current line to the new line.
    while (document[row].len > column) {
        // Add last character of current line to the end of the next line.
        insert_char(document[row].text[column], 
                    document[row+1].len, 
                    row+1);
        delete_char(column, row); // Shorten tail of current line.
    }
    cursor_x = 0;
    cursor_y = row+1;
    chain_end(cursor_x, cursor_y);
}

extern int num_undone; // Didn't want this in the header. Used to clear out the redo stack.
// Process a keystroke from the user in INSERT_MODE.
void insert(char c) {
    int line_len = document[cursor_y].len;
    num_undone = 0; // Clear out the redo stack.
    if (c == BACKSPACE) {
        if (cursor_x == 0 && cursor_y > 0) { // merge current and previous lines
            merge_line_upwards(cursor_y);
        }
        else if (line_len > 0) {             // delete the character next to the cursor
            delete_char(cursor_x-1, cursor_y);
        }
        else if (line_len == 0 && cursor_y > 0) { // delete an empty line
            delete_empty_line(cursor_x, cursor_y);
        }
        return;
    }
    if (document[cursor_y].len < LINE_WIDTH) { // insert the current character
        if (c == '\n') {
            handle_insert_mode_newline(cursor_x, cursor_y);
        }
        else if (c != ESCAPE && c != '\t') { // TODO more sanitization
            insert_char(c, cursor_x, cursor_y);
        }
        else if (c == '\t') {
            chain_start(cursor_x, cursor_y);
            if (!use_tabulators) {
                for (int i=0; i<num_tab_spaces; i++)
                    insert_char(' ', cursor_x, cursor_y);
            }
            else {
                insert_char('\t', cursor_x, cursor_y);
            }         
            chain_end(cursor_x, cursor_y);
        }
    }
}
