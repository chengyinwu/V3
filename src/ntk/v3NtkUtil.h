/****************************************************************************
  FileName     [ v3NtkUtil.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Generic Utility Functions for V3 Ntk Manipulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_UTIL_H
#define V3_NTK_UTIL_H

#include "v3NtkHandler.h"

/* GENERAL V3 NTK CONSTRUCTION FUNCTIONS : Implemented in v3NtkConstruct.cpp */
// General Construction Functions for V3 Ntk
V3Ntk* const createV3Ntk(const bool&);
V3Ntk* const copyV3Ntk(V3Ntk* const&);
const bool replaceV3Ntk(V3Ntk* const, V3Ntk* const);
const V3NetId createNet(V3Ntk* const, uint32_t = 1);
const bool createInput(V3Ntk* const, const V3NetId&);
const bool createOutput(V3Ntk* const, const V3NetId&);
const bool createInout(V3Ntk* const, const V3NetId&);
// General Gate Type Construction Functions for V3 Ntk
const bool createV3BufGate(V3Ntk* const, const V3NetId&, const V3NetId&);
const bool createV3FFGate(V3Ntk* const, const V3NetId&, const V3NetId&, const V3NetId&);
const bool createV3AndGate(V3Ntk* const, const V3NetId&, const V3NetId&, const V3NetId&);
// General Gate Type Construction Functions for AIG Ntk
const bool createAigAndGate(V3AigNtk* const, const V3NetId&, const V3NetId&, const V3NetId&);
const bool createAigFalseGate(V3AigNtk* const, const V3NetId&);
// General Gate Type Construction Functions for BV Ntk
const bool createBvReducedGate(V3BvNtk* const, const V3GateType&, const V3NetId&, const V3NetId&);
const bool createBvPairGate(V3BvNtk* const, const V3GateType&, const V3NetId&, const V3NetId&, const V3NetId&);
const bool createBvMuxGate(V3BvNtk* const, const V3NetId&, const V3NetId&, const V3NetId&, const V3NetId&);
const bool createBvSliceGate(V3BvNtk* const, const V3NetId&, const V3NetId&, uint32_t, uint32_t);
const bool createBvConstGate(V3BvNtk* const, const V3NetId&, const string&);
// General Utilities for the Construction of Bit-Width Matching Ntk
const V3NetId createNetExtensionGate(V3BvNtk* const, const V3NetId&, const uint32_t&);
const bool createIOExtensionGate(V3BvNtk* const, const V3NetId&, const V3NetId&);
// General Ntk Topology Validation Functions for V3 Ntk
const uint32_t reportCombinationalLoops(V3Ntk* const);
// General Hierarchical Ntk Construction Functions for V3 Ntk
const bool createModule(V3Ntk* const, const V3NetVec&, const V3NetVec&, V3NtkHandler* const, const bool& = false);
const bool updateModule(V3Ntk* const, V3NtkModule* const, V3NtkHandler* const, const bool& = false);
const bool reportInconsistentRefNtk(V3Ntk* const);
const bool reportIncompatibleModule(V3NtkHandler* const, V3NtkHandler* const);
const bool reportIncompatibleModule(V3Ntk* const, const V3NetVec&, const V3NetVec&, V3NtkHandler* const);

/* GENERAL V3 NTK DUPLICATION FUNCTIONS : Implemented in v3NtkDuplicate.cpp */
// General Duplication Functions for V3 Ntk  (Note that PO will NOT be created !!)
V3Ntk* const duplicateNtk(V3NtkHandler* const, V3NetVec&, V3NetVec&);
V3Ntk* const elaborateNtk(V3NtkHandler* const, const V3NetVec&, V3NetVec&, V3NetVec&, V3PortableType&);
V3Ntk* const flattenNtk(V3NtkHandler* const, V3NetVec&, V3NtkHierInfo&, const uint32_t&);
V3Ntk* const attachToNtk(V3NtkHandler* const, V3Ntk* const, const V3NetVec&, V3NetVec&, V3NetVec&, V3PortableType&);
// General Gate Type Elaboration Functions for V3 Ntk
const V3NetId elaborateAigGate(V3AigNtk* const, const V3GateType&, V3InputVec&, V3PortableType&);
const V3NetId elaborateBvGate(V3BvNtk* const, const V3GateType&, V3InputVec&, V3PortableType&);
const V3NetId elaborateAigAndOrAndGate(V3AigNtk* const, V3InputVec&, V3PortableType&);

/* GENERAL V3 NTK TRAVERSAL FUNCTIONS : Implemented in v3NtkTraverse.cpp */
// General Fanout Computation Functions for V3 Ntk
void computeFanout(V3Ntk* const, V3NetTable&, const V3NetVec& = V3NetVec());
// General Level Computation Functions for V3 Ntk
const uint32_t computeLevel(V3Ntk* const, V3UI32Vec&, const V3NetVec& = V3NetVec());
// General DFS Traversal Functions for V3 Ntk
const uint32_t dfsNtkForGeneralOrder(V3Ntk* const, V3NetVec&, const V3NetVec& = V3NetVec(), const bool& = false);
const uint32_t dfsNtkForSimulationOrder(V3Ntk* const, V3NetVec&, const V3NetVec& = V3NetVec(), const bool& = false);
const uint32_t dfsNtkForReductionOrder(V3Ntk* const, V3NetVec&, const V3NetVec& = V3NetVec(), const bool& = false);
// General DFS Fanin Cone Marking Functions for V3 Ntk
void dfsMarkFaninCone(V3Ntk* const, const V3NetId&, V3BoolVec&);
void dfsMarkFaninCone(V3Ntk* const, const V3NetId&, const V3BoolVec&, V3BoolVec&);
void dfsMarkFaninCone(V3Ntk* const, const V3NetTable&, V3BoolTable&);
// General BFS Fanin Cone Indexing Functions for V3 Ntk
void bfsIndexFaninConeFF(V3Ntk* const, V3NetVec&, const V3NetVec&);

#endif

