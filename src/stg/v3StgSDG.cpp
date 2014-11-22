/****************************************************************************
  FileName     [ v3StgSDG.cpp ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ Sequential Dependency Graph for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_SDG_C
#define V3_STG_SDG_C

#include "v3Msg.h"
#include "v3StgSDG.h"
#include "v3NtkUtil.h"

/* -------------------------------------------------- *\
 * Class V3SDGBase Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3SDGBase::V3SDGBase() {
   _depFFSet.clear(); _depMuxSet.clear(); _depFFCone = 0; _depMuxCone = 0;
}

V3SDGBase::V3SDGBase(const V3SDGBase& node) {
   _depFFSet = node._depFFSet; _depMuxSet = node._depMuxSet; _depFFCone = 0; _depMuxCone = 0;
}

V3SDGBase::~V3SDGBase() {
   if (_depFFCone) delete _depFFCone; if (_depMuxCone) delete _depMuxCone;
   _depFFSet.clear(); _depMuxSet.clear(); _depFFCone = 0; _depMuxCone = 0;
}

// Special Handling Functions
void
V3SDGBase::markDepFF(V3BitVec& marker) const {
   for (V3UI32Set::const_iterator it = _depFFSet.begin(); it != _depFFSet.end(); ++it) marker.set1(*it);
}

void
V3SDGBase::markDepMux(V3BitVec& marker) const {
   for (V3UI32Set::const_iterator it = _depMuxSet.begin(); it != _depMuxSet.end(); ++it) marker.set1(*it);
}

// Print and Debug Functions
void
V3SDGBase::print() const {
   Msg(MSG_IFO) << "DepFF = (";
   for (V3UI32Set::const_iterator it = _depFFSet.begin(); it != _depFFSet.end(); ++it) 
      Msg(MSG_IFO) << (it == _depFFSet.begin() ? "" : ", ") << *it;
   Msg(MSG_IFO) << "); DepMUX = (";
   for (V3UI32Set::const_iterator it = _depMuxSet.begin(); it != _depMuxSet.end(); ++it) 
      Msg(MSG_IFO) << (it == _depMuxSet.begin() ? "" : ", ") << *it;
   Msg(MSG_IFO) << ")" << endl;
}

/* -------------------------------------------------- *\
 * Class V3SDGNode Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3SDGNode::V3SDGNode(const V3NetId& id) : _netId(id), _base(new V3SDGBase()) {
}

V3SDGNode::~V3SDGNode() {
   delete _base;
}

/* -------------------------------------------------- *\
 * Class V3SDGMUX Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3SDGMUX::V3SDGMUX(const V3NetId& id) : V3SDGNode(id), _tBase(new V3SDGBase()), _fBase(new V3SDGBase()) {
}

V3SDGMUX::~V3SDGMUX() {
   delete _tBase; delete _fBase;
}

/* -------------------------------------------------- *\
 * Class V3SDG Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3SDG::V3SDG(V3NtkHandler* const handler) : _handler(handler) {
   assert (_handler); assert (_handler->getNtk());
   // Initialize SDG Node List
   const uint32_t netSize = _handler->getNtk()->getNetSize();
   _nodeList.clear(); _nodeList.reserve(netSize);
   for (uint32_t i = 0; i < netSize; ++i) _nodeList.push_back(reinterpret_cast<V3SDGNode*>(0));
}

V3SDG::~V3SDG() {
   for (uint32_t i = 0; i < _nodeList.size(); ++i) if (_nodeList[i]) delete _nodeList[i];
   _nodeList.clear();
}

// SDG Construction Function
V3SDGBase* const
V3SDG::constructSDG(const V3NetId& id) {
   assert (id.id < _handler->getNtk()->getNetSize());
   V3SDGBase* const node = new V3SDGBase(); assert (node);
   // Collect Dependent FFs
   V3UI32Vec newFF; newFF.clear(); V3NetId ffId;
   for (uint32_t i = 0; i < _handler->getNtk()->getLatchSize(); ++i) {
      ffId = _handler->getNtk()->getLatch(i); if (_nodeList[ffId.id]) continue;
      _nodeList[ffId.id] = new V3SDGNode(ffId); assert (_nodeList[ffId.id]); newFF.push_back(i);
   }
   if (!constructBvSDG(id, node)) { delete node; return 0; }
   for (uint32_t i = 0; i < newFF.size(); ++i) {
      ffId = _handler->getNtk()->getLatch(newFF[i]); assert (_nodeList[ffId.id]);
      V3SDGBase* const ffNode = _nodeList[ffId.id]->getBase(); assert (ffNode);
      if (!constructBvSDG(_handler->getNtk()->getInputNetId(ffId, 0), ffNode)) { delete node; return 0; }
      if (!constructBvSDG(_handler->getNtk()->getInputNetId(ffId, 1), ffNode)) { delete node; return 0; }
   }
   return node;
}

void
V3SDG::collectFaninFF(V3SDGBase* const node, V3SDGBase* const depNode) {
   // This Function Collects and Set all FFs in the Fanin Cone of node to be Dependent FFs of node
   assert (node); V3SDGBase* depFFCone = 0; V3UI32Set::const_iterator is;
   const V3UI32Set& depMuxSet = (depNode) ? depNode->getDepMuxSet() : node->getDepMuxSet();
   for (V3UI32Set::const_iterator it = depMuxSet.begin(); it != depMuxSet.end(); ++it) {
      V3SDGMUX* const depMux = dynamic_cast<V3SDGMUX*>(_nodeList[*it]); assert (depMux);
      // Recursive Collect FaninFF of Select
      depFFCone = depMux->getBase()->getDepFFCone();
      if (!depFFCone) {
         depMux->getBase()->newDepFFCone(); depFFCone = depMux->getBase()->getDepFFCone();
         assert (depFFCone); collectFaninFF(depFFCone, depMux->getBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
      // Recursive Collect FaninFF of True
      depFFCone = depMux->getTBase()->getDepFFCone();
      if (!depFFCone) {
         depMux->getTBase()->newDepFFCone(); depFFCone = depMux->getTBase()->getDepFFCone();
         assert (depFFCone); collectFaninFF(depFFCone, depMux->getTBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
      // Recursive Collect FaninFF of False
      depFFCone = depMux->getFBase()->getDepFFCone();
      if (!depFFCone) {
         depMux->getFBase()->newDepFFCone(); depFFCone = depMux->getFBase()->getDepFFCone();
         assert (depFFCone); collectFaninFF(depFFCone, depMux->getFBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
   }
   if (!depNode) return; const V3UI32Set& depFFSet = depNode->getDepFFSet();
   for (V3UI32Set::const_iterator it = depFFSet.begin(); it != depFFSet.end(); ++it) node->insertFF(*it);
}

void
V3SDG::collectFaninMuxFF(V3SDGBase* const node, V3SDGBase* const depNode) {
   // This Function Collects and Set all FFs in the Fanin Cone of Dependent MUXes of node to be Dependent FFs of node
   assert (node); V3SDGBase* depFFCone = 0; V3UI32Set::const_iterator is;
   const V3UI32Set& depMuxSet = (depNode) ? depNode->getDepMuxSet() : node->getDepMuxSet();
   for (V3UI32Set::const_iterator it = depMuxSet.begin(); it != depMuxSet.end(); ++it) {
      V3SDGMUX* const depMux = dynamic_cast<V3SDGMUX*>(_nodeList[*it]); assert (depMux);
      // Recursive Collect FaninMuxFF of Select
      depFFCone = depMux->getBase()->getDepMuxCone();
      if (!depFFCone) {
         depMux->getBase()->newDepMuxCone(); depFFCone = depMux->getBase()->getDepMuxCone();
         assert (depFFCone); collectFaninFF(depFFCone, depMux->getBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
      // Recursive Collect FaninMuxFF of True
      depFFCone = depMux->getTBase()->getDepMuxCone();
      if (!depFFCone) {
         depMux->getTBase()->newDepMuxCone(); depFFCone = depMux->getTBase()->getDepMuxCone();
         assert (depFFCone); collectFaninMuxFF(depFFCone, depMux->getTBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
      // Recursive Collect FaninMuxFF of False
      depFFCone = depMux->getFBase()->getDepMuxCone();
      if (!depFFCone) {
         depMux->getFBase()->newDepMuxCone(); depFFCone = depMux->getFBase()->getDepMuxCone();
         assert (depFFCone); collectFaninMuxFF(depFFCone, depMux->getFBase());
      }
      for (is = depFFCone->getDepFFSet().begin(); is != depFFCone->getDepFFSet().end(); ++is) node->insertFF(*is);
   }
}

// Private SDG Construction Function
const bool
V3SDG::constructBvSDG(const V3NetId& id, V3SDGBase* const n) {
   assert (id.id < _handler->getNtk()->getNetSize()); assert (n);
   const V3GateType type = _handler->getNtk()->getGateType(id); assert (V3_XD > type);
   if (V3_FF == type) {
      n->insertFF(id.id);  // Record FF
      assert (_nodeList[id.id]);
      if (!_nodeList[id.id]) {
         V3SDGNode* const node = new V3SDGNode(id); assert (node); _nodeList[id.id] = node;
         if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, 0), node->getBase())) return false;
         if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, 1), node->getBase())) return false;
      }
   }
   else if (BV_MUX == type) {
      n->insertMux(id.id);  // Record MUX
      if (!_nodeList[id.id]) {
         // NOTE: The _netId of V3SDGMUX is the NetId of the Select Signal, NOT MUX Output Signal !!
         V3SDGMUX* const mux = new V3SDGMUX(_handler->getNtk()->getInputNetId(id, 2));
         assert (mux); _nodeList[id.id] = mux;
         if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, 2), mux->getBase())) return false;
         if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, 1), mux->getTBase())) return false;
         if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, 0), mux->getFBase())) return false;
      }
   }
   else if (V3_FF > type || BV_CONST == type) return true;
   else if (V3_MODULE == type) return false;
   else {
      assert (AIG_NODE != type && AIG_FALSE != type);
      if (!_nodeList[id.id]) {
         V3SDGNode* const node = new V3SDGNode(id); assert (node); _nodeList[id.id] = node;
         for (uint32_t i = 0, j = isV3PairType(type) ? 2 : 1; i < j; ++i)
            if (!constructBvSDG(_handler->getNtk()->getInputNetId(id, i), node->getBase())) return false;
      }
      // Put SDG into Parent SDG
      const V3UI32Set& depFFSet = _nodeList[id.id]->getBase()->getDepFFSet();
      for (V3UI32Set::const_iterator it = depFFSet.begin(); it != depFFSet.end(); ++it) n->insertFF(*it);
      const V3UI32Set& depMuxSet = _nodeList[id.id]->getBase()->getDepMuxSet();
      for (V3UI32Set::const_iterator it = depMuxSet.begin(); it != depMuxSet.end(); ++it) n->insertMux(*it);
   }
   return true;
}

/* -------------------------------------------------- *\
 * Class V3AigSDG Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AigSDG::V3AigSDG(V3NtkHandler* const handler) : V3SDG(handler) {
   assert (!dynamic_cast<V3BvNtk*>(_handler->getNtk()));
   _muxCandidate.clear(); _mexCandidate.clear(); identifyCandidates();
   assert (_muxCandidate.size()); assert (_mexCandidate.size());
}

// Private SDG Construction Functions
void
V3AigSDG::identifyCandidates() {
   assert (!_muxCandidate.size()); assert (!_mexCandidate.size());
   // Compute Order List (follows simulation)
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3NetVec orderMap; dfsNtkForSimulationOrder(ntk, orderMap);
   V3NetTable fanoutTable; computeFanout(ntk, fanoutTable);
   // Initialize Tables
   _muxCandidate = V3UI32Vec(ntk->getNetSize(), V3NtkUD);
   _mexCandidate = V3BoolVec(ntk->getNetSize(), false);
   uint32_t bound = ntk->getConstSize() + ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize();
   uint32_t i = orderMap.size(), j, k; assert (orderMap.size());
   V3NetId id, id1, id2, ida1, ida2, idb1, idb2;
   while (i-- > bound) {
      id = orderMap[i]; assert (AIG_NODE == ntk->getGateType(id));
      // Identify Mutex Candidates
      k = fanoutTable[id.id].size();
      if (k) {
         // NO Confirmation : Usually Not Practical ...
         //_mexCandidate[id.id] = true;
         // Moderate Confirmation : Set if all fanouts are MUX candidates
         for (j = 0; j < k; ++j) if (V3NtkUD == _muxCandidate[fanoutTable[id.id][j].id]) break;
         if (fanoutTable[id.id].size() == j) _mexCandidate[id.id] = true;
         // Erodic Confirmation : Set if the only fanout is a MUX candidate
         //if ((!(--k)) && (V3NtkUD != _muxCandidate[fanoutTable[id.id][k].id])) _mexCandidate[id.id] = true;
      }
      // Identify MUX Candidates
      id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
      id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
      if (!id1.cp || AIG_NODE != ntk->getGateType(id1)) continue;
      if (!id2.cp || AIG_NODE != ntk->getGateType(id2)) continue;
      ida1 = ntk->getInputNetId(id1, 0); assert (ida1.id < ntk->getNetSize());
      ida2 = ntk->getInputNetId(id1, 1); assert (ida2.id < ntk->getNetSize());
      idb1 = ntk->getInputNetId(id2, 0); assert (idb1.id < ntk->getNetSize());
      idb2 = ntk->getInputNetId(id2, 1); assert (idb2.id < ntk->getNetSize());
      if (ida1.id == idb1.id && ida1.cp != idb1.cp) _muxCandidate[id.id] = (ida1.id << 2);
      else if (ida1.id == idb2.id && ida1.cp != idb2.cp) _muxCandidate[id.id] = (ida1.id << 2) | 2ul;
      else if (ida2.id == idb1.id && ida2.cp != idb1.cp) _muxCandidate[id.id] = (ida2.id << 2) | 1ul;
      else if (ida2.id == idb2.id && ida2.cp != idb2.cp) _muxCandidate[id.id] = (ida2.id << 2) | 3ul;
   }
   assert (AIG_NODE != ntk->getGateType(orderMap[i]));
}

// SDG Construction Function
V3SDGBase* const
V3AigSDG::constructSDG(const V3NetId& id) {
   assert (id.id < _handler->getNtk()->getNetSize());
   V3SDGBase* const node = new V3SDGBase(); assert (node);
   // Collect Dependent FFs
   V3UI32Vec newFF; newFF.clear(); V3NetId ffId;
   for (uint32_t i = 0; i < _handler->getNtk()->getLatchSize(); ++i) {
      ffId = _handler->getNtk()->getLatch(i); if (_nodeList[ffId.id]) continue;
      _nodeList[ffId.id] = new V3SDGNode(ffId); assert (_nodeList[ffId.id]); newFF.push_back(i);
   }
   if (!constructAigSDG(id, node)) { delete node; return 0; }
   for (uint32_t i = 0; i < newFF.size(); ++i) {
      ffId = _handler->getNtk()->getLatch(newFF[i]); assert (_nodeList[ffId.id]);
      V3SDGBase* const ffNode = _nodeList[ffId.id]->getBase(); assert (ffNode);
      if (!constructAigSDG(_handler->getNtk()->getInputNetId(ffId, 0), ffNode)) { delete node; return 0; }
      if (!constructAigSDG(_handler->getNtk()->getInputNetId(ffId, 1), ffNode)) { delete node; return 0; }
   }
   return node;
}
const bool
V3AigSDG::constructAigSDG(const V3NetId& id, V3SDGBase* const n) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (id.id < ntk->getNetSize()); assert (n);
   const V3GateType type = ntk->getGateType(id); assert (AIG_FALSE >= type);
   if (V3_FF == type) {
      n->insertFF(id.id);  // Record FF
      assert (_nodeList[id.id]);
      if (!_nodeList[id.id]) {
         V3SDGNode* const node = new V3SDGNode(id); assert (node); _nodeList[id.id] = node;
         if (!constructAigSDG(ntk->getInputNetId(id, 0), node->getBase())) return false;
         if (!constructAigSDG(ntk->getInputNetId(id, 1), node->getBase())) return false;
      }
   }
   else if (AIG_NODE == type) {
      // Confirm it it is a MUX
      if (V3NtkUD != _muxCandidate[id.id]) {  // Candidate
         const V3NetId muxNetId = V3NetId::makeNetId(_muxCandidate[id.id] >> 2);
         V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.cp && AIG_NODE == ntk->getGateType(id1));
         V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.cp && AIG_NODE == ntk->getGateType(id2));
         if (_mexCandidate[id1.id] && _mexCandidate[id2.id]) {  // Real MUX
            n->insertMux(id.id);  // Record MUX
            if (!_nodeList[id.id]) {
               // NOTE: The _netId of V3SDGMUX is the NetId of the Select Signal, NOT MUX Output Signal !!
               V3SDGMUX* const mux = new V3SDGMUX(muxNetId); assert (mux); _nodeList[id.id] = mux;
               if (!constructAigSDG(muxNetId, mux->getBase())) return false;
               // Identify True and False Nets
               assert (muxNetId.id == ntk->getInputNetId(id1, (_muxCandidate[id.id] & 1ul) ? 1 : 0).id);
               assert (muxNetId.id == ntk->getInputNetId(id2, (_muxCandidate[id.id] & 2ul) ? 1 : 0).id);
               assert (ntk->getInputNetId(id1, (_muxCandidate[id.id] & 1ul) ? 1 : 0).cp ^ 
                       ntk->getInputNetId(id2, (_muxCandidate[id.id] & 2ul) ? 1 : 0).cp);
               if (muxNetId == ntk->getInputNetId(id1, (_muxCandidate[id.id] & 1ul) ? 1 : 0)) {
                  id1 = ntk->getInputNetId(id1, (_muxCandidate[id.id] & 1ul) ? 0 : 1);
                  id2 = ntk->getInputNetId(id2, (_muxCandidate[id.id] & 2ul) ? 0 : 1);
                  if (!constructAigSDG(id1, mux->getTBase())) return false;
                  if (!constructAigSDG(id2, mux->getFBase())) return false;
               }
               else {
                  assert (muxNetId == ntk->getInputNetId(id2, (_muxCandidate[id.id] & 2ul) ? 1 : 0));
                  id1 = ntk->getInputNetId(id1, (_muxCandidate[id.id] & 1ul) ? 0 : 1);
                  id2 = ntk->getInputNetId(id2, (_muxCandidate[id.id] & 2ul) ? 0 : 1);
                  if (!constructAigSDG(id2, mux->getTBase())) return false;
                  if (!constructAigSDG(id1, mux->getFBase())) return false;
               }
            }
            return true;
         }
      }
      if (!_nodeList[id.id]) {
         V3SDGNode* const node = new V3SDGNode(id); assert (node); _nodeList[id.id] = node;
         if (!constructAigSDG(ntk->getInputNetId(id, 0), node->getBase())) return false;
         if (!constructAigSDG(ntk->getInputNetId(id, 1), node->getBase())) return false;
         // Put SDG into Parent SDG
         const V3UI32Set& depFFSet = _nodeList[id.id]->getBase()->getDepFFSet();
         for (V3UI32Set::const_iterator it = depFFSet.begin(); it != depFFSet.end(); ++it) n->insertFF(*it);
         const V3UI32Set& depMuxSet = _nodeList[id.id]->getBase()->getDepMuxSet();
         for (V3UI32Set::const_iterator it = depMuxSet.begin(); it != depMuxSet.end(); ++it) n->insertMux(*it);
      }
   }
   else if (V3_MODULE == type) return false;
   return true;
}

#endif

