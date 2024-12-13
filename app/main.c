#include <stdio.h>

int get_string_length(char* str);

int main() {
    // Uncomment this block to pass the first stage
    printf("$ ");
    fflush(stdout);

    // Wait for user input
    char input[100];
    fgets(input, 100, stdin);

    // remove the trailing '\n' character from input string.
    input[get_string_length(input) - 1] = '\0';

    // The last character in the command is a '\n', we must remove this character.
    printf("%s: not found\n", input);
    fflush(stdout);

    return 0;
}

int get_string_length(char* str) {
    if (str == NULL) {
        return 0;
    }

    int length = 0;

    while (*str != '\0') {
        length++;
        str++;
    }

    return length;
}
