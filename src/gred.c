//See LICENSE file for copyright and license details.
// gred, the Grid-based text-Editor 
#include "gred.h" 

// The document being edited:
struct line document[MAX_LINES];
struct line file_name = {13, 0, "new_file.txt"}; // document name

// cursor location in the document
int cursor_x = 0; // current character
int cursor_y = 0; // current line
int command = 0; // current command
// special cursor for the menu
int menu_cursor_x = 0;
// message that can be changed at any time, resets after draw_screen() is done
char* menu_alert = 0;
// Text input to the menu.
struct line menu_input;
int in_escape_sequence = 0;
struct line cur_escape_sequence = {0, 0, ""};
int previous_mode = ESCAPE_MODE;
int mode = ESCAPE_MODE;
int quit = 0;
int debug = 0;

// Editor settings:
int show_line_numbers = 1;
int colorize = 1;
int mode_specific_cursors_enabled = 0; // If switching modes can change the cursor's appearance.
int use_tabulators = 0;
int num_tab_spaces = 4;

// Editor state information:
int in_menu = 0;
int menu_height = 2; // number of lines occupied by the menu
int screen_height = 0;
int total_screen_height = 0;
int screen_width = 0;
int top_line_of_screen = 0; // vertical scrolling
int text_display_x_start = 0; // horizontal scrolling
int text_display_x_end = 0; // last column of text to show
int total_line_number_width = 0; // space occupied by the line number section
int text_display_width = 0; // width of displayed lines
int redraw_full_screen = 1; // whether to redraw the entire screen

// Keep value within [min, max]
int bound_value(int v, int min, int max) {
    v = (v < min) ? min : v; // Return min if too small.
    v = (v > max) ? max : v; // Return max if too big.
    return v;
}

// keep the cursor in the grid, scrolling the screen if necessary
void clip_cursor_to_grid() {
    int bottom_line_of_screen = top_line_of_screen+screen_height-1;
    //
    // horizontal adjustment
    //
    cursor_x = 
        bound_value(cursor_x, 0, (mode == INSERT_MODE) ? document[cursor_y].len : LINE_WIDTH);
    text_display_x_start = 
        bound_value(text_display_x_start, 0, LINE_WIDTH);
    if (cursor_x < text_display_x_start) {
        text_display_x_start = cursor_x;
    }
    int last_screen_column = screen_width-total_line_number_width-1;
    int column_cursor_delta = cursor_x - text_display_x_start;
    if (column_cursor_delta > last_screen_column) {
        text_display_x_start = (cursor_x-last_screen_column);
    }
    text_display_x_end = text_display_x_start+screen_width-total_line_number_width;
    text_display_width = screen_width-total_line_number_width; // width of text display area
    //
    // vertical adjustment
    //
    cursor_y = bound_value(cursor_y, 0, MAX_LINES-1);
    if (cursor_y > bottom_line_of_screen && 
        (bottom_line_of_screen <= MAX_LINES)) {
        top_line_of_screen += (cursor_y - bottom_line_of_screen);
    }
    if (cursor_y < top_line_of_screen && top_line_of_screen > 0) {
        top_line_of_screen -= (top_line_of_screen - cursor_y);
    }
    if (top_line_of_screen < 0)
        top_line_of_screen = 0;
    // if we scrolled the screen, redraw the whole thing
    if (top_line_of_screen != old_top_line_of_screen)
        redraw_full_screen = 1;
}

void load_file(char* fname) {
    int row = 0;
    FILE* fp = fopen(fname, "r");
    char fchar = '?';
    if (fp == 0) {
        printf("File %s could not be opened.\n", fname);
        return;
    }
    // read the file into the document line by line
    while (1) {
        // TODO allow larger documents in the future TODO <----------------------- TODO
        if (row == MAX_LINES) {
            printf("*** File was more than %d lines!\n", MAX_LINES);
            exit(-1);
        }
        int line_length = document[row].len = 0;
        while (line_length < LINE_WIDTH) {
            fchar = fgetc(fp);
            if (feof(fp) || fchar == '\n')
                break;
            document[row].text[line_length] = fchar;
            document[row].len++;
            line_length++;
            // filled the current document line, but file line still has more TODO add wrap option
            if (line_length == LINE_WIDTH) {
                printf("*** Max line width %d exceeded on line index %d of %s.\n", LINE_WIDTH, row, fname);
                exit(-1);
            }
        }
        if (feof(fp))
            break;
        row++; // next line of the document
    }
    fclose(fp);
    // Update the settings for tabulators/spaces based on the filename suffix.
    update_settings_from_file_type(get_file_type(&file_name));
}

void save_file(char* fname) {
    FILE* fp = fopen(fname ,"w");
    // remove empty lines in the document
    int num_empty_lines = 0;
    for (int line_number=MAX_LINES-1; line_number>0; line_number--) {
        if (document[line_number].len != 0)
            break;
        num_empty_lines++;
    }
    int total_lines = MAX_LINES - num_empty_lines;
    // newlines need to be added back in TODO handle extended lines maybe? TODO
    for (int line_number=0; line_number<total_lines; line_number++) { // put the text in there
        fprintf(fp, "%.*s\n", (int)document[line_number].len, document[line_number].text);
    }
    fclose(fp);
    
}
// DEBUG HACK <----------------------------------------- TODO remove
char cur_char = '?';
char last_command[64];
extern struct line last_input;
void (*menu)(char) = 0; // pointer to a menu function
int main(int argc, char* argv[]) {
    // Make sure that the terminal supports alternative cursor types.
    mode_specific_cursors_enabled = check_vertical_bar_cursor_supported();
    if (argc > 1) {
        load_file(argv[1]);
        strncpy(file_name.text, argv[1], LINE_WIDTH-1);
        file_name.len = strnlen(file_name.text, LINE_WIDTH-1);
    }
    update_settings_from_file_type(get_file_type(&file_name));
    cursor_y = 0;
    cursor_x = 0;
    switch_mode(ESCAPE_MODE);
    remember_mode(ESCAPE_MODE);
    redraw_full_screen = 1; // Fill the screen with the document.
    while (!quit) {
        if (debug)
            debug_input(cur_char); // Show detailed input info!
        else
            draw_screen();
        cur_char = getch();
        command = get_command(cur_char);
        /*
        if (menu != 0)
            menu(cur_char); // menus handle commands differently
        else
        */
            run_command(command);
        snprintf(last_command, 64, "COMMAND: %.*s", last_input.len, last_input.text);
        menu_alert = last_command;
    }
    system("clear");
    return 0;
}
