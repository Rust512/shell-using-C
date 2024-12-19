#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#define EXIT "exit"
#define ECHO "echo"
#define TYPE "type"

#define NOT_FOUND "NOT_FOUND"
#define ECHO_PATH "echo $PATH"
#define BUFFER_SIZE 1024

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

char *is_external(const char *);

char *get_path_variable();

char *get_command_type(char *);

int get_number_of_paths(const char *);

void segregate_paths(char **, char *);

char *find_command(char **, int, const char *);

char *find_in_directory(char *, const char *);

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
    int end_index = (int) strlen(str) - 1;
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
    char *true_name = get_command_type(arguments);

    if (strcmp(true_name, NOT_FOUND) == 0) {
        printf("%s: not found\n", arguments);
        return;
    }

    if (strcmp(true_name, arguments) == 0) {
        printf("%s is a shell builtin\n", true_name);
        return;
    }

    printf("%s is %s\n", arguments, true_name);
}

void not_found(char *command_name) {
    printf("%s: command not found\n", command_name);
}

int is_built_in(const char *command_name) {
    return (!strcmp(command_name, ECHO) || !strcmp(command_name, EXIT) || !strcmp(command_name, TYPE));
}

char *is_external(const char *command_name) {
    char *paths = get_path_variable();

    int number_of_paths = get_number_of_paths(paths);
    char *all_paths[number_of_paths];
    segregate_paths(all_paths, paths);

    return find_command(all_paths, number_of_paths, command_name);
}

char *get_path_variable() {
    FILE *fp;
    char path_value[BUFFER_SIZE];

    if ((fp = popen(ECHO_PATH, "r")) != NULL) {
        fgets(path_value, BUFFER_SIZE, fp);
    }

    return trim(path_value);
}

char *get_command_type(char *command_name) {
    if (is_built_in(command_name)) {
        return command_name;
    }

    char *absolute_path = is_external(command_name);
    if (absolute_path != NULL) {
        return absolute_path;
    }

    return NOT_FOUND;
}

int get_number_of_paths(const char *paths) {
    int colon_count = 0;

    while (*paths != '\0') {
        if (*paths == ':') {
            colon_count++;
        }
        paths++;
    }

    return colon_count + 1;
}

void segregate_paths(char **path_array, char *paths) {
    int start_index = 0;
    int path_index = 0;
    for (int i = 0; i < strlen(paths); i++) {
        if (paths[i] == ':') {
            path_array[path_index] = strndup(paths + start_index, i - start_index);
            path_index++;
            start_index = i + 1;
        }
    }

    path_array[path_index] = strndup(paths + start_index, strlen(paths) - start_index);
}

char *find_command(char **all_paths, int number_of_paths, const char *command_name) {
    for (int i = 0; i < number_of_paths; i++) {
        char *true_name = find_in_directory(all_paths[i], command_name);
        if (true_name == NULL) {
            continue;
        }
        return true_name;
    }

    return NULL;
}

char *find_in_directory(char *directory_path, const char *command_name) {
    DIR *dir = opendir(directory_path);
    struct dirent *dir_pointer;

    char *true_name = (char *) calloc(strlen(directory_path) + strlen(command_name) + 2, sizeof(char));
    strcat(true_name, directory_path);
    strcat(true_name, "/");
    strcat(true_name, command_name);
    while ((dir_pointer = readdir(dir)) != NULL) {
        if (dir_pointer->d_type == DT_DIR) {
            continue;
        }
        struct stat entry_info;
        if (stat(true_name, &entry_info) == 0) {
            return true_name;
        }
    }

    return NULL;
}
