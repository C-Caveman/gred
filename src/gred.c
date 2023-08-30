//See LICENSE file for copyright and license details.
// gred, the Grid-based text-Editor 
#include "gred.h" 

// This is where the text goes.
struct line document[MAX_LINES];
// cursor location in the document
int cursor_x = 0; // current character
int cursor_y = 0; // current line
// special cursor for the menu
int menu_cursor_x = 0;
// message that can be changed at any time, resets after draw_screen() is done
char* menu_alert = 0;
int in_escape_sequence = 0;
struct line cur_escape_sequence = {0, 0, ""};
int previous_mode = ESCAPE_MODE;
int mode = ESCAPE_MODE;
int quit = 0;
int debug = 0;

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

// Editor settings:
int show_line_numbers = 1;
int old_show_line_numbers = 1;
int keyword_coloring = 1;
int mode_specific_cursors_enabled = 0; // If switching modes can change the cursor's appearance.
int use_tabulators = 0;
int num_tab_spaces = 4;

// Text input to the menu.
struct line menu_input;

// Editor state information:
int in_menu = 0;
int menu_height = 2; // number of lines occupied by the menu
int screen_height = 0;
int total_screen_height = 0;
int screen_width = 0;
int top_line_of_screen = 0; // vertical scrolling
int old_top_line_of_screen = 0; // previous vertical scroll distance
int text_display_x_start = 0; // horizontal scrolling
int old_text_display_x_start = 0; // previous horizontal scroll distance
int text_display_x_end = 0; // last column of text to show
int total_line_number_width = 0; // space occupied by the line number section
int text_display_width = 0; // width of displayed lines
int redraw_full_screen = 1; // whether to redraw the entire screen

struct line file_name = {13, 0, "new_file.txt"}; // default file name

