#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define BUFFER_SIZE 2048

static const char *EXIT = "exit";
static const char *ECHO = "echo";
static const char *TYPE = "type";

typedef struct command {
    char *name;
    char *all_args;
    size_t arg_count;
    char **args;
} Command;

typedef enum command_type {
    INTERNAL,
    EXTERNAL,
    UNKNOWN
} CommandType;

typedef enum command_behavior {
    TERMINATE,
    EXECUTE,
    BACKGROUND
} CommandBehavior;

typedef CommandBehavior (execute_command)(const Command *);

char *ltrim(char *);

char *rtrim(char *);

char *trim(char *);

char *get_command_name(const char *);

char *get_all_args(char *);

size_t get_number_of_args(const char *);

void initialize_args_array(char **, const char *, size_t);

Command parse_command(const char *);

void free_command(const Command *);

execute_command *factory(const Command *);

CommandType get_command_type(const Command *);

CommandBehavior execute_generic_command(const Command *);

CommandBehavior execute_inbuilt_command(const Command *);

CommandBehavior unknown(const Command *);

CommandBehavior type_command(const Command *);

CommandBehavior exit_command(__attribute__((unused)) const Command *);

CommandBehavior echo_command(const Command *);

int main() {
    while (1) {
        printf("$ ");
        fflush(stdout);

        char *input = (char *) calloc(BUFFER_SIZE, sizeof(char));
        fgets(input, BUFFER_SIZE, stdin);

        const char *command = trim(input);

        if (!strlen(command)) {
            continue;
        }

        Command parsed = parse_command(command);
        CommandBehavior behavior = execute_generic_command(&parsed);

        if (behavior == TERMINATE) {
            break;
        }

        switch (behavior) {
            case EXECUTE:
            case BACKGROUND:
                free_command(&parsed);
                free(input);
                continue;
            default:
                break;
        }

        free_command(&parsed);
        free(input);
        fflush(stdout);
    }
    return 0;
}

char *ltrim(char *str) {
    while (isspace(*str)) str++;
    return str;
}

char *rtrim(char *str) {
    int end_index = (int) strlen(str) - 1;
    if (end_index == -1) return NULL;
    while (isspace(*(str + end_index))) end_index--;
    *(str + end_index + 1) = '\0';

    return str;
}

char *trim(char *str) {
    return ltrim(rtrim(str));
}

char *get_command_name(const char *input) {
    assert(input != NULL);
    size_t index = 0;
    while (input[index] != ' ' && input[index]) index++;
    return strndup(input, index);
}

char *get_all_args(char *command) {
    assert(command != NULL);
    char *command_name = get_command_name(command);

    size_t index = strlen(command_name);
    assert(command[index] == ' ' || command[index] == '\0');

    command = command + index;

    free(command_name);
    return ltrim(command);
}

size_t get_number_of_args(const char *command) {
    assert(command != NULL);
    char *dup_command = strdup(command);
    const char *all_args = get_all_args(dup_command);

    size_t total_args_size = strlen(all_args);
    if (total_args_size == 0) {
        return 0;
    }

    bool is_space = false;
    size_t count = 1;

    for (int index = 0; index < total_args_size; index++) {
        if (!is_space && all_args[index] == ' ') {
            is_space = true;
        }

        if (is_space && all_args[index] != ' ') {
            is_space = false;
            count++;
        }
    }

    free(dup_command);
    return count;
}

void initialize_args_array(char **args, const char *all_args, size_t args_count) {
    assert(args != NULL);
    assert(all_args != NULL);
    size_t start_index = 0;
    size_t arg_index = 0;
    bool is_space = false;
    size_t all_args_size = strlen(all_args);
    size_t index = 0;

    while (index < all_args_size && arg_index < args_count) {
        if (!is_space && all_args[index] == ' ') {
            is_space = true;
            args[arg_index++] = strndup(all_args + start_index, index - start_index);
        }

        if (is_space && all_args[index] != ' ') {
            is_space = false;
            start_index = index;
        }
        index++;
    }

    args[arg_index] = strdup(all_args + start_index);
}

Command parse_command(const char *input) {
    assert(input != NULL);

    char *command_name = get_command_name(input);
    size_t number_of_args = get_number_of_args(input);
    if (number_of_args == 0) {
        Command command = {
                command_name,
                "",
                0,
                NULL
        };
        return command;
    }

    char *dup_input = strdup(input);
    char *all_args = get_all_args(dup_input);

    char *args[number_of_args];
    initialize_args_array(args, all_args, number_of_args);
    Command command = {
            command_name,
            all_args,
            number_of_args,
            args
    };

    return command;
}

void free_command(const Command *command) {
    for (size_t i = 0; i < command->arg_count; i++) {
        free(command->args[i]);
    }
}

execute_command *factory(const Command *command) {
    const char *name = command->name;
    if (!strcmp(name, EXIT)) {
        return &exit_command;
    } else if (!strcmp(name, ECHO)) {
        return &echo_command;
    } else if (!strcmp(name, TYPE)) {
        return &type_command;
    }

    return &unknown;
}

CommandType get_command_type(const Command *command) {
    const char *name = command->name;
    bool is_inbuilt = !strcmp(name, EXIT) || !strcmp(name, ECHO) || !strcmp(name, TYPE);

    if (is_inbuilt) {
        return INTERNAL;
    }

    return UNKNOWN;
}

CommandBehavior execute_generic_command(const Command *command) {
    CommandType type = get_command_type(command);

    switch (type) {
        case INTERNAL:
            return execute_inbuilt_command(command);
        case EXTERNAL:
            return EXECUTE;
        case UNKNOWN:
            return unknown(command);
    }
}

CommandBehavior execute_inbuilt_command(const Command *command) {
    execute_command *execute = factory(command);
    return execute(command);
}

CommandBehavior unknown(const Command *command) {
    printf("%s: command not found\n", command->name);
    return EXECUTE;
}

CommandBehavior type_command(const Command *command) {
    //there should be at least one argument
    assert(command->arg_count >= 1);

    char *type_argument = command->args[0];
    Command type_command = parse_command(type_argument);
    CommandType type = get_command_type(&type_command);

    if (type == INTERNAL) {
        printf("%s is a shell builtin\n", type_argument);
    } else {
        printf("%s: not found\n", type_argument);
    }
    return EXECUTE;
}

CommandBehavior exit_command(__attribute__((unused)) const Command *command) {
    return TERMINATE;
}

CommandBehavior echo_command(const Command *command) {
    printf("%s\n", command->all_args);
    return EXECUTE;
}
