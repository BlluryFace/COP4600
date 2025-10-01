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
#include <signal.h>
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
void execute_exe(char **args, int argc, char program[], char* file) {

    pid_t pid = fork();
    if (pid < 0) {
        error();
    } else if (pid == 0) {
        // child process
        if (file != NULL) {
            // handle > redirection
            int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                error();
                exit(1);
            }
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
        // parent waits but with timeout
        int status;
        int waited = 0;

        while (waited < 50000) {   // 5-second timeout
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == 0) {
                // child still running
                usleep(1000);
                waited++;
            } else {
                // child finished
                return;
            }
        }

        // timeout exceeded → kill child
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0); // reap it
        error(); // optional: notify timeout
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
        char *file = NULL;
        printf("rush> ");
        fflush(stdout);

        if (getline(&line, &len, stdin) == -1) {
            break; // EOF
        }

        char *args[MAX_ARGS];
        int arg_count = 0;

        // ================= Tokenize =================
        char *tok = strtok(line, " \t\n");
        while (tok != NULL && arg_count < MAX_ARGS - 1) {
            if (strcmp(tok, ">") == 0) {
                tok = strtok(NULL, " \t\n"); // filename
                if (tok == NULL || strtok(NULL, " \t\n") != NULL) {
                    error();
                    file = NULL;
                    arg_count = 0; // skip command
                    break;
                }
                file = tok;
                break; // stop tokenizing after >
            } else {
                args[arg_count++] = tok;
            }
            tok = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) continue; // nothing to execute

        // ============== Check for invalid redirection outside loop =================
        if (file != NULL) {
            int invalid_redirect = 0;
            for (int i = 0; i < arg_count; i++) {
                if (strcmp(args[i], file) == 0) {
                    error();           // cannot redirect to same file being read
                    invalid_redirect = 1;
                    break;
                }
            }
            if (invalid_redirect) continue; // skip executing
        }
        // exit takes no args
        if (strcmp(args[0], "exit") == 0) {
            if (args[1] != NULL) {
                error();
                continue;
            }
            exit(0);
        } else if (strcmp(args[0], "cd") == 0) {
            if (arg_count != 2 || chdir(args[1]) != 0) {
                error();
            }
            continue;
        }
            // Built-in: path
        else if (strcmp(args[0], "path") == 0) {
            // free old pathlist
            for (int i = 0; i < pathcount; i++) {
                free(pathlist[i]);
            }
            free(pathlist);
            pathlist = NULL;
            pathcount = 0;

            // if no args given -> empty pathlist
            if (args[1] == NULL) {
                continue;
            }

            // temporary storage
            char *newpaths[MAX_PATHS];
            int count = 0;
            int valid = 1;

            for (int i = 1; args[i] != NULL; i++) {
                if (access(args[i], X_OK) == 0) {
                    newpaths[count++] = strdup(args[i]);
                } else {
                    valid = 0; // mark invalid path
                }
            }

            if (!valid) {
                // free allocated copies so far
                for (int i = 0; i < count; i++) {
                    free(newpaths[i]);
                }
                error();
                continue;
            }

            // commit new pathlist
            pathlist = malloc(sizeof(char*) * count);
            for (int i = 0; i < count; i++) {
                pathlist[i] = newpaths[i];
            }
            pathcount = count;
        }
        else {
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

            execute_exe(args, arg_count, program, file);
        }
    }
    return 0;
}