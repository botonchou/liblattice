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

// Functionos goes here
void retrieve(Corpus& corpus, string query);
TempList retrieveSingleWord(Corpus& corpus, string query);
void innerJoin(Corpus& corpus, string table1, string table2);

int main(int argc, char* argv[]) {

  List<string> tList;

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

    for(size_t i=0; i<queries.size(); ++i)
      retrieve(corpus, queries[i]); 
  }
  else
    retrieve(corpus, cmdParser.find("-q"));
}

void retrieve(Corpus& corpus, string query) {
  vector<string> queries = split(query, '_');
  vector<TempList> result(queries.size());

  for(size_t i=0; i<queries.size(); ++i) {
    result[i] = retrieveSingleWord(corpus, queries[i]);
    result[i].print();
  }

  if(result.size() > 1)
    innerJoin(corpus, result[0].getTempTableName(), result[1].getTempTableName());
}

void innerJoin(Corpus& corpus, string table1, string table2) {
  char sql[512];
  sprintf(sql, "SELECT t1.u_id, t1.prev_arc_id, t2.arc_id, t1.likelihood + t2.likelihood AS likelihood FROM %s as t1 INNER JOIN %s as t2 ON t1.u_id = t2.u_id WHERE t1.arc_id = t2.prev_arc_id;", table1.c_str(), table2.c_str());
  
  List<HypothesisRegion> hrs;
  corpus.getDatabase().get(sql, &hrs);

  for(size_t i=0; i<hrs.size(); ++i)
    cout << hrs[i] << endl;
}

TempList retrieveSingleWord(Corpus& corpus, string singleWord) {
  char sql[512];
  TempList tempList;
  sprintf(sql, "SELECT i.u_id, i.arc_id, i.prev_arc_id, i.likelihood FROM inverted_index i NATURAL JOIN vocabulary v WHERE v.word='%s' ORDER BY i.u_id;", singleWord.c_str());
  corpus.getDatabase().get(sql, &tempList);
  return tempList;
}
