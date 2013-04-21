#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "server_tools.h"
using namespace std;

const int EXCEPTION_NUM = 6;
const string EXCEPTIONS[] = {".","..","index", "printenv.wsf", "printenv.vbs", "printenv"};

bool name_exception(string d_name) {
  for (int i = 0; i < EXCEPTION_NUM; i += 1) {
    if (d_name == EXCEPTIONS[i]) return true;
  }
  return false;
}


int main(int n, char* args[]) {
  DIR* dir;
  struct dirent* ent;
  printf("%s", "Content-type:text/plain\n\n");
  if ((dir = opendir("/srv/http/cgi-bin")) != NULL) {
    string json = "{\"projects\":[";
    while ((ent = readdir (dir)) != NULL) {
      string d_name = ent->d_name;
      if (!name_exception(d_name)) json += (json_stringify(ent->d_name) + ',');
    }
    json.resize(json.length() - 1);
    json += "]}";
    printf("%s", json.c_str());
  }
}
