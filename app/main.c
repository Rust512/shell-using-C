#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 2048

static const char *EXIT = "exit";
static const char *ECHO = "echo";
static const char *TYPE = "type";

size_t DIR_COUNT;
char *PATH_VARIABLE;
char **DIRECTORIES;

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

typedef CommandBehavior (instructions)(const Command *);

char *ltrim(char *);

char *rtrim(char *);

char *trim(char *);

char *get_command_name(const char *);

char *get_all_args(char *);

size_t get_number_of_args(const char *);

void initialize_args_array(char **, const char *, size_t);

Command parse_command(const char *);

instructions *factory(const Command *);

CommandType get_command_type(const Command *);

CommandBehavior execute_generic_command(const Command *);

CommandBehavior execute_internal_command(const Command *);

CommandBehavior execute_external_command(const Command *);

CommandBehavior unknown(const Command *);

CommandBehavior type_command(const Command *);

void process_external_command(const Command *);

CommandBehavior exit_command(__attribute__((unused)) const Command *);

CommandBehavior echo_command(const Command *);

CommandBehavior external_command(__attribute__((unused)) const Command *);
void get_path_variable();
void set_dir_count();
void set_dirs();
int external_command_exists(const Command *);
bool command_exists_in_dir(const Command *, const char *);
void free_path_details();
void get_full_name(const char *, const char *, char *);
void initialize_path_cache();

int main() {
    initialize_path_cache();
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
                free(input);
                continue;
            default:
                break;
        }

        free(input);
        fflush(stdout);
    }

    free_path_details();
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

    if (arg_index < args_count)
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

instructions *factory(const Command *command) {
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

    if (external_command_exists(command) != -1) {
        return EXTERNAL;
    }

    return UNKNOWN;
}

CommandBehavior execute_generic_command(const Command *command) {
    CommandType type = get_command_type(command);

    switch (type) {
        case INTERNAL:
            return execute_internal_command(command);
        case EXTERNAL:
            return execute_external_command(command);
        case UNKNOWN:
            return unknown(command);
    }
}

CommandBehavior execute_internal_command(const Command *command) {
    instructions *execute = factory(command);
    return execute(command);
}

CommandBehavior execute_external_command(const Command *command) {
    // TODO: implement this.
    return external_command(command);
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

    switch (type) {
        case INTERNAL:
            printf("%s is a shell builtin\n", type_argument);
            break;
        case EXTERNAL:
            process_external_command(&type_command);
            break;
        case UNKNOWN:
            printf("%s: not found\n", type_argument);
            break;
    }

    return EXECUTE;
}

void process_external_command(const Command *command) {
    int dir_index = external_command_exists(command);
    char *full_name = (char *) calloc(strlen(DIRECTORIES[dir_index]) + strlen(command->name) + 2, sizeof(char));
    get_full_name(DIRECTORIES[dir_index], command->name, full_name);
    printf("%s is %s\n", command->name, full_name);
    free(full_name);
}

CommandBehavior exit_command(__attribute__((unused)) const Command *command) {
    return TERMINATE;
}

CommandBehavior echo_command(const Command *command) {
    printf("%s\n", command->all_args);
    return EXECUTE;
}

CommandBehavior external_command(const Command *command) {
    assert(command != NULL);
    assert(command->name != NULL);

    int dir_index = external_command_exists(command);
    assert(dir_index != -1);
    char *full_name = (char *) calloc(strlen(DIRECTORIES[dir_index]) + strlen(command->name) + 2, sizeof(char));
    get_full_name(DIRECTORIES[dir_index], command->name, full_name);

    char *string_command = (char*) calloc(strlen(full_name) + strlen(command->all_args) + 2, sizeof(char));

    strcat(string_command, full_name);
    strcat(string_command, " ");
    strcat(string_command, command->all_args);

    FILE *fp = popen(string_command, "r");

    if (fp == NULL) {
        printf("Failed to execute command: %s\n", string_command);
        free(string_command);
        free(full_name);
        return EXECUTE;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE - 1, fp) != NULL) {
        printf("%s", buffer);
    }

    free(string_command);
    free(full_name);
    return EXECUTE;
}


void get_path_variable() {
    FILE *fp = popen("echo $PATH", "r");
    assert(fp != NULL);
    char buffer[BUFFER_SIZE];
    assert(fgets(buffer, BUFFER_SIZE - 1, fp) != NULL);
    PATH_VARIABLE = strdup(buffer);
    pclose(fp);
}

void set_dir_count() {
    size_t index = 0;
    while (PATH_VARIABLE[index] != '\0') {
        DIR_COUNT += (PATH_VARIABLE[index] == ':');
        index++;
    }

    DIR_COUNT++;
    assert(DIR_COUNT >= 1);
    DIRECTORIES = (char **) calloc(DIR_COUNT, sizeof(char *));
}

void set_dirs() {
    if (DIRECTORIES == NULL) {
        return;
    }
    size_t index = 0;
    size_t dir_index = 0;
    size_t start_index = 0;
    while (PATH_VARIABLE[index] != '\0') {
        if (PATH_VARIABLE[index] == ':') {
            DIRECTORIES[dir_index] = strndup(PATH_VARIABLE + start_index, index - start_index);
            dir_index++;
            start_index = index + 1;
        }
        index++;
    }

    DIRECTORIES[dir_index] = trim(strdup(PATH_VARIABLE + start_index));
}

int external_command_exists(const Command *command) {
    for (int index = 0; index < DIR_COUNT; index++) {
        if (command_exists_in_dir(command, DIRECTORIES[index])) {
            return index;
        }
    }
    return -1;
}

bool command_exists_in_dir(const Command *command, const char *directory) {
    DIR *dir_pointer = opendir(directory);

    if (dir_pointer == NULL) {
        // directory does not exist
        return false;
    }

    const struct dirent *dir;

    while ((dir = readdir(dir_pointer)) != NULL) {
        if (dir->d_type == DT_DIR) {
            continue;
        }

        struct stat stats;
        // The + 2 in the end is for the '/' character and '\0' characters in the full path.
        char *full_path = (char *) calloc(strlen(directory) + strlen(command->name) + 2, sizeof(char));
        strcat(full_path, directory);
        strcat(full_path, "/");
        strcat(full_path, command->name);
        if (stat(full_path, &stats) == -1) {
            free(full_path);
            continue;
        }

        if (stats.st_mode & S_IXUSR) {
            free(full_path);
            closedir(dir_pointer);
            return true;
        }
        free(full_path);
    }

    closedir(dir_pointer);
    return false;
}

void free_path_details() {
    for (int i = 0; i < DIR_COUNT; i++) {
        free(DIRECTORIES[i]);
    }
    free(DIRECTORIES);
}

void get_full_name(const char *directory, const char *name, char *full_name) {
    assert(directory != NULL);
    assert(strlen(directory) > 0);
    assert(name != NULL);
    assert(strlen(name) > 0);
    assert(full_name != NULL);

    strcat(full_name, directory);
    strcat(full_name, "/");
    strcat(full_name, name);
}

void initialize_path_cache() {
    get_path_variable();
    set_dir_count();
    set_dirs();
}
