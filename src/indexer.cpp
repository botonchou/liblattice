#include <iostream>

#include <cmdParser.h>
#include <array.h>

#include <stdio.h>
#include <sqlite3.h>
#include <lattice.h>
#include <index_builder.h>
#include <progress_bar.h>
#include <profile.h>

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

void processLattice(Corpus& corpus, string listFilename) {
  //TTKLatticeParser ttkLatticeParser;
  //Lattice* lattice = ttkLatticeParser.createLattice("N200108011200-04-07.lat");
  HTKLatticeParser htkLatticeParser;
  vector<Lattice*> lattices;

  Array<string> utteranceIds(listFilename);
  lattices.resize(utteranceIds.size());

  Vocabulary vocabulary;

  ProgressBar pBar("Parsing lattice...");
  for(size_t i=0; i<lattices.size(); ++i) {
    pBar.refresh((double) (i + 1)/lattices.size());
    lattices[i] = htkLatticeParser.createLattice(utteranceIds[i]);
    vocabulary.add(lattices[i]->getWordSet());
  }

  cout << GREEN << "Establishing Vocabulary..." << COLOREND << endl;
  vocabulary.reindex();
  corpus.establishVocabulary();
  corpus.updateVocabulary(vocabulary);

  Profile profile;
  profile.tic();

  ProgressBar pBar2("Inserting lattice into database...");
  int nInsertion = 0;
  corpus.getDatabase().beginTransaction();
  for(size_t i=0; i<lattices.size(); ++i) {
    pBar2.refresh((double) (i + 1)/lattices.size());
    corpus.add(lattices[i], vocabulary);
    nInsertion += dynamic_cast<HTKLattice*>(lattices[i])->getArcs().size();
  }
  cout << "#insertion in total: " << nInsertion << endl;
  
  profile.toc();

  cout << "Committing..." << endl;
  corpus.getDatabase().endTransaction();
}

