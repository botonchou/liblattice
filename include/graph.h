#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <iostream>
#include <vector>
#include <tr1/unordered_map>
#include <stack>
using namespace std;
using namespace std::tr1;

template<typename T, typename S>
class Graph;

template<typename T, typename S>
class GraphNode {
public:
  GraphNode() {}
  GraphNode(T data): _data(data) {}

  void setData(T data) { _data = data; }
  T getData() const { return _data; }

  size_t indexOf(GraphNode<T, S>* node) const {
    for(int i=0; i<_neighbor.size(); ++i)
      if(_neighbor[i] == node)
	return i;
    return -1;
  }

  bool addNeighbor(GraphNode<T, S>* node, S cost) {
    if(indexOf(node) != -1)
      return false;
      
    _neighbor.push_back(node);
    _cost.push_back(cost);
    return true;
  }

  bool removeNeighbor(GraphNode<T, S>* node) {
    size_t index = indexOf(node);
    if(index == -1)
      return false;

    removeNeighbor(index);
  }

  bool removeNeighbor(size_t index) {
    if(index >= _neighbor.size())
      return false;

    _neighbor.erase(_neighbor.begin() + index);
    _cost.erase(_cost.begin() + index);
    return true;
  }

  void print() const {
    cout << _data << endl;
    cout << "neighbor:\t";
    for(int i=0; i<_neighbor.size(); ++i)
      cout << _neighbor[i] << " ";
    cout << endl;

    cout << "cost    :\t";
    for(int i=0; i<_cost.size(); ++i)
      cout << _cost[i] << "         ";
    cout << endl << endl;;
  }

  friend class Graph<T, S>;
private:
  T _data;
  vector<GraphNode<T, S>* > _neighbor;
  vector<S> _cost;
};

template<typename T, typename S>
class Graph {
public:
  Graph() {}
  ~Graph() {
    for(int i=0; i<_nodes.size(); ++i)
      delete _nodes[i];
  }

  void addNode(GraphNode<T, S>* node) {
    _nodes.push_back(node); 
  }

  void addNode(T data) {
    _nodes.push_back(new GraphNode<T, S>(data)); 
  }

  void addEdge(GraphNode<T, S>* from, GraphNode<T, S>* to, S cost) {
    from->_neighbor.push_back(to);
    from->_cost.push_back(cost);
    //to->_neighbor.push_back(from);
    //to->_cost.push_back(cost);
  }

  void addEdge(size_t from, size_t to, S cost) {
    addEdge(_nodes[from], _nodes[to], cost);
  }

  bool contains(T data) const {
    return findByValue(data) != -1;
  }

  size_t indexOf(GraphNode<T, S>* node) const {
    for(int i=0; i<_nodes.size(); ++i)
      if(_nodes[i] == node)
	return i;
    return -1;
  }

  GraphNode<T, S>* findByValue(T data) const {
    for(int i=0; i<_nodes.size(); ++i)
      if(_nodes[i]->_data == data)
	return _nodes[i];
    return NULL;
  }

  bool remove(T data) {
    GraphNode<T, S>* nodeToRemove = findByValue(data);
    if(nodeToRemove == NULL)
      return false;

    for(int i=0; i<_nodes.size(); ++i) {
      if(_nodes[i] == nodeToRemove) {
	_nodes.erase(_nodes.begin() + i);
	break;
      }
    }

    for(int i=0; i<_nodes.size(); ++i) {
      size_t index = _nodes[i].indexOf(nodeToRemove);
      if(index != -1)
	_nodes[i].removeNeighbor(index);
    }
  }

  void print() const {
    for(int i=0; i<_nodes.size(); ++i) {
      cout << "#" << i << " (" << _nodes[i] << "): ";
      _nodes[i]->print();
    }
  }

  size_t size() const {
    return _nodes.size();
  }

  virtual void saveToPNG(string pngFileName) const {
    string tempFilename = "." + pngFileName + ".cache";
    fstream file(tempFilename.c_str(), ios::out);
      
    file << "digraph G {" << endl;
    file << "rankdir=LR;" << endl;
    for(int i=0; i<_nodes.size(); ++i)
      file << "\tnode_" << i << " [label=\"" << _nodes[i]->_data << "\"];" << endl;

    for(int i=0; i<_nodes.size(); ++i)
      for(int j=0; j<_nodes[i]->_neighbor.size(); ++j)
	file << "\tnode_" << i << "->" << "node_" << this->indexOf(_nodes[i]->_neighbor[j]) << " [label=\"" << _nodes[i]->_cost[j] << "\"]; " << endl;

    file << "}" << endl;

    file.close();

    string drawPng = "dot -Tpng " + tempFilename + " -o " + pngFileName;
    system(drawPng.c_str());
    string deleteCache = "rm " + tempFilename;
    system(deleteCache.c_str());
  }

   // DO NOT add any more data member or function for class iterator
   class iterator {
      friend class Graph;

   public:
      iterator(): _node(NULL) {
      }

      iterator(GraphNode<T, S>* node, unordered_map<GraphNode<T, S>*, bool> visited): _node(node), _visited(visited) {
	_stack.push(node);
	_visited[node] = true;
      }

      iterator(const iterator& i) : _node(i._node), _stack(i._stack), _visited(i._visited) {
      }

      ~iterator() {} 

      const T& operator * () const { return _node->_data; }
      T& operator * () { return _node->_data; }

      // prefix increment  ++i
      iterator& operator ++ () {
	for(int i=0; i<_node->_neighbor.size(); ++i)
	  _stack.push(_node->_neighbor[i]);

	GraphNode<T, S>* top;

	_node = NULL;
	while (true) {
	  if(_stack.empty())
	    return (*this);

	  top = _stack.top();
	  _stack.pop();
	  if(!isVisited(top))
	    break;
	}
	_visited[top] = true;

	_node = top;
	return (*this);
      }

      // postfix increment i++
      iterator operator ++ (int) {
	iterator temp(_node); 
	++(*this);
	return temp; 
      }

      /*
      // prefix decrement  --i
      iterator& operator -- () {
	return (*this);
      }
      // postfix decrement i--
      iterator operator -- (int) {
	iterator temp(_node);
	--(*this);
	return temp;
      }*/

      iterator& operator = (const iterator& i) {
	_node = i._node;
	_visited = i._visited;
	_stack = i._stack;
	return *(this);
      }

      bool operator != (const iterator& i) const { return !(_node == i._node); }
      bool operator == (const iterator& i) const { return (_node == i._node); }

   private:
      bool isVisited(GraphNode<T, S>* node) {
	return _visited[node];
      }

      GraphNode<T, S>* _node;
      stack<GraphNode<T, S>* > _stack;
      unordered_map<GraphNode<T, S>*, bool> _visited;
   };

   // TODO: implement these functions
   iterator begin() const { 
     return iterator(_nodes[0], createVisitation(false));
   }
   
   iterator end() const {
     return iterator(NULL, createVisitation(true));
   }

   unordered_map<GraphNode<T, S>*, bool> createVisitation(bool flag) const {
     unordered_map<GraphNode<T, S>*, bool> visited;
     for(int i=0; i<_nodes.size(); ++i)
       visited[_nodes[i]] = flag;
     return visited;
   }

private:
  vector<GraphNode<T, S>* > _nodes;
};

#endif // _GRAPH_H_
