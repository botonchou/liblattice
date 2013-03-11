#ifndef _INDEX_BUILDER_H_
#define _INDEX_BUILDER_H_

#include <iostream>
#include <sqlite_wrapper.h>
#include <lattice.h>

class IndexBuilder {
public:

  void build(const SQLiteDatabase& src) {
    if(src.hasTable("inverted_index"))
      src.exec("DROP TABLE inverted_index;");

    src.exec("CREATE TABLE inverted_index (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, word_id INTEGER NOT NULL, u_id INTEGER NOT NULL, arc_id INTEGER NOT NULL, prev_arc_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT vocabulary_word_id_fk FOREIGN KEY (word_id) REFERENCES vocabulary(word_id), CONSTRAINT utterances_id_fk FOREIGN KEY (u_id) REFERENCES utterances(u_id));");

    char sql[512];
    sprintf(sql, "SELECT doc_id FROM utterances WHERE has_word_lattice == 1;" );
    SQLiteTable table = src.get(sql);

    src.exec("BEGIN TRANSACTION;");
    for(int i=1; i<table.getRows(); ++i) {
      string doc_id = table.get(i);
      cout << doc_id << endl;

      sprintf(sql, "INSERT INTO inverted_index (word_id, u_id, arc_id, prev_arc_id, likelihood, begin_time, end_time) SELECT word_id, %d, arc_id, prev_arc_id, likelihood, begin_time, end_time FROM u_%s_word_lattice AS u ORDER BY u.word_id;", i, doc_id.c_str());
      src.exec(sql);

    }
    src.exec("END TRANSACTION;");
  }
};

class Corpus {
public:
  Corpus(string dbFilename);
  void establishVocabulary() ;
  void createUtteranceTableIfNotExist() ;
  void updateVocabulary(map<string, bool>& vocabulary) ;
  void add(Lattice* lattice) ;
  
  void createInsertSQL(char* sql, const int& i, HTKLattice* htkLattice, string u) const ;
  void addNewLatticeToDatabase(string tableName, string uid);

  string getValidTableName(string str) const;
  string getValidUtteranceId(string str) const;

  SQLiteDatabase& getDatabase();
  int size() const;
  void list() const;

private:
  SQLiteDatabase _db;
};

class InvertedIndex {
public:
  void buildFrom(Corpus& corpus) {
    IndexBuilder indexBuilder;
    indexBuilder.build(corpus.getDatabase());
  }
};



#endif // _INDEX_BUILDER_H_
