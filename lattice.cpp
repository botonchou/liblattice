#include <lattice.h>

void Lattice::print() const {
}

bool Lattice::saveToVocabulary(map<string, bool>& vocabulary) const {
}

bool Lattice::saveToDatabase(SQLiteDatabase& db) const {
}
// ******************************
// ***** HTK Lattice Parser *****
// ******************************
string HTKLatticeParser::getNext(iostream& file) {
  string str;
  file >> str;
  return str.substr(str.find('=') + 1);
}

HTKLattice::Node HTKLatticeParser::createNode(fstream& file) {
  getNext(file);
  HTKLattice::Node node;

  node._time = str2float(getNext(file));
  node._word = getNext(file);
  node._variation = str2int(getNext(file));
  return node;
}

HTKLattice::Arc HTKLatticeParser::createArc(fstream& file) {
  // Handle different version of HTK lattice format
  string dumb;
  while(file >> dumb && dumb[0] != 'J');

  HTKLattice::Arc arc;
  arc._startNode = str2int(getNext(file));
  arc._endNode = str2int(getNext(file));
  arc._acScore = str2float(getNext(file));
  arc._lmScore = str2float(getNext(file));

  return arc;
}

Lattice* HTKLatticeParser::createLattice(string filename) {
  fstream file(filename.c_str(), ios::in);
  
  Lattice* lattice = new HTKLattice;
  HTKLattice::Header& header = dynamic_cast<HTKLattice*>(lattice)->_header;
  header.version = str2float(getNext(file));
  header.utterance  = getNext(file);
  header.lmname	    = getNext(file);
  header.lmscale    = str2float(getNext(file));
  header.wdpenalty  = str2float(getNext(file));
  //getNext(file);    // get prScale
  header.acscale    = str2float(getNext(file));
  header.vocab	    = getNext(file);
  header.hmms	    = getNext(file);
  header.nNodes	    = str2int(getNext(file));
  header.nArcs	    = str2int(getNext(file));

  vector<HTKLattice::Node>& nodes = dynamic_cast<HTKLattice*>(lattice)->_nodes;
  nodes.resize(header.nNodes);
  
  getNext(file);
  nodes[0].setTime(str2float(getNext(file)));
  nodes[0].setWord(getNext(file));
  nodes[0].setVariation(0);

  for(int i=1; i<nodes.size(); ++i)
    nodes[i] = this->createNode(file);

  vector<HTKLattice::Arc>& arcs = dynamic_cast<HTKLattice*>(lattice)->_arcs;
  arcs.resize(header.nArcs);
  
  for(int i=0; i<arcs.size(); ++i)
    arcs[i] = this->createArc(file);

  file.close();

  return lattice;
}

void HTKLattice::print() const {
  for(int i=0; i<this->_nodes.size(); ++i)
    cout << "I=" << i << "\t" << this->_nodes[i] << endl;

  for(int i=0; i<this->_arcs.size(); ++i)
    cout << "J=" << i << "\t" << this->_arcs[i] << endl;
}

void HTKLattice::addNewLatticeToDatabase(SQLiteDatabase& db, string tableName) const {
  char sql[512];

  const char* CREATE_LATTICE_TABLE_SQL = "CREATE TABLE %s (arc_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, prev_arc_id INTEGER NOT NULL, word_id INTEGER NOT NULL, likelihood REAL NOT NULL, begin_time INTEGER NOT NULL, end_time INTEGER NOT NULL, CONSTRAINT %s_prev_arc_id_fk FOREIGN KEY (prev_arc_id) REFERENCES %s (arc_id));";
  // , CONSTRAINT vocabulary_word_id_fk FOREIGN KEY (word_id) REFERENCES vocabulary (word_id)

  if(db.hasTable(tableName)) {
    sprintf(sql, "DROP TABLE %s;", tableName.c_str());
    db.exec(sql);
  }

  sprintf(sql, CREATE_LATTICE_TABLE_SQL, tableName.c_str(), tableName.c_str(), tableName.c_str());
  db.exec(sql);

  sprintf( sql, "SELECT u_id FROM utterances WHERE doc_id = '%s';", tableName.c_str() );
  SQLiteTable result = db.get(sql);

  if(result.getRows() <= 1)
    sprintf( sql, "INSERT INTO utterances (doc_id, has_word_lattice) VALUES ('%s', 1);", this->getValidUtteranceId().c_str() ); 
  db.exec(sql);

}

