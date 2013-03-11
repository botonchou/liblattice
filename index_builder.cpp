#include <index_builder.h>

Corpus::Corpus(string dbFilename): _db(dbFilename) {

}

void Corpus::establishVocabulary() {
  if(_db.hasTable("vocabulary"))
    _db.exec("DROP TABLE vocabulary;");

  _db.exec("CREATE TABLE vocabulary (word_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, word TEXT NOT NULL);");
}

void Corpus::createUtteranceTableIfNotExist() {
  if(!_db.hasTable("utterances"))
    _db.exec("CREATE TABLE utterances (u_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, doc_id int NOT NULL, has_word_lattice bool NOT NULL);");
}

void Corpus::updateVocabulary(map<string, bool>& vocabulary) {

  _db.exec("BEGIN TRANSACTION;");
  map<string, bool>::iterator itr = vocabulary.begin();
  char sql[512];
  for(; itr != vocabulary.end(); ++itr) {
    sprintf(sql, "INSERT INTO vocabulary (word) VALUES ('%s');", itr->first.c_str());
    _db.exec(sql);
  }
  _db.exec("END TRANSACTION;");
}

void Corpus::add(Lattice* lattice) {

  HTKLattice* htkLattice = dynamic_cast<HTKLattice*>(lattice);
  createUtteranceTableIfNotExist();

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

void Corpus::createInsertSQL(char* sql, const int& i, HTKLattice* htkLattice, string u) const {

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

string Corpus::getValidTableName(string str) const {
  return "u_" + this->getValidUtteranceId(str) + "_word_lattice";
}

string Corpus::getValidUtteranceId(string str) const {
  string id = str.substr(str.find_last_of("/") + 1);
  id = id.substr(0, id.find_last_of("."));

  for(int i=0; i<id.size(); ++i)
    if(id[i] == '-' || id[i] == '.')
      id[i] = '_';
  return id;
}

void Corpus::addNewLatticeToDatabase(string tableName, string uid) {

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

SQLiteDatabase& Corpus::getDatabase() {
  return _db;
}
