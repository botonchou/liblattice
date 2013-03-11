#ifndef _LATTICE_2_INVERTED_INDEX_H
#define _LATTICE_2_INVERTED_INDEX_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <map>

#include <sqlite3.h>
using namespace std;

int str2int(string str);
float str2float(string str);
double str2double(string str);
string getValueStr(string& str);
// ***************************
// *****     Lattice     *****
// ***************************
class Lattice {
public:
  virtual void print() const;
};

typedef string Word;
typedef double Likelihood;
class TimeFrame {
public:
  TimeFrame(): _beginTime(0), _endTime(0) {}
  TimeFrame(int beginTime, int endTime): _beginTime(beginTime), _endTime(endTime) {}
  
  int getBeginTime() const { return _beginTime; }
  int getEndTime() const { return _endTime; }
  int getDuration() const { return _endTime - _beginTime; }
private:
  int _beginTime;
  int _endTime;
};

class SimpleArc {
  public:
    SimpleArc() {}
    SimpleArc(Word word, TimeFrame timeFrame, Likelihood likelihood): _word(word), _timeFrame(timeFrame), _likelihood(likelihood) {} 

    Word getWord() const { return _word; }
    TimeFrame getTimeFrame() const { return _timeFrame; }
    Likelihood getLikelihood() const { return _likelihood; }
    
  private:
    Word _word;
    TimeFrame _timeFrame;
    Likelihood _likelihood;
};

