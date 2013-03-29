#ifndef __RETRIEVE_CORE_H_
#define __RETRIEVE_CORE_H_

#include <index_builder.h>

class RetrieveCore {
public:

  static TempList search(const Corpus& corpus, const string& query);

private:
  RetrieveCore();
  static void _cacheCorpus(const Corpus& corpus);

  static TempList retrieve(string query);
  static TempList innerJoin(TempList& table1, TempList& table2);

  static Corpus* _corpus;
};


#endif // __RETRIEVE_CORE_H_
