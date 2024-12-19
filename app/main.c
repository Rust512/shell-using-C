#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define EXIT "exit"
#define ECHO "echo"
#define TYPE "type"

typedef enum repl_state {
    CONTINUE,
    BREAK
} repl_state;

char *ltrim(char *);
char *rtrim(char *);
char *trim(char *);
char *get_command_name(char *);
repl_state execution_sequence(char *);
void echo_command(char *);
void type_command(char *);
void not_found(char *);
int is_built_in(const char *);

int main() {
    while (1) {
        // Uncomment this block to pass the first stage
        printf("$ ");
        fflush(stdout);

        // Wait for user input
        char *input = (char *) calloc(100, sizeof(char));
        fgets(input, 100, stdin);

        // trim the command from left and right for any whitespace characters.
        char *command = trim(input);
        repl_state state = execution_sequence(command);

        if (state == BREAK) {
            break;
        }

        fflush(stdout);
        free(input);
    }

    return 0;
}

char *ltrim(char *str) {
    while (isspace(*str)) str++;
    return str;
}

char *rtrim(char *str) {
    int end_index = (int)strlen(str) - 1;
    if (end_index == -1) return NULL;
    while (isspace(*(str + end_index))) end_index--;
    *(str + end_index + 1) = '\0';

    return str;
}

char *trim(char *str) {
    return ltrim(rtrim(str));
}

char *get_command_name(char *command) {
    size_t index = 0;
    while (command[index] != ' ' && command[index] != '\0') {
        index++;
    }
    return strndup(command, index);
}

repl_state execution_sequence(char *command) {
    if (strlen(command) == 0) {
        return CONTINUE;
    }
    char *command_name = get_command_name(command);

    if (strcmp(command_name, ECHO) == 0) {
        echo_command(command);
        return CONTINUE;
    } else if (strcmp(command_name, EXIT) == 0) {
        return BREAK;
    } else if (strcmp(command_name, TYPE) == 0) {
        type_command(command);
        return CONTINUE;
    } else {
        not_found(command_name);
        return CONTINUE;
    }
}

void echo_command(char *command) {
    if (command == NULL) {
        return;
    }
    char *arguments = ltrim(command + strlen(ECHO));
    printf("%s\n", arguments);
}

void type_command(char *command) {
    char *arguments = ltrim(command + strlen(ECHO));
    if (is_built_in(arguments)) {
        printf("%s is a shell builtin\n", arguments);
    } else {
        printf("%s: not found\n", arguments);
    }
}

void not_found(char *command_name) {
    printf("%s: command not found\n", command_name);
}

int is_built_in(const char *command_name) {
    return (!strcmp(command_name, ECHO) || !strcmp(command_name, EXIT) || !strcmp(command_name, TYPE));
}