class MyLattice : Lattice {
public:
  MyLattice() {
      _elements.push_back(Element(SimpleArc("A", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("B", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("C", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("D", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("E", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("D", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("E", TimeFrame(0, 1), 0.1), 1));
      _elements.push_back(Element(SimpleArc("F", TimeFrame(0, 1), 0.1), 1));
  }
  MyLattice(Lattice* lattice) {
    //TTKLattice* ttkLattice = dynamic_cast<TTKLattice*>(lattice);
  }
  class Element {
  public:
    Element(SimpleArc arc, int prevIndex): _arc(arc), _prevIndex(prevIndex) {
    }
    friend class MyLattice;
  private:
    SimpleArc _arc;
    int _prevIndex;
  };

  void save(sqlite3* db) const {
    char *errMsg = NULL;

    string createLatticeTable = "create Table Lattice (uid INTEGER PRIMARY KEY AUTOINCREMENT, UtteranceId INTEGER, ArcId INTEGER, PrevArcId INTEGER, Word TEXT, BeginTime INTEGER, EndTime INTEGER, Likelihood REAL);";
    sqlite3_exec(db, createLatticeTable.c_str(), 0, 0, &errMsg);
    
    sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, &errMsg);

    cout << "size of _elements = " << _elements.size() << endl;
    for(int i=0; i<_elements.size(); ++i) {
      string sql = createSqlCommand("INSERT INTO 'Lattice' (UtteranceId, ArcId, PrevArcId, Word, BeginTime, EndTime, Likelihood) VALUES ('%d', '%d', '%d', '%s', '%d', '%d', '%f');", i);
      sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    }

    sqlite3_exec(db, "COMMIT;", 0, 0, &errMsg);

    char** result;
    int rows, cols;

    sqlite3_get_table(db, "SELECT * FROM 'Lattice'", &result, &rows, &cols, &errMsg);
    cout << "rows = " << rows << ", cols = " << cols << endl;

    for (int i=0; i<rows; i++) {
      for (int j=0; j<cols; j++)
	printf("%s\t", result[i*cols+j]);
      printf("\n");
    }
  }


  string createSqlCommand(string prototype, int counter) const {
    char buffer[1024];
    const Element& e = _elements[counter];
    sprintf(buffer,
	prototype.c_str(),
	_utteranceId.c_str(),
	counter,
	e._prevIndex,
	e._arc.getWord().c_str(),
	e._arc.getTimeFrame().getBeginTime(),
	e._arc.getTimeFrame().getEndTime(),
	e._arc.getLikelihood()
    );
    return buffer;
  }

  void print () const {}

private:
  string _utteranceId;
  vector<Element> _elements;
};

class HTKLattice : public Lattice {

public:
  friend class HTKLatticeParser;

  typedef struct Header {
    float version;
    string utterance;
    string lmname;
    float lmscale;
    float wdpenalty;
    float acscale;
    string vocab;
    string hmms;
    int nNodes;
    int nArcs;
  } Header;

  class Node {
    typedef float Time;
    typedef short Variation;
    public:
      Node() {}
      Node(Time time, Word word, Variation variation): _time(time), _word(word), _variation(variation) {}

      void setTime(Time time) { _time = time; }
      void setWord(Word word) { _word = word; }
      void setVariation(Variation variation) { _variation = variation; }

      Time getTime() const { return _time; }
      Word getWord() const { return _word; }
      Variation getVariation() const { return _variation; }

      friend class HTKLatticeParser;
      friend ostream& operator << (ostream& os, const Node& node);
    private:
      Time _time;
      Word _word;
      Variation _variation;
  };

  class Arc {
    typedef float Score;
    public:
      Arc() {}
      Arc(int startNode, int endNode, Score acScore, Score lmScore): _startNode(startNode), _endNode(endNode), _acScore(acScore), _lmScore(lmScore) {}
  
      void setStartNode(int startNode) { _startNode = startNode; }
      void setEndNode(int endNode) { _endNode = endNode; }
      void setAcScore(Score acScore) { _acScore = acScore; }
      void setLmScore(Score lmScore) { _lmScore = lmScore; }

      int getStartNode() const { return _startNode; }
      int getEndNode() const { return _endNode; }
      Score getAcScore() const { return _acScore; }
      Score getLmScore() const { return _lmScore; }

      friend class HTKLatticeParser;
      friend ostream& operator << (ostream& os, const HTKLattice::Arc& arc);
    private:
      int _startNode;
      int _endNode;
      Score _acScore;
      Score _lmScore;
  };

  const Header& getHeader() const { return _header; }
  const vector<Node>& getNodes() const { return _nodes; }
  const Node& getNode(size_t idx) const { return _nodes[idx]; }
  const vector<HTKLattice::Arc>& getArcs() const { return _arcs; }
  const HTKLattice::Arc& getArc(size_t idx) const { return _arcs[idx]; }

  void print() const;

private:
  HTKLattice() {
  }

  Header _header;
  vector<Node> _nodes;
  vector<HTKLattice::Arc> _arcs;
};

ostream& operator << (ostream& os, const HTKLattice::Node& node);
ostream& operator << (ostream& os, const HTKLattice::Arc& arc);

class TTKLattice : public Lattice {
public:
  friend class TTKLatticeParser;
  void print() const;

  class Arc {
    typedef int Time;
    typedef double Score;
    typedef double ConfidenceMeasure;
    public:
      Arc() {}
      Arc(Word prevWord, Word currWord, Time beginTime, Time endTime, Score score, Score globalScore, ConfidenceMeasure confidenceMeasure): _prevWord(prevWord), _currWord(currWord), _beginTime(beginTime), _endTime(endTime), _score(score), _globalScore(globalScore), _confidenceMeasure(confidenceMeasure) {}

      void setPrevWord(Word prevWord) { _prevWord = prevWord; }
      void setCurrWord(Word currWord) { _currWord = currWord; }
      void setBeginTime(Time beginTime) { _beginTime = beginTime; }
      void setEndTime(Time endTime) { _endTime = endTime; }
      void setScore(Score score) { _score = score; }
      void setGlobalScore(Score globalScore) { _globalScore = globalScore; }
      void setConfidenceMeasure(ConfidenceMeasure confidenceMeasure) { _confidenceMeasure = confidenceMeasure; }

      Word getPrevWord() const { return _prevWord; }
      Word getCurrWord() const { return _currWord; }
      Time getBeginTime() const { return _beginTime; }
      Time getEndTime() const { return _endTime; }
      Score getScore() const { return _score; }
      Score getGlobalScore() const { return _globalScore; }
      ConfidenceMeasure getConfidenceMeasure() const { return _confidenceMeasure; }

      friend class TTKLatticeParser;
      friend ostream& operator << (ostream& os, const TTKLattice::Arc& arc);
      
    private:
      Word _prevWord;
      Word _currWord;
      Time _beginTime;
      Time _endTime;
      Score _score;
      Score _globalScore;
      ConfidenceMeasure _confidenceMeasure;
  };

  vector<TTKLattice::Arc> _arcs;
};

ostream& operator << (ostream& os, const TTKLattice::Arc& arc);

// ***************************
// ***** Lattice Factory *****
// ***************************

class LatticeParser {
public:
  // Factory Method
  virtual Lattice* createLattice(string filename) = 0;
};

class HTKLatticeParser : public LatticeParser {
public:
  Lattice* createLattice(string filename);
  string getNext(iostream& file);

  HTKLattice::Node createNode(fstream& file);
  HTKLattice::Arc createArc(fstream& file);
};

class TTKLatticeParser : public LatticeParser {
public:
  Lattice* createLattice(string filename);
  string getNext(iostream& str);

  TTKLattice::Arc createArc(string& str);

  bool isEndOfLattice(string& str);
  void replaceParenthesisWithSpace(string& str);
};

// *********************************
// ***** Lattice2InvertedIndex *****
// *********************************


/*
class HypothesisRegion {
public:
  HypothesisRegion(TimeFrame timeFrame, Likelihood posteriorProb): _timeFrame(timeFrame), _posteriorProb(posteriorProb) {}
private:
  TimeFrame _timeFrame;
  Likelihood _posteriorProb;
};

class Utterance {
public:
  Utterance(string id): _id(id) {}
  bool operator < (const Utterance& utterance) const { return (_id < utterance._id); }
  friend ostream& operator << (ostream& os, const Utterance& utterance);

  string getId() const { return _id; }
  string getPathToFile() const { return _pathToFile; }
private:
  string _id;
  string _pathToFile;
};
ostream& operator << (ostream& os, const Utterance& utterance);

class InvertedIndexingStructure {
public:
  vector<HypothesisRegion> operator() (Word word, Utterance utterance) {
    return _map[word][utterance];
  }
private:
  map<Word, map<Utterance, vector<HypothesisRegion> > > _map;
};

class IISBuilder {
public:
  static void buildFromLattice(Lattice* lattice, InvertedIndexingStructure& iis);
};
*/

#endif // _LATTICE_2_INVERTED_INDEX_H
