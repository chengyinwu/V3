/****************************************************************************
  FileName     [ v3Graph.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Data Structure and Algorithms to Graph. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_GRAPH_H
#define V3_GRAPH_H

#include "v3Misc.h"

template<class T>
class V3Graph {
   // Defines for Graph Structure
   typedef typename V3HashMap<T, uint32_t>::Hash   V3NodeIndexMap;
   typedef pair<V3UI32Set, V3UI32Set>              V3DirectedEdge;
   typedef pair<T, V3DirectedEdge>                 V3DirectedNode;
   typedef typename V3Vec<V3DirectedNode>::Vec     V3NodeVec;
   // Defines for Algorithms
   typedef typename V3Stack<T>::Stack              V3NodeStack;
   typedef typename V3Queue<T>::Queue              V3NodeLayer;
   typedef typename V3Stack<V3NodeLayer>::Stack    V3NodeFlow;
   typedef typename V3Set<T>::Set                  V3NodeSet;
   typedef typename V3Vec<V3NodeSet>::Vec          V3NodeField;
   typedef typename V3Queue<V3UI32Queue>::Queue    V3IndexFlow;
   typedef typename V3Vec<V3UI32Set>::Vec          V3IndexField;
   public : 
      // Constructor and Destructor
      V3Graph();
      ~V3Graph();
      // Graph Interface Functions
      inline const uint32_t getNodeSize() const;
      inline const uint32_t getInNodeSize(const uint32_t&) const;
      inline const uint32_t getOutNodeSize(const uint32_t&) const;
      inline const T& getNode(const uint32_t&) const;
      inline const uint32_t getNodeIndex(const T&) const;
      inline const uint32_t getInNode(const uint32_t&, const uint32_t&) const;
      inline const uint32_t getOutNode(const uint32_t&, const uint32_t&) const;
      inline const bool isIsolatedNode(const uint32_t&) const;
      inline const uint32_t insertNode(const T&);
      inline void insertEdge(const T&, const T&);
      inline void deleteEdge(const T&, const T&);
      inline void clearGraph();
      // Graph Algorithms
      void getLeaves(V3NodeLayer&) const;
      void topologicalSort(V3NodeFlow&) const;
      void getStronglyConnectedComponents(V3NodeField&) const;
      void shortestPath(const T&, const T&, V3NodeStack&) const;
   private : 
      // Private Member Functions
      inline const uint32_t getToSize(const uint32_t&) const;
      inline const uint32_t getFromSize(const uint32_t&) const;
      inline void renderConnection(const uint32_t&, const uint32_t&);
      inline void removeConnection(const uint32_t&, const uint32_t&);
      // Private Graph Algorithms
      void topologicalSortIndex(V3IndexFlow&) const;
      void getStronglyConnectedComponentsIndex(V3IndexField&) const;
      void strongConnect(const uint32_t&, V3IndexField&, V3UI32Vec&, V3UI32Vec&, V3BoolVec&, V3UI32Stack&, uint32_t&) const;
      // Private Data Members
      V3NodeIndexMap       _nodeIndexMap;
      V3NodeVec            _nodeList;
};

/* -------------------------------------------------- *\
 * Class V3Graph Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
template<class T>
V3Graph<T>::V3Graph() { clearGraph(); }

template<class T>
V3Graph<T>::~V3Graph() { clearGraph(); }

// Graph Interface Functions
template<class T> inline const uint32_t
V3Graph<T>::getNodeSize() const { return _nodeList.size(); }

template<class T> inline const uint32_t
V3Graph<T>::getInNodeSize(const uint32_t& i) const {
   assert (i < getNodeSize()); return _nodeList[i].second.first.size(); }

template<class T> inline const uint32_t
V3Graph<T>::getOutNodeSize(const uint32_t& i) const {
   assert (i < getNodeSize()); return _nodeList[i].second.second.size(); }

template<class T> inline const T&
V3Graph<T>::getNode(const uint32_t& i) const {
   assert (i < getNodeSize()); return _nodeList[i].first; }

template<class T> inline const uint32_t
V3Graph<T>::getNodeIndex(const T& t) const {
   typename V3NodeIndexMap::const_iterator it = _nodeIndexMap.find(t);
   return (_nodeIndexMap.end() == it) ? V3NtkUD : it->second; }

template<class T> inline const uint32_t
V3Graph<T>::getInNode(const uint32_t& i, const uint32_t& s) const {
   assert (s < getInNodeSize(i));
   V3UI32Set::const_iterator it = _nodeList[i].second.first.begin();
   for (uint32_t x = 0; x < s; ++x) ++it; return *it;
}

template<class T> inline const uint32_t
V3Graph<T>::getOutNode(const uint32_t& i, const uint32_t& s) const {
   assert (s < getOutNodeSize(i));
   V3UI32Set::const_iterator it = _nodeList[i].second.second.begin();
   for (uint32_t x = 0; x < s; ++x) ++it; return *it;
}

template<class T> inline const bool
V3Graph<T>::isIsolatedNode(const uint32_t& i) const {
   assert (i < getNodeSize());
   return (!getInNodeSize(i) && !getOutNodeSize(i));
}

template<class T> inline const uint32_t
V3Graph<T>::insertNode(const T& n) {
   assert (_nodeIndexMap.size() == _nodeList.size());
   assert (_nodeIndexMap.end() == _nodeIndexMap.find(n));
   _nodeIndexMap.insert(make_pair(n, _nodeList.size()));
   _nodeList.push_back(V3DirectedNode(n, V3DirectedEdge()));
   return _nodeList.size() - 1;
}

template<class T> inline void
V3Graph<T>::insertEdge(const T& f, const T& t) {
   typename V3NodeIndexMap::iterator it = _nodeIndexMap.find(f);
   const uint32_t fIndex = (_nodeIndexMap.end() == it) ? insertNode(f) : it->second;
   typename V3NodeIndexMap::iterator is = _nodeIndexMap.find(t);
   const uint32_t tIndex = (_nodeIndexMap.end() == is) ? insertNode(t) : is->second;
   renderConnection(fIndex, tIndex);
}

template<class T> inline void
V3Graph<T>::deleteEdge(const T& f, const T& t) {
   typename V3NodeIndexMap::iterator it = _nodeIndexMap.find(f), is = _nodeIndexMap.find(t);
   assert (_nodeIndexMap.end() != it); assert (_nodeIndexMap.end() != is);
   removeConnection(it->second, is->second);
}

template<class T> inline void
V3Graph<T>::clearGraph() {
   _nodeIndexMap.clear(); _nodeList.clear();
}

// Private Member Functions
template<class T> inline const uint32_t
V3Graph<T>::getToSize(const uint32_t& f) const {
   assert (_nodeList.size() > f); return _nodeList[f].second.second.size();
}

template<class T> inline const uint32_t
V3Graph<T>::getFromSize(const uint32_t& t) const {
   assert (_nodeList.size() > t); return _nodeList[t].second.first.size();
}

template<class T> inline void
V3Graph<T>::renderConnection(const uint32_t& f, const uint32_t& t) {
   assert (_nodeList.size() > f); assert (_nodeList.size() > t);
   _nodeList[f].second.second.insert(t); _nodeList[t].second.first.insert(f);
}

template<class T> inline void
V3Graph<T>::removeConnection(const uint32_t& f, const uint32_t& t) {
   assert (_nodeList.size() > f); assert (_nodeList.size() > t);
   assert (_nodeList[f].second.second.end() != _nodeList[f].second.second.find(t));
   assert (_nodeList[t].second.first.end() != _nodeList[t].second.first.find(f));
   _nodeList[f].second.second.erase(_nodeList[f].second.second.find(t));
   _nodeList[t].second.first.erase(_nodeList[t].second.first.find(f));
}

// Graph Algorithms
template<class T> void
V3Graph<T>::getLeaves(V3NodeLayer& leafList) const {
   while (!leafList.empty()) leafList.pop();
   for (uint32_t i = 0; i < _nodeList.size(); ++i)
      if (!getToSize(i)) leafList.push(_nodeList[i].first);
}

template<class T> void
V3Graph<T>::topologicalSort(V3NodeFlow& orderList) const {
   V3IndexFlow orderGraph; topologicalSortIndex(orderGraph);
   while (!orderList.empty()) orderList.pop();
   // Map Node Index back to Type T
   while (!orderGraph.empty()) {
      orderList.push(V3NodeLayer());
      while (!orderGraph.front().empty()) {
         orderList.top().push(_nodeList[orderGraph.front().front()].first);
         orderGraph.front().pop();
      }
      orderGraph.pop();
   }
}

template<class T> void
V3Graph<T>::getStronglyConnectedComponents(V3NodeField& scc) const {
   V3IndexField sccIndex; getStronglyConnectedComponentsIndex(sccIndex);
   if (scc.size()) scc.clear(); scc.reserve(sccIndex.size());
   for (uint32_t i = 0; i < sccIndex.size(); ++i) {
      scc.push_back(V3NodeSet()); scc.back().clear();
      for (V3UI32Set::const_iterator it = sccIndex[i].begin(); it != sccIndex[i].end(); ++it) {
         assert (_nodeList.size() > *it); scc.back().insert(_nodeList[*it].first);
      }
   }
}

template<class T> void
V3Graph<T>::shortestPath(const T& node1, const T& node2, V3NodeStack& path) const {
   // Trivial Case Check
   while (!path.empty()) { path.pop(); }
   uint32_t s = getNodeIndex(node1); if (s >= _nodeList.size()) return;
   uint32_t t = getNodeIndex(node2); if (t >= _nodeList.size()) return;
   if (s == t) { path.push(node1); return; }
   // Since the Graph has every edge with Weight = 1, use BFS instead of Dijkstra
   V3UI32Vec preNode(_nodeList.size(), V3NtkUD); preNode[s] = s;
   V3UI32Vec record; record.reserve(_nodeList.size()); record.push_back(s);
   V3UI32Set::const_iterator it;
   for (uint32_t i = 0; i < record.size(); ++i) {
      assert (record[i] < _nodeList.size()); assert (V3NtkUD != preNode[record[i]]);
      it = _nodeList[record[i]].second.second.begin();
      for (; it != _nodeList[record[i]].second.second.end(); ++it) {
         assert (*it < _nodeList.size());
         if (V3NtkUD == preNode[*it]) {
            preNode[*it] = record[i]; record.push_back(*it); if (t != *it) continue;
            while (s != t) { path.push(getNode(t)); t = preNode[t]; } path.push(node1); return;
         }
      }
   }
}

// Private Graph Algorithms
template<class T> void
V3Graph<T>::topologicalSortIndex(V3IndexFlow& orderList) const {
   assert (orderList.empty());
   V3UI32Queue eqList; V3Graph G(*this);
   while (true) {
      // Delete All Leaves
      typename V3NodeIndexMap::iterator it = G._nodeIndexMap.begin(); assert (eqList.empty());
      while (it != G._nodeIndexMap.end()) {
         if (G.getToSize(it->second)) ++it;
         else { eqList.push(it->second); G._nodeIndexMap.erase(it++); }
      }
      // Record Nodes with Same Topological Order
      if (eqList.empty()) break;
      else orderList.push(eqList);
      // Remove Edges to Leaves
      V3UI32Set::const_iterator is;
      while (!eqList.empty()) {
         const uint32_t node = eqList.front(); eqList.pop();
         for (is = G._nodeList[node].second.first.begin(); is != G._nodeList[node].second.first.end(); ++is) {
            assert (G._nodeList[*is].second.second.end() != G._nodeList[*is].second.second.find(node));
            assert (node != *is); G._nodeList[*is].second.second.erase(G._nodeList[*is].second.second.find(node));
         }
      }
   }
   // Check if Cycle Exists
   if (G._nodeIndexMap.size()) while (!orderList.empty()) orderList.pop();
}

template<class T> void
V3Graph<T>::getStronglyConnectedComponentsIndex(V3IndexField& scc) const {
   // Reference: Tarjan's strongly connected components algorithm
   V3UI32Vec nodeIndex(getNodeSize(), 0), nodeLowlink(getNodeSize(), 0);
   V3BoolVec inStack(getNodeSize(), false); V3UI32Stack nodeStack;
   uint32_t curIndex = 1; assert (!scc.size());
   for (uint32_t i = 0; i < _nodeList.size(); ++i)
      if (!nodeIndex[i] && !isIsolatedNode(i)) 
         strongConnect(i, scc, nodeIndex, nodeLowlink, inStack, nodeStack, curIndex);
}

template<class T> void
V3Graph<T>::strongConnect(const uint32_t& curNode, V3IndexField& scc, V3UI32Vec& nodeIndex, V3UI32Vec& nodeLowlink, V3BoolVec& inStack, V3UI32Stack& nodeStack, uint32_t& curIndex) const {
   // Set Index of curNode to the Smallest Unused Index
   assert (!nodeIndex[curNode]); nodeIndex[curNode] = curIndex; nodeLowlink[curNode] = curIndex;
   assert (!inStack[curNode]); inStack[curNode] = true; nodeStack.push(curNode); ++curIndex;
   // Check its Successors
   V3UI32Set::const_iterator it;
   for (it = _nodeList[curNode].second.second.begin(); it != _nodeList[curNode].second.second.end(); ++it) {
      if (!nodeIndex[*it] && !isIsolatedNode(*it)) {
         strongConnect(*it, scc, nodeIndex, nodeLowlink, inStack, nodeStack, curIndex);
         if (nodeLowlink[*it] < nodeLowlink[curNode]) nodeLowlink[curNode] = nodeLowlink[*it];
      }
      else if (inStack[*it])
         if (nodeIndex[*it] < nodeLowlink[curNode]) nodeLowlink[curNode] = nodeIndex[*it];
   }
   // If curNode is the Root Node, pop nodeStack and Generate an SCC
   if (nodeLowlink[curNode] == nodeIndex[curNode]) {
      uint32_t node; scc.push_back(V3UI32Set());
      while (true) {
         assert (!nodeStack.empty()); node = nodeStack.top();
         scc.back().insert(node); nodeStack.pop();
         inStack[node] = false; if (curNode == node) break;
      }
   }
}

#endif

