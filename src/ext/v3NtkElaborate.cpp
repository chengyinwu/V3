/****************************************************************************
  FileName     [ v3NtkElaborate.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Ntk Elaboration. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_ELABORATE_C
#define V3_NTK_ELABORATE_C

#include "v3Msg.h"
#include "v3StgFSM.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkElaborate.h"

/* -------------------------------------------------- *\
 * Class V3NtkElaborate Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3NtkElaborate::V3NtkElaborate(V3NtkHandler* const handler, const V3NetVec& targetNets)
   : V3NtkHandler(handler) {
   assert (_handler); assert (!_ntk); _c2pMap.clear(); _p2cMap.clear(); _pOutput.clear();
   const uint32_t shadowSize = _handler->getNtk()->getLatchSize();
   _nextOpId = V3NetUD; _saved = _1stSave = _inLoop = _looped = V3NetUD;
   _mirror.clear(); _shadow.clear(); _shadow.reserve(shadowSize);
   for (uint32_t i = 0; i < shadowSize; ++i) _shadow.push_back(V3NetUD);
   // Duplicate from Parent Network if Necessary
   if (targetNets.size()) _ntk = elaborateNtk(_handler, targetNets, _p2cMap, _c2pMap, _netHash);
}

V3NtkElaborate::~V3NtkElaborate() {
   for (uint32_t i = 0; i < _pOutput.size(); ++i) delete _pOutput[i];
   _c2pMap.clear(); _p2cMap.clear(); _pOutput.clear(); _shadow.clear();
}

// Net Ancestry Functions
const V3NetId
V3NtkElaborate::getParentNetId(const V3NetId& id) const {
   if (V3NetUD == id || _c2pMap.size() <= id.id) return V3NetUD;
   return (isV3NetInverted(id) ? getV3InvertNet(_c2pMap[id.id]) : _c2pMap[id.id]);
}

const V3NetId
V3NtkElaborate::getCurrentNetId(const V3NetId& id, const uint32_t& index) const {
   if (V3NetUD == id || _p2cMap.size() <= id.id) return V3NetUD;
   return (isV3NetInverted(id) ? getV3InvertNet(_p2cMap[id.id]) : _p2cMap[id.id]);
}

// Elaboration Functions
const uint32_t
V3NtkElaborate::elaborateProperty(V3Property* const p, V3NetVec& constrList, const bool& l2s) {
   assert (p);
   V3LTLFormula* const ltlFormula = p->getLTLFormula(); assert (ltlFormula);
   const uint32_t pIndex = elaborateLTLFormula(ltlFormula, l2s);
   V3NetVec invList; invList.clear(); constrList.clear();
   // Elaborate Invariants
   for (uint32_t i = 0; i < p->getInvariantSize(); ++i) {
      const V3NetId id = elaborateInvariants(p->getInvariant(i));
      if (V3NetUD != id) invList.push_back(id);
   }
   // Elaborate Invariant Constraints
   V3NetVec invConstrList;
   for (uint32_t i = 0; i < p->getInvConstrSize(); ++i) {
      invConstrList.clear(); elaborateConstraints(p->getInvConstr(i), invConstrList);
      if (invConstrList.size()) constrList.push_back(combineConstraints(invConstrList));
   }
   // Elaborate Fairness Constraints
   if (l2s && p->getFairConstrSize()) {
      V3NetVec fairList;
      for (uint32_t i = 0; i < p->getFairConstrSize(); ++i) {
         fairList.clear(); elaborateFairnessL2S(p->getFairConstr(i), fairList);
         if (!fairList.size()) continue; fairList.push_back(_ntk->getOutput(pIndex));
         _ntk->replaceOutput(pIndex, combineConstraints(fairList));
      }
   }
   // Combine Invariants to Property Outpins
   if (invList.size()) combineInvariantToOutputs(pIndex, invList);
   assert ((1 + pIndex) == _ntk->getOutputSize()); return pIndex;
}

const uint32_t
V3NtkElaborate::elaborateLTLFormula(V3LTLFormula* const ltlFormula, const bool& l2s) {
   // Make Sure Mapping Tables are Maintained, or NO properties can be proliferated
   if (!isMutable() || (_ntk && (!_p2cMap.size()))) return V3NtkUD;
   // Elaborate LTL Formula into Ntk
   assert (ltlFormula); assert (_handler == ltlFormula->getHandler()); elaborate(ltlFormula);
   // Create Formula for this Ntk, and Perform LTL Formula Rewriting if Enabled
   V3LTLFormula* const formula = ltlFormula->createSuccessor(this);
   // Currently Support ONLY p, G(p) and F(p)
   const uint32_t rootIndex = formula->getRoot();
   V3NetId id = elaborateLTLFormula(formula, rootIndex); if (V3NetUD == id) return V3NtkUD;
   if (!formula->isLeaf(rootIndex))
      id = (l2s && V3_LTL_T_F == formula->getOpType(rootIndex)) ? elaborateL2S(~id) : ~id;
   // Record LTL Formula and Set to Output
   _pOutput.push_back(formula); _ntk->createOutput(id);
   if (_pOutput.back()->getName().size() && !existNetName(_pOutput.back()->getName()))
      resetOutName(_ntk->getOutputSize() - 1, _pOutput.back()->getName());
   return _ntk->getOutputSize() - 1;
}

const V3NetId
V3NtkElaborate::elaborateInvariants(V3Constraint* const constr) {
   assert (constr); assert (constr->getHandler());
   V3NetVec constrList; assert (_handler == constr->getHandler());
   if (constr->isFSMConstr()) elaborateFSMInvariants(constr->getFSM(), constrList);
   else elaboratePOConstraints(constr->getStart(), constr->getEnd(), constrList);
   return constrList.size() ? combineConstraints(constrList) : V3NetId::makeNetId(0, 1);
}

void
V3NtkElaborate::elaborateConstraints(V3Constraint* const constr, V3NetVec& constrList) {
   assert (constr); assert (constr->getHandler()); assert (_handler == constr->getHandler());
   if (constr->isFSMConstr()) elaborateFSMConstraints(constr->getFSM(), constrList);
   else elaboratePOConstraints(constr->getStart(), constr->getEnd(), constrList);
}

void
V3NtkElaborate::elaborateFairness(V3Constraint* const constr, V3NetVec& constrList) {
   assert (constr); assert (!constr->isFSMConstr()); constrList.clear();
   elaboratePOConstraints(constr->getStart(), constr->getEnd(), constrList);
   if (!constrList.size()) return;
   // Merge Fairness Constraints
   V3GateType type = dynamic_cast<V3BvNtk*>(_ntk) ? BV_AND : AIG_NODE;
   V3NetVec latchNets(constrList.size(), V3NetUD);
   V3NetVec logicNets(constrList.size(), V3NetUD);
   V3NetId id1; V3InputVec inputs(2, V3NetUD);
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      // Create NetId of Latch for Recording the Satisfaction of Each Fairness Constraint
      latchNets[i] = _ntk->createNet(); assert (V3NetUD != latchNets[i]);
      // Satisfaction of Each Fairness Constraint
      logicNets[i] = _ntk->createNet(); assert (V3NetUD != logicNets[i]);
      inputs[0] = ~constrList[i]; inputs[1] = ~latchNets[i];
      _ntk->setInput(logicNets[i], inputs); _ntk->createGate(type, logicNets[i]);
   }
   // Create Input Logic of finalConstr
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      if (!i) inputs[0] = ~logicNets[i];
      else {
         inputs[1] = ~logicNets[i]; id1 = _ntk->createNet(); assert (V3NetUD != id1);
         _ntk->setInput(id1, inputs); _ntk->createGate(type, id1); inputs[0] = id1;
      }
   }
   // Consider In the Loop in L2S Transformation
   if (V3NetUD != _inLoop) {
      id1 = _ntk->createNet(); assert (V3NetUD != id1);
      inputs[0] = ~inputs[0].id; inputs[1] = _inLoop;
      _ntk->setInput(id1, inputs); _ntk->createGate(type, id1); inputs[0] = ~id1;
   }
   const V3NetId finalConstr = inputs[0].id; assert (V3NetUD != finalConstr);
   // Create Input Logics for Latch
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      // MUX for Input of the Latch
      id1 = _ntk->createNet(); assert (V3NetUD != id1);
      inputs[0] = ~finalConstr; inputs[1] = ~logicNets[i];
      _ntk->setInput(id1, inputs); _ntk->createGate(type, id1);
      // Create Latch
      inputs[0] = id1; inputs[1] = V3NetId::makeNetId(0);
      _ntk->setInput(latchNets[i], inputs); _ntk->createLatch(latchNets[i]);
   }
   // Update constrList
   constrList.clear(); constrList.push_back(finalConstr);
}

void
V3NtkElaborate::elaborateFairnessL2S(V3Constraint* const constr, V3NetVec& constrList) {
   assert (V3NetUD != _saved); assert (V3NetUD != _1stSave);
   assert (V3NetUD != _inLoop); assert (V3NetUD != _looped);
   assert (constr); assert (!constr->isFSMConstr()); constrList.clear();
   elaboratePOConstraints(constr->getStart(), constr->getEnd(), constrList);
   // Create Latches for Fairness Constraints (f)
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   V3NetId id; V3GateType type; V3InputVec inputs(2);
   if (bvNtk) {
      for (uint32_t i = 0; i < constrList.size(); ++i) {
         // Create Input of Latch (F) : "F || (f && _inLoop)"
         inputs.clear(); inputs.push_back(_inLoop); inputs.push_back(constrList[i]);
         type = BV_AND; id = elaborateBvGate(bvNtk, type, inputs, _netHash);
         constrList[i] = _ntk->createNet(1); assert (V3NetUD != constrList[i]);
         inputs.clear(); inputs.push_back(~constrList[i]); inputs.push_back(~id);
         type = BV_AND; id = elaborateBvGate(bvNtk, type, inputs, _netHash);
         inputs.clear(); inputs.push_back(~id); inputs.push_back(0);
         _ntk->setInput(constrList[i], inputs); _ntk->createLatch(constrList[i]);
      }
   }
   else {
      for (uint32_t i = 0; i < constrList.size(); ++i) {
         // Create Input of Latch (F) : "F || (f && _inLoop)"
         inputs.clear(); inputs.push_back(_inLoop); inputs.push_back(constrList[i]);
         type = AIG_NODE; id = elaborateAigGate(_ntk, type, inputs, _netHash);
         constrList[i] = _ntk->createNet(1); assert (V3NetUD != constrList[i]);
         inputs.clear(); inputs.push_back(~constrList[i]); inputs.push_back(~id);
         type = AIG_NODE; id = elaborateAigGate(_ntk, type, inputs, _netHash);
         inputs.clear(); inputs.push_back(~id); inputs.push_back(0);
         _ntk->setInput(constrList[i], inputs); _ntk->createLatch(constrList[i]);
      }
   }
}

void
V3NtkElaborate::createConstrOutputs(const V3NetVec& constrList) {
   assert (constrList.size());
   for (uint32_t i = 0; i < constrList.size(); ++i) _ntk->createOutput(constrList[i]);
}

void
V3NtkElaborate::combineInvariantToOutputs(const uint32_t& pIndex, const V3NetVec& invList) {
   assert (pIndex < _ntk->getOutputSize()); assert (invList.size());
   // Combine All Invariants
   V3NetId id = combineConstraints(invList); V3InputVec inputs(2, 0);
   // Combine Invariants with the Property Output
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   inputs[0] = id; inputs[1] = ~(_ntk->getOutput(pIndex));
   id = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
              : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
   _ntk->replaceOutput(pIndex, id);
}

void
V3NtkElaborate::combineConstraintsToOutputs(const uint32_t& pIndex, const V3NetVec& constrList) {
   assert (pIndex < _ntk->getOutputSize()); assert (constrList.size());
   // Combine All Constraints
   V3NetId id = combineConstraints(constrList); V3InputVec inputs(2, 0);
   // Combine Constraints with the Property Output
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   const V3NetId latchId = _ntk->createNet(); assert (V3NetUD != latchId);
   inputs[0] = ~latchId; inputs[1] = id;
   id = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash)
              : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
   inputs[0] = id; inputs[1] = V3NetId::makeNetId(0);
   _ntk->setInput(latchId, inputs); _ntk->createLatch(latchId);
   inputs[0] = ~id; inputs[1] = _ntk->getOutput(pIndex);
   id = bvNtk ? elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
              : elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
   _ntk->replaceOutput(pIndex, id);
}

void
V3NtkElaborate::combineFSMInvariantsToOutputs(const uint32_t& pIndex, V3FSM* const fsm) {
   assert (pIndex < _ntk->getOutputSize()); assert (fsm);
   V3NetVec constrList; constrList.clear(); elaborateFSMInvariants(fsm, constrList);
   if (constrList.size()) combineInvariantToOutputs(pIndex, constrList);
}

const V3NetId
V3NtkElaborate::combineConstraints(const V3NetVec& constrList) {
   assert (constrList.size()); V3InputVec inputs(2, 0);
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   // Combine All Constraints
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      assert (constrList[i].id < _ntk->getNetSize());
      assert (1 == _ntk->getNetWidth(constrList[i]));
      if (!i) inputs[0] = constrList[i];
      else {
         inputs[1] = constrList[i];
         inputs[0] = bvNtk ? elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
                           : elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
      }
   }
   return inputs[0].id;
}

// Private Member Functions
void
V3NtkElaborate::elaborate(V3LTLFormula* const ltlFormula) {
   // Collect Leaf Formula, and Retain Only New Ones
   V3UI32Set targetNetIdSet; targetNetIdSet.clear(); assert (isMutable());
   assert (ltlFormula); ltlFormula->collectLeafFormula(targetNetIdSet);
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(targetNetIdSet.size());
   if (_p2cMap.size())
      for (V3UI32Set::const_iterator it = targetNetIdSet.begin(); it != targetNetIdSet.end(); ++it) {
         assert ((*it) < _p2cMap.size()); if (V3NetUD != _p2cMap[*it]) continue;
         targetNets.push_back(V3NetId::makeNetId(*it));
      }
   else 
      for (V3UI32Set::const_iterator it = targetNetIdSet.begin(); it != targetNetIdSet.end(); ++it)
         targetNets.push_back(V3NetId::makeNetId(*it));
   // Create Ntk if NOT Exist, or Augment New Formula Logic to Existing Ntk
   if (!_ntk) {
      assert (!_p2cMap.size()); assert (!_c2pMap.size()); assert (!_pOutput.size());
      _ntk = elaborateNtk(_handler, targetNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
   }
   else if (targetNets.size()) {
      attachToNtk(_handler, _ntk, targetNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
   }
   // Reserve Mapping Tables if Needed
   if (!V3NtkHandler::P2CMapON()) _p2cMap.clear();
   if (!V3NtkHandler::C2PMapON()) _c2pMap.clear();
}

void
V3NtkElaborate::elaboratePOConstraints(const uint32_t& start, const uint32_t& end, V3NetVec& constrList) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (start < ntk->getOutputSize() && end < ntk->getOutputSize());
   constrList.clear(); if (!_ntk || !_p2cMap.size()) return;
   // Elaborate Constraint Logics
   if (start <= end) {  // Fwd-Order Construction
      constrList.reserve(end - start);
      for (uint32_t i = start; i <= end; ++i) {
         assert (ntk->getOutput(i).id < _p2cMap.size());
         assert (1 == ntk->getNetWidth(ntk->getOutput(i)));
         if (V3NetUD != _p2cMap[ntk->getOutput(i).id]) continue;
         constrList.push_back(V3NetId::makeNetId(ntk->getOutput(i).id));
      }
   }
   else {  // Rev-Order Construction
      uint32_t i = 1 + start; assert (i > start); constrList.reserve(start - end);
      while (i-- > end) {
         assert (ntk->getOutput(i).id < _p2cMap.size());
         assert (1 == ntk->getNetWidth(ntk->getOutput(i)));
         if (V3NetUD != _p2cMap[ntk->getOutput(i).id]) continue;
         constrList.push_back(V3NetId::makeNetId(ntk->getOutput(i).id));
      }
   }
   if (constrList.size()) attachToNtk(_handler, _ntk, constrList, _p2cMap, _c2pMap, _netHash);
   // Put Constraints into List
   constrList.clear();
   if (start <= end) {  // Fwd-Order Construction
      for (uint32_t i = start; i <= end; ++i) {
         const V3NetId id = ntk->getOutput(i); assert (V3NetUD != _p2cMap[id.id]);
         constrList.push_back(isV3NetInverted(id) ? getV3InvertNet(_p2cMap[id.id]) : _p2cMap[id.id]);
      }
   }
   else {  // Rev-Order Construction
      uint32_t i = 1 + start; assert (i > start);
      while (i-- > end) {
         const V3NetId id = ntk->getOutput(i); assert (V3NetUD != _p2cMap[id.id]);
         constrList.push_back(isV3NetInverted(id) ? getV3InvertNet(_p2cMap[id.id]) : _p2cMap[id.id]);
      }
   }
}

void
V3NtkElaborate::elaborateFSMInvariants(V3FSM* const fsm, V3NetVec& constrList) {
   // NOTE: We add constraints follow the reachability, and
   //       For FSM with Milestones, we also add constraints follows the milestones.
   assert (fsm); assert (_handler == fsm->getNtkHandler());
   constrList.clear(); if (!_ntk || !_p2cMap.size()) return;
   // Omit FSM Without Invariants: Every milestone is reachable from any states
   if (!fsm->getStateSize()) return;
   uint32_t comb = 0, seq = 0;
   // Extract Combinational Invariants : G (state != some forward unreachable state)
   V3NetId id1; V3NetVec stateNets; V3UI32Vec reachStates; V3InputVec inputs(2);
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   for (uint32_t j = 0; j < fsm->getStateSize(); ++j) {
      if (V3NtkUD != fsm->getStepFwdFromInit(j)) continue;
      stateNets.clear(); fsm->getStateNetId(j, stateNets); assert (stateNets.size());
      attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
      id1 = elaborateFSMState(stateNets, false); assert (id1.id < _ntk->getNetSize());
      constrList.push_back(~id1); ++comb;
   }
   // Elaborate Sequential Invariants : nextOpState -> (j -> Prev(OR (reachStates)))
   const bool hasMileStone = fsm->getMileStoneSize();
   if (hasMileStone) {
      for (uint32_t j = 0; j < fsm->getStateSize(); ++j) {
         if (!fsm->isMileStone(j)) continue;
         assert (V3NtkUD != fsm->getStepBwdFromTerm(j));
         fsm->getStateBwdReachable(j, reachStates, true);
         if (fsm->getStateSize() == reachStates.size()) continue;
         if (fsm->getStateSize() < (reachStates.size() << 2)) continue;
         stateNets.clear(); fsm->getStateNetId(j, stateNets); assert (stateNets.size());
         attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
         id1 = elaborateFSMState(stateNets, false); assert (id1.id < _ntk->getNetSize());
         inputs[0] = ~id1;
         for (uint32_t k = 0; k < reachStates.size(); ++k) {
            stateNets.clear(); fsm->getStateNetId(reachStates[k], stateNets); assert (stateNets.size());
            attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
            id1 = elaborateFSMState(stateNets, true); assert (id1.id < _ntk->getNetSize());
            inputs[0] = ~(inputs[0].id); inputs[1] = ~id1; ++seq;
            inputs[0] = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
                              : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
         }
         // Record Invariants
         inputs[0] = ~(inputs[0].id); inputs[1] = _nextOpId; assert (V3NetUD != _nextOpId);
         inputs[0] = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
                           : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
         constrList.push_back(inputs[0].id);
      }
   }
}

void
V3NtkElaborate::elaborateFSMConstraints(V3FSM* const fsm, V3NetVec& constrList) {
   // NOTE: We add constraints follow the reachability, and
   //       For FSM with Milestones, we also add constraints follows the milestones.
   assert (fsm); assert (_handler == fsm->getNtkHandler());
   constrList.clear(); if (!_ntk || !_p2cMap.size()) return;
   // Omit FSM Without Invariants: Every milestone is reachable from any states
   if (!fsm->getStateSize()) return;
   // Extract Invariant Constraints : G (state != some backward unreachable state)
   V3NetId id1; V3NetVec stateNets; V3UI32Vec reachStates; V3InputVec inputs(2);
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   for (uint32_t j = 0; j < fsm->getStateSize(); ++j) {
      if (V3NtkUD != fsm->getStepBwdFromTerm(j)) continue;
      stateNets.clear(); fsm->getStateNetId(j, stateNets); assert (stateNets.size());
      attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
      id1 = elaborateFSMState(stateNets, false); assert (id1.id < _ntk->getNetSize());
      constrList.push_back(~id1);
   }
   // Elaborate Transition Constraints : nextOpState -> (j -> Next(OR (reachStates)))
   const bool hasMileStone = fsm->getMileStoneSize();
   if (hasMileStone) {
      for (uint32_t j = 0; j < fsm->getStateSize(); ++j) {
         if (!fsm->isMileStone(j)) continue;
         assert (V3NtkUD != fsm->getStepFwdFromInit(j));
         fsm->getStateFwdReachable(j, reachStates, true);
         if (reachStates.size() == fsm->getStateSize()) continue;
         if (fsm->getStateSize() < (reachStates.size() << 2)) continue;
         stateNets.clear(); fsm->getStateNetId(j, stateNets); assert (stateNets.size());
         attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
         id1 = elaborateFSMState(stateNets, true); assert (id1.id < _ntk->getNetSize());
         inputs[0] = ~id1;
         for (uint32_t k = 0; k < reachStates.size(); ++k) {
            stateNets.clear(); fsm->getStateNetId(reachStates[k], stateNets); assert (stateNets.size());
            attachToNtk(_handler, _ntk, stateNets, _p2cMap, _c2pMap, _netHash); assert (_ntk);
            id1 = elaborateFSMState(stateNets, false); assert (id1.id < _ntk->getNetSize());
            inputs[0] = ~(inputs[0].id); inputs[1] = ~id1;
            inputs[0] = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
                              : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
         }
         // Record Invariants
         inputs[0] = ~(inputs[0].id); inputs[1] = _nextOpId; assert (V3NetUD != _nextOpId);
         inputs[0] = bvNtk ? ~elaborateBvGate(bvNtk, BV_AND, inputs, _netHash) 
                           : ~elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
         constrList.push_back(inputs[0].id);
      }
   }
}

const V3NetId
V3NtkElaborate::elaborateFSMState(const V3NetVec& stateNets, const bool& nextState) {
   assert (stateNets.size()); V3InputVec inputs(2, V3NetUD); V3NetId id;
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   if (!nextState) {
      for (uint32_t k = 0; k < stateNets.size(); ++k) {
         assert (1 == _handler->getNtk()->getNetWidth(stateNets[k]));
         id = getCurrentNetId(V3NetId::makeNetId(stateNets[k].id));
         inputs[1] = (stateNets[k].cp) ? ~id : id;
         if (!k) { inputs[0] = inputs[1]; continue; }
         inputs[0] = bvNtk ? elaborateBvGate(bvNtk, BV_AND, inputs, _netHash)
                           : elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
      }
   }
   else {
      V3UI32Hash::const_iterator is; V3InputVec ffInputs(2, 0);
      if (V3NetUD == _nextOpId) {
         _nextOpId = _ntk->createNet(1); assert (V3NetUD != _nextOpId);
         ffInputs[0] = V3NetId::makeNetId(0, 1); ffInputs[1] = V3NetId::makeNetId(0);
         _ntk->setInput(_nextOpId, ffInputs); _ntk->createLatch(_nextOpId);
      }
      for (uint32_t k = 0; k < stateNets.size(); ++k) {
         assert (1 == _handler->getNtk()->getNetWidth(stateNets[k]));
         is = _mirror.find(_p2cMap[stateNets[k].id].id);
         if (_mirror.end() == is) {
            id = _ntk->createNet(1); _mirror.insert(make_pair((uint32_t)(_p2cMap[stateNets[k].id].id), id));
            ffInputs[0] = V3NetId::makeNetId(_p2cMap[stateNets[k].id].id); ffInputs[1] = V3NetId::makeNetId(0);
            _ntk->setInput(id, ffInputs); _ntk->createLatch(id);
         }
         else id = is->second; inputs[1] = (_p2cMap[stateNets[k].id].cp ^ stateNets[k].cp) ? ~id : id;
         if (!k) { inputs[0] = inputs[1]; continue; }
         inputs[0] = bvNtk ? elaborateBvGate(bvNtk, BV_AND, inputs, _netHash)
                           : elaborateAigGate(_ntk, AIG_NODE, inputs, _netHash);
      }
   }
   assert (V3NetUD != inputs[0].id); return inputs[0].id;
}

const V3NetId
V3NtkElaborate::elaborateLTLFormula(V3LTLFormula* const ltlFormula, const uint32_t& rootIndex) {
   assert (ltlFormula); assert (ltlFormula->isValid(rootIndex)); assert (isMutable());
   assert (_ntk); assert (this == ltlFormula->getHandler());
   if (ltlFormula->isLeaf(rootIndex)) {
      V3Formula* const formula = ltlFormula->getFormula(rootIndex); assert (formula);
      assert (ltlFormula->getHandler() == formula->getHandler());
      return dynamic_cast<V3BvNtk*>(ltlFormula->getHandler()->getNtk()) ? 
             elaborateBvFormula(formula, formula->getRoot(), _netHash) : 
             elaborateAigFormula(formula, formula->getRoot(), _netHash);
   }
   switch (ltlFormula->getOpType(rootIndex)) {
      case V3_LTL_T_G :  // Currently Support Single Layered LTL
         assert (1 == ltlFormula->getBranchSize(rootIndex));
         assert (ltlFormula->isLeaf(ltlFormula->getBranchIndex(rootIndex, 0)));
         return elaborateLTLFormula(ltlFormula, ltlFormula->getBranchIndex(rootIndex, 0));
      case V3_LTL_T_F :  // Currently Support Single Layered LTL
         assert (1 == ltlFormula->getBranchSize(rootIndex));
         assert (ltlFormula->isLeaf(ltlFormula->getBranchIndex(rootIndex, 0)));
         return elaborateLTLFormula(ltlFormula, ltlFormula->getBranchIndex(rootIndex, 0));
      default      : 
         Msg(MSG_WAR) << "Verification of General LTL Formula Has NOT Been Supported Yet !!" << endl;
   }
   return V3NetUD;
}

const V3NetId
V3NtkElaborate::elaborateL2S(const V3NetId& id) {
   assert (_ntk); assert (_p2cMap.size()); assert (id.id < _ntk->getNetSize());
   assert (isMutable()); assert (_shadow.size() == _handler->getNtk()->getLatchSize());
   // Create L2S Data Members if NOT Exist
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   V3InputVec inputs; inputs.clear(); inputs.reserve(4);
   if (V3NetUD == _saved || V3NetUD == _1stSave || V3NetUD == _inLoop) {
      assert (V3NetUD == _saved && V3NetUD == _1stSave && V3NetUD == _inLoop);
      _saved = _ntk->createNet(1);  // Create Latch (_saved) for "In the Loop"
      const V3NetId oracle = _ntk->createNet(1), inSaved = _ntk->createNet(1);
      inputs.push_back(~_saved); inputs.push_back(~oracle); _ntk->setInput(inSaved, inputs);
      _ntk->createGate((bvNtk ? BV_AND : AIG_NODE), inSaved); inputs.clear();
      inputs.push_back(~inSaved); inputs.push_back(0); _ntk->setInput(_saved, inputs);
      _ntk->createLatch(_saved); inputs.clear(); _ntk->createInput(oracle);
      _1stSave = _ntk->createNet(1);  // Create Net (_1stSave) for "Loop Start"
      inputs.push_back(oracle); inputs.push_back(~_saved); _ntk->setInput(_1stSave, inputs);
      _ntk->createGate((bvNtk ? BV_AND : AIG_NODE), _1stSave); inputs.clear();
      _inLoop = _ntk->createNet(1);  // Create Net (_inLoop) for "_1stSave || _saved"
      inputs.push_back(~_1stSave); inputs.push_back(~_saved); _ntk->setInput(_inLoop, inputs);
      _ntk->createGate((bvNtk ? BV_AND : AIG_NODE), _inLoop); inputs.clear(); _inLoop = ~_inLoop;
   }
   // Create Equivalence Logic and Shadow FF if NOT Exist
   V3NetId ffId; V3GateType type; V3UI32Vec newShadow; newShadow.clear();
   for (uint32_t i = 0; i < _shadow.size(); ++i) {
      ffId = _p2cMap[_handler->getNtk()->getLatch(i).id]; assert (V3_FF == _ntk->getGateType(ffId));
      if (V3NetUD == ffId || V3NetUD != _shadow[i]) continue; newShadow.push_back(i);
      // Create Net for Shadow FF
      _shadow[i] = _ntk->createNet(_ntk->getNetWidth(ffId)); assert (V3NetUD != _shadow[i]);
      V3NetId shadowMux = V3NetUD;  // Create Input MUX of Shadow FF
      if (bvNtk) {
         inputs.push_back(_shadow[i]); inputs.push_back(ffId); inputs.push_back(_1stSave);
         type = BV_MUX; shadowMux = elaborateBvGate(bvNtk, type, inputs, _netHash);
      }
      else {
         inputs.push_back(_1stSave); inputs.push_back(ffId);
         inputs.push_back(~_1stSave); inputs.push_back(_shadow[i]);
         type = AIG_NODE; shadowMux = elaborateAigAndOrAndGate(_ntk, inputs, _netHash);
      }
      // Create Shadow FF
      assert (V3NetUD != shadowMux); inputs.clear(); inputs.push_back(shadowMux);
      if (_ntk->getNetWidth(ffId) == 1) inputs.push_back(0);
      else {
         V3InputVec constInputs(1, bvNtk->hashV3ConstBitVec(v3Int2Str(_ntk->getNetWidth(ffId)) + "'d0"));
         type = BV_CONST; inputs.push_back(elaborateBvGate(bvNtk, type, constInputs, _netHash));
      }
      _ntk->setInput(_shadow[i], inputs); _ntk->createLatch(_shadow[i]); inputs.clear();
      // Create Equivalence Gate and Update _shadow
      if (bvNtk) {
         inputs.push_back(ffId); inputs.push_back(_shadow[i]); type = BV_EQUALITY;
         _shadow[i] = elaborateBvGate(bvNtk, type, inputs, _netHash);
      }
      else {
         inputs.push_back(ffId); inputs.push_back(_shadow[i]);
         inputs.push_back(~ffId); inputs.push_back(~_shadow[i]);
         _shadow[i] = elaborateAigAndOrAndGate(_ntk, inputs, _netHash);
      }
      assert (V3NetUD != _shadow[i]); assert (1 == _ntk->getNetWidth(_shadow[i])); inputs.clear();
   }
   // Create or Update Net (_looped) for "Loop Found" if NOT Exist
   if (V3NetUD == _looped) _looped = _saved;
   for (uint32_t i = 0; i < newShadow.size(); ++i) {
      assert (V3NetUD != _p2cMap[_handler->getNtk()->getLatch(i).id]); assert (V3NetUD != _shadow[i]);
      inputs.push_back(_looped); inputs.push_back(_shadow[i]);
      if (bvNtk) { type = BV_AND; _looped = elaborateBvGate(bvNtk, type, inputs, _netHash); }
      else { type = AIG_NODE; _looped = elaborateAigGate(_ntk, type, inputs, _netHash); }
      assert (V3NetUD != _looped); inputs.clear();
   }
   if (V3NetId::makeNetId(0) == ~id) return _looped;
   // Create Latch (pLatch) for Recording Existence of Violation to id
   const V3NetId pLatch = _ntk->createNet(1), in_pLatch = _ntk->createNet(1);
   inputs.push_back(~pLatch); inputs.push_back(id); _ntk->setInput(in_pLatch, inputs);
   _ntk->createGate((bvNtk ? BV_AND : AIG_NODE), in_pLatch); inputs.clear();
   inputs.push_back(~in_pLatch); inputs.push_back(0); _ntk->setInput(pLatch, inputs);
   _ntk->createLatch(pLatch); inputs.clear();
   // Create L2S Property Output (to Witness "_looped && in_pLatch")
   V3NetId pId = V3NetUD; inputs.push_back(_looped); inputs.push_back(in_pLatch);
   if (bvNtk) { type = BV_AND; pId = elaborateBvGate(bvNtk, type, inputs, _netHash); }
   else { type = AIG_NODE; pId = elaborateAigGate(_ntk, type, inputs, _netHash); }
   assert (V3NetUD != pId); inputs.clear(); return pId;
}

#endif

