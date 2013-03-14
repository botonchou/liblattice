#ifndef _UTILITY_H_
#define _UTILITY_H_
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
using namespace std;

string int2str(int n);
int str2int(const string& str);
float str2float(const string& str);
double str2double(const string& str);
string getValueStr(string& str);

vector<string> split(const string &s, char delim);
vector<string>& split(const string &s, char delim, vector<string>& elems);
#endif // _UTILITY_H_
