#include <iostream>

#include <cmdParser.h>
#include <array.h>

#include <stdio.h>
#include <sqlite3.h>
#include <lattice.h>
#include <index_builder.h>
#include <progress_bar.h>
#include <profile.h>

#include <common/common-config.h>
#include <common/common-io.h>
#include <htk/htk-lattice.h>

//#include <graph.h>
using namespace std;
void processLattice(Corpus& corpus, string listFilename);

int main(int argc, char* argv[]) {

  CmdParser cmdParser(argc, argv);
  cmdParser.regOpt("-o", "output filename of corpus database");
  cmdParser.regOpt("-f", "filename of lattice list");
  if(!cmdParser.isOptionLegal())
    cmdParser.showUsageAndExit();
  
  Corpus corpus(cmdParser.find("-o"));
  processLattice(corpus, cmdParser.find("-f"));

  InvertedIndex invertedIndex;
  invertedIndex.buildFrom(corpus);  

  return 0;
}

vector<string> getWordSet(vulcan::HtkLattice* htkLattice) {
  vector<string> wordSet(htkLattice->_node.size());
  for(size_t i=0; i<wordSet.size(); ++i)
    wordSet[i] = htkLattice->_node[i]->_word;
  return wordSet;
}

void processLattice(Corpus& corpus, string listFilename) {
  Array<string> utteranceIds(listFilename);
  size_t nUtterances = utteranceIds.size();

  // ============================
  // === Construct Vocabulary ===
  // ============================
  Vocabulary vocabulary;
  ProgressBar pBar1("Establishing Vocabulary...");
  HTKLatticeParser htkLatticeParser;
  for(size_t i=0; i<nUtterances; ++i) {
    pBar1.refresh(double (i+1) / nUtterances);
    Lattice* lattice = htkLatticeParser.createLattice(utteranceIds[i]);
    vocabulary.add(lattice->getWordSet());
    delete lattice;
  }

  cout << GREEN << "Saving Vocabulary to Database..." << COLOREND << endl;
  vocabulary.reindex();
  corpus.establishVocabulary();
  corpus.updateVocabulary(vocabulary);

  // ===============================================================
  // === Load HTK Lattices again to compute posterior probabilty ===
  // ===============================================================

  Profile profile;
  profile.tic();

  const size_t MAX_LATTICES = 1000;
  vector<vulcan::HtkLattice*> lattices;
  lattices.reserve(MAX_LATTICES);
  size_t nTimes = nUtterances / MAX_LATTICES + 1;

  ProgressBar pBar2("Compute Posterior Probability and Insert lattice into Database...");

  size_t idx = 0;
  for(size_t i=0; i<nTimes; ++i) {
    corpus.getDatabase().beginTransaction();

    for(size_t j=0; j<MAX_LATTICES; ++j) {
      if(idx >= nUtterances)
	break;

      pBar2.refresh(double (idx + 1) / nUtterances);

      vulcan::HtkLattice* l = new vulcan::HtkLattice;
      l->LoadHtkText(utteranceIds[idx++]);
      lattices.push_back(l);
      corpus.add(l, vocabulary);
    }

    for(size_t j=0; j<lattices.size(); ++j) {
      lattices[j]->ReleaseMemory();
      delete lattices[j];
    }

    lattices.clear();
    lattices.reserve(MAX_LATTICES);

    corpus.getDatabase().endTransaction();
  }
  
  profile.toc();

}

