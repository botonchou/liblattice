#include <index_builder.h>

Vocabulary::Vocabulary() {
}

void Vocabulary::add(vector<string> words) {
  for(size_t i=0; i<words.size(); ++i)
    _words[words[i]] = 1;
}

void Vocabulary::add(string word) {
  _words[word] = 1;
}

void Vocabulary::remove(string word) {
  _words.erase(word);
}

void Vocabulary::reindex() {
  int counter = 0;
  map<string, int>::iterator itr = _words.begin();
  for(; itr != _words.end(); ++itr)
    itr->second = ++counter;
}

int Vocabulary::getIndex(string word) {
  return _words[word];
}

size_t Vocabulary::size() const {
  return _words.size();
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
  map<string, int>::iterator itr = vocabulary._words.begin();
  char sql[512];
  for(; itr != vocabulary._words.end(); ++itr) {
    sprintf(sql, "INSERT INTO vocabulary (word) VALUES ('%s');", itr->first.c_str());
    _db.exec(sql);
  }
  _db.endTransaction();
}

int getPreviousArcIndex(vulcan::HtkLattice* htkLattice, size_t idx) {
  for(size_t i=0; i<idx; ++i)
    if(htkLattice->_arc[i]->_endNode->_index == htkLattice->_arc[idx]->_startNode->_index)
      return i;
  return 0;
}

void Corpus::add(vulcan::HtkLattice* htkLattice, Vocabulary& vocabulary) {
  static const int BUFFER_SIZE = 512;

  //HTKLattice* htkLattice = dynamic_cast<HTKLattice*>(lattice);
  createUtteranceTableIfNotExist();

  // Build table name
  string u = htkLattice->_utterance;
  createLatticeTable(getValidTableName(u), getValidUtteranceId(u));

  static const char* INSERT_SQL_TEMPLATE="INSERT INTO %s (arc_id, prev_arc_id, word_id, likelihood, begin_time, end_time) VALUES (@aid, @paid, @wid, @l, @t1, @t2);";

  sqlite3_stmt* stmt;
  const char* tail = NULL;

  char sSQL[BUFFER_SIZE] = "\0";
  sprintf(sSQL, INSERT_SQL_TEMPLATE, getValidTableName(u).c_str());
  sqlite3_prepare_v2(_db.getDatabase(), sSQL, BUFFER_SIZE, &stmt, &tail);

  vulcan::HtkLatticeForwardBackwardOperator* htkFB = new vulcan::HtkLatticeForwardBackwardOperator;
  htkFB->DoForwardBackward(htkLattice);
  
  map<vulcan::HtkArc*, float>::iterator it;
  for(it = htkFB->_arcPosterior.begin(); it != htkFB->_arcPosterior.end(); it++) {
    vulcan::HtkArc* htkArc = it->first;
    float posterior = it->second;
    int i = htkArc->_index;
    sqlite3_bind_int(stmt   , 1, i);
    sqlite3_bind_int(stmt   , 2, getPreviousArcIndex(htkLattice, i));
    sqlite3_bind_int(stmt   , 3, vocabulary.getIndex(htkArc->_endNode->_word));
    sqlite3_bind_double(stmt, 4, posterior);
    sqlite3_bind_int(stmt   , 5, (int) (htkArc->_startNode->_time*1000));
    sqlite3_bind_int(stmt   , 6, (int) (htkArc->_endNode->_time*1000));

    //fprintf(f, "htkArc->_endNode->_word = [ %s ]\n", htkArc->_endNode->_word.c_str());
    //fprintf(f, "posterior = [ %#.6e ]\n", posterior);

    sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
  }
  //_db.exec("PRAGMA foreign_keys = ON;");
  /*
  for(size_t i=0; i<htkLattice->getArcs().size(); ++i) {
    const HTKLattice::Arc& arc = htkLattice->getArc(i);
    const HTKLattice::Node& node = htkLattice->getNode(arc.getEndNode());

    sqlite3_bind_int(stmt   , 1, i);
    sqlite3_bind_int(stmt   , 2, htkLattice->getPreviousArcIndex(i));
    sqlite3_bind_int(stmt   , 3, vocabulary.getIndex(node.getWord()));
    sqlite3_bind_double(stmt, 4, htkLattice->computeLikelihood(arc));
    sqlite3_bind_int(stmt   , 5, (int) (htkLattice->getNodes()[arc.getStartNode()].getTime()*1000));
    sqlite3_bind_int(stmt   , 6, (int) (htkLattice->getNodes()[arc.getEndNode()].getTime()*1000));

    sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
  }
  */

}

string Corpus::getValidTableName(string str) {
  return "u_" + Corpus::getValidUtteranceId(str) + "_word_lattice";
}

string Corpus::getValidUtteranceId(string str) {
  string id = str.substr(str.find_last_of("/") + 1);
  id = id.substr(0, id.find_last_of("."));

  for(size_t i=0; i<id.size(); ++i)
    if(id[i] == '-' || id[i] == '.')
      id[i] = '_';
  return id;
}

void Corpus::createLatticeTable(string tableName, string uid) {
  //const char* CREATE_LATTICE_TABLE_SQL = "CREATE TABLE %s (arc_id INTEGER, prev_arc_id INTEGER NOT NULL, word_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL);";
  const char* CREATE_LATTICE_TABLE_SQL = "CREATE TABLE %s (arc_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, prev_arc_id INTEGER NOT NULL, word_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT %s_prev_arc_id_fk FOREIGN KEY (prev_arc_id) REFERENCES %s (arc_id));";

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
  for(size_t i=0; i<list.size(); ++i) {
    string doc_id = list[i];

    sprintf(sql, "INSERT INTO inverted_index (word_id, u_id, arc_id, prev_arc_id, likelihood, begin_time, end_time) SELECT word_id, %d, arc_id, prev_arc_id, likelihood, begin_time, end_time FROM u_%s_word_lattice AS u ORDER BY u.word_id;", (int) i + 1, doc_id.c_str());
    db.exec(sql);

  }
  db.endTransaction();
}
