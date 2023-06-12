/*

   ******************************
   *                            *
   *   Author: Akshay Kumar     *
   *   Student ID: 110090317    *
   *                            *
   ******************************

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMANDS 6
#define MAX_CMD_ARGS 6
#define MAX_CMD_LENGTH 1024
#define MAX_PIPE 5

int argCount = 0;

// function will count spaces in the given command
int count_spaces(char* cmd) {
    int count = 0;
    for (int i = 0; i < strlen(cmd); i++) {
        if (cmd[i] == ' ') {
            count++;
        }
    }
    return count;
}

// split the command based on space delimiter, also removes `&` from the end in case of background process
void split_command(char *cmd, char *args[], int isBgProcess) {
    // if no spaces in the command, no need to split the command
    if (count_spaces(cmd) < 1) {
        args[0] = cmd;
        args[1] = NULL;
        argCount = 1;
        return;
    }
    char *token = strtok(cmd, " ");
    int argc = 0;
    while (token != NULL) {
        args[argc] = token;
        argc++;
        token = strtok(NULL, " ");
    }
    
    // if a commnad needs to be executed in background, remove `&` from the end
    if (isBgProcess) {
        args[argc] = NULL;
    } else 
        args[argc] = NULL;
    argCount = argc + 1;
}

// given an `args` array, which consists of program name and its arguments, execute it based on `type`
int execute_command(char *args[], int type) {

    // fork a new process to execute the command
    pid_t pid = fork();
    if (pid == -1) {
        printf("Error: failed to fork\n");
    } else if (pid == 0) {
        // executing the command
        if (execvp(args[0], args) == -1) {
            printf("Error: failed to execute command\n");
            exit(1);
        }
    } else {
        int status;
        if (type == 1) {
            // background execution
            // do nothing
        } else if (type == 2) {
            // if not a bg execution, wait for child proces to complete
            waitpid(pid, &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                printf("Error: command %s returned a non-zero exit status\n", args[0]);
            }
        }
        return status;
    }
}

// function to execute redirection commands
void execute_redirection(char *args[]) {
    int status;
    char *outfile = NULL;
    char *infile = NULL;
    int append = 0;

    // iterate through all args and update infile, outfile and append variable based on redirection operator used
    for (int i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] != NULL) {
                outfile = args[i + 1];
                args[i] = NULL;
                i++;
            } else {
                printf("Invalid command: no output file specified after >\n");
                return;
            }
        } else if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] != NULL) {
                infile = args[i + 1];
                args[i] = NULL;
                i++;
            } else {
                printf("Invalid command: no input file specified after <\n");
                return;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (args[i + 1] != NULL) {
                outfile = args[i + 1];
                args[i] = NULL;
                i++;
                append = 1;
            } else {
                printf("Invalid command: no output file specified after >>\n");
                return;
            }
        }
    }

    pid_t pid = fork();
    if (pid == -1) {
        printf("Failed to fork\n");
    } else if (pid == 0) {
        int file;
        
        // redirection using dup2
        if (infile != NULL) {
            // Redirect input from file
            file = open(infile, O_RDONLY);
            dup2(file, 0);
        }
        if (outfile != NULL) {
            // Redirect output to file
            if (append) {
                file = open(outfile, O_CREAT | O_APPEND | O_WRONLY, 0777);
                lseek(file, 0, SEEK_END);
            } else {
                file = open(outfile, O_CREAT | O_WRONLY, 0777);
            }
            dup2(file, 1);
        }
        // exexuting the command
        execvp(args[0], args);
        printf("Failed to execute the command\n");
    } else {
        // waiting for the command to execute
        waitpid(pid, &status, 0);
    }
}

// execute command consisting of pipes
void execute_pipeline(char **commands[], int num_commands) {
    int i;
    // to store fds for pipes
    int pipes[MAX_PIPE][2];

    // iterating over all the commands
    for (i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            // create a pipe for each command except the last
            if (pipe(pipes[i]) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) {
                // redirect stdin to the read end of the previous pipe
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < num_commands - 1) {
                // redirect stdout to the write end of the current pipe
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            // close all pipe file descriptors
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            if (execvp(commands[i][0], commands[i]) < 0) {
                // if execvp returns, there was an error
                printf("Failed to execute command\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    // close all pipe file descriptors in the parent process
    for (i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    // wait for all child processes to finish
    for (i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

int check_arg_constraints(int count){
    if (count < 1 || count > 6) {
        printf("Argument limit exceeded\n");
        return 1;
    }
    return 0;
}

int check_command_constraints(int count, int limit) {
    if (count < 1 || count > limit) {
        printf("Command limit exceeded\n");
        return 1;
    }
    return 0;
}

// parse command consisting of pipes
void parse_pipeline(char *pipeline, char ***commands, int *num_commands) {
    char *command_tokens[MAX_PIPE + 1];
    int num_tokens = 0;
    char *token = strtok(pipeline, "|");
    while (token != NULL && num_tokens < MAX_PIPE + 1) {
        command_tokens[num_tokens++] = token;
        token = strtok(NULL, "|");
    }

    *num_commands = 0;
    for (int i = 0; i < num_tokens; i++) {
        // allocate memory for the arguments array
        char **args = malloc(MAX_CMD_ARGS * sizeof(char *));
        if (args == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        split_command(command_tokens[i], args, 0);
        if (check_arg_constraints(argCount)) {
            return;
        }
        // add the arguments array to the commands array
        commands[(*num_commands)++] = args;
    }
}

void background_execution(char* command) {
    char *args[MAX_CMD_ARGS + 1];
    split_command(strtok(command, "&"), args, 1);
    if (check_arg_constraints(argCount)) {
        return;
    }
    execute_command(args, 1);
}

void sequential_execution(char* command) {
    char *commands[5];
    char *cmd;
    int num_cmds = 0;
    cmd = strtok(command, ";");
    // split the command by `;`
    while (cmd != NULL) {
        commands[num_cmds] = (char*) malloc(strlen(cmd) + 1);
        strcpy(commands[num_cmds], cmd);
        num_cmds++;
        cmd = strtok(NULL, ";");
    }

    if (check_command_constraints(num_cmds, 6))
        return;

    // iterate over each command and exexute sequentially
    for (int i = 0; i < num_cmds; i++) {
        cmd = commands[i];
        char *args[MAX_CMD_ARGS + 1];
        split_command(cmd, args, 0);
        if (check_arg_constraints(argCount)) {
            return;
        }
        // execute the command and wait for it to complete
        execute_command(args, 2);
    }
}

void redirection_execution(char* command) {
    char *args[MAX_CMD_ARGS + 1];
    split_command(command, args, 1);
    if (check_arg_constraints(argCount)) {
        return;
    }
    execute_redirection(args);
}

void conditional_execution(char* command) {
    // Allocate memory for commands and conditional operator(delimiters) arrays
    char** commands = (char**) malloc(MAX_COMMANDS * sizeof(char*));
    char** delimiters = (char**) malloc(MAX_COMMANDS * sizeof(char*));
    char *cmd;
    char *rest = command;

    int i = 0, j = 0;
    // split the command by conditional operators
    while ((cmd = strtok_r(rest, "&&||", &rest))) {
        if (i >= MAX_COMMANDS) {
            printf("Too many commands\n");
            exit(1);
        }
        commands[i++] = cmd;
        if (rest[0] == '&' ) {
            delimiters[j++] = "&&";
        } else if (rest[0] == '|') {
            delimiters[j++] = "||";
        }
    }

    // iterate over all commands and execute based on status and conditional operator mentioned
    for (i = 0; i < MAX_COMMANDS && commands[i]; i++) {
        char *args[MAX_CMD_ARGS + 1];
        split_command(commands[i], args, 1);
        if (check_arg_constraints(argCount)) {
            return;
        }
        int status = execute_command(args, 2);
        if (i < j) {
            if (strcmp(delimiters[i], "&&") == 0 && status != 0) {
                return;
            } else if (strcmp(delimiters[i], "||") == 0 && status == 0){
                return;
            }
        }
    }

    // Free allocated memory
    free(commands);
    free(delimiters);
}

void pipe_execution(char* command) {
    char **commands[MAX_PIPE + 1];
    int num_commands;
    parse_pipeline(command, commands, &num_commands);
    if (num_commands > 0) {
        execute_pipeline(commands, num_commands);
        // free the memory allocated for the commands array
        for (int i = 0; i < num_commands; i++) {
            free(commands[i]);
        }
    }
}

int main() {
    char command[MAX_CMD_LENGTH];
    while (1) {
        printf("\nms$ ");
        fgets(command, MAX_CMD_LENGTH, stdin);
        command[strcspn(command, "\n")] = '\0';

        // if command is empty, do nothing
        if (strlen(command) == 0) {
            continue;
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting...\n");
            exit(0);
        }

        // Check if the command string contains special characters
        if (strstr(command, "||") != NULL || strstr(command, "&&") != NULL) {
            conditional_execution(command);
        } else if (strchr(command, '|') != NULL) {
            pipe_execution(command);
        } else if (strchr(command, '&') != NULL) {
            background_execution(command);
        }else if (strchr(command, ';') != NULL) {
            sequential_execution(command);
        } else if (strchr(command, '>') != NULL || strchr(command, '<') != NULL || strstr(command, ">>") != NULL) {
            redirection_execution(command);
        } else {
            // in case command does not contain any special characters, execute as is
            char *args[MAX_CMD_ARGS + 1];
            split_command(command, args, 0);
            if (check_arg_constraints(argCount)) {
                return 1;
            }
            execute_command(args, 2);
        }
    }

    return 0;
}