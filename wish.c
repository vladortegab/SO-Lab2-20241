#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_ARGS 100
#define MAX_PATHS 100

char error_message[30] = "An error has occurred\n";

void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int execute_builtin(char **args, char **paths) {
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            print_error();
            return -1;  
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            print_error();
        } else {
            if (chdir(args[1]) != 0) {
                print_error();
            }
        }
        return 1;
    } else if (strcmp(args[0], "path") == 0) {
        for (int i = 0; i < MAX_PATHS; i++) {
            paths[i] = NULL;
        }
        int i = 0;
        while (args[i + 1] != NULL) {
            paths[i] = strdup(args[i + 1]);
            i++;
        }
        return 1;
    }
    return 0;
}

void execute_external(char **args, char **paths, char *output_file) {
    pid_t pid = fork();
    if (pid == 0) {
        
        if (output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        for (int i = 0; paths[i] != NULL; i++) {
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "%s/%s", paths[i], args[0]);
            if (access(cmd, X_OK) == 0) {
                execv(cmd, args);
            }
        }

        print_error();
        exit(1);
    } else if (pid > 0) {
        wait(NULL);  
    } else {
        print_error();
    }
}


int parse_input(char *line, char **args, char **output_file) {
    int i = 0;
    char *token;
    *output_file = NULL;

    token = strtok(line, " \t\n");
    if (token == NULL) {
        print_error();
        return 1;
    }

    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            if (token == NULL || *output_file != NULL) {
                print_error();
                return 1;
            }
            *output_file = token;
            token = strtok(NULL, " \t\n");
            if (token != NULL) {
                print_error();
                return 1;
            }
            break;  
        } else {
            args[i] = token;
            i++;
        }
        token = strtok(NULL, " \t\n");
    }

    args[i] = NULL;

    if (i == 0) {
        print_error();
        return 1;
    }

    return 0;
}

void execute_parallel(char *line, char **paths) {
    char *commands[MAX_ARGS];
    int i = 0;
    char *token = strtok(line, "&");

    while (token != NULL) {
        commands[i] = token;
        i++;
        token = strtok(NULL, "&");
    }
    commands[i] = NULL;

    for (int j = 0; j < i; j++) {
        char *args[MAX_ARGS];
        char *output_file = NULL;

        if (parse_input(commands[j], args, &output_file) == 0) {
            if (!execute_builtin(args, paths)) {
                execute_external(args, paths, output_file);
            }
        }
        
    }

    for (int j = 0; j < i; j++) {
        wait(NULL);
    }
}

void trim_whitespace(char **line) {
    char *start = *line;
    char *end;

    while (isspace((unsigned char)*start)) {
        start++;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    *line = start;
}


int main(int argc, char *argv[]) {
    char *paths[MAX_PATHS] = {"/bin", NULL};  // Initial path
    char line[1024];

    FILE *input_stream = stdin;
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            print_error();
            exit(1);
        }
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

while (1) {
    if (input_stream == stdin) {
        printf("wish> ");
    }

    if (fgets(line, sizeof(line), input_stream) == NULL) {
        break;  
    }

    char *trimmed_line = line;
    trim_whitespace(&trimmed_line);

    if (strlen(trimmed_line) == 0) {
        continue;
    }

    if (strchr(trimmed_line, '&') != NULL) {
        execute_parallel(trimmed_line, paths);
    } else {
        char *args[MAX_ARGS];
        char *output_file = NULL;

        if (parse_input(trimmed_line, args, &output_file) == 0) {
            int builtin_status = execute_builtin(args, paths);

            if (builtin_status == -1) {
                
                continue;
            }
            if (builtin_status == 0) {
                execute_external(args, paths, output_file);
            }
        }
    }
}

    if (input_stream != stdin) {
        fclose(input_stream);  
    }

    return 0;
}
