//See LICENSE file for copyright and license details.
// Menus such as the save_file() menu.
#include "gred.h" 

void (*menu)() = 0; // Pointer to the current menu function.
int cursor_in_menu = 0;
int reading_menu_input = 0;
int menu_was_just_opened = 0;
// special cursor for the menu
int menu_cursor_x = 0;

char insert_mode_help[] = "INSERT_MODE:    Press <escape> for COMMAND_MODE";
char escape_mode_help[] = "COMMAND_MODE:   Press ? for help, or i to insert text.";
char* menu_prompt = escape_mode_help;

// set which menu is shown (using the menu's function name)
void open_menu(void (*men)()) {
    menu = men;
    // Reset menu cursor.
    menu_cursor_x = 0;
    // Put cursor in the menu.
    cursor_in_menu = 1;
    // Empty out the menu text box.
    menu_input.len = 0;
    menu_input.text[0] = 0;
    // Start reading into menu_input.
    reading_menu_input = 1;
    // Ignore the first character input.
    cur_char = 0;
    // Tell the menu that is was just opened.
    menu_was_just_opened = 1;
}
// hide the menu
void close_menu() {
    menu = 0;
    cursor_in_menu = 0;
    menu_input.len = 0;
    menu_input.text[0] = 0;
    menu_cursor_x = 0;
    if (mode == COMMAND_MODE)
        menu_prompt = escape_mode_help;
    if (mode == INSERT_MODE)
        menu_prompt = insert_mode_help;
    // Stop reading into menu_input
    reading_menu_input = 0;
    // Make sure this gets reset, just in case.
    menu_was_just_opened = 0;
}

// Exit menu if escape was just pressed twice.
char menu_exit_prompt[] = "Press <escape> again to exit the menu.";
void double_escape_menu_exit() {
    if (cur_char == ESCAPE && prev_char == ESCAPE)
        close_menu();
    else if (cur_char == ESCAPE && menu != 0) // Only show this if in a menu.
        menu_alert = menu_exit_prompt;
}

// Return 1 if still reading input, return 0 when finished.
int read_menu_input() {
    if (reading_menu_input == 0)
        return 0;
    // Handle the menu input.
    switch (cur_char) {
        case '\n':
            reading_menu_input = 0;
            return 1;
        case ESCAPE:
            break;
        case BACKSPACE:
            menu_cursor_x = line_backspace(menu_cursor_x, &menu_input);
            break;
        default:
            // Allow cursor movement.
            if (command == LEFT && cur_char != 'h')
                menu_cursor_x -= 1;
            else if (command == RIGHT && cur_char != 'l')
                menu_cursor_x += 1;
            // Insert text if not in an escape sequence.
            else if (input.text[0] != '[' && isprint(cur_char))
                menu_cursor_x = line_insert(cur_char, menu_cursor_x, &menu_input);
            break;
    }
    menu_cursor_x = bound_value(menu_cursor_x, 0, menu_input.len);
    return 1; // Still reading input.
}

// menu for saving to a file
char save_menu_prompt[] = "Save as: ";
void menu_save_file() {
    menu_prompt = save_menu_prompt;
    // Put the file name into menu_input if it is empty.
    if (menu_was_just_opened) {
        copy_line(&menu_input, &file_name);
        menu_cursor_x = menu_input.len;
    }
    menu_was_just_opened = 0;
    read_menu_input();
    if (reading_menu_input) {
        return;
    }
    save_file(menu_input.text);
    strncpy(file_name.text, menu_input.text, LINE_WIDTH-1); // update cur filename
    file_name.len = menu_input.len;
    close_menu();
    update_settings_from_file_type(get_file_type(&file_name));
}

// Search cursor
int search_x = 0;
int search_y = 0;

// Look forwards for the word in menu_input.text in line l, return the position (or -1 on failure).
int search_line_forwards() {
    struct line* l = &document[search_y];
    int found_index = -1;
    int max_len = 0;
    for (int i=search_x; i<l->len; i++) {
        max_len = menu_input.len;//(i > l->len - menu_input.len) ? (l->len - menu_input.len) : menu_input.len;
        if (strncmp(&l->text[i], menu_input.text, max_len) == 0) {
            found_index = i;
            break;
        }
    }
    return found_index;
}

// Look backwards for the word in menu_input.text in line l, return the position (or -1 on failure).
int search_line_backwards() {
    struct line* l = &document[search_y];
    int found_index = -1;
    int max_len = 0;
    for (int i=search_x; i>-1; i--) {
        max_len = menu_input.len;//(i > l->len - menu_input.len) ? (l->len - menu_input.len) : menu_input.len;
        if (strncmp(&l->text[i], menu_input.text, max_len) == 0) {
            found_index = i;
            break;
        }
    }
    return found_index;
}

// search the document for the word in menu_input, starting from line search_y up,
// return index of line (or -1 for failure)
int search_forwards() {
    int found_y = -1;
    int found_x = -1;
    while (search_y < MAX_LINES) {
        if (search_y != cursor_y)
            search_x = 0;
        else
            search_x = cursor_x+1;
        found_x = search_line_forwards();
        if (found_x != -1) {
            found_y = search_y;
            search_x = found_x;
            break;
        }
        search_y += 1;
    }
    search_y = found_y;
    search_x = found_x;
    return found_y;
}

// search the document for the word in menu_input, starting from line search_y down,
// return index of line (or -1 for failure)
int search_backwards() {
    int found_y = -1;
    int found_x = -1;
    while (search_y > -1) {
        if (search_y != cursor_y)
            search_x = document[cursor_y].len;
        else
            search_x = search_x-1;
        found_x = search_line_backwards();
        if (found_x != -1) {
            found_y = search_y;
            search_x = found_x;
            break;
        }
        search_y -= 1;
    }
    search_y = found_y;
    search_x = found_x;
    return found_y;
}

// Search for a pattern in the document.
#define MATCH_INFO_SIZE 32
char match_info[MATCH_INFO_SIZE];
char search_menu_prompt[] = "Find: ";
char search_menu_prompt_2[] = "Finding: ";
char search_menu_help[] = "Search UP or DOWN (k/j)";
void menu_search() {
    // Set the prompt.
    menu_prompt = search_menu_prompt;
    // Get the search pattern before we enter the menu.
    read_menu_input();
    if (reading_menu_input)
        return;
    // Indicate that menu_input is done reading.
    menu_prompt = search_menu_prompt_2;
    // Put cursor in the document.
    cursor_in_menu = 0;
    // Show the help info for this menu.
    if (menu_was_just_opened == 1)
        menu_alert = search_menu_help;
    menu_was_just_opened = 0;
    int found_y = -1;
    search_x = cursor_x;
    search_y = cursor_y;
    switch(command) {
        case DOWN: // down
            found_y = search_forwards(cursor_x, cursor_y);
            break;
        case UP: // up
            found_y = search_backwards(cursor_x, cursor_y);
            break;
        case QUIT:
            menu_alert = 0;
            close_menu(); // exit the menu
            break;
        default:
            if (command != NO_COMMAND || cur_char == '/') {
                menu_alert = 0;
                close_menu(); // exit the menu
            }
            break;
    }
    if (found_y != -1) {
        cursor_y = search_y;
        cursor_x = search_x;
        snprintf(match_info, MATCH_INFO_SIZE, "Match!");
        // Scroll the screen as far left as possible
        text_display_x_start = 0;
    }
}
