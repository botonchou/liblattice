#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>

#include <cmdParser.h>
#include <array.h>

#include <stdio.h>
#include <sqlite3.h>
#include <lattice.h>
#include <index_builder.h>

#include <retrieve-core.h>
using namespace std;

// Functionos goes here

int main(int argc, char* argv[]) {

  CmdParser cmdParser(argc, argv);
  cmdParser.regOpt("-c", "corpus database");
  cmdParser.regOpt("-q", "query");
  if(!cmdParser.isOptionLegal())
    cmdParser.showUsageAndExit();

  Corpus corpus(cmdParser.find("-c"));
  string query = cmdParser.find("-q");

  TempList list = RetrieveCore::search(corpus, query);
}

