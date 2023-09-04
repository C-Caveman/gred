//See LICENSE file for copyright and license details.
// Screen for displaying the document, line numbers, menus, ect.
#include "gred.h"

// Previous values of settings.
int old_show_line_numbers = 1;
int old_top_line_of_screen = 0; // vertical scroll distance
int old_text_display_x_start = 0; // horizontal scroll distance

// set cursor position in terminal (where cursor is shown, and text from printf goes)
void move_cursor(int x, int y) {
    x = bound_value(x, 0, screen_width); // prevent shennanigans
    printf("\033[%d;%dH", y+1, x+1); // move cursor to x,y
}

// print a line of the document
void display_line(struct line* l, int start_index, int stop_index) {
    //stop_index = bound_value(stop_index, start_index, document[line_number].len);
    stop_index = bound_value(l->len, 0, stop_index);
    start_index = bound_value(start_index, 0, stop_index);
    char c;
    for (int i=start_index; i<stop_index; i++) {
        c = l->text[i];
        if (c == '\t')
            printf("%s", "░"); // could use "»" instead
        else
            putchar(c);
    }
}

int find_num_width(int num) {
    int size = 0;
    while (num > 0) {
        size += 1;
        num /= 10;
    }
    return size;
}

void clear_line(int i) {
    printf("\033[%d;0H", i+1);
    printf("\r%*c\r", screen_width, ' ');
}

void draw_menu() {
    // clear menu line 1
    move_cursor(0, total_screen_height-2);
    clear_line(total_screen_height-2);
    // line 1
    printf("%s", (menu_alert != 0) ? menu_alert : ""); // empty line
    // clear menu line 2
    move_cursor(0, total_screen_height-1);
    clear_line(total_screen_height-1);
    // line 2
    printf("%s%.*s", menu_prompt, menu_input.len, menu_input.text);
    // remove the current menu_alert
    menu_alert = 0;
}

// Redraw the screen (skipping unchanged lines)
void draw_screen() {
    // If we are debugging, show the input info, not the document.
    if (debug) {
        debug_input(cur_char); // Show detailed input info!
        return;
    }
    // Keep the cursor inside the document.
    clip_cursor_to_grid();
    // Redraw the screen if needed.
    if ((old_show_line_numbers != show_line_numbers) ||
        (old_top_line_of_screen != top_line_of_screen) ||
        (old_text_display_x_start != text_display_x_start)) {
        redraw_full_screen = 1;
    }
    // get info about the screen
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    total_screen_height = w.ws_row;
    screen_height = w.ws_row-menu_height;
    screen_width = w.ws_col;
    // determine the width occupied by the line numbers
    int line_number_width = find_num_width(top_line_of_screen+screen_height);
    total_line_number_width = 0;
    if (show_line_numbers)
        total_line_number_width = line_number_width + 2; // The 2 extra chars: ": "
    clip_cursor_to_grid(); // clip the horizontal scrolling as well
    for (int i=top_line_of_screen; i<top_line_of_screen+screen_height; i++) {
        // Skip unedited lines (if not refreshing whole screen)
        if (!redraw_full_screen && !(document[i].flags & CHANGED))
            continue;
        // mark line as unchanged again
        document[i].flags &= ~CHANGED;
        move_cursor(0, i); // Move cursor to start of current line.
        clear_line(i-top_line_of_screen); // Clear the current line.
        if (show_line_numbers == 1)
            printf("%*d: ", line_number_width, i);
        if (colorize)
            display_line_highlighted(&document[i], text_display_x_start, text_display_x_end);
        else
            display_line(&document[i], text_display_x_start, text_display_x_end);
        // bottom of the screen reached?
        if (i-top_line_of_screen > total_screen_height-menu_height) {
            break;
        }
    }
    // Always redraw the menu.
    draw_menu();
    // Put the cursor in correct part of the screen.
    if (cursor_in_menu) // If menu_prompt is longer than 32 chars, cut it off.
        move_cursor(strnlen(menu_prompt, 32)+menu_cursor_x, screen_height+menu_height);
    else
        move_cursor(cursor_x-text_display_x_start+total_line_number_width, cursor_y-top_line_of_screen);
    // Change the cursor.
    if (mode == COMMAND_MODE) {
        // Solid block cursor shown when in escape mode.
        if (mode_specific_cursors_enabled)
            printf("\033[2 q");
    }
    else if (mode == INSERT_MODE) {
        // Vertical bar cursor when in insert mode.
        if (mode_specific_cursors_enabled)
            printf("\033[6 q");
    }
    // Vars that trigger full screen redrawing when changed.
    old_top_line_of_screen = top_line_of_screen;
    old_show_line_numbers = show_line_numbers;
    old_text_display_x_start = text_display_x_start;
    // No longer need to refresh the entire screen.
    redraw_full_screen = 0;
}



#define MAX_TERMINAL_NAME_LEN 64
char terminal_name[MAX_TERMINAL_NAME_LEN];
char supported_terminals[][MAX_TERMINAL_NAME_LEN] = { // list of terminals that allow vertical bar cursor
    "xterm-256color",
    "st-256color",
    "\0", // null-terminate the list
};
// Check if the terminal supports having a vertical bar for the cursor.
int check_vertical_bar_cursor_supported() {
    // Print what the $TERM environment variable is:
    snprintf(terminal_name, MAX_TERMINAL_NAME_LEN, "%s", getenv("TERM"));
    int alt_cursor_supported = 0;
    int i = 0;
    while (supported_terminals[i][0] != '\0') { // stop at null terminator
        if (strcmp(terminal_name, supported_terminals[i]) == 0) {
            alt_cursor_supported = 1;
            break;
        }
        i += 1;
    }
    // Warn the user if alternate cursors are not supported.
    if (!alt_cursor_supported)
        snprintf(terminal_name, 
                 MAX_TERMINAL_NAME_LEN, 
                 "$TERM=\"%s\" may not support mode-specific cursors!", 
                 getenv("TERM"));
    menu_alert = terminal_name; // Show the result to the user.
    return alt_cursor_supported;
}
