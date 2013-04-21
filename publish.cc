#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sqlite3.h>
#include <iterator>
#include <map>
#include <string.h>
#include <sstream>
#include <fstream>
#include <hashlib++/hashlibpp.h>
#include "server_tools.h"
using namespace std;


int main(int n, char* argv[]) {
  //Setup for all requests:
  printf("%s","Content-type:text/plain\n\n");
  sqlite3* db;
  sqlite3_open("news.db", &db);
  string* path;
  int path_length;
  if (getenv("PATH_INFO") != NULL) {
    path_length = parse_path(getenv("PATH_INFO"), path);
  }
  else {
    path = new string[0];
    path_length = 0;
  }
  map<string, string> args = parse_query(getenv("QUERY_STRING"));
  if (strcmp(getenv("REQUEST_METHOD"), "POST") == 0) {
    //Slurp the input to satisfy Apache:
    char c;
    string results = "";
    while (cin.get(c)) {
      results += c;
    }
    fclose(stdin);
    //Then make sure we are authorized:
    if (args.count("hash") > 0) {
      hashwrapper* md5wrap = new md5wrapper();
      stringstream hash;
      stringstream salt;
      salt << ((time(NULL) / 1000) * 1000);
      hash << md5wrap->getHashFromString("11c0485572f3bb482a462b6eb54ba81e" + salt.str());
      if (hash.str() != args["hash"]) {
        printf("{\"error\":\"Authorization failed.\", \"salt\":\"%s\", \"exactTime\":%d}", salt.str().c_str(), time(NULL));
        sqlite3_close(db);
        return 0;
      }
    }
    else {
      printf("%s", "{\"error\":\"No authorization data.\"}");
      return 0;
    }
    sqlite3_stmt* stmt;
    const char* pzTail;
    sqlite3_prepare_v2(db, "INSERT INTO news (timestamp, title, body) VALUES ((JULIANDAY('now')-2440587.5)*86400.0, ?, ?)", -1, &stmt, &pzTail);
    sqlite3_bind_text(stmt, 1, args["title"].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, results.c_str(), -1, SQLITE_TRANSIENT);
    int result = sqlite3_step(stmt);
    if (result == SQLITE_DONE) {
      printf("%s", "{\"success\":\"true\"}");
    }
    else {
      printf("{\"success\":false, \"sql\":%d}", result);
    }
    sqlite3_finalize(stmt);
  }
  else {
    printf("%s", "{\"error\":\"Unrecognizable request method.\"}");
  }
  sqlite3_close(db);
}
