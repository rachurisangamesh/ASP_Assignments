#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>

int root_process_id;
char *option1;
int option2 = 0;

// Function to return the process elapsed time in minutes, given process Id
long int getProcessElapsedTime(int pId) {

  char cmd[256];
  
  // command to get elapsed time for a process
  snprintf(cmd, sizeof(cmd), "ps -o etimes= -p %d", pId);
  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
      fprintf(stderr, "Unable to run command %s\n", cmd);
      return 1;
  }
  char output[256];
  fgets(output, sizeof(output), fp);
  pclose(fp);
  long int elapsed_seconds = strtol(output, NULL, 10);
  long int elapsed_minutes = elapsed_seconds / 60;
  return elapsed_minutes;
}

// Given process Id, returns the name of the process
char* getProcessName(int pid) {
  char* name = NULL;
  char path[40];
  FILE* file = NULL;

  // path to the status file for the given PID
  sprintf(path, "/proc/%d/status", pid);
  file = fopen(path, "r");

  if (file != NULL) {
    char line[128];
    if (fgets(line, sizeof(line), file) != NULL) {
      const char* name_start = strstr(line, "Name:");
      if (name_start != NULL) {
        name_start += 6;
        const char* name_end = strchr(name_start, '\n');
        if (name_end != NULL) {
          size_t name_len = name_end - name_start;
          name = (char*) malloc(name_len + 1);
          strncpy(name, name_start, name_len);
          name[name_len] = '\0';
        }
      }
    }
    fclose(file);
  }
  return name;
}

// Returns parent proces Id, given process Id
int getParentProcess(int pId) {

  FILE *fp;
  int fd;
  char buffer[1024];
  ssize_t n;
  char command[256];

  // command to get parent process Id
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

// iterate over all child processes
void children(int pId) {
  FILE *fp;
  char path[1035];

  char command[256];

  // command to get child process pids given parent process
  sprintf(command, "ps -o pid= --ppid %d\n", pId);
  fp = popen(command, "r");
  if (fp == NULL) {
     exit(1);
  }

  // iterate over all child processes and kill the parent processes if child process is a zombie
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    int childId = atoi(path);
    if (is_zombie(childId)) {
      printf("Child process %d is a zombie, so killing the parent process with id: %d\n", childId, pId);
      int parentId = getParentProcess(childId);
      do {
        kill(parentId, SIGKILL);
        printf("Killed parent with id: %d\n", parentId);
        if (parentId == root_process_id) {
          return;
        }
        parentId = getParentProcess(parentId);
      } while (getProcessName(parentId) != NULL && strcmp(getProcessName(parentId), "bash") != 0);
    } else
      children(childId);
  }

  pclose(fp);
}

// iteratae over all child processes
int children_b(int pId) {
  int count = 0;
  FILE *fp;
  char path[1035];

  char command[256];

  // command to get child process pids given parent process
  sprintf(command, "ps -o pid= --ppid %d\n", pId);
  fp = popen(command, "r");
  if (fp == NULL) {
     exit(1);
  }

  // iterate over all child processes, and based on options entered by user, kill the process
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    int childId = atoi(path);
    if (is_zombie(childId)) {
      if (strcmp(option1, "-t") == 0) {
        if (getProcessElapsedTime(pId) > option2) {
          printf("%d\n", pId);
          kill(pId, SIGKILL);
        }
      } else {
        count += 1;
      }
    } else {
      count += children_b(childId);
    }
  }
  printf("Process: %d  Defunct child: %d\n", pId, count);
  if (strcmp(option1, "-b") ==0 ) {
    if (count >= option2) {
      printf("%d\n", pId);
      kill(pId, SIGKILL);
    }
  }

  pclose(fp);
  return count;
}

// main method
int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    printf("Usage: %s <pid>\n", argv[0]);
    exit(1);
  }

  if (argc == 3) {
    printf("Usage: %s <pid> option1 option2\n", argv[0]);
  }

  if (argc == 4) {
    option1 = argv[2];
    option2 = atoi(argv[3]);
    if (option2 < 1) {
      printf("Invalid option2: should be greater than zero\n");
      exit(1);
    }
  }

  root_process_id = atoi(argv[1]);

  if (root_process_id <= 0) {
    printf("Invalid PID: %s\n", argv[1]);
    exit(1);
  }
  
  if (strcmp(option1, "-b") == 0 || strcmp(option1, "-t") == 0) {
    // check number of defunct child processes
    int count = children_b(root_process_id);
    if (count >= option2 && strcmp(option1, "-b") == 0) {
      kill(root_process_id, SIGKILL);
      printf("%d\n", root_process_id);
    }
  } else {
    children(root_process_id);
  }
  return 0;
}
