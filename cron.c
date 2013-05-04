#define _GNU_SOURCE
#define _POSIX_C_SOURCE = 200000L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

FILE* DEBUG;

int run_cron(const char* file) {
  //Open the manifest file:
  FILE* m = fopen(file, "r");
  size_t size = 100;
  char* line = (char*) malloc (100 * sizeof(char));
  int at_cron = 0;
  time_t now = time(NULL);
  int line_n = 0;
  while (getline(&line, &size, m) > 0) {
    ++line_n;
    if (!at_cron) {
      //Skip comments:
      if (line[0] == '#') continue;

      //Otherwise, if we are at the beginning of our cron jobs, set the flag to start running them.
      if (strcmp(line, "--CRON--\n") == 0) {
        at_cron = 1;
        continue;
      }
      
      //See if this is a link to a manifest file:
      char* before_path = strchr(line, ' ');
      char* after_path = strrchr(line, ' ');
      if (strcmp(after_path, "MANIFEST") == 0) {
        //If it is, run its cron jobs as well:
        char new_file[after_path - before_path];
        memcpy(new_file, before_path, after_path - before_path);
      }
    }
    else {
      //Get the frequency of the cron job:
      char freq[20];
      int i = 0;
      for (; line[i] != ' '; ++i) freq[i] = line[i];
      freq[i] = '\0';
      ++i;

      //Assemble command line arguments for this job:
      int nargs = 2;
      for (int s = i; line[s] != '\n'; ++s) if (line[s] == ' ') ++nargs;
      char* argv[nargs];
      for (int s = i, n = 0, last = i; line[s] != '\0'; ++s) {
        if (line[s] == ' ' || line[s] == '\n') {
          //Put this argument into our array:
          char* narg = (char*) malloc ((s - last + 1) * sizeof(char));
          memcpy(narg, line + last, s - last);
          narg[s - last] = '\0';
          argv[n] = narg;
          last = (s += 1);
          ++n;
        }
      }
      //Null-terminate the argv.
      argv[nargs - 1] = NULL; 

      //If that frequency applies now, execute the cron job:
      int ifreq = atoi(freq);

      if (ifreq < 1) {
        fprintf(DEBUG, "Invalid frequency %d: %s, %d\n", ifreq, file, line_n);
        continue;
      }
      
      if (!(((int)now / 60) % atoi(freq))) {
        int pid = fork();
        if (pid < 0) {
          //An error has occurred:
          fprintf(DEBUG, "Failed to fork process: %s, %d\n", file, line_n);
        }
        else if (pid == 0) {
          //If we are the child, execute this process:
          execvp(argv[0], argv);
          fprintf(DEBUG, "Failed to execute cron job %s: %s, %d\n", argv[0], file, line_n);
          
          //Free all this memory:
          for (int i = 0; i < nargs; ++i) free(argv[i]);
        }
        else {
          //If we are the parent, wait for the child to finish:
          int status;
          waitpid(pid, &status, 0);

          //Free all this memory:
          for (int i = 0; i < nargs; ++i) free(argv[i]);
        }
      }
    }
  }
  return 0;
}

int main(int n, char* args[]) {
  DEBUG = fopen("/home/anthony/cron.debug", "w");
  return run_cron("/home/anthony/manifest.txt");
}
