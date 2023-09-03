//See LICENSE file for copyright and license details.
// Menus such as the save_file() menu.
#include "gred.h" 

static void (*menu)(char) = 0;

char insert_mode_help[] = "INSERT_MODE:    Press <escape> for ESCAPE_MODE";
char escape_mode_help[] = "ESCAPE_MODE:    Press ? for help, or i to insert text.";
char* menu_prompt = escape_mode_help;

// Used for managing the current menu. See menu_save_file() for an example.
enum menu_states {
    MENU_CLOSED,
    MENU_READING_INPUT,  // Typing into the menu_input buffer.
    MENU_ENTER_DETECTED, // Hit enter after typing into menu_input.
    MENU_ESCAPE_DETECTED,
    MENU_DOUBLE_ESCAPE_DETECTED, // Confirmed exit from menu.
    MENU_IN_ESCAPE_SEQUENCE, // Handling arrow keys in menu.
    NUM_MENU_STATES
};
int menu_state = MENU_CLOSED;

// set which menu is shown (using the menu's function name)
void open_menu(void* men) {
    menu = (void (*)(char))(men);
    menu_state = MENU_READING_INPUT;
}
// hide the menu
void close_menu() {
    menu = 0;
    menu_state = MENU_CLOSED;
    menu_input.len = 0;
    menu_cursor_x = 0;
    if (mode == ESCAPE_MODE)
        menu_prompt = escape_mode_help;
    if (mode == INSERT_MODE)
        menu_prompt = insert_mode_help;
}

// read input for a menu (see menu_save_file() for an example of how to use this)
char menu_exit_prompt[] = "Press Escape again to exit menu.";
void input_menu_char() {
    char c = getch();
    if (menu_state == MENU_ESCAPE_DETECTED && c == ESCAPE) {
        menu_state = MENU_DOUBLE_ESCAPE_DETECTED;
        return;
    }
    if (menu_state == MENU_ESCAPE_DETECTED && c == '[') {
        menu_state = MENU_IN_ESCAPE_SEQUENCE;
        return;
    }
    if (menu_state == MENU_IN_ESCAPE_SEQUENCE) {
            switch(c) {
            // These are the escape codes for the arrow keys!
            case 'D': // left
                menu_cursor_x -= 1;
                break;
            case 'C': // right
                menu_cursor_x += 1;
                break;
            case 'A': // up
                //menu_cursor_y -= 1;
                break;
            case 'B': // down
                //menu_cursor_y += 1;
                break;
            default:
        }
        if (menu_cursor_x < 0)
            menu_cursor_x = 0;
        if (menu_cursor_x > menu_input.len)
            menu_cursor_x = menu_input.len;
        menu_state = MENU_READING_INPUT; // return to reading input normally
        return;
    }
    switch (c) {
        case '\n':
            menu_state = MENU_ENTER_DETECTED;
            return;
        case ESCAPE:
            menu_state = MENU_ESCAPE_DETECTED;
            menu_alert = menu_exit_prompt;
            return;
        case BACKSPACE:
            menu_cursor_x = line_backspace(menu_cursor_x, &menu_input);
            break;
        default:
            menu_cursor_x = line_insert(c, menu_cursor_x, &menu_input);
            break;
    }
    menu_state = MENU_READING_INPUT;
    return;
}

// menu for saving to a file
char save_menu_prompt[] = "Save as: ";
void menu_save_file() {
    open_menu(menu);
    menu_prompt = save_menu_prompt;
    strncpy(menu_input.text, file_name.text, LINE_WIDTH-1);
    menu_input.len = strnlen(menu_input.text, LINE_WIDTH-1);
    menu_cursor_x = menu_input.len;
    while (1) {
        draw_screen();
        input_menu_char();
        if (menu_state == MENU_ENTER_DETECTED)
            break;
        if (menu_state == MENU_DOUBLE_ESCAPE_DETECTED) {
            close_menu();
            return; // cancel the file save
        }
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

#define MATCH_INFO_SIZE 32
char match_info[MATCH_INFO_SIZE];
void search_loop() {
    // Show how to navigate.
    snprintf(match_info, MATCH_INFO_SIZE, "Search down/up: j/k");
    menu_alert = match_info;
    char c;
    int found_y = -1;
    while (1) {
        draw_screen();
        menu_alert = match_info;
        c = getch();
        search_x = cursor_x;
        search_y = cursor_y;
        switch(c) {
            case 'j': // down
                found_y = search_forwards(cursor_x, cursor_y);
                snprintf(match_info, MATCH_INFO_SIZE, "No matches below.");
                break;
            case 'k': // up
                found_y = search_backwards(cursor_x, cursor_y);
                snprintf(match_info, MATCH_INFO_SIZE, "No matches above.");
                break;
            default:
                snprintf(match_info, MATCH_INFO_SIZE, "Search terminated.");
                return;
        };
        if (found_y != -1) {
            cursor_y = search_y;
            cursor_x = search_x;
            snprintf(match_info, MATCH_INFO_SIZE, "Match!");
            // Scroll the screen as far left as possible
            text_display_x_start = 0;
        }
    }
}

// menu for searching for a word
char search_menu_prompt[] = "Find: ";
void menu_search() {
    open_menu(menu);
    menu_prompt = search_menu_prompt;
    menu_cursor_x = 0;
    while (1) {
        draw_screen();
        input_menu_char();
        if (menu_state == MENU_ENTER_DETECTED) { // TODO search!
            if (menu_input.len < 1) // Anything to search for?
                break;
            // Search for a match!
            search_loop();
            break;
        }
        if (menu_state == MENU_DOUBLE_ESCAPE_DETECTED) {
            close_menu();
            return; // cancel the file save
        }
    }
    close_menu();
}
