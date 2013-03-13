#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>

#include <cmdParser.h>
#include <array.h>

#include <stdio.h>
#include <sqlite3.h>
#include <lattice.h>
#include <index_builder.h>
using namespace std;

class HypothesisRegion {
public:
  HypothesisRegion(int argc, char** argv): u_id(0), arc_id(0), prev_arc_id(0), likelihood(0) {
    u_id = atoi(argv[0]);
    arc_id = atoi(argv[1]);
    prev_arc_id = atoi(argv[2]);
    likelihood = atof(argv[3]);
  }

  friend ostream& operator << (ostream& os, const HypothesisRegion& hr);

  int u_id;
  int arc_id;
  int prev_arc_id;
  double likelihood;
};

ostream& operator << (ostream& os, const HypothesisRegion& hr) {
  return os << hr.u_id << "\t" << hr.arc_id << "\t" << hr.prev_arc_id << "\t" << hr.likelihood;
}

// Utilities function 
vector<string> split(const string &s, char delim);
vector<string>& split(const string &s, char delim, vector<string>& elems);

// Functionos goes here
void retrieve(Corpus& corpus, string query);
void retrieve(Corpus& corpus, string query, string tempTableName);
void innerJoin(Corpus& corpus, string table1, string table2);

int main(int argc, char* argv[]) {

  CmdParser cmdParser(argc, argv);
  cmdParser.regOpt("-c", "corpus database");
  cmdParser.regOpt("-n", "# of queries");
  cmdParser.regOpt("-q", "query / filename of a querylist");
  if(!cmdParser.isOptionLegal())
    cmdParser.showUsageAndExit();

  Corpus corpus(cmdParser.find("-c"));

  int nQuery = str2int(cmdParser.find("-n"));
  if(nQuery > 1) {
    Array<string> queries(cmdParser.find("-q"));

    for(int i=0; i<queries.size(); ++i)
      retrieve(corpus, queries[i]); 
  }
  else
    retrieve(corpus, cmdParser.find("-q"));
}

void retrieve(Corpus& corpus, string query) {
  vector<string> queries = split(query, '_');

  for(int i=0; i<queries.size(); ++i)
    retrieve(corpus, queries[i], "tt_" + queries[i]);

  innerJoin(corpus, "tt_" + queries[0], "tt_" + queries[1]);
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

void innerJoin(Corpus& corpus, string table1, string table2) {
  char sql[512];
  sprintf(sql, "SELECT t1.u_id, t1.prev_arc_id, t2.arc_id, t1.likelihood + t2.likelihood AS likelihood FROM %s as t1 INNER JOIN %s as t2 ON t1.u_id = t2.u_id WHERE t1.arc_id = t2.prev_arc_id;", table1.c_str(), table2.c_str());
  
  List<HypothesisRegion> hrs;
  corpus.getDatabase().exec(sql, &hrs);

  for(int i=0; i<hrs.size(); ++i)
    cout << hrs[i] << endl;
}

void retrieve(Corpus& corpus, string singleWord, string tempTableName) {
  char sql[512];
  sprintf(sql, "CREATE TEMP TABLE %s as SELECT i.u_id, i.arc_id, i.prev_arc_id, i.likelihood FROM inverted_index i NATURAL JOIN vocabulary v WHERE v.word='%s' ORDER BY i.u_id;", tempTableName.c_str(), singleWord.c_str());
  corpus.getDatabase().exec(sql);
}
