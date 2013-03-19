#include <lattice.h>


HypothesisRegion::HypothesisRegion(): u_id(0), arc_id(0), prev_arc_id(0), likelihood(0) {
}
HypothesisRegion::HypothesisRegion(int argc, char** argv): u_id(0), arc_id(0), prev_arc_id(0), likelihood(0) {
  u_id = atoi(argv[0]);
  arc_id = atoi(argv[1]);
  prev_arc_id = atoi(argv[2]);
  likelihood = atof(argv[3]);
}
ostream& operator << (ostream& os, const HypothesisRegion& hr) {
  return os << hr.u_id << "\t" << hr.arc_id << "\t" << hr.prev_arc_id << "\t" << hr.likelihood;
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
  static char str[128];
  
  file >> str;
  node._time = atof(str + 2);
  
  file >> str;
  node._word = str + 2;

  file >> str;
  node._variation = atoi(str + 2);
  
  return node;
}

HTKLattice::Arc HTKLatticeParser::createArc(fstream& file) {
  // Handle different version of HTK lattice format
  static char str[256];
  while(file >> str && str[0] != 'J');

  HTKLattice::Arc arc;
  file >> str;
  arc._startNode = atoi(str + 2);

  file >> str;
  arc._endNode = atoi(str + 2);

  file >> str;
  arc._acScore = atof(str + 2);

  file >> str;
  arc._lmScore = atof(str + 2);

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

bool HTKLattice::saveToVocabulary(map<string, bool>& vocabulary) const {
  for(int i=0; i<_nodes.size(); ++i)
    vocabulary[_nodes[i].getWord()] = true;
}

vector<string> HTKLattice::getWordSet() const {
  vector<string> wordSet(_nodes.size());
  for(int i=0; i<_nodes.size(); ++i)
    wordSet[i] = _nodes[i].getWord();
  return wordSet;
}

size_t HTKLattice::getPreviousArcIndex(size_t idx) const {
  for(int i=0; i<idx; ++i)
    if(_arcs[i].getEndNode() == _arcs[idx].getStartNode())
      return i;
  return 0;
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
