#include <stdio.h>
#include <string.h>

int main() {
    // Uncomment this block to pass the first stage
    printf("$ ");
    fflush(stdout);

    // Wait for user input
    char input[100];
    fgets(input, 100, stdin);

    // The last character in the command is a '\n', we must remove this character.
    input[(int)strlen(input) - 1] = '\0';

    // An invalid command:
    char invalid_command[16] = "invalid_command";
    if (strcmp(input, invalid_command) == 0) {
        printf("%s: not found\n", input);
        fflush(stdout);
    }

    return 0;
}
