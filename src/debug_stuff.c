//See LICENSE file for copyright and license details.
#include "gred.h"

extern int mode;
extern int previous_mode;
#define MAX_MODE_NAME_LEN 32
char mode_names[NUM_MODES][MAX_MODE_NAME_LEN] = {
    "INSERT_MODE",
    "ESCAPE_MODE",
};

void doi() {
    system("clear");
    printf("DOI!!!\n");
    exit(0);
}

extern struct line last_input;
void debug_input(char c) {
    //
    print_debug_mode(previous_mode);
    printf(", ");
    print_debug_mode(mode);
    //
    printf(": ASCII % 5d: \"", (int)c);
    print_debug_input(c);
    printf("\", I:");
    for (int i=0; i<5; i++) {
        putchar('\'');
        print_debug_input(last_input.text[i]);
        putchar('\'');
        putchar(' ');
    }
    printf("\n");
} 

void print_debug_input(char c) {
    switch (c) {
        case ESCAPE:
            printf("ESCAPE");
            break;
        case BACKSPACE:
            printf("BACKSPACE");
            break;
        case '\n':
            printf("NEWLINE");
            break;
        case '\r':
            printf("CARRIAGE_RETURN");
            break;
        default:
            putchar(c);
            break;
    }
}

void print_debug_mode(int state) {
    switch (state) {
        case ESCAPE_MODE:
            printf("ESCAPE_MODE");
            break;
        case INSERT_MODE:
            printf("INSERT_MODE");
            break;
        default:
            printf("INVALID INPUT STATE");
            break;
    }
}
