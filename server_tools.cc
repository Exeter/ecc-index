#include <sstream>
#include <map>
using namespace std;

string decodeURIComponent (string component) {
  //Decode a URI component into a normal string.
  int len = component.length();
  string r = "";
  int decoded;
  for (int i = 0; i < len; i += 1) {
    if (component[i] == '%') {
      char escape[] = {component[i+1], component[i+2]};
      stringstream d;
      d << hex << escape;
      d >> decoded;
      r += (char) decoded;
      i += 2;
    }
    else r += component[i]; 
  }
  return r;
}

map<string, string> parse_query(string query) {
  //Parse a query into a key/value map.
  int len = query.length();
  map<string, string> r;
  string key = "";
  string current = "";
  for (int i = 0; i < len; i += 1) {
    if (query[i] == '=') {
      key = current;
      current = "";
    }
    else if (query[i] == '&') {
      r[key] = decodeURIComponent(current);
      key = "";
      current = "";
    }
    else current += query[i];
  }
  r[key] = decodeURIComponent(current);
  return r;
}

int parse_path(string path, string*& dest) {
  //Parses a path into an array of directory names; returns the length of the array.
  int len = path.length();
  int depth = 0;
  for (int i = 0; i < len; i += 1) if (path[i] == '/') depth += 1;
  dest = new string[depth];
  string current = "";
  int x = 0;
  for (int i = 1; i < len; i += 1) {
    if (path[i] == '/') {
      dest[x] = current;
      x += 1;
      current = "";
    }
    else current += path[i];
  }
  dest[x] = current;
  return depth;
}

string rejoin_path(int n, string* path) {
  string r = "./";
  for (int i = 0; i < n; i += 1) {
    r += "/" + path[i];
  }
  return r;
}

string json_stringify(string a) {
  int len = a.length();
  stringstream r;
  r << "\"";
  for (int i = 0; i < len; i += 1) {
    switch (a[i]) {
      case '\\': r << "\\\\"; break;
      case '"': r << "\\\""; break;
      case '\n': r << "\\n"; break;
      case '\f': r << "\\f"; break;
      case '\b': r << "\\b"; break;
      case '\r': r << "\\r"; break;
      case '\t': r << "\\t"; break;
      default: r << a[i]; break;
    }
  }
  r << "\"";
  return r.str();
}