void HTKLattice::createUtteranceTableIfNotExist(SQLiteDatabase& db) const {
  // Check if utterances exists OR NOT
  if(!db.hasTable("utterances"))
    db.exec("CREATE TABLE utterances (u_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, doc_id int NOT NULL, has_word_lattice bool NOT NULL);");
}

string HTKLattice::getTableName() const {
  return "u_" + this->getValidUtteranceId() + "_word_lattice";
}

void HTKLattice::createInsertSQL(char* sql, const int& i) const {
  static const char* INSERT_SQL_TEMPLATE = "INSERT INTO %s (arc_id, prev_arc_id, word_id, likelihood, begin_time, end_time) VALUES (%d, %d, '%s', %f, %d, %d);";

  const Arc& arc = _arcs[i];
  const Node& node = _nodes[arc.getEndNode()];
  sprintf( sql, 
      INSERT_SQL_TEMPLATE, 
      getTableName().c_str(), 
      i, 
      this->getPreviousArcIndex(i), 
      node.getWord().c_str(), 
      this->computeLikelihood(arc),
      (int) (_nodes[arc.getStartNode()].getTime()*1000),
      (int) (_nodes[arc.getEndNode()].getTime()*1000) 
  );
}

bool HTKLattice::saveToDatabase(SQLiteDatabase& db) const {
  this->createUtteranceTableIfNotExist(db);

  // Build table name
  addNewLatticeToDatabase(db, getTableName());

  char sql[512];
  createInsertSQL(sql, 0);
  db.exec(sql);

  db.exec("PRAGMA foreign_keys = ON;");
  db.exec("BEGIN TRANSACTION;");
  for(int i=1; i<_arcs.size(); ++i) {
    createInsertSQL(sql, i);
    db.exec(sql);
  }
  db.exec("END TRANSACTION;");

  sprintf(sql, "UPDATE %s SET word_id=(SELECT v.word_id FROM vocabulary AS v WHERE %s.word_id == v.word);", getTableName().c_str(), getTableName().c_str());
  db.exec(sql);
}

bool HTKLattice::saveToVocabulary(map<string, bool>& vocabulary) const {
  for(int i=0; i<_nodes.size(); ++i)
    vocabulary[_nodes[i].getWord()] = true;
}

size_t HTKLattice::getPreviousArcIndex(size_t idx) const {
  for(int i=0; i<idx; ++i)
    if(_arcs[i].getEndNode() == _arcs[idx].getStartNode())
      return i;
  return 0;
}

string HTKLattice::getValidUtteranceId() const {
  string utteranceId = _header.utterance.substr(_header.utterance.find_last_of("/") + 1);
  utteranceId = utteranceId.substr(0, utteranceId.find_last_of("."));

  for(int i=0; i<utteranceId.size(); ++i)
    if(utteranceId[i] == '-' || utteranceId[i] == '.')
      utteranceId[i] = '_';
  return utteranceId;
}

Likelihood HTKLattice::computeLikelihood(const Arc& arc) const {
  return _header.acscale * arc._acScore + _header.lmscale * arc._lmScore;
}

// ******************************
// ***** TTK Lattice Parser *****
// ******************************
TTKLattice::Arc TTKLatticeParser::createArc(string& line) {
// preWord:(SIL) curWord:(SIL) beginTime: 0 endTime: 3 score: -87.893532 globalScore: -108.077667 cm: 1.000000  

  this->replaceParenthesisWithSpace(line);
  stringstream ss(line);
  
  TTKLattice::Arc arc;

  arc._prevWord = this->getNext(ss);
  arc._currWord = this->getNext(ss);

  arc._beginTime = str2int(this->getNext(ss));
  arc._endTime = str2int(this->getNext(ss));

  arc._score = str2double((this->getNext(ss)));
  arc._globalScore = str2double((this->getNext(ss)));

  arc._confidenceMeasure = str2double((this->getNext(ss)));
  
  return arc;
}

