//See LICENSE file for copyright and license details.
// Screen for displaying the document, line numbers, menus, ect.
#include "gred.h"

int sel_bottom = 0;
int sel_top = 0;
int sel_left = 0;
int sel_right = 0;

// Previous values of settings.
int old_show_line_numbers = 1;
int display_text_top_old = 0; // First visible document line on screen.
int old_display_text_x_start = 0; // horizontal scroll distance
int display_text_bottom = 0; // Last visible document line on screen.

// set cursor position in terminal (where cursor is shown, and text from printf goes)
void move_cursor(int x, int y) {
    x = bound_value(x, 0, display_full_width); // keep cursor in the display space
    //y = bound_value(y, 0, display_full_height); // keep cursor in the display space
    printf("\033[%d;%dH", y+1, x+1); // move cursor to x,y
}

// print a line of the document
void display_line(int row, int start_index, int stop_index) {
    struct line* l = &document[row];
    stop_index = bound_value(l->len, 0, stop_index);
    start_index = bound_value(start_index, 0, stop_index);
    char c;
    for (int i=start_index; i<stop_index; i++) {
        if (i == sel_left && selecting && row >= sel_top && row <= sel_bottom)
            printf("\033[107m");
        if (i == sel_right && selecting)
            printf("\033[0m");
        c = l->text[i];
        if (c == '\t')
            printf("%s", "░"); // could use "»" instead
        else
            putchar(c);
    }
    printf("\033[0m"); // Stop coloring just in case.
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
    printf("\033[%d;0H", i+1); // Go to line.
    printf("\033[2K\r"); // Clear, return to left side of display.
    //printf("\r%*c\r", display_full_width, ' ');
}

void draw_menu() {
    // clear menu line 1
    move_cursor(0, display_full_height-2);
    clear_line(display_full_height-2);
    // line 1
    printf("%s", (menu_alert != 0) ? menu_alert : ""); // empty line
    // clear menu line 2
    move_cursor(0, display_full_height-1);
    clear_line(display_full_height-1);
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
    // Update the selection.
    sel_bottom = (sel_cursor_y > cursor_y) ? sel_cursor_y : cursor_y;
    sel_top = (sel_cursor_y < cursor_y) ? sel_cursor_y : cursor_y;
    sel_left = (cursor_x < sel_cursor_x) ? cursor_x : sel_cursor_x;
    sel_right = (cursor_x > sel_cursor_x) ? cursor_x : sel_cursor_x;
    // Redraw the screen if needed.
    if ((old_show_line_numbers != show_line_numbers) ||
        (display_text_top_old != display_text_top) ||
        (old_display_text_x_start != display_text_x_start)) {
        display_redraw_all = 1;
    }
    // get info about the screen
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    display_full_height = w.ws_row;
    display_text_height = w.ws_row-menu_height;
    display_text_bottom = display_text_top + display_text_height-1;
    display_full_width = w.ws_col;
    // determine the width occupied by the line numbers
    int line_number_width = find_num_width(display_text_top+display_text_height);
    display_line_number_width = 0;
    if (show_line_numbers)
        display_line_number_width = line_number_width + 2; // The 2 extra chars: ": "
    clip_cursor_to_grid(); // clip the horizontal scrolling as well
    for (int i=display_text_top; i<display_text_top+display_text_height; i++) {
        // Skip unedited lines (if not refreshing whole screen)
        if (!display_redraw_all && !(document[i].flags & CHANGED) && !(selecting))
            continue;
        // mark line as unchanged again
        document[i].flags &= ~CHANGED;
        move_cursor(0, i); // Move cursor to start of current line.
        clear_line(i-display_text_top); // Clear the current line.
        if (show_line_numbers == 1)
            printf("%*d: ", line_number_width, i);
        if (colorize)
            display_line_highlighted(i, display_text_x_start, display_text_x_end);
        else
            display_line(i, display_text_x_start, display_text_x_end);
        // bottom of the screen reached?
        if (i-display_text_top > display_full_height-menu_height) {
            break;
        }
    }
    // Always redraw the menu.
    draw_menu();
    // Put the cursor in correct part of the screen.
    if (cursor_in_menu) // If menu_prompt is longer than 32 chars, cut it off.
        move_cursor(strnlen(menu_prompt, 32)+menu_cursor_x, display_text_height+menu_height);
    else
        move_cursor(cursor_x-display_text_x_start+display_line_number_width, cursor_y-display_text_top);
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
    display_text_top_old = display_text_top;
    old_show_line_numbers = show_line_numbers;
    old_display_text_x_start = display_text_x_start;
    // No longer need to refresh the entire screen.
    display_redraw_all = 0;
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
