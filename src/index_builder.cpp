#include <index_builder.h>


Vocabulary::Vocabulary() {
}

void Vocabulary::add(vector<string> words) {
  for(int i=0; i<words.size(); ++i)
    _words[words[i]] = true;
}

void Vocabulary::add(string word) {
  _words[word] = true;
}

void Vocabulary::remove(string word) {
  _words.erase(word);
}

size_t Vocabulary::size() const {
  return _words.size();
}

map<string, bool>& Vocabulary::getWords() {
  return _words;
}

// ******************
// ***** Corpus *****
// ******************

Corpus::Corpus(string dbFilename): _db(dbFilename) {
}

void Corpus::establishVocabulary() {
  _db.dropTable("vocabulary");
  _db.exec("CREATE TABLE vocabulary (word_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, word TEXT NOT NULL);");
}

void Corpus::createUtteranceTableIfNotExist() {
  _db.exec("CREATE TABLE IF NOT EXISTS utterances (u_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, doc_id int NOT NULL, has_word_lattice bool NOT NULL);");
}

void Corpus::updateVocabulary(Vocabulary& vocabulary) {

  _db.beginTransaction();
  map<string, bool>::iterator itr = vocabulary.getWords().begin();
  char sql[512];
  for(; itr != vocabulary.getWords().end(); ++itr) {
    //cout << itr->first.c_str() << endl;
    sprintf(sql, "INSERT INTO vocabulary (word) VALUES ('%s');", itr->first.c_str());
    _db.exec(sql);
  }
  _db.endTransaction();
}

void Corpus::add(Lattice* lattice) {

  HTKLattice* htkLattice = dynamic_cast<HTKLattice*>(lattice);
  createUtteranceTableIfNotExist();

  // Build table name
  string u = htkLattice->getHeader().utterance;
  createLatticeTable(getValidTableName(u), getValidUtteranceId(u));

  _db.exec(createInsertSQL(htkLattice, 0, getValidTableName(u)));

  //_db.exec("PRAGMA foreign_keys = ON;");
  //_db.beginTransaction();
  for(int i=1; i<htkLattice->getArcs().size(); ++i)
    _db.exec(createInsertSQL(htkLattice, i, getValidTableName(u)));
  //_db.endTransaction();

  char sql[512];
  sprintf(sql, "UPDATE %s SET word_id=(SELECT v.word_id FROM vocabulary AS v WHERE %s.word_id == v.word);", getValidTableName(u).c_str(), getValidTableName(u).c_str());
  _db.exec(sql);
}

string Corpus::createInsertSQL(HTKLattice* htkLattice, size_t arcIdx, string tableName) const {

  static const char* INSERT_SQL_TEMPLATE = "INSERT INTO %s (arc_id, prev_arc_id, word_id, likelihood, begin_time, end_time) VALUES (%d, %d, '%s', %f, %d, %d);";

  const HTKLattice::Arc& arc = htkLattice->getArc(arcIdx);
  const HTKLattice::Node& node = htkLattice->getNode(arc.getEndNode());

  char sql[512];
  sprintf( sql, INSERT_SQL_TEMPLATE, tableName.c_str(), arcIdx, htkLattice->getPreviousArcIndex(arcIdx), node.getWord().c_str(), htkLattice->computeLikelihood(arc), (int) (htkLattice->getNodes()[arc.getStartNode()].getTime()*1000), (int) (htkLattice->getNodes()[arc.getEndNode()].getTime()*1000));
  //sprintf( sql, INSERT_SQL_TEMPLATE, tableName.c_str(), arcIdx, 0, "A", 0.0, 0, 0);
  return sql;
}

string Corpus::getValidTableName(string str) {
  return "u_" + Corpus::getValidUtteranceId(str) + "_word_lattice";
}

string Corpus::getValidUtteranceId(string str) {
  string id = str.substr(str.find_last_of("/") + 1);
  id = id.substr(0, id.find_last_of("."));

  for(int i=0; i<id.size(); ++i)
    if(id[i] == '-' || id[i] == '.')
      id[i] = '_';
  return id;
}

void Corpus::createLatticeTable(string tableName, string uid) {
  const char* CREATE_LATTICE_TABLE_SQL = "CREATE TABLE %s (arc_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, prev_arc_id INTEGER NOT NULL, word_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT %s_prev_arc_id_fk FOREIGN KEY (prev_arc_id) REFERENCES %s (arc_id));";
  // , CONSTRAINT vocabulary_word_id_fk FOREIGN KEY (word_id) REFERENCES vocabulary (word_id)

  char sql[512];
  _db.dropTable(tableName);

  sprintf(sql, CREATE_LATTICE_TABLE_SQL, tableName.c_str(), tableName.c_str(), tableName.c_str());
  _db.exec(sql);

  sprintf( sql, "SELECT u_id FROM utterances WHERE doc_id = '%s' LIMIT 1;", uid.c_str() );
  List<string> list;
  _db.get(sql, &list);

  if(list.size() == 0) {
    sprintf( sql, "INSERT INTO utterances (doc_id, has_word_lattice) VALUES ('%s', 1);", uid.c_str() ); 
    _db.exec(sql);
  }
}

SQLiteDatabase& Corpus::getDatabase() {
  return _db;
}

// *************************
// ***** InvertedIndex *****
// *************************
void InvertedIndex::buildFrom(Corpus& corpus) {
  const SQLiteDatabase& db = corpus.getDatabase();
  db.dropTable("inverted_index");
  db.exec("CREATE TABLE inverted_index (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, word_id INTEGER NOT NULL, u_id INTEGER NOT NULL, arc_id INTEGER NOT NULL, prev_arc_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT vocabulary_word_id_fk FOREIGN KEY (word_id) REFERENCES vocabulary(word_id));"); //, CONSTRAINT utterances_id_fk FOREIGN KEY (u_id) REFERENCES utterances(u_id));");

  char sql[512];
  sprintf(sql, "SELECT doc_id FROM utterances WHERE has_word_lattice == 1;" );
  List<string> list;
  db.get(sql, &list);

  db.beginTransaction();
  for(int i=0; i<list.size(); ++i) {
    string doc_id = list[i];

    sprintf(sql, "INSERT INTO inverted_index (word_id, u_id, arc_id, prev_arc_id, likelihood, begin_time, end_time) SELECT word_id, %d, arc_id, prev_arc_id, likelihood, begin_time, end_time FROM u_%s_word_lattice AS u ORDER BY u.word_id;", i + 1, doc_id.c_str());
    db.exec(sql);

  }
  db.endTransaction();
}
