/****************************************************************************
  FileName     [ v3NtkTraverse.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Generic Functions for V3 Ntk Traversal. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_TRAVERSE_C
#define V3_NTK_TRAVERSE_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"

// General Fanout Computation Functions for V3 Ntk
void dfsComputeFanout(V3Ntk* const ntk, const V3NetId& pId, V3BoolVec& m, V3NetTable& outputTable) {
   assert (ntk); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId); V3BoolVec t(ntk->getNetSize(), false);
   V3NetId id; V3GateType type; uint32_t inSize;
   // Traverse Fanin Logics
   while (s.size()) {
      id = s.top(); assert (id.id < m.size()); if (m[id.id]) { s.pop(); continue; }
      type = ntk->getGateType(id); assert (V3_XD > type);
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(id) == 1); assert (ntk->getModule(id));
         const V3NetVec& inputs = ntk->getModule(id)->getInputList(); inSize = inputs.size();
         if (t[id.id]) {
            while (inSize--) outputTable[inputs[inSize].id].push_back(id);
            m[id.id] = true; s.pop(); continue;
         }
         while (inSize--) if (!t[inputs[inSize].id]) s.push(inputs[inSize]);
      }
      else {
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize;
         if (t[id.id]) {
            while (inSize--) outputTable[ntk->getInputNetId(id, inSize).id].push_back(id);
            m[id.id] = true; s.pop(); continue;
         }
         while (inSize--) if (!t[ntk->getInputNetId(id, inSize).id]) s.push(ntk->getInputNetId(id, inSize));
      }
      t[id.id] = true;
   }
}

void computeFanout(V3Ntk* const ntk, V3NetTable& outputTable, const V3NetVec& targetNets) {
   assert (ntk); outputTable.clear(); outputTable.reserve(ntk->getNetSize());
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) outputTable.push_back(V3NetVec());
   V3BoolVec m(ntk->getNetSize(), false);
   // Set Latest Misc Data on (Pseudo) PI / PIO / Const
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) m[ntk->getInput(i).id] = true;
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) m[ntk->getInout(i).id] = true;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) m[ntk->getLatch(i).id] = true;
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) m[ntk->getConst(i).id] = true;
   // DFS Compute Fanout List From (Pseudo) PO / PIO
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      dfsComputeFanout(ntk, ntk->getInputNetId(ntk->getLatch(i), 0), m, outputTable);
      dfsComputeFanout(ntk, ntk->getInputNetId(ntk->getLatch(i), 1), m, outputTable);
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i)
      dfsComputeFanout(ntk, ntk->getInputNetId(ntk->getInout(i), 0), m, outputTable);
   if (targetNets.size())
      for (uint32_t i = 0; i < targetNets.size(); ++i) dfsComputeFanout(ntk, targetNets[i], m, outputTable);
   else
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) dfsComputeFanout(ntk, ntk->getOutput(i), m, outputTable);
}

// General Level Computation Functions for V3 Ntk
void dfsComputeLevel(V3Ntk* const ntk, V3NetVec& orderMap, V3UI32Vec& levelData) {
   assert (ntk); assert (orderMap.size() <= ntk->getNetSize());
   assert (levelData.size() == ntk->getNetSize());
   // Set Net Levels According to the DFS Order
   V3NetId id, id1; V3GateType type; uint32_t inSize;
   for (uint32_t i = 0; i < orderMap.size(); ++i) {
      id = orderMap[i]; assert (id.id < levelData.size());
      if (V3NtkUD != levelData[id.id]) continue; levelData[id.id] = 0;
      type = ntk->getGateType(id); assert (V3_XD > type); assert (V3_PI == type || V3_FF < type);
      if (V3_MODULE == type) {
         V3NtkModule* const moduleNtk = ntk->getModule(id); assert (moduleNtk);
         const V3NetVec& inputs = moduleNtk->getInputList(); inSize = inputs.size();
         while (inSize--) {
            id1 = inputs[inSize]; assert (V3NtkUD != levelData[id1.id]);
            if (levelData[id.id] < levelData[id1.id]) levelData[id.id] = levelData[id1.id];
         }
      }
      else {
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize;
         while (inSize--) {
            id1 = ntk->getInputNetId(id, inSize); assert (V3NtkUD != levelData[id1.id]);
            if (levelData[id.id] < levelData[id1.id]) levelData[id.id] = levelData[id1.id];
         }
      }
      ++levelData[id.id];
   }
}

const uint32_t computeLevel(V3Ntk* const ntk, V3UI32Vec& levelData, const V3NetVec& targetNets) {
   assert (ntk); levelData = V3UI32Vec(ntk->getNetSize(), V3NtkUD);
   // Set Level 0 on (Pseudo) PI / PIO / Const
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) levelData[ntk->getInput(i).id] = 0;
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) levelData[ntk->getInout(i).id] = 0;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) levelData[ntk->getLatch(i).id] = 0;
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) levelData[ntk->getConst(i).id] = 0;
   // Compute General DFS Order
   V3NetId id; V3NetVec orderMap; uint32_t levelSize = 0;
   dfsNtkForGeneralOrder(ntk, orderMap, targetNets);
   // Compute Net Levels
   dfsComputeLevel(ntk, orderMap, levelData);
   // Update Global Level
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      id = ntk->getInputNetId(ntk->getLatch(i), 0); assert (V3NtkUD != levelData[id.id]);
      if (levelData[id.id] > levelSize) levelSize = levelData[id.id];
      id = ntk->getInputNetId(ntk->getLatch(i), 1); assert (V3NtkUD != levelData[id.id]);
      if (levelData[id.id] > levelSize) levelSize = levelData[id.id];
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      id = ntk->getInputNetId(ntk->getInout(i), 0); assert (V3NtkUD != levelData[id.id]);
      if (levelData[id.id] > levelSize) levelSize = levelData[id.id];
   }
   if (targetNets.size()) {
      for (uint32_t i = 0; i < targetNets.size(); ++i) {
         id = targetNets[i]; assert (V3NtkUD != levelData[id.id]);
         if (levelData[id.id] > levelSize) levelSize = levelData[id.id];
      }
   }
   else {
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
         id = ntk->getOutput(i); assert (V3NtkUD != levelData[id.id]);
         if (levelData[id.id] > levelSize) levelSize = levelData[id.id];
      }
   }
   ++levelSize; return levelSize;
}

// General DFS Traversal Recursive Functions for V3 Ntk
void dfsGeneralOrder(V3Ntk* const ntk, const V3NetId& pId, V3BoolVec& m, V3NetVec& orderMap) {
   assert (ntk); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId); V3BoolVec t(ntk->getNetSize(), false);
   V3NetId id; V3GateType type; uint32_t inSize;
   // Traverse Fanin Logics
   while (s.size()) {
      id = s.top(); assert (id.id < m.size()); if (m[id.id]) { s.pop(); continue; }
      type = ntk->getGateType(id); assert (V3_XD > type);
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(id) == 1); assert (ntk->getModule(id));
         orderMap.push_back(id); m[id.id] = t[id.id] = true; s.pop();
         const V3NetVec& inputs = ntk->getModule(id)->getInputList(); inSize = inputs.size();
         while (inSize--) if (!t[inputs[inSize].id]) s.push(inputs[inSize]);
      }
      else {
         if (t[id.id]) { orderMap.push_back(id); m[id.id] = true; s.pop(); continue; }
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize; t[id.id] = true;
         while (inSize--) if (!t[ntk->getInputNetId(id, inSize).id]) s.push(ntk->getInputNetId(id, inSize));
      }
   }
}

void dfsSimulationOrder(V3Ntk* const ntk, const V3NetId& pId, V3BoolVec& m, V3NetVec& orderMap) {
   assert (ntk); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId); V3BoolVec t(ntk->getNetSize(), false);
   V3NetId id; V3GateType type; uint32_t inSize;
   // Traverse Fanin Logics
   while (s.size()) {
      id = s.top(); assert (id.id < m.size()); if (m[id.id]) { s.pop(); continue; }
      type = ntk->getGateType(id); assert (V3_XD > type);
      assert (V3_PI == type || (V3_MODULE < type && AIG_FALSE != type && BV_CONST != type));
      if (t[id.id]) { orderMap.push_back(id); m[id.id] = true; s.pop(); continue; }
      inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize; t[id.id] = true;
      while (inSize--) if (!t[ntk->getInputNetId(id, inSize).id]) s.push(ntk->getInputNetId(id, inSize));
   }
}

// General DFS Fanin Cone Marking Functions for V3 Ntk
void dfsMarkFaninCone(V3Ntk* const ntk, const V3NetId& pId, V3BoolVec& m) {
   assert (ntk); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId);
   // Traverse Fanin Logics
   V3NetId id; V3GateType type; uint32_t inSize;
   while (s.size()) {
      id = s.top(); s.pop(); assert (id.id < m.size()); if (m[id.id]) continue;
      type = ntk->getGateType(id); assert (V3_XD > type); m[id.id] = true;
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(id) == 1); assert (ntk->getModule(id));
         const V3NetVec& inputs = ntk->getModule(id)->getInputList();
         inSize = inputs.size(); while (inSize--) s.push(inputs[inSize]);
      }
      else {
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize;
         while (inSize--) s.push(ntk->getInputNetId(id, inSize));
      }
   }
   m[id.id] = true;
}

void dfsMarkFaninCone(V3Ntk* const ntk, const V3NetId& pId, const V3BoolVec& insensitiveList, V3BoolVec& m) {
   assert (ntk); assert (insensitiveList.size() == ntk->getNetSize()); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId);
   // Traverse Fanin Logics
   V3NetId id; V3GateType type; uint32_t inSize;
   while (s.size()) {
      id = s.top(); s.pop(); assert (id.id < m.size()); if (m[id.id] || !insensitiveList[id.id]) continue;
      type = ntk->getGateType(id); assert (V3_XD > type); m[id.id] = true;
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(id) == 1); assert (ntk->getModule(id));
         const V3NetVec& inputs = ntk->getModule(id)->getInputList();
         inSize = inputs.size(); while (inSize--) s.push(inputs[inSize]);
      }
      else {
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize;
         while (inSize--) s.push(ntk->getInputNetId(id, inSize));
      }
   }
   m[id.id] = true;
}

void dfsMarkFaninCone(V3Ntk* const ntk, const V3NetTable& idGroup, V3BoolTable& v) {
   assert (ntk); assert (idGroup.size()); v.clear(); v.reserve(ntk->getNetSize());
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) v.push_back(V3BoolVec(idGroup.size(), false));
   // Record Latch Markers
   V3BoolTable ff(ntk->getLatchSize(), V3BoolVec(idGroup.size(), false));
   // Set Marker for Each idGroup
   for (uint32_t i = 0; i < idGroup.size(); ++i)
      for (uint32_t j = 0; j < idGroup[i].size(); ++j) v[idGroup[i][j].id][i] = true;
   // Propagate the Marker to Primary Inputs and Latches
   V3NetVec orderMap, targetNets; targetNets.clear();
   for (uint32_t i = 0; i < idGroup.size(); ++i)
      targetNets.insert(targetNets.end(), idGroup[i].begin(), idGroup[i].end());
   while (targetNets.size()) {
      // Make a DFS Order of Nets
      dfsNtkForGeneralOrder(ntk, orderMap, targetNets, false);
      V3NetTable outputTable; computeFanout(ntk, outputTable, targetNets);
      // Update Markers In Reversed DFS Order
      uint32_t i = orderMap.size();
      while (i--) {
         for (uint32_t j = 0; j < outputTable[orderMap[i].id].size(); ++j) {
            for (uint32_t k = 0; k < idGroup.size(); ++k)
               if (v[outputTable[orderMap[i].id][j].id][k]) v[orderMap[i].id][k] = true;
         }
      }
      // Check if Any Latches Are Updated
      targetNets.clear();
      for (i = 0; i < ff.size(); ++i) {
         if (ff[i] == v[ntk->getLatch(i).id]) continue; ff[i] = v[ntk->getLatch(i).id];
         targetNets.push_back(ntk->getInputNetId(ntk->getLatch(i), 0));
         targetNets.push_back(ntk->getInputNetId(ntk->getLatch(i), 1));
      }
   }
}

// General DFS Traversal Functions for V3 Ntk
const uint32_t dfsNtkForGeneralOrder(V3Ntk* const ntk, V3NetVec& orderMap, const V3NetVec& targetNets, const bool& allNets) {
   assert (ntk); V3BoolVec m(ntk->getNetSize(), false);
   orderMap.clear(); orderMap.reserve(ntk->getNetSize());
   orderMap.push_back(V3NetId::makeNetId(0));  // AIG_FALSE or BV_CONST (1'b0)
   // (Pseudo) Primary Inputs
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) orderMap.push_back(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) orderMap.push_back(ntk->getInout(i));
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) orderMap.push_back(ntk->getLatch(i));
   for (uint32_t i = 0; i < orderMap.size(); ++i) m[orderMap[i].id] = true;
   // Pseudo Primary Input Initial State Logics
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i)
      dfsGeneralOrder(ntk, ntk->getInputNetId(ntk->getLatch(i), 1), m, orderMap);
   // Record End of Initial Logic if Needed  (e.g. simulator)
   const uint32_t initEndIndex = orderMap.size();
   // (Pseudo) Primary Output Fanin Logics
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
      dfsGeneralOrder(ntk, ntk->getInputNetId(ntk->getLatch(i), 0), m, orderMap);
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
      dfsGeneralOrder(ntk, ntk->getInputNetId(ntk->getInout(i), 0), m, orderMap);
   if (targetNets.size())
      for (uint32_t i = 0; i < targetNets.size(); ++i) dfsGeneralOrder(ntk, targetNets[i], m, orderMap);
   else
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) dfsGeneralOrder(ntk, ntk->getOutput(i), m, orderMap);
   // Put Nets Not in COI into orderMap
   if (allNets)
      for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id)
         if (!m[id.id]) dfsGeneralOrder(ntk, id, m, orderMap);
   assert (orderMap.size() <= ntk->getNetSize()); return initEndIndex;
}

const uint32_t dfsNtkForSimulationOrder(V3Ntk* const ntk, V3NetVec& orderMap, const V3NetVec& targetNets, const bool& allNets) {
   assert (ntk); V3BoolVec m(ntk->getNetSize(), false);
   orderMap.clear(); orderMap.reserve(ntk->getNetSize());
   // Constants
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) orderMap.push_back(ntk->getConst(i));
   // (Pseudo) Primary Inputs
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) orderMap.push_back(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) orderMap.push_back(ntk->getInout(i));
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) orderMap.push_back(ntk->getLatch(i));
   for (uint32_t i = 0; i < orderMap.size(); ++i) m[orderMap[i].id] = true;
   // Pseudo Primary Input Initial State Logics
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
      dfsSimulationOrder(ntk, ntk->getInputNetId(ntk->getLatch(i), 1), m, orderMap);
   // Record End of Initial Logic if Needed  (e.g. simulator)
   const uint32_t initEndIndex = orderMap.size();
   // (Pseudo) Primary Output Fanin Logics
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
      dfsSimulationOrder(ntk, ntk->getInputNetId(ntk->getLatch(i), 0), m, orderMap);
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
      dfsSimulationOrder(ntk, ntk->getInputNetId(ntk->getInout(i), 0), m, orderMap);
   if (targetNets.size())
      for (uint32_t i = 0; i < targetNets.size(); ++i) dfsSimulationOrder(ntk, targetNets[i], m, orderMap);
   else
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) dfsSimulationOrder(ntk, ntk->getOutput(i), m, orderMap);
   // Put Nets Not in COI into orderMap
   if (allNets)
      for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id)
         if (!m[id.id]) dfsSimulationOrder(ntk, id, m, orderMap);
   assert (orderMap.size() <= ntk->getNetSize()); return initEndIndex;
}

const uint32_t dfsNtkForReductionOrder(V3Ntk* const ntk, V3NetVec& orderMap, const V3NetVec& targetNets, const bool& reduceLatch) {
   assert (ntk); V3BoolVec m(ntk->getNetSize(), false);
   orderMap.clear(); orderMap.reserve(ntk->getNetSize());
   orderMap.push_back(V3NetId::makeNetId(0));  // AIG_FALSE or BV_CONST (1'b0)
   // Mark Fanin Cone of targetNets Recursively  (Through FF Boundaries)
   if (targetNets.size()) for (uint32_t i = 0; i < targetNets.size(); ++i) dfsMarkFaninCone(ntk, targetNets[i], m);
   else for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) dfsMarkFaninCone(ntk, ntk->getOutput(i), m);
   // (Pseudo) Primary Inputs
   V3NetVec latchList; latchList.clear(); latchList.reserve(ntk->getLatchSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) orderMap.push_back(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) orderMap.push_back(ntk->getInout(i));
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
      if (!reduceLatch || m[ntk->getLatch(i).id]) {
         orderMap.push_back(ntk->getLatch(i)); latchList.push_back(ntk->getLatch(i)); }
   // DFS Traverse Marked Nets
   for (uint32_t i = 0; i < m.size(); ++i) m[i] = false;
   for (uint32_t i = 0; i < orderMap.size(); ++i) m[orderMap[i].id] = true;
   // Pseudo Primary Input Initial State Logics
   for (uint32_t i = 0; i < latchList.size(); ++i) 
      dfsGeneralOrder(ntk, ntk->getInputNetId(latchList[i], 1), m, orderMap);
   // Record End of Initial Logic if Needed  (e.g. simulator)
   const uint32_t initEndIndex = orderMap.size();
   // (Pseudo) Primary Output Fanin Logics
   for (uint32_t i = 0; i < latchList.size(); ++i) 
      dfsGeneralOrder(ntk, ntk->getInputNetId(latchList[i], 0), m, orderMap);
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
      dfsGeneralOrder(ntk, ntk->getInputNetId(ntk->getInout(i), 0), m, orderMap);
   if (targetNets.size()) for (uint32_t i = 0; i < targetNets.size(); ++i) dfsGeneralOrder(ntk, targetNets[i], m, orderMap);
   else for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) dfsGeneralOrder(ntk, ntk->getOutput(i), m, orderMap);
   assert (orderMap.size() <= ntk->getNetSize()); return initEndIndex;
}

// General BFS Fanin Cone Indexing Recursive Functions for V3 Ntk
void bfsIndexFaninConeFF(V3Ntk* const ntk, V3NetVec& ffList, const V3NetId& pId, V3BoolVec& m) {
   assert (ntk); assert (m.size() == ntk->getNetSize());
   V3Stack<V3NetId>::Stack s; s.push(pId);
   // Traverse Fanin Logics
   V3NetId id; V3GateType type; uint32_t inSize;
   while (s.size()) {
      id = s.top(); s.pop(); assert (id.id < m.size()); if (m[id.id]) continue;
      type = ntk->getGateType(id); assert (V3_XD > type); m[id.id] = true;
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(id) == 1); assert (ntk->getModule(id));
         const V3NetVec& inputs = ntk->getModule(id)->getInputList();
         inSize = inputs.size(); while (inSize--) s.push(inputs[inSize]);
      }
      else if (V3_FF == type) ffList.push_back(id);
      else {
         inSize = ntk->getInputNetSize(id); if (BV_SLICE == type || BV_CONST == type) --inSize;
         while (inSize--) s.push(ntk->getInputNetId(id, inSize));
      }
   }
   m[id.id] = true;
}

// General BFS Fanin Cone Indexing Functions for V3 Ntk
void bfsIndexFaninConeFF(V3Ntk* const ntk, V3NetVec& ffList, const V3NetVec& sourceNets) {
   assert (ntk); V3BoolVec m(ntk->getNetSize(), false); ffList.clear();
   V3NetVec source = sourceNets; uint32_t end = 0;
   while (true) {
      for (uint32_t i = 0; i < source.size(); ++i) bfsIndexFaninConeFF(ntk, ffList, source[i], m);
      if (end == ffList.size()) break; assert (end < ffList.size());
      source.clear(); source.reserve(ffList.size() - end);
      while (end != ffList.size()) {
         source.push_back(ntk->getInputNetId(ffList[end], 0));
         source.push_back(ntk->getInputNetId(ffList[end], 1));
         ++end;
      }
   }
}

#endif

