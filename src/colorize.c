//See LICENSE file for copyright and license details.
// Functions for color-coding keywords.
#include "gred.h"

// On/Off setting
extern int colorize;

// See your terminal's supported colors with this script:
//for ((X=85; X<110; X++)) do N="$X"; printf "$X \033[${X}mhello\033[0m\n"; done;

// List of terminal colors to use:
#define RED 91
#define YELLOW 93
#define GREEN 92
#define BLUE 94
#define BRIGHT_BLUE 95
#define WHITE 37
#define GREY 90
#define DARK_GREY 97


#define MAX_KEYWORD_LEN 8
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
    "+-=(){}[]<>?!&|;*/:";
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

// set the terminal printing color
void set_color(int color) {
    printf("\033[%dm", color);
}
// reset the terminal printing color
void reset_color() {
    printf("\033[0m");
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

// print a line, highlighting key words
void display_line_highlighted(struct line* l, int start_index, int stop_index) {
    struct line word = {0, 0, ""};
    int len = l->len;
    stop_index = bound_value(len, 0, stop_index);
    start_index = bound_value(start_index, 0, stop_index);
    int color = 0;
    // for safety, null terminate the line
    l->text[len] = '\0';
    int i = start_index;
    int word_start = 0;
    // Get each word one-by-one.
    while (i<stop_index) {
        word_start = i;
        word.len = 0;
        // Search to the end of the current word.
        do {
            word.text[i-word_start] = l->text[i];
            word.len += 1;
            i += 1;
            if (word.text[0] == '/' && l->text[i] == '/') { // Reached a comment?
                set_color(COMMENT_COLOR);
                printf("//%s", &l->text[i+1]); // Print the rest of the line and exit.
                reset_color();
                return;
            }
            if (word.text[0] == '"') // handle the quote outside the loop
                break;
        }
        while (
            i < stop_index && 
            !is_operator(word.text[word.len-1], c_operators) && // split if prev was operator
            !isspace(word.text[word.len-1]) &&         // split if previous was space
            !isspace(l->text[i]) &&                    // split if space is next
            !is_operator(l->text[i], c_operators) // split if operator is next
        );
        if (word.text[0] == '"') {
            do {
                word.text[word.len] = l->text[i];
                word.len += 1;
                i += 1;
            } while (i < stop_index && l->text[i-1] != '"');
        }
        // Null terminate the word.
        word.text[word.len] = '\0';
        // Set the color based on which list the word is found in.
        if (word.len == 1 && is_operator(word.text[0], c_operators))
            set_color(OPERATOR_COLOR);
        else if (in_list(&word, c_type_names))
            set_color(TYPE_COLOR);
        else if (in_list(&word, c_macro_names))
            set_color(MACRO_COLOR);
        else if (word.text[0] == '"')
            set_color(QUOTE_COLOR);
        else
            set_color(DEFAULT_COLOR);
        if (word.text[0] == '\t')
            printf("%s", "░"); // replace tabs
        else
            printf("%s", word.text);
        // Turn off color mode.
        printf("\033[0m");
    }
}

void test_highlighting() {
    struct line l = {0,0, "char insert_mode_help[] =  {0, 0, 0}"};
    l.len = strlen(l.text);
    display_line_highlighted(&l, 0, l.len);
    printf("\n");
}
