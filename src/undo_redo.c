//See LICENSE file for copyright and license details.
//
// Undo/Redo system: uses stacks of (struct edit) to track changes to the document.
//
#include "gred.h"

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
