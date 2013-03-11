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

#include <graph.h>
#include <sqlite_wrapper.h>
#include <utility.h>
using namespace std;

// ***************************
// *****     Lattice     *****
// ***************************
class Lattice {
public:
  virtual void print() const;
  virtual bool saveToDatabase(SQLiteDatabase& db) const;
  virtual bool saveToVocabulary(map<string, bool>& vocabulary) const;
};

typedef string Word;
typedef double Score;
typedef double Likelihood;

// ***************************
// *****   HTK Lattice   *****
// ***************************
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
      friend class HTKLattice;
      friend ostream& operator << (ostream& os, const Node& node);
    private:
      Time _time;
      Word _word;
      Variation _variation;
  };

  class Arc {
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
      friend class HTKLattice;
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

  size_t getPreviousArcIndex(size_t idx) const;
  string getValidUtteranceId() const;
  Likelihood computeLikelihood(const Arc& arc) const;
  void addNewLatticeToDatabase(SQLiteDatabase& db, string tableName) const;
  void createUtteranceTableIfNotExist(SQLiteDatabase& db) const;
  string getTableName() const;

  void print() const;
  void createInsertSQL(char* sql, const int& i) const;
  bool saveToDatabase(SQLiteDatabase& db) const;
  bool saveToVocabulary(map<string, bool>& vocabulary) const;

private:
  HTKLattice() {}

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
  bool saveToDatabase(SQLiteDatabase& db) const {}
  bool saveToVocabulary(map<string, bool>& vocabulary) const {}

  class Arc {
    typedef int Time;
    typedef double ConfidenceMeasure;
    public:
      Arc() {}
      Arc(Word prevWord, Word curWord, Time beginTime, Time endTime, Score score, Score globalScore, ConfidenceMeasure confidenceMeasure): _prevWord(prevWord), _currWord(curWord), _beginTime(beginTime), _endTime(endTime), _score(score), _globalScore(globalScore), _confidenceMeasure(confidenceMeasure) {}

      void setPrevWord(Word prevWord) { _prevWord = prevWord; }
      void setCurrWord(Word curWord) { _currWord = curWord; }
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
  
  const vector<Arc>& getArcs() const { return _arcs; }
private:
  vector<Arc> _arcs;
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
class LatticeNode {
  typedef int Time; // time in milliseconds
public:
  LatticeNode(Word word, Time time): _word(word), _time(time) {}

  bool operator == (const LatticeNode& rhs) const { return (_word == rhs._word) && (_time == rhs._time); }
  bool operator != (const LatticeNode& rhs) const { return !(*this == rhs); }

  friend ofstream& operator << (ofstream& fs, const LatticeNode& node);
  friend ostream& operator << (ostream& os, const LatticeNode& node);
private:
  Word _word;
  Time _time;
};
ofstream& operator << (ofstream& fs, const LatticeNode& node);
ostream& operator << (ostream& os, const LatticeNode& node);

class SimpleLattice {
public:
  void cvtFromHTKLattice(Lattice* lattice);
  void cvtFromTTKLattice(Lattice* lattice);

  const Graph<LatticeNode, Likelihood>& getGraph() const { return _graph; }
  void save(sqlite3* db) const {
  }
private:
  Graph<LatticeNode, Likelihood> _graph;
};
*/

#endif // _LATTICE_2_INVERTED_INDEX_H
