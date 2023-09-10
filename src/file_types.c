//See LICENSE file for copyright and license details.
// File-specific settings such as using tabulators/spaces.
#include "gred.h"

enum file_types_enum {
    FILE_MAKEFILE, // Makefiles require tabulators.
    FILE_OTHER,    // Use default settings.
    NUM_FILE_TYPES
};

#define MAX_SUFFIX_LEN 32
char file_type_suffixes[][MAX_SUFFIX_LEN] = {
    "Makefile",
    "\0" // Null-terminated list.
};

void update_settings_from_file_type(int ftype) {
    switch(ftype) {
    case FILE_MAKEFILE: // Makefiles require tabulators.
        use_tabulators = 1;
        break;
    default:
        use_tabulators = 0;
        break;
    }
}

struct line suffix;
#define MAX_SUFFIX_MESSAGE_LEN MAX_SUFFIX_LEN * 2
char suffix_message[MAX_SUFFIX_MESSAGE_LEN] = "";
// Extract the filename suffix and then return the file type.
int get_file_type(struct line* fname) {
    int ftype = FILE_OTHER;
    int suffix_index = 0;
    // Get the current file name suffix.
    for (int i=0; i<fname->len; i++) {
        // Terminate the suffix.
        if (fname->text[i] == '.' || fname->text[i] == '/') {
            suffix_index = 0;
            continue;
        }
        suffix.text[suffix_index] = fname->text[i];
        suffix_index += 1;
        suffix.len = suffix_index;
    }
    suffix.text[suffix.len] = '\0'; // null terminate
    int i=0;
    // Check for a matching suffix.
    while (file_type_suffixes[i][0] != '\0') {
        if (strcmp(suffix.text, file_type_suffixes[i]) == 0) {
            ftype = i;
            break;
        }
        i += 1;
    }
/*
    snprintf(suffix_message, MAX_SUFFIX_MESSAGE_LEN, "File type: \"%.*s\"", MAX_SUFFIX_LEN, suffix.text);
    menu_alert = suffix_message;
*/
    return ftype;
}