// Keep value within [min, max]
int bound_value(int v, int min, int max) {
    v = (v < min) ? min : v; // Return min if too small.
    v = (v > max) ? max : v; // Return max if too big.
    return v;
}

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
        if (keyword_coloring)
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
    if (in_menu) // If menu_prompt is longer than 32 chars, cut it off.
        move_cursor(strnlen(menu_prompt, 32)+menu_cursor_x, screen_height+menu_height);
    else
        move_cursor(cursor_x-text_display_x_start+total_line_number_width, cursor_y-top_line_of_screen);
    // Change the cursor.
    if (mode == ESCAPE_MODE) {
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

int line_backspace(int cursor_x_pos, struct line* l) {
    if (cursor_x_pos == 0 || cursor_x_pos > l->len) // invalid delete
        return cursor_x_pos;
    l->text[l->len] = '\0'; // prevent out-of-range data from being pulled in
    for (int i=cursor_x_pos-1; i < l->len; i++)
        l->text[i] = l->text[i+1];
    l->text[l->len] = '0';
    l->len -= 1;
    return (cursor_x_pos - 1);
}

// copy a region of a line b into a region of line a
void line_copy_range(struct line* a, int a_first, int a_last,
                     struct line* b, int b_first, int b_last) 
{
    // keep the ranges within acceptable bounds
    a_first = bound_value(a_first, 0, a->len);
    b_first = bound_value(b_first, 0, b->len);
    a_last = bound_value(a_last, a_first, LINE_WIDTH);
    b_last = bound_value(b_last, b_first, b->len);
    int len_a_range = a_last - a_first;
    int len_b_range = b_last - b_first;
    int min_copy_len = (len_a_range < len_b_range) ? len_a_range : len_b_range;
    for (int i=0; i<min_copy_len; i++) {
        a->text[a_first+i] = b->text[b_first+i];
    }
    int copy_ending = a_first + min_copy_len; // used to check if copied past end of line.len
    a->len = (copy_ending > a->len) ? copy_ending : a->len;
}

int find_num_empty_lines() {
    int num_empty_lines = 0;
    for (int line_number=MAX_LINES-1; line_number>0; line_number--) {
        if (document[line_number].len != 0)
            break;
        num_empty_lines++;
    }
    return num_empty_lines;
}

void merge_line_upwards(int row) {
    if (row <= 0 || (document[row-1].len + document[row].len) > LINE_WIDTH)
        return;
    int last_line_len = document[row-1].len;
    // move the current line up
    record_before_edit(cursor_x, row-1, FIRST_OF_MULTIPLE_EDITS);
    line_copy_range(&document[row-1], document[row-1].len, LINE_WIDTH, 
                    &document[row],   0,                   LINE_WIDTH);
    //document[row-1].len = document[row-1].len + document[row].len;
    record_after_edit(cursor_x, row-1, EDIT_CHANGE_LINE);
    // empty out the old line
    record_before_edit(cursor_x, row, MIDDLE_OF_MULTIPLE_EDITS);
    document[row].len = 0;
    record_after_edit(cursor_x, cursor_y, EDIT_CHANGE_LINE);
    // remove the old line
    record_before_edit(cursor_x, row, LAST_OF_MULTIPLE_EDITS);
    delete_empty_line(row);
    record_after_edit(cursor_x, cursor_y, EDIT_DELETE_LINE);
    cursor_x = last_line_len;
}

void delete_empty_line(int row) {
    //TODO more safety checks TODO
    if (row < 0)
        return;
    int num_empty_lines = find_num_empty_lines();
    int last_line = MAX_LINES - num_empty_lines;
    // move all lines up
    for (int line_number=row; line_number<last_line-1; line_number++) {
        // move it up
        document[line_number].len = document[line_number+1].len;
        for (int i=0; i<document[line_number+1].len; i++) {
            // current_line <- next_line
            document[line_number].text[i] = document[line_number+1].text[i];
            document[i].flags |= CHANGED;
        }
    }
    if (row < last_line)
        document[last_line-1].len = 0;
}

// make room for a new line after the specified row
void insert_new_empty_line(int row) {
    int num_empty_lines = find_num_empty_lines();
    // Make sure there is space for the new line.
    if (num_empty_lines <= 0 || row >= MAX_LINES-1) {
        cursor_y -= 1;
        return;
    }
    // shift lines below row, down one line TODO add safety checks here TODO
    for (int i=MAX_LINES-1-num_empty_lines; i>row; i--) {
        copy_line(&document[i], &document[i-1]);
        document[i].flags |= CHANGED;
    }
    // empty out the current line
    document[row].len = 0;
}

// insert char c on line l, return new cursor_x position
int line_insert(char c, int cursor_x_pos, struct line* l) {
    // TODO check if valid character TODO <------------------------------------- TODO
    if (c == '\n' || l->len > LINE_WIDTH-2)
        return cursor_x_pos;
    for (int i=l->len; i>cursor_x_pos; i--)
        l->text[i] = l->text[i-1];
    l->text[cursor_x_pos] = c;
    l->len += 1;
    l->text[l->len] = '\0';
    return (cursor_x_pos + 1);
}

// (line a) = (line b)
void copy_line(struct line* a, struct line* b) {
    int b_len = (b->len < LINE_WIDTH) ? b->len : LINE_WIDTH;
    for (int i=0; i<b_len; i++)
        a->text[i] = b->text[i];
    a->len = b_len;
    a->flags = b->flags;
}

//
// Undo/Redo system: uses stacks of (struct edit) to track changes to the document.
//
void copy_edit(struct edit* a, struct edit* b) {
    a->type = b->type;
    a->before_x = b->before_x;
    a->before_y = b->before_y;
    a->after_x = b->after_x;
    a->after_y = b->after_y;
    a->start_of_sequence = b->start_of_sequence;
    a->end_of_sequence = b->end_of_sequence;
    copy_line(&a->after, &b->after);
    copy_line(&a->before, &b->before);
}
#define MAX_UNDOS 256
struct edit done[MAX_UNDOS];
struct edit undone[MAX_UNDOS];
int num_done = 0;
int num_undone = 0;

void push_done(struct edit* e) {
    num_done = bound_value(num_done, 0, MAX_UNDOS-2);
    for (int i=num_done; i>0; i--) {
        copy_edit(&done[i], &done[i-1]);
    }
    copy_edit(&done[0], e);
    num_done += 1;
}
void push_undone(struct edit* e) {
    num_undone = bound_value(num_undone, 0, MAX_UNDOS-2);
    for (int i=num_undone; i>0; i--) {
        copy_edit(&undone[i], &undone[i-1]);
    }
    copy_edit(&undone[0], e);
    num_undone += 1;
}
void pop_done() {
    num_done = bound_value(num_done, 0, MAX_UNDOS-2);
    for (int i=0; i<num_done-1; i++) {
        copy_edit(&done[i], &done[i+1]);
    }
    num_done -= 1;
}
void pop_undone() {
    num_undone = bound_value(num_undone, 0, MAX_UNDOS-2);
    for (int i=0; i<num_undone-1; i++) {
        copy_edit(&undone[i], &undone[i+1]);
    }
    num_undone -= 1;
}

// record line before edit so it can be undone later WARNING: do this BEFORE the edit is done!
void record_before_edit(int edit_x, int edit_y, int sequence_of_edits) {
    // destroy undo history
    num_undone = 0;
    // set the sequence data (how the edits will be strung together)
    int is_start = 0;
    int is_end = 0;
    if (sequence_of_edits == SINGLE_EDIT) {
        is_start = is_end = 1;
    }
    else if (sequence_of_edits == FIRST_OF_MULTIPLE_EDITS) { //WARNING end all such sequences!!!
        is_start = 1;
        is_end = 0;
    }
    else if (sequence_of_edits == LAST_OF_MULTIPLE_EDITS) {
        is_start = 0;
        is_end = 1;
    }
    else if (sequence_of_edits == MIDDLE_OF_MULTIPLE_EDITS) {
        is_start = 0;
        is_end = 0;
    }
    else {
        system("clear");
        perror("*** Unknown sequence_of_edits in record_before_edit()!\n");
        exit(-1);
    }
    // store info about the line before the edit
    struct edit e;
    e.type = BEFORE_EDIT;
    e.before_x = edit_x;
    e.before_y = edit_y;
    e.start_of_sequence = is_start;
    e.end_of_sequence = is_end;
    // mark the line as changed
    document[edit_y].flags |= CHANGED;
    // record the unchanged line
    copy_line(&e.before, &document[edit_y]);
    push_done(&e);
}

// save line after edit so it can be redone later WARNING: do this ONLY AFTER record_before_edit()!
void record_after_edit(int new_cursor_x, int new_cursor_y, int edit_type) {
    // save the new cursor position
    done[0].after_x = new_cursor_x;
    done[0].after_y = new_cursor_y;
    // save what type of edit this is
    done[0].type = edit_type;
    // record the changed line
    if (edit_type == EDIT_CHANGE_LINE)
        copy_line(&done[0].after, &document[done[0].before_y]); // store line after edit
}

// reverse the previous edit
void undo() {
    if (num_done <= 0) // nothing has been done yet
        return;
    struct edit* undone_edit;
    while (1) {
        // move edit (done -> undone)
        undone_edit = &done[0];
        push_undone(undone_edit);
        // undo the edit
        int type = undone_edit->type;
        if (type == EDIT_CHANGE_LINE) {
            // revert the line to the after in the done buffer
            copy_line(&document[undone_edit->after_y], &undone_edit->before);
        }
        else if (type == EDIT_DELETE_LINE) {
            insert_new_empty_line(undone_edit->before_y);
        }
        else if (type == EDIT_INSERT_LINE) {
            delete_empty_line(undone_edit->before_y);
        }
        if (undone_edit->start_of_sequence || num_done == 0) {
            // put the cursor back where it was
            cursor_x = undone_edit->before_x;
            cursor_y = undone_edit->before_y;
            pop_done();
            return;
        }
        pop_done();
    }
}

void redo() {
    if (num_undone <= 0) // nothing has been undone yet
        return;
    struct edit* redone_edit;
    while (1) {
        // move the edit (undone -> done)
        redone_edit = &undone[0];
        push_done(redone_edit);
        // redo the previously undone edit
        int type = redone_edit->type;
        if (type == EDIT_CHANGE_LINE) {
            // revert the line to the after in the undone buffer
            copy_line(&document[redone_edit->before_y], &redone_edit->after);
        }
        else if (type == EDIT_DELETE_LINE) {
            delete_empty_line(redone_edit->before_y);
        }
        else if (type == EDIT_INSERT_LINE) {
            insert_new_empty_line(redone_edit->before_y);
        }
        if (redone_edit->end_of_sequence || num_undone == 0) {
            // put the cursor back where it was
            cursor_x = redone_edit->after_x;
            cursor_y = redone_edit->after_y;
            pop_undone();
            return;
        }
        pop_undone();
    }
    // mark the line as changed again for redrawing
    document[cursor_y].flags |= CHANGED;
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
    // INSERT_MODE TODO better input sanitization TODO
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
    char c = '?';
    switch_mode(ESCAPE_MODE);
    redraw_full_screen = 1; // Fill the screen with the document.
    while (!quit) {
        if (debug)
            debug_input(c); // Show detailed input info!
        else
            draw_screen();
        c = getch();
        handle_input(c);
    }
    system("clear");
    return 0;
}
