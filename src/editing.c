//See LICENSE file for copyright and license details.
// Editing lines of text.
#include "gred.h"
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

// Copy given region of line b into given region of line a
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
    for (int i=row; i<last_line-1; i++) {
        // move it up
        document[i].len = document[i+1].len;
        copy_line(&document[i], &document[i+1]);
        document[i].flags |= CHANGED;
        document[i+1].flags |= CHANGED;
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
    for (int i=MAX_LINES-num_empty_lines; i>row; i--) {
        copy_line(&document[i], &document[i-1]);
        document[i].flags |= CHANGED;
        document[i-1].flags |= CHANGED;
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
