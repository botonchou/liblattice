#include <lattice2invertedindex.h>

double str2double(string str) {
  double d;
  stringstream ss;
  ss << str;
  ss >> d;
  return d;
}

float str2float(string str) {
  float f;
  stringstream ss;
  ss << str;
  ss >> f;
  return f;
}

int str2int(string str) {
  return atoi(str.c_str());
}

void Lattice::print() const {
  cout << "Lattice::print() ..." << endl;
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
  getNext(file);

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
