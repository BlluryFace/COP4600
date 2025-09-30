/******************************************************
 * Name: Huy Bui
 * NetID: U823909003
 * Description: Project 1 – Simple Unix Shell (rush)
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT 256
#define MAX_ARGS 64
#define MAX_PATHS 10
#define MAX_PATH_LEN 128
// print out error whenever error is raised
void error() {
    //•	Writes directly to the file descriptor for stderr.
    //•Completely unbuffered (appears immediately).
    write(STDERR_FILENO, "An error has occurred\n", 22);
}

// =============== HELPER FUNCTIONS ====================

/// Execute commands that the user passes in
/// \param args: parsed user's command
/// \param argc: number of arguments
void execute_exe(char **args, int argc, char program[]) {
// Check for output redirection
    int fd = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], ">") == 0) {
            // Must have exactly one argument after '>'
            if (i != argc - 2) {
                write(STDERR_FILENO, "An error has occurred\n", 22);
                return;
            }
            fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                return;
            }
            args[i] = NULL; // truncate args at '>'
            break;
        }
    }

    // Fork and exec
    pid_t pid = fork();
    if (pid < 0) {
        error();
    } else if (pid == 0) {
        // Child
        if (fd != -1) {
            if (dup2(fd, STDOUT_FILENO) < 0) {
                error();
                _exit(1);
            }
            close(fd);
        }
        execv(program, args);
        // If execv fails
        error();
        _exit(1);
    } else {
        // Parent waits
        waitpid(pid, NULL, 0);
        if (fd != -1) close(fd);
    }
}


// =============== MAIN SHELL LOOP =====================

int main(int argc, char *argv[]) {
    // If rush is started with any arguments -> error + exit(1)
    if (argc != 1) {
        error();
        exit(1);
    }
    // Init path
    char path_dirs[MAX_PATHS][MAX_PATH_LEN];
    int path_count = 1;

    // Initialize default path
    strncpy(path_dirs[0], "/bin", MAX_PATH_LEN - 1);
    path_dirs[0][MAX_PATH_LEN - 1] = '\0';  // ensure null-terminated

    char *line = NULL;
    size_t len = 0;

    // Parsing user's command into arguments and count it
    while (1) {
        printf("rush> "); // The start of the command
        fflush(stdout); // Force buffer to flush word immediately into the terminal
        if (getline(&line, &len, stdin) == -1) {
            break; // End Of File
        }

        // Parse input into args[]
        char *args[MAX_ARGS]; // array of argument strings
        int arg_count = 0;
        // Splitting strings into token everytime it meets a space, tab or newline character
        char *token = strtok(line, " \t\n");
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            // Splitting strings into token everytime it meets a space, tab or newline character
            token = strtok(NULL, " \t\n");
        }
        // Signify end of arguments
        args[arg_count] = NULL;
        if (arg_count == 0) {
            continue;
        }
        // Built-in: exit
        if (strcmp(args[0], "exit") == 0) {
            if (arg_count != 1) {
                error();
                continue;
            }
            exit(0);
        }

            // Built-in: cd
        else if (strcmp(args[0], "cd") == 0) {
            if (arg_count != 2 || chdir(args[1]) != 0) {
                error();
            }
            continue;
        }

            // Built-in: path
        else if (strcmp(args[0], "path") == 0) {
            path_count = argc - 1; // number of directories provided
            if (path_count > MAX_PATHS) {
                path_count = MAX_PATHS; // limit to max paths
            }

            for (int i = 0; i < path_count; i++) {
                // Copy the path string safely into the fixed array
                strncpy(path_dirs[i], args[i + 1], MAX_PATH_LEN - 1);
                path_dirs[i][MAX_PATH_LEN - 1] = '\0'; // ensure null-termination
            }

            // If no paths are given, path_count becomes 0
            continue;
        }

        char program[MAX_PATH_LEN + 128]; // buffer for full path

        // Find executable in path_dirs
        int found = 0;
        for (int i = 0; i < path_count; i++) {
            snprintf(program, sizeof(program), "%s/%s", path_dirs[i], args[0]);
            if (access(program, X_OK) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // Not found
            write(STDERR_FILENO, "An error has occurred\n", 22);
            continue;
        }
        execute_exe(args, arg_count, program);
    }
    return 0;
}