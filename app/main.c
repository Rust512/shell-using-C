#include <stdio.h>
#include <string.h>

#define EXIT "exit 0"
#define ECHO "echo "

int main() {
    while (1) {
        // Uncomment this block to pass the first stage
        printf("$ ");
        fflush(stdout);

        // Wait for user input
        char input[100];
        fgets(input, 100, stdin);

        // remove the trailing '\n' character from input string.
        input[strlen(input) - 1] = '\0';

        if (strcmp(EXIT, input) == 0L) {
            break;
        }

        if (strncmp(ECHO, input, strlen(ECHO)) == 0L) {
            printf("%s\n", input + strlen(ECHO));
            fflush(stdout);
            continue;
        }

        // The last character in the command is a '\n', we must remove this character.
        printf("%s: not found\n", input);
        fflush(stdout);
    }

    return 0;
}
