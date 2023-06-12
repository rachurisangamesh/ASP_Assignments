#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

char *option;
int processId;
int rootProcessId;

// Given process Id, return parent process Id
int getParentProcess(int pId) {
  FILE *fp;
  int fd;
  char buffer[1024];
  ssize_t n;
  char command[256];
  
  // command to get ppid of a process
  sprintf(command, "ps -o ppid= -p %d", pId);
  fp = popen(command, "r");
  if (fp == NULL) {
    return 0;
  }

  fd = fileno(fp);
  n = read(fd, buffer, sizeof(buffer));

  if (n == 0) {
    return 0;
  } else {
    return atoi(buffer);
  }
  pclose(fp);
  return 0;
}

// given process Id, return 1 if process is zombie else 0
int is_zombie(pid_t pid) {
  char path[1024];
  char buffer[4096];
  int fd, nread, is_zombie = 0;

  // path to the process status file
  snprintf(path, sizeof(path), "/proc/%d/status", pid);
  fd = open(path, O_RDONLY);
  if (fd == -1) {
      perror("open");
      return -1;
  }
  
  // get the status of the process
  while ((nread = read(fd, buffer, sizeof(buffer))) > 0) {
      if (strstr(buffer, "State:\tZ (zombie)\n")) {
          is_zombie = 1;
          break;
      }
  }

  close(fd);
  return is_zombie;
}

// iterate over all the processes
void iterateOverAllProcesses(int pId, int is_grand_children) {
  if (strcmp(option, "-c") == 0)
    printf("Child processes for PID %d:\n", pId);
  else if (strcmp(option, "-s") == 0) {
    printf("Sibling processes of process %d are: \n", processId);
    printf("PID :: PPID\n");
  } else if (strcmp(option, "-zl") == 0) {
    printf("Child process in defunct state are:\n");
  } else if (strcmp(option, "-gc") == 0 && is_grand_children == 0) {
    printf("Grand children of process %d:\n", processId);
    printf("PID :: PPID\n");
  }

  DIR *proc_dir = opendir("/proc");
  if (proc_dir == NULL) {
    perror("opendir() failed");
    exit(1);
  }

  struct dirent *entry;
  while ((entry = readdir(proc_dir)) != NULL) {
    if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
      pid_t childPid = atoi(entry->d_name);
      if (childPid > 0 && childPid != pId && getParentProcess(childPid) == pId) {
        if (strcmp(option, "-s") == 0) {
          if (childPid != processId) {
            printf("%d  :: %d\n", childPid, getParentProcess(childPid));
          }
        } else if (strcmp(option, "-c") == 0) {
          // display pids of all child processes
          printf("%d\n", childPid);
        } else if (strcmp(option, "-zl") == 0 && is_zombie(childPid)) {
          printf("%d\n", childPid);
        } else if (strcmp(option, "-gc") == 0) {
          //printf("child: %d  %d\n", pId, childPid);
          if (is_grand_children == 0) {
            iterateOverAllProcesses(childPid, 1);
          } else {
            printf("%d :: %d\n", childPid, pId);
          }
        }
      }
    }
  }

  closedir(proc_dir);
}

// call iterateOverAllProcesses() based on the option entered
void performOperation(int processId) {
  if (option == NULL) {
    return;
  } else if (strcmp(option, "-z") == 0) {
    if (is_zombie(processId)) {
      printf("%d: Zombie process\n", processId);
    }
  } else if (strcmp(option, "-c") == 0 || strcmp(option, "-zl") == 0 || strcmp(option, "-gc") == 0) {
    iterateOverAllProcesses(processId, 0);
  } else if (strcmp(option, "-s") == 0) {
    iterateOverAllProcesses(getParentProcess(processId), 0);
  } else if (strcmp(option, "-gp") == 0) {
    printf("PID of grandparent of %d is %d\n", processId, getParentProcess(getParentProcess(processId)));
  }
}

// main method
int main(int argC, char *argv[]) {

  // invalid input
  if (argC != 4 && argC != 3) {
    printf("Usage: prctree root_process_id process_id [option]\n");
    exit(1);
  }

  rootProcessId = atoi(argv[1]);
  processId = atoi(argv[2]);

  if (argC == 3) {
    option = NULL;
  } else {
    option = argv[3];
  }

  printf("PId of process %d is %d\n", processId, getParentProcess(processId));
  performOperation(processId);

  return 0;
}
