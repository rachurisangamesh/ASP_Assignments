#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_CMD_LEN 256

// Function to forcefully terminate a process
void kill_process(pid_t pid) {
    if (kill(pid, SIGKILL) == -1) {
        perror("kill");
    } else {
        printf("Terminated process with pid %d\n", pid);
    }
}

// Function to check if a process is defunct
bool is_defunct(pid_t pid) {
    char status_path[MAX_CMD_LEN];
    sprintf(status_path, "/proc/%d/status", pid);
    FILE* status_file = fopen(status_path, "r");
    if (status_file == NULL) {
        return false;
    }
    char line[MAX_CMD_LEN];
    while (fgets(line, MAX_CMD_LEN, status_file) != NULL) {
        if (strncmp(line, "State:", 6) == 0) {
            char* state = line + 7;
            if (strncmp(state, "Z", 1) == 0) {
                fclose(status_file);
                return true;
            } else {
                fclose(status_file);
                return false;
            }
        }
    }
    fclose(status_file);
    return false;
}

// Function to get the elapsed time of a process in minutes since it was created
int get_elapsed_time(pid_t pid) {
    char stat_path[MAX_CMD_LEN];
    sprintf(stat_path, "/proc/%d/stat", pid);
    FILE* stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
        return -1;
    }
    char line[MAX_CMD_LEN];
    if (fgets(line, MAX_CMD_LEN, stat_file) == NULL) {
        fclose(stat_file);
        return -1;
    }
    fclose(stat_file);
    char* start_time_str = strrchr(line, ')') + 2;
    time_t start_time = strtol(start_time_str, NULL, 10) / sysconf(_SC_CLK_TCK);
    time_t current_time = time(NULL);
    return (int) difftime(current_time, start_time) / 60;
}

// Function to count the number of defunct child processes of a process
int count_defunct_children(pid_t pid) {
    char proc_path[MAX_CMD_LEN];
    sprintf(proc_path, "/proc/%d/task", pid);
    DIR* proc_dir = opendir(proc_path);
    if (proc_dir == NULL) {
        return -1;
    }
    int num_defunct = 0;
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            char task_stat_path[MAX_CMD_LEN];
            sprintf(task_stat_path, "%s/%s/stat", proc_path, entry->d_name);
            FILE* task_stat_file = fopen(task_stat_path, "r");
            if (task_stat_file != NULL) {
                char line[MAX_CMD_LEN];
                if (fgets(line, MAX_CMD_LEN, task_stat_file) != NULL) {
                    char* state = strrchr(line, ')') + 3;
                    if (strncmp(state, "Z", 1) == 0) {
                        num_defunct++;
                    }
                }
                fclose(task_stat_file);
            }
        }
    }
    closedir(proc_dir);
    return num_defunct;

}

// Function to forcefully terminate all the parent processes (except BASH) of defunct processes
void kill_parent_processes(pid_t pid) {
char proc_path[MAX_CMD_LEN];
sprintf(proc_path, "/proc/%d", pid);
while (strcmp(proc_path, "/proc/1") != 0) {
char status_path[MAX_CMD_LEN];
sprintf(status_path, "%s/status", proc_path);
FILE* status_file = fopen(status_path, "r");
if (status_file == NULL) {
break;
}
char line[MAX_CMD_LEN];
pid_t ppid = 0;
while (fgets(line, MAX_CMD_LEN, status_file) != NULL) {
if (strncmp(line, "PPid:", 5) == 0) {
ppid = strtol(line + 6, NULL, 10);
break;
}
}
fclose(status_file);
if (ppid == 0 || ppid == 1) {
break;
}
kill_process(ppid);
sprintf(proc_path, "/proc/%d", ppid);
}
}

int main(int argc, char* argv[]) {
if (argc < 2) {
fprintf(stderr, "Usage: %s root_process [OPTION1] [OPTION2]\n", argv[0]);
return EXIT_FAILURE;
}
pid_t root_pid = strtol(argv[1], NULL, 10);
if (root_pid == 0) {
fprintf(stderr, "Invalid root process PID\n");
return EXIT_FAILURE;
}
// Parse command line options
bool opt_t = false;
bool opt_b = false;
int opt_proc_eltime = 0;
int opt_no_of_dfcs = 0;
for (int i = 2; i < argc; i++) {
if (strcmp(argv[i], "-t") == 0) {
opt_t = true;
} else if (strcmp(argv[i], "-b") == 0) {
opt_b = true;
} else if (strncmp(argv[i], "-t=", 3) == 0) {
opt_t = true;
opt_proc_eltime = strtol(argv[i] + 3, NULL, 10);
if (opt_proc_eltime < 1) {
fprintf(stderr, "Invalid PROC_ELTIME value\n");
return EXIT_FAILURE;
}
} else if (strncmp(argv[i], "-b=", 3) == 0) {
opt_b = true;
opt_no_of_dfcs = strtol(argv[i] + 3, NULL, 10);
if (opt_no_of_dfcs < 1) {
fprintf(stderr, "Invalid NO_OF_DFCS value\n");
return EXIT_FAILURE;
}
} else {
fprintf(stderr, "Unknown option: %s\n", argv[i]);
return EXIT_FAILURE;
}
}
// Check if root process is defunct
if (!is_defunct(root_pid)) {
printf("Root process %d is not defunct\n", root_pid);
return EXIT_SUCCESS;
}
// Kill parent processes based on command line options
if (opt_t) {
int elapsed_time = get_elapsed_time(root_pid);
if (elapsed_time < 0) {
fprintf(stderr, "Failed to get elapsed time of process %d\n", root_pid);
return EXIT_FAILURE;
}
if (elapsed_time >= opt_proc_eltime) {
kill_parent_processes(root_pid);
}
}
if (opt_b) {
int num_defunct_children = count_defunct_children(root_pid);
    if (num_defunct_children >= opt_no_of_dfcs) {
        kill_process(root_pid);
    }
}
// Success
return EXIT_SUCCESS;
}