bool TTKLatticeParser::isEndOfLattice(string& str) {
  return str[0] == '.';
}

string TTKLatticeParser::getNext(iostream& ss) {
  string str;
  ss >> str; ss >> str;
  return str;
}

void TTKLatticeParser::replaceParenthesisWithSpace(string& str) {
  str[str.find('(')] = ' ';
  str[str.find(')')] = ' ';
  str[str.find('(')] = ' ';
  str[str.find(')')] = ' ';
}

Lattice* TTKLatticeParser::createLattice(string filename) {
  fstream file(filename.c_str(), ios::in);

  Lattice* lattice = new TTKLattice;
  TTKLattice* ttkLattice = dynamic_cast<TTKLattice*>(lattice);

  string line;
  while(getline(file, line) && !isEndOfLattice(line))
    ttkLattice->_arcs.push_back(this->createArc(line));

  file.close();
  return lattice;
}

ostream& operator << (ostream& os, const TTKLattice::Arc& arc) {
  return os << 
    " prev:" << arc._prevWord << 
    " curr:" << arc._currWord <<
    " begin:" << arc._beginTime <<
    " end:" << arc._endTime <<
    " score:" << arc._score <<
    " gScore:" << arc._globalScore <<
    " cm:" << arc._confidenceMeasure;
}

// ***********************
// ***** TTK Lattice *****
// ***********************

void TTKLattice::print() const {
  for(int i=0; i<_arcs.size(); ++i)
    cout << _arcs[i] << endl;
}

ostream& operator << (ostream& os, const HTKLattice::Node& node) {
  os << "t=" << node._time << "\tW=" << node._word << "\t\t\tv=" << node._variation;
  return os;
}

ostream& operator << (ostream& os, const HTKLattice::Arc& arc) {
  os << "S=" << arc._startNode << "\tE=" << arc._endNode << "\ta=" << arc._acScore << "\tl=" << arc._lmScore;
  return os;
}

/*
ofstream& operator << (ofstream& fs, const LatticeNode& node) {
  fs << node._word << "(" << node._time << ")";
  return fs;
}

ostream& operator << (ostream& os, const LatticeNode& node) {
  os << node._word << "(" << node._time << ")";
  return os;
}

// **************************
// ***** Simple Lattice *****
// **************************
void SimpleLattice::cvtFromHTKLattice(Lattice* lattice) {
  HTKLattice* htkLattice = dynamic_cast<HTKLattice*>(lattice);
  for(int i=0; i<htkLattice->getNodes().size(); ++i)
    _graph.addNode( LatticeNode(htkLattice->getNode(i).getWord(), htkLattice->getNode(i).getTime()) );

  float lmscale = htkLattice->getHeader().lmscale;
  float acscale = htkLattice->getHeader().acscale;

  for(int i=0; i<htkLattice->getArcs().size(); ++i) {
    HTKLattice::Arc arc = htkLattice->getArc(i);
    _graph.addEdge(arc.getStartNode(), arc.getEndNode(), lmscale * arc.getLmScore() + acscale * arc.getAcScore());
  }
}

void SimpleLattice::cvtFromTTKLattice(Lattice* lattice) {
  TTKLattice* ttkLattice = dynamic_cast<TTKLattice*>(lattice);
  _graph.addNode(LatticeNode("SIL", 0));

  for(int i=0; i<ttkLattice->getArcs().size(); ++i) {
    TTKLattice::Arc arc = ttkLattice->getArcs()[i];

    LatticeNode rNode(arc.getCurrWord(), arc.getEndTime());
    LatticeNode lNode(arc.getPrevWord(), arc.getBeginTime());

    if(_graph.findByValue(rNode) == NULL)
      _graph.addNode(rNode);
    _graph.addEdge(_graph.findByValue(lNode), _graph.findByValue(rNode), arc.getConfidenceMeasure());

  }
}
*/
