#include <stdio.h>

int main() {
    // Uncomment this block to pass the first stage
    printf("$ ");
    fflush(stdout);

    // Wait for user input
    char input[100];
    fgets(input, 100, stdin);

    // The last character in the command is a '\n', we must remove this character.
    printf("%s: not found\n", input);
    fflush(stdout);

    return 0;
}
