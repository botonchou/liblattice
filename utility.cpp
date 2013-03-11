#include <utility.h>

double str2double(string str) {
  double d;
  stringstream ss;
  ss << str;
  ss >> d;
  return d;
}

float str2float(string str) {
  float f;
  stringstream ss;
  ss << str;
  ss >> f;
  return f;
}

int str2int(string str) {
  return atoi(str.c_str());
}

