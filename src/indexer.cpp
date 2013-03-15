#include <iostream>

#include <cmdParser.h>
#include <array.h>

#include <stdio.h>
#include <sqlite3.h>
#include <lattice.h>
#include <index_builder.h>

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

  for(int i=0; i<lattices.size(); ++i) {
    cout << "Parsing lattice " << utteranceIds[i] << endl;
    lattices[i] = htkLatticeParser.createLattice(utteranceIds[i]);
    vocabulary.add(lattices[i]->getWordSet());
  }

  cout << "Establishing Vocabulary..." << endl;
  corpus.establishVocabulary();
  corpus.updateVocabulary(vocabulary);

  for(int i=0; i<lattices.size(); ++i) {
    cout << "Processing " << i << "th lattice" << endl;
    corpus.add(lattices[i]);
  }
}

/*
int main() {

  MyGraph graph;

  VertexID v1 = boost::add_vertex(graph);
  graph[v1].name = "node 1";

  VertexID v2 = boost::add_vertex(graph);
  graph[v2].name = "node 2";

  EdgeID edge;
  bool ok;
  boost::tie(edge, ok) = boost::add_edge(v1, v2, graph);
  if(ok)
    graph[edge].cost = 1.234;

  // Iterate all nodes in graph
  MyGraph::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(graph);
  for(; vertexIt != vertexEnd; ++vertexIt) {
    VertexID vid = *vertexIt;
    Vertex& vertex = graph[vid];
    cout << vertex.name << endl;
  }

  // Iterate all adjacent nodes to some nodes
  MyGraph::adjacency_iterator neighborIt, neighborEnd;
  boost::tie(neighborIt, neighborEnd) = adjacent_vertices(v1, graph);
  for(; neighborIt != neighborEnd; ++neighborIt)
    cout << graph[*neighborIt].name << endl;

  // Iterate all edges in graph
  typedef boost::graph_traits<MyGraph>::edge_iterator edge_iter;
  pair<edge_iter, edge_iter> ep;
  edge_iter ei, ei_end;
  for (tie(ei, ei_end) = edges(graph); ei != ei_end; ++ei)
    cout << graph[*ei].cost << endl;

  Graph<int, int> graph;

  for(int i=0; i<20; ++i)
    graph.addNode(i);

  graph.addEdge(0, 1, 1);
  graph.addEdge(1, 2, 2);
  graph.addEdge(1, 3, 2);
  graph.addEdge(2, 4, 9);
  graph.addEdge(2, 5, 9);
  graph.addEdge(4, 8, 7);
  graph.addEdge(5, 8, 9);
  graph.addEdge(8, 9, 8);
  graph.addEdge(5, 6, 1);
  graph.addEdge(3, 6, 4);
  graph.addEdge(6, 9, 2);
  graph.addEdge(3, 7, 5);
  graph.addEdge(7, 9, 3);
  graph.addEdge(3, 10, 2);
  graph.addEdge(9, 10, 9);
  graph.addEdge(8, 11, 9);
  graph.addEdge(8, 12, 7);
  graph.addEdge(9, 13, 9);
  graph.addEdge(12, 14, 2);
  graph.addEdge(12, 15, 4);
  graph.addEdge(13, 16, 5);
  graph.addEdge(14, 17, 8);
  graph.addEdge(16, 18, 2);
  graph.addEdge(18, 19, 1);
  graph.addEdge(12, 13, 7);
  graph.addEdge(14, 18, 6);
  graph.addEdge(11, 15, 8);
  graph.addEdge(15, 17, 4);
  graph.addEdge(17, 19, 2);
  graph.addEdge(10, 18, 6);

  graph.saveToPNG("test.png");

  Graph<int, int>::iterator itr = graph.begin();
  for(; itr != graph.end(); ++itr)
    cout << (*itr) << endl;
}
*/

//#include <boost/graph/adjacency_list.hpp>
//#include <boost/graph/astar_search.hpp>
//#include <boost/graph/graph_traits.hpp>
/*
struct Vertex {
  string name;
};
struct Edge {
  double cost;
};

typedef boost::adjacency_list<
  boost::listS,
  boost::vecS,
  boost::directedS,
  Vertex,
  Edge
> MyGraph;

typedef MyGraph::vertex_descriptor VertexID;
typedef MyGraph::edge_descriptor   EdgeID;
*/
