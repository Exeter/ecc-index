#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fstream>
#include <errno.h>
#include <algorithm>
using namespace std;

fstream DEBUG;

string findmime(string ext) {
  FILE* mimetypes;
  mimetypes = fopen("mime.types", "r");
  if (mimetypes != NULL) {
    int nbytes = 100;
    char* line = new char[100];

    //Read out lines from the mime file:
    while (getline(&line, (size_t*) &nbytes, mimetypes) > 0) {
      if (line[0] != '#') {
        int i = 0;

        //Extract the mimetype for this line:
        string mimetype;
        while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
          mimetype += line[i];
          ++i;
        }
        
        //Skip the following whitespace:
        while (line[i] == ' ' || line[i] == '\t') ++i;

        //TODO exceptions for format .ice?
        while (line[i] != '\n') {
          //Now until we reach the end of the line, we check 
          int s = 0;
          while (ext[s] == line[i]) {
            ++s;
            ++i;
          }
          if (s == ext.length() && (line[i] == '\n' || line[i] == ' ')) {
            //We have now found the proper mimetype.
            return mimetype;
          }
          else {
            //If we have not found the proper mimetype, we discard the rest of this extension.
            while (line[i] != ' ' && line[i] != '\n') {
              ++i;
            }
            if (line[i] == ' ') ++i;
          }
        }
      }
    }
    return "text/plain";
  }
}

bool matches(string*& backref, char* path, string match) {
  backref = new string[count(match.begin(), match.end(), '?')];
  int len = match.length();
  int n = 0;
  for (int i = 0, x = 0; true; ++i && ++x) {
    //TODO add support for "*".
    if ((i == len) != (path[x] == '\0')) {
      return false;
    }
    else if (i == len && path[x] == '\0') {
      return true;
    }
    else if (match[i] == '?') {
      string ref = "";
      while (path[x] != '/' && path[x] != '\0') {
        ref += path[x];
        ++x;
      }
      --x;
      backref[n] = ref;
      n += 1;
    }
    else if (match[i] != path[x]) return false;
  }
}

char* format(string* backref, string path) {
  int len = path.length();
  string r = "";
  int n;
  for (int i = 0; i < len; i += 1) {
    //TODO add support for more than 10 backrefs.
    if (path[i] == '$') {
      r += backref[int(path[i + 1] - '0') - 1];
      i += 1;
    } 
    else {
      r += path[i];
    }
  }
  return (char*) r.c_str();
}

int run_dynamic(char* file) {
  //Declare pipes:
  int stdin_pipe[2];
  int stdout_pipe[2];

  //Make pipes:
  pipe2(stdin_pipe, O_NONBLOCK);
  pipe2(stdout_pipe, O_NONBLOCK);

  //Fork ourselves:
  int pid = fork();
  if (pid < 0) {
    //This means we have an error:
    return 1;
  }
  if (pid == 0) {
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
      return 1;
    }
    if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
      return 1;
    }

    char* nargs[] = {file, NULL};
    execvp(nargs[0], nargs);

    //We should be done now:
    return 1;
  }
  else {
    //Otherwise, we are the parent:
    int status;
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    //Get the entire standard input:
    char c;
    string all_cin = "";
    while ((c = getc(stdin)) != EOF) {
      all_cin += c;
    }

    //Write the entire standard input to the child:
    write(stdin_pipe[1], all_cin.c_str(), all_cin.length());

    char r;
    waitpid(pid, &status, 0);
    while (read(stdout_pipe[0], &r, 1) > 0) {
      putc(r, stdout);
    }
  }
}

int run_static(char* fcstr) {
  string file (fcstr);
  //Determine the file extension:
  int i = file.rfind('.');
  string ext = "";
  if (file[i] != '\0') ++i;
  while (file[i] != '\0') {
    ext.push_back(file[i]);
    ++i;
  }
 
  //TODO accomodate for more file extensions, perhaps load from file:
  printf("Content-Type: %s\n\n", findmime(ext).c_str());
  
  FILE* f = fopen(file.c_str(), "r");
  if (f == NULL) {
    printf("Error: could not find file %s", file.c_str());
    return 1;
  }
  char c;
  while ((c = getc(f)) != EOF) {
    putc(c, stdout);
  }
  return 0;
}

int run_manifest(char* file) {
  FILE* f = fopen(file, "r");
  long unsigned int manifest_size_limit = 100;
  char* path = getenv("PATH_INFO");
  char* line = new char[100];
  string strict_file (file);

  while (getline(&line, &manifest_size_limit, f) > 0) {
    //Get the path descriptor:
    int i = 0;
    string match;
    while (line[i] != ' ') {
      match += line[i];
      ++i;
    }
    ++i;

    //If the path descrptor matches, get the filename
    string* backref;
    char* new_file;
    string filename;
    if (matches(backref, path, match)) {
      while (line[i] != ' ') {
        filename += line[i];
        ++i;
      }
      new_file = format(backref, filename);
      DEBUG << "In [" << strict_file << "] " << path << " matched " << match << ". Redirecting you to " << new_file << endl;
    }
    else {
      continue;
    }

    ++i;
    if (line[i] == 'S') {
      run_static(new_file);
    }
    else if (line[i] == 'D') {
      run_dynamic(new_file);
    }
    else if (line[i] == 'M') {
      run_manifest(new_file);
    }
    return 0;
  }
}

int main() {
  DEBUG.open("manifester.debug", fstream::out | fstream::app);
  DEBUG << endl
        << "REQUEST FOR: " << getenv("PATH_INFO") << endl;
  run_manifest("test.txt");
}
