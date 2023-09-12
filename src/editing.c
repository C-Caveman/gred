//See LICENSE file for copyright and license details.
//
// Editing lines of text.    ::    See /gred/guide/editing.txt for more info.
//
#include "gred.h"
int in_chain = 0; // TODO use these!
int in_macro = 0;
struct line macro_buffer;

// A change to the document. Can be undone/redone.    ::
struct edit {
    char type;
    char flags;
    char c;
    char x;
    int y;
};
//                   WARNING!!!!
// Helper functions. Do not use these, they don't hook into the undo/redo system!
//
void push_done(struct edit* e);
int line_insert_char(char c, int x, struct line* l);
extern int num_undone; // Didn't want this in the header.
int unused = 0;
enum edit_direction {DO_EDIT=0, UNDO_EDIT=1};
void edit(struct edit* e, 
               int* chain_depth, 
               int* macro_depth,
               int undoing_edit
          );

//
// Edit the document with these!    ::
//
void insert_char(char c, int x, int y) {
    struct edit ed = {INSERT_CHAR, (in_chain) ? IN_CHAIN : 0, c, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void delete_char(int x, int y) {
    struct edit ed = {DELETE_CHAR, (in_chain) ? IN_CHAIN : 0, document[y].text[x], (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void insert_empty_line(int x, int y) {
    struct edit ed = {INSERT_EMPTY_LINE, (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void delete_empty_line(int x, int y) {
    struct edit ed = {DELETE_EMPTY_LINE, (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void cursor_move(int x, int y) {
    struct edit ed = {CURSOR_MOVE, (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void chain_start(int x, int y) {
    struct edit ed = {CHAIN_START, (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void chain_end(int x, int y) {
    struct edit ed = {CHAIN_END,   (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void macro_start(int x, int y) {
    struct edit ed = {MACRO_START, (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}
void macro_end(int x, int y) {
    struct edit ed = {MACRO_END,   (in_chain) ? IN_CHAIN : 0, 0, (char)x, y};
    edit(&ed, &unused, &unused, DO_EDIT); // Apply and store the edit in the undo/redo system.
}

//                WARNING!
// Change a line. Avoid calling these on the document, use the ones above to get undo/redo saved! ::
//
// insert char c on line l, return new cursor_x position
int line_insert_char(char c, int cursor_x_pos, struct line* l) {
    if (c == '\n' || l->len > LINE_WIDTH-2 || c == ESCAPE) // Skip invalid characters.
        return cursor_x_pos;
    for (int i=l->len; i>cursor_x_pos; i--) // Trailing letters move to the right.
        l->text[i] = l->text[i-1];
    l->text[cursor_x_pos] = c;
    l->len += 1;
    l->text[l->len] = '\0';
    l->flags |= CHANGED;
    return (cursor_x_pos + 1);
}
int line_delete_char(int deleted_pos, struct line* l) { // TODO clean this up a bit.
    if (deleted_pos < 0 || deleted_pos > l->len) // invalid delete
        return deleted_pos;
    l->text[l->len] = '\0';// Terminated!
    for (int i=deleted_pos; i < l->len; i++) // Trailing letters move to the left.
        l->text[i] = l->text[i+1];
    l->text[l->len] = '0'; // Terminated!
    l->len = (l->len > 0) ? l->len-1 : 0; // Reduce length if greater than zero.
    l->flags |= CHANGED;
    return (deleted_pos);
}
void line_clear(struct line* l) { // Empty out a line.
    l->len = 0;
    l->text[0] = 0; // Null-terminate.
}

//
// Add/remove an empty line. ::
//
// Utility for inserting/deleting lines.
int find_num_empty_lines() {
    int num_empty_lines = 0;
    for (int line_number=MAX_LINES-1; line_number>0; line_number--) {
        if (document[line_number].len != 0)
            break;
        num_empty_lines++;
    }
    return num_empty_lines;
}
int add_line(int row) {
    int num_empty_lines = find_num_empty_lines();
    if (num_empty_lines <= 0 || row >= MAX_LINES-1) { // Make sure there is space for the new line.
        return row;
    }
    for (int i=MAX_LINES-num_empty_lines; i>row; i--) { // Shift the lines downwards.
        copy_line(&document[i], &document[i-1]);
        document[i].flags |= CHANGED;
        document[i-1].flags |= CHANGED;
    }
    // empty out the current line
    document[row].len = 0;
    return (row+1); // Increment the cursor_y position.
}
int delete_line(int row) {
    if (row < 0)
        return 0;
    int num_empty_lines = find_num_empty_lines();
    int last_line = MAX_LINES - num_empty_lines;
    // move all lines up
    for (int i=row; i<last_line-1; i++) { // Shift the lines upwards.
        // move it up
        document[i].len = document[i+1].len;
        copy_line(&document[i], &document[i+1]);
        document[i].flags |= CHANGED;
        document[i+1].flags |= CHANGED;
    }
    if (row < last_line)
        document[last_line-1].len = 0;
    return (row-1); // Decrement the cursor_y position.
}

//
// Move data between lines.   ::
//
// Deep copy line (a = b).
void copy_line(struct line* a, struct line* b) {
    int b_len = (b->len < LINE_WIDTH) ? b->len : LINE_WIDTH;
    for (int i=0; i<b_len; i++)
        a->text[i] = b->text[i];
    a->len = b_len;
    a->flags = b->flags;
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

//
// Big mess!   ::
//
void merge_line_upwards(int row) { // TODO clean this up! <----------------------------- TODO
    if (row <= 0 || (document[row-1].len + document[row].len) > LINE_WIDTH)
        return;
    int last_line_len = document[row-1].len;
    
    // move the current line up
    chain_start(cursor_x, cursor_y);
    while (document[row].len > 0) {
        insert_char(document[row].text[0], document[row-1].len, row-1);
        delete_char(0, row);
    }
    
    // empty out the old line
    while (document[row].len > 0)
        delete_char(document[row].len, row);
    
    // remove the old line
    delete_empty_line(cursor_x, row);
    chain_end(last_line_len, row-1);
    cursor_x = last_line_len;
}




//
// Undo/Redo system: uses stacks of (struct edit) to track changes to the document.   ::
//
// Deep copy an edit (a = b).
void copy_edit(struct edit* a, struct edit* b) {
    a->type = b->type;
    a->flags = b->flags;
    a->x = b->x;
    a->c = b->c;
    a->y = b->y;
}
#define MAX_UNDOS 2048
struct edit done[MAX_UNDOS];
struct edit undone[MAX_UNDOS]; //TODO make these circular buffers.
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
// Apply an edit (or its opposite).
void edit(struct edit* e, 
               int* chain_depth, 
               int* macro_depth,
               int undoing_edit
          ) {
    int initial_cursor_y = cursor_y;
    int edit_type = e->type;
    if (undoing_edit) {
        edit_type *= -1; // Mark as the opposite kind of edit.
        push_undone(e);  // Put this on the redo stack.
    }
    else {
        edit_type = edit_type;
        push_done(e);    // Put this on the undo stack.
    }
    switch(edit_type) {
    case CURSOR_MOVE: // CURSOR_MOVE is is the same for doing/undoing. 0 * (-1) = 0
        cursor_x = e->x;
        break;
    case DELETE_CHAR:
        cursor_x = line_delete_char(e->x, &document[e->y]);
        break;
    case INSERT_CHAR:
        cursor_x = line_insert_char(e->c, e->x, &document[e->y]);
        break;
    case DELETE_EMPTY_LINE:
        delete_line(e->y);
        cursor_x = e->x;
        break;
    case INSERT_EMPTY_LINE:
        add_line(e->y);
        cursor_x = e->x;
        break;
    case CHAIN_END:
        cursor_x = e->x;
        *chain_depth -= 1;
        break;
    case CHAIN_START:
        cursor_x = e->x;
        *chain_depth += 1;
        break;
    case MACRO_END:
        cursor_x = e->x;
        *macro_depth -= 1;
        break;
    case MACRO_START:
        cursor_x = e->x;
        *macro_depth += 1;
        break;
    }
    if (cursor_y == initial_cursor_y) // Restore the cursor_y position if not already updated.
        cursor_y = e->y;
        document[e->y].flags |= CHANGED; // Redraw this line.
    if (undoing_edit)
        pop_done();    // Remove the edit from the undo stack.
    else
        pop_undone();  // Remove the edit from the redo stack.
}
void undo() {
    if (num_done <= 0) // nothing has been done yet
        return;
    struct edit* undone_edit;
    int chain_depth = 0;
    int macro_depth = 0; //TODO this
    while (1) {
        // Put previously done edit onto the undone stack.
        undone_edit = &done[0];
        edit(undone_edit, &chain_depth, &macro_depth, UNDO_EDIT);
        if ( (chain_depth < 1) //((chain_depth < 1) && ((undone_edit->flags | IN_CHAIN) == 0)) 
            || num_done == 0) { // TODO add macro_depth
            return;
        }
    }
}
void redo() {
    if (num_undone <= 0) // nothing has been undone yet
        return;
    struct edit* redone_edit;
    int chain_depth = 0;
    int macro_depth = 0;
    while (1) {
        // Put previously undone edit onto the done stack.
        redone_edit = &undone[0];
        edit(redone_edit, &chain_depth, &macro_depth, DO_EDIT);
        if ( (chain_depth < 1) //((chain_depth < 1) && ((redone_edit->flags | IN_CHAIN) == 0)) 
            || num_undone == 0) {
            return;
        }
    }
}
