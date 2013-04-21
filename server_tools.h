#ifndef SERVER_TOOLS
#define SERVER_TOOLS
#include <sstream>
#include <map>
using namespace std;

string decodeURIComponent (string);
map<string, string> parse_query(string);
int parse_path (string, string*&);
string rejoin_path(int, string*);
string json_stringify(string);

#endif
