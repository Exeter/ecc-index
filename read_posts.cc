#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include "server_tools.h"
using namespace std;

const int MAX_POSTS = 10;

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
  sqlite3_stmt* stmt;
  const char* pzTail;
  if (args.count("last") > 0) {
    char* backwash;
    sqlite3_int64 date = (sqlite3_int64) strtoll(args["last"].c_str(), &backwash, 10);
    sqlite3_prepare_v2(db, "SELECT * FROM news WHERE timestamp < ? ORDER BY timestamp DESC", -1, &stmt, &pzTail);
    sqlite3_bind_int64(stmt, 1, date);
  }
  else {
    sqlite3_prepare_v2(db, "SELECT * FROM news ORDER BY timestamp DESC", -1, &stmt, &pzTail);
  }
  stringstream out;
  printf("%s","{\"posts\":[");
  int64_t date;
  int step = sqlite3_step(stmt);
  for (int i = 0; i < MAX_POSTS; i += 1) {
    printf("{\"timestamp\":%d, \"title\":%s, \"body\":%s}", sqlite3_column_int(stmt, 1), json_stringify((const char*) sqlite3_column_text(stmt, 2)).c_str(), json_stringify((const char*) sqlite3_column_text(stmt, 3)).c_str());
    date = (int64_t) sqlite3_column_int64(stmt, 1);
    step = sqlite3_step(stmt);
    if (step == SQLITE_ROW && i < MAX_POSTS - 1) putc(',', stdout);
    else break;
  }
  printf("],\"last\":%d}", date);
  printf("%s",out.str().c_str());
  sqlite3_finalize(stmt);
  sqlite3_close(db);
}
