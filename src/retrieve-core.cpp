#include <retrieve-core.h>

Corpus* RetrieveCore::_corpus = NULL;

RetrieveCore::RetrieveCore() {

}

TempList RetrieveCore::search(const Corpus& corpus, const string& query) {
  RetrieveCore::_cacheCorpus(corpus);

  vector<string> queries = split(query, '_');

  vector<TempList> result(queries.size());
  //const size_t N_GRAM = MIN(queries.size(), 3);

  for(size_t j=0; j<queries.size(); ++j) {
    result[j] = retrieve(queries[j]);
    printf("%s====================%s %s%s%s %s====================%s\n", BLUE, COLOREND, GREEN, queries[j].c_str(), COLOREND, BLUE, COLOREND);
    result[j].print();
  }

  return TempList();
}


// ===================================================
// === Private Member Function -- Hidden from User ===
// ===================================================
void RetrieveCore::_cacheCorpus(const Corpus& corpus) {
  RetrieveCore::_corpus = const_cast<Corpus*>(&corpus);
}

TempList RetrieveCore::retrieve(string query) {
  char sql[512];
  TempList tempList;
  sprintf(sql, "SELECT i.u_id, i.arc_id, i.prev_arc_id, i.likelihood FROM inverted_index i NATURAL JOIN vocabulary v WHERE v.word='%s' ORDER BY i.u_id;", query.c_str());
  RetrieveCore::_corpus->getDatabase().get(sql, &tempList);
  return tempList;
}

TempList RetrieveCore::innerJoin(TempList& table1, TempList& table2) {
  TempList templist;
  char sql[512];
  sprintf(sql, "SELECT t1.u_id, t1.prev_arc_id, t2.arc_id, t1.likelihood + t2.likelihood AS likelihood FROM %s as t1 INNER JOIN %s as t2 ON t1.u_id = t2.u_id WHERE t1.arc_id = t2.prev_arc_id;", table1.getTempTableName().c_str(), table2.getTempTableName().c_str());
  RetrieveCore::_corpus->getDatabase().get(sql, &templist);
  return templist;
}
