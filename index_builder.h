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
  Corpus(string dbFilename): _db(dbFilename) {
  }

  void establishVocabulary() {
    if(_db.hasTable("vocabulary"))
      _db.exec("DROP TABLE vocabulary;");

    _db.exec("CREATE TABLE vocabulary (word_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, word TEXT NOT NULL);");
  }

  void updateVocabulary(map<string, bool>& vocabulary) {

    _db.exec("BEGIN TRANSACTION;");
    map<string, bool>::iterator itr = vocabulary.begin();
    char sql[512];
    for(; itr != vocabulary.end(); ++itr) {
      sprintf(sql, "INSERT INTO vocabulary (word) VALUES ('%s');", itr->first.c_str());
      _db.exec(sql);
    }
    _db.exec("END TRANSACTION;");
  }

  void add(Lattice* lattice) {

    HTKLattice* htkLattice = dynamic_cast<HTKLattice*>(lattice);
    htkLattice->createUtteranceTableIfNotExist(_db);

    // Build table name
    string u = htkLattice->getHeader().utterance;
    addNewLatticeToDatabase(getValidTableName(u), getValidUtteranceId(u));

    char sql[512];
    createInsertSQL(sql, 0, htkLattice, u);
    _db.exec(sql);

    _db.exec("PRAGMA foreign_keys = ON;");
    _db.exec("BEGIN TRANSACTION;");
    for(int i=1; i<htkLattice->getArcs().size(); ++i) {
      createInsertSQL(sql, i, htkLattice, u);
      _db.exec(sql);
    }
    _db.exec("END TRANSACTION;");

    sprintf(sql, "UPDATE %s SET word_id=(SELECT v.word_id FROM vocabulary AS v WHERE %s.word_id == v.word);", getValidTableName(u).c_str(), getValidTableName(u).c_str());
    _db.exec(sql);
  }

  void createInsertSQL(char* sql, const int& i, HTKLattice* htkLattice, string u) const {

    static const char* INSERT_SQL_TEMPLATE = "INSERT INTO %s (arc_id, prev_arc_id, word_id, likelihood, begin_time, end_time) VALUES (%d, %d, '%s', %f, %d, %d);";

    const HTKLattice::Arc& arc = htkLattice->getArc(i);
    const HTKLattice::Node& node = htkLattice->getNode(arc.getEndNode());
    sprintf( sql, 
	INSERT_SQL_TEMPLATE, 
	getValidTableName(u).c_str(), 
	i, 
	htkLattice->getPreviousArcIndex(i), 
	node.getWord().c_str(), 
	htkLattice->computeLikelihood(arc),
	(int) (htkLattice->getNodes()[arc.getStartNode()].getTime()*1000),
	(int) (htkLattice->getNodes()[arc.getEndNode()].getTime()*1000) 
	);
  }

  string getValidTableName(string str) const {
    return "u_" + this->getValidUtteranceId(str) + "_word_lattice";
  }

  string getValidUtteranceId(string str) const {
    string id = str.substr(str.find_last_of("/") + 1);
    id = id.substr(0, id.find_last_of("."));

    for(int i=0; i<id.size(); ++i)
      if(id[i] == '-' || id[i] == '.')
	id[i] = '_';
    return id;
  }

  void addNewLatticeToDatabase(string tableName, string uid) {

    char sql[512];

    const char* CREATE_LATTICE_TABLE_SQL = "CREATE TABLE %s (arc_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, prev_arc_id INTEGER NOT NULL, word_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT %s_prev_arc_id_fk FOREIGN KEY (prev_arc_id) REFERENCES %s (arc_id));";
    // , CONSTRAINT vocabulary_word_id_fk FOREIGN KEY (word_id) REFERENCES vocabulary (word_id)

    if(_db.hasTable(tableName)) {
      sprintf(sql, "DROP TABLE %s;", tableName.c_str());
      _db.exec(sql);
    }

    sprintf(sql, CREATE_LATTICE_TABLE_SQL, tableName.c_str(), tableName.c_str(), tableName.c_str());
    _db.exec(sql);

    sprintf( sql, "SELECT u_id FROM utterances WHERE doc_id = '%s';", tableName.c_str() );
    SQLiteTable result = _db.get(sql);

    if(result.getRows() <= 1)
      sprintf( sql, "INSERT INTO utterances (doc_id, has_word_lattice) VALUES ('%s', 1);", uid.c_str() ); 
    _db.exec(sql);

  }

  SQLiteDatabase& getDatabase() {
    return _db;
  }
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
