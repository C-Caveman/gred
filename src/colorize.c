//See LICENSE file for copyright and license details.
// Functions for color-coding keywords.
#include "gred.h"

// On/Off setting
extern int colorize;

/* See your terminal's supported colors with this script:
for ((X=85; X<110; X++)) do N="$X"; printf "$X \033[${X}mhello\033[0m\n"; done;
*/
// List of terminal colors to use:
enum term_color_enum {
    RED=        91,
    YELLOW=     93,
    GREEN=      92,
    BLUE=       94,
    BRIGHT_BLUE=95,
    WHITE=      37,
    GREY=       90,
    DARK_GREY=  97,
    
    HIGHLIGHT=  107,
};
int line_colors[LINE_WIDTH]; // details for each char's color

#define MAX_KEYWORD_LEN 16
//
// Keywords colors:
//
#define OPERATOR_COLOR DARK_GREY
#define TYPE_COLOR RED
#define MACRO_COLOR GREEN
#define COMMENT_COLOR DARK_GREY
#define QUOTE_COLOR WHITE
#define DEFAULT_COLOR GREY
//
// Keywords:
//
char c_operators[] = 
    "+-=(){}[]<>?!&|;*/:.";
char c_type_names[][MAX_KEYWORD_LEN] = {
    "int",
    "char",
    "float",
    "unsigned",
    "struct",
    "extern",
    "static",
    "if",
    "else",
    "for",
    "do",
    "while",
    "return",
    "void",
    "switch",
    "break",
    "case",
    "\0" // null terminate the keyword list
};
char c_macro_names[][MAX_KEYWORD_LEN] = {
    "#define",
    "#include",
    "#ifdef",
    "#endif",
    "\0" // null terminate the list
};

int is_operator(char c, char* operators) {
    int is_op = 0;
    int i=0;
    while (operators[i] != '\0') { // stop at null terminator
        if (c == operators[i]) {
            is_op = 1;
            break;
        }
        i += 1;
    }
    return is_op;
}

// Search for word in list (list must be MAX_KEYWORD_LEN wide AND null terminated)
int in_list(struct line* word, char list[][MAX_KEYWORD_LEN]) {
    int i = 0;
    int item_found = 0;
    word->text[word->len] = '\0'; // null terminate for safety in strcmp()
    while (list[i][0] != '\0') {
        if (strcmp(word->text, list[i]) == 0) {
            item_found = 1;
            break;
        }
        i += 1;
    }
    return item_found;
}

void get_next_word(struct line* l, struct line* w, int* i, int* word_start, int* word_end) {
    char c = ' ';
    char c_next = '?';
    *word_start = *i;
    *word_end = *i;
    line_clear(w); // reset the current word
    while (*i < l->len) {
        c = l->text[*i];
        line_insert_char(c, w->len, w); // add to the current word
        c_next = l->text[*i+1];
        *i += 1;
        // Get comment.
        if (c == '/' && c_next == '/') {
            *word_start = *i-1;
            *word_end = l->len;
            while (*i < l->len) { line_insert_char(l->text[*i], w->len, w); *i += 1; }
            break;
        }
        // Get quote.
        if (c == '"') {
            *word_start = *i-1;
            while (*i < l->len) {
                c = l->text[*i];
                line_insert_char(c, w->len, w); // add to the current word
                c_next = l->text[*i+1];
                *word_end = *i;
                *i += 1;
                if (c == '"') break;
            }
            break;
        }
        // Get regular word.
        if (isspace(c_next) || is_operator(c_next, c_operators) ||
            isspace(c)      || is_operator(c, c_operators)
        )
            break;
        else
            *word_end += 1;
    }
    //printf("(%d, %d) \'%s\' ", *word_start, *word_end, w->text);
}

void set_word_color(int color, int* color_array, int start, int end) {
    for (int i=start; i<=end; i++) {
        color_array[i] = color;
    }
}

// set details for each char's color
void set_line_colors(struct line* l) {
    struct line word = {0, 0, ""}; // current word
    int i=0;
    char c;
    int word_start = 0;
    int word_end = 0;
    int color = 0;
    while (i < l->len) {
        get_next_word(l, &word, &i, &word_start, &word_end);
        // Set the word color.
        if (is_operator(word.text[0], c_operators))          // operator
            color = OPERATOR_COLOR;
        else if (word.text[0] == '/' && word.text[1] == '/') // comment
            color = COMMENT_COLOR;
        else if (word.text[0] == '"')                        // quote
            color = QUOTE_COLOR;
        else if (in_list(&word, c_type_names))               // type
            color = TYPE_COLOR;
        else if (in_list(&word, c_macro_names))              // macro
            color = MACRO_COLOR;
        else                                                 // default
            color = DEFAULT_COLOR;
        set_word_color(color, line_colors, word_start, word_end);
        //printf("%d\n", color);
    }
}

// print a line, highlighting key words
void display_line_highlighted(int row, int start_index, int stop_index) {
    struct line* l = &document[row];
    stop_index = bound_value(stop_index, 0, l->len);
    start_index = bound_value(start_index, 0, stop_index);
    // Set the color information for the line.
    set_line_colors(l);
    char cur_color = line_colors[0];
    printf("\033[0m\033[%dm", cur_color);
    int in_highlight = 0;
    for (int i=start_index; i<stop_index; i++) {
        // Highlighting start.
        if (i == sel_left && selecting && row >= sel_top && row <= sel_bottom) {
            in_highlight = 1;
            cur_color = -1;
            printf("\033[0m\033[107m");
        }
        // Highlighting end.
        if (i == sel_right && selecting) {
            in_highlight = 0;
            cur_color = -1;
            printf("\033[0m");
        }
        // Coloring start/end.
        if ((in_highlight == 0) && (line_colors[i] != cur_color)) { // Switch to the new color.
            cur_color = line_colors[i];
            printf("\033[0m\033[%dm", cur_color);
        }
        if (l->text[i] == '\t')
            printf("%s", "░"); // replace tabs
        else
            putchar(l->text[i]);
        int highlight_ended_inside_unicode_chunk = 0;
        // Fill out the rest of the character if it is a multi-byte utf-8 character.
        while (((l->text[i+1] & 0b10000000) != 0) && ((i+1)<stop_index)) {
            putchar(l->text[++i]);
            if (i == sel_right && selecting)
                highlight_ended_inside_unicode_chunk = 1;
        }
        // Highlighting end.
        if (highlight_ended_inside_unicode_chunk) {
            in_highlight = 0;
            cur_color = -1;
            printf("\033[0m");
        }
    }
    printf("\033[0m"); // Reset to the default print color.
}
