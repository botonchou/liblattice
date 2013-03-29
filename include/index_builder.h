#ifndef _INDEX_BUILDER_H_
#define _INDEX_BUILDER_H_

#include <iostream>
#include <sqlite_wrapper.h>
#include <lattice.h>

#include <common/common-config.h>
#include <common/common-io.h>
#include <htk/htk-lattice.h>

class Vocabulary {
public:
  Vocabulary();
  void add(vector<string> words);
  void add(string word);
  void remove(string word);
  void reindex();
  size_t size() const;
  int getIndex(string word);

  friend class Corpus;
private:
  map<string, int> _words;
};

class Corpus {
public:
  Corpus(string dbFilename);
  void establishVocabulary();
  void createUtteranceTableIfNotExist();
  void updateVocabulary(Vocabulary& vocabulary);
  void add(vulcan::HtkLattice* htkLattice, Vocabulary& vocabulary);
  
  void createLatticeTable(string tableName, string uid);

  static string getValidTableName(string str);
  static string getValidUtteranceId(string str);

  SQLiteDatabase& getDatabase();
  int size() const;
  void list() const;

private:
  SQLiteDatabase _db;
};

class InvertedIndex {
public:
  void buildFrom(Corpus& corpus);
};



#endif // _INDEX_BUILDER_H_
