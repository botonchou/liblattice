#include <utility.h>

string int2str(int n) {
  char buf[32];
  sprintf(buf, "%d", n);
  return buf;
}

double str2double(const string& str) {
  return atof(str.c_str());
}

float str2float(const string& str) {
  return atof(str.c_str());
}

int str2int(const string& str) {
  return atoi(str.c_str());
}

vector<string>& split(const string &s, char delim, vector<string>& elems) {
  stringstream ss(s);
  string item;
  while(getline(ss, item, delim))
    elems.push_back(item);
  return elems;
}

vector<string> split(const string &s, char delim) {
  vector<string> elems;
  return split(s, delim, elems);
}
