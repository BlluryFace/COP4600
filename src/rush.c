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

#define MAX_INPUT 256
#define MAX_ARGS 64
#define MAX_PATHS 10
#define MAX_PATH_LEN 128

// global path list and size
char **pathlist = NULL;
int pathcount = 0;

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
                error();
                return;
            }
            fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                error();
                return;
            }
            args[i] = NULL; // truncate args at '>'
            break;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        error();
    } else if (pid == 0) {
        // child process
        if (fd != -1){
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // adjust argv[0] to full path (/bin/ls)
        args[0] = program;
        execv(program, args);

        // if exec fails
        error();
        exit(1);

    } else {
        // parent waits
        waitpid(pid, NULL, 0);
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
    pathlist = malloc(sizeof(char*));
    pathlist[0] = strdup("/bin");
    pathcount = 1;

    char *line = NULL;
    size_t len = 0;

    // Parsing user's command into arguments and count it
    while (1) {
        printf("rush> "); // The start of the command
        fflush(stdout); // Force buffer to flush word immediately into the terminal

        if (getline(&line, &len, stdin) == -1) {
            break; // End Of File
        }

        char *args[MAX_ARGS];
        int argc = 0;

        // tokenize by spaces/tabs/newline
        char *tok = strtok(line, " \t\n");
        while (tok != NULL && argc < MAX_ARGS-1) {
            if (strcmp(tok, ">") == 0) {
                // > with no command is invalid
                if (argc == 0) {
                    error();
                    continue;
                }
                tok = strtok(NULL, " \t\n");
                // > must have exactly one filename
                if (tok == NULL || strtok(NULL, " \t\n") != NULL) {
                    error();
                    continue;
                }
                break;
            } else {
                args[argc++] = tok;
            }
            tok = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;

        if (argc == 0) continue; // nothing typed

        // exit takes no args
        if (strcmp(args[0], "exit") == 0) {
            if (args[1] != NULL) {
                error();
                continue;
            }
            exit(0);
        } else if (strcmp(args[0], "cd") == 0) {
            if (argc != 2 || chdir(args[1]) != 0) {
                error();
            }
            continue;
        }
            // Built-in: path
        else if (strcmp(args[0], "path") == 0) {
            // wipe old pathlist
            for (int i = 0; i < pathcount; i++) {
                free(pathlist[i]);
            }
            free(pathlist);

            // count new paths
            int count = 0;
            for (int i = 1; args[i] != NULL; i++) count++;

            // allocate new list
            pathlist = malloc(sizeof(char*) * count);
            pathcount = count;

            // copy new entries
            for (int i = 0; i < count; i++) {
                pathlist[i] = strdup(args[i+1]);
            }
        } else {
            char program[MAX_PATH_LEN + 128]; // buffer for full path
            int found = 0; // flag to indicate if executable is found

            static char full[256];
            for (int i = 0; i < pathcount; i++) {
                snprintf(full, sizeof(full), "%s/%s", pathlist[i], args[0]);
                if (access(full, X_OK) == 0) {
                    strncpy(program, full, sizeof(program) - 1);
                    program[sizeof(program) - 1] = '\0'; // null terminate
                    found = 1;
                    break; // stop at first match
                }
            }
            if (!found) {
                error();      // or write to stderr
                continue;     // skip this command
            }

            execute_exe(args, argc, program);
        }
    }
    return 0;
}