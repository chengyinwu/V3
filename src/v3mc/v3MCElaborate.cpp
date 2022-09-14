/****************************************************************************
  FileName     [ v3MCElaborate.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Network Elaboration Utilities. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_ELABORATE_C
#define V3_MC_ELABORATE_C

#include "v3MCMain.h"
#include "v3NtkUtil.h"

// V3 Vernfication Instance Elaboration Functions
void mergeFairnessConstraints(V3Ntk* const ntk, V3NetTable& constr) {
   assert (ntk); assert (constr.size());
   const V3NetId const0 = V3NetId::makeNetId(0);
   const V3GateType type = dynamic_cast<V3BvNtk*>(ntk) ? BV_AND : AIG_NODE;
   V3InputVec inputs(2, V3NetUD); V3NetId id;
   for (uint32_t p = 0; p < constr.size(); ++p) {
      if (constr[p].size() < 2) continue;
      V3NetVec fairLatch(constr[p].size(), V3NetUD);
      V3NetVec fairFound(constr[p].size(), V3NetUD);
      for (uint32_t i = 0; i < constr[p].size(); ++i) {
         // Create Representative Signals for Each Fairness Constraints f
         fairLatch[i] = ntk->createNet(); assert (V3NetUD != fairLatch[i]);
         fairFound[i] = ntk->createNet(); assert (V3NetUD != fairFound[i]);
         // Create Logic for "f happens now or after RESET"
         inputs[0] = ~(constr[p][i]); inputs[1] = ~(fairLatch[i]);
         ntk->setInput(fairFound[i], inputs); ntk->createGate(type, fairFound[i]);
      }
      // Create the "RESET" Logic for the Resulting Fairness Constraint
      inputs[0] = ~fairFound[0];
      for (uint32_t i = 1; i < fairFound.size(); ++i) {
         id = ntk->createNet(); assert (V3NetUD != id); inputs[1] = ~fairFound[i];
         ntk->setInput(id, inputs); ntk->createGate(type, id); inputs[0] = id;
      }
      constr[p].clear(); constr[p].push_back(inputs[0].id);
      for (uint32_t i = 0; i < fairFound.size(); ++i) {
         // Create Input Logic for Latch Representing "f happens after RESET"
         id = ntk->createNet(); assert (V3NetUD != id);
         inputs[0] = ~(constr[p][0]); inputs[1] = ~(fairFound[i]);
         ntk->setInput(id, inputs); ntk->createGate(type, id);
         inputs[0] = id; inputs[1] = const0;
         ntk->setInput(fairLatch[i], inputs); ntk->createLatch(fairLatch[i]);
      }
   }
}

void combineConstraintsToOutputs(V3Ntk* const ntk, const V3UI32Vec& poList, const V3NetVec& constr) {
   assert (ntk); assert (poList.size()); assert (constr.size());
   const V3GateType type = dynamic_cast<V3BvNtk*>(ntk) ? BV_AND : AIG_NODE;
   // Combine All Constraints
   V3InputVec inputs(2, 0); V3NetId id;
   for (uint32_t i = 0; i < constr.size(); ++i) {
      assert (constr[i].id < ntk->getNetSize());
      assert (1 == ntk->getNetWidth(constr[i]));
      if (!i) { inputs[0] = constr[i]; continue; }
      inputs[1] = constr[i]; id = ntk->createNet(); assert (V3NetUD != id);
      ntk->setInput(id, inputs); ntk->createGate(type, id); inputs[0] = id;
   }
   id = inputs[0].id;
   // Create Constraint Transformation Logic
   const V3NetId latchId = ntk->createNet(); assert (V3NetUD != latchId);
   inputs[0] = ~latchId; inputs[1] = id; id = ntk->createNet(); assert (V3NetUD != id);
   ntk->setInput(id, inputs); ntk->createGate(type, id);
   inputs[0] = ~id; inputs[1] = V3NetId::makeNetId(0);
   ntk->setInput(latchId, inputs); ntk->createLatch(latchId); inputs[0] = id;
   // Combine Constraints with the Property Output
   for (uint32_t i = 0; i < poList.size(); ++i) {
      assert (poList[i] < ntk->getOutputSize()); inputs[1] = ntk->getOutput(poList[i]);
      id = ntk->createNet(); assert (V3NetUD != id); ntk->setInput(id, inputs);
      ntk->createGate(type, id); ntk->replaceOutput(poList[i], id);
   }
}

V3NtkHandler* const elaborateSafetyNetwork(V3NtkHandler* const handler, const V3UI32Vec& poList, V3NetTable& invConstr) {
   // This Function Creates a Simplified Network with Outputs Ordered by poList
   assert (handler); assert (handler->getNtk()); assert (poList.size());
   assert (poList.size() == invConstr.size());
   // Collect Property Signals and Invariant Constraints
   uint32_t invSize = 0; for (uint32_t i = 0; i < invConstr.size(); ++i) invSize += invConstr[i].size();
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(poList.size() + invSize);
   for (uint32_t i = 0; i < poList.size(); ++i) {
      assert (poList[i] < handler->getNtk()->getOutputSize());
      targetNets.push_back(handler->getNtk()->getOutput(poList[i]));
   }
   for (uint32_t i = 0; i < invConstr.size(); ++i) {
      for (uint32_t j = 0; j < invConstr[i].size(); ++j) {
         assert (invConstr[i][j].id < handler->getNtk()->getNetSize());
         targetNets.push_back(invConstr[i][j]);
      }
   }
   // Elaborate and Simplify the Network
   V3NetVec p2cMap, c2pMap; p2cMap.clear(); c2pMap.clear(); V3PortableType netHash;
   V3Ntk* const simpNtk = elaborateNtk(handler, targetNets, p2cMap, c2pMap, netHash); assert (simpNtk);
   // Set Output Names for Properties
   V3NtkHandler* const pNtk = new V3NtkHandler(0, simpNtk); assert (pNtk); V3NetId id;
   for (uint32_t i = 0; i < poList.size(); ++i) {
      id = handler->getNtk()->getOutput(poList[i]); assert (V3NetUD != p2cMap[id.id]);
      id = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
      assert (id.id < simpNtk->getNetSize()); simpNtk->createOutput(id); assert (simpNtk->getOutputSize() == (1 + i));
      assert (handler->getOutputName(poList[i]).size()); pNtk->resetOutName(i, handler->getOutputName(poList[i]));
   }
   // Update Invariant Constraints
   for (uint32_t i = 0; i < invConstr.size(); ++i) {
      for (uint32_t j = 0; j < invConstr[i].size(); ++j) {
         id = invConstr[i][j]; assert (V3NetUD != p2cMap[id.id]);
         invConstr[i][j] = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
      }
   }
   return pNtk;
}

V3NtkHandler* const elaborateLivenessNetwork(V3NtkHandler* const handler, const V3UI32Vec& poList, V3NetTable& invConstr, V3NetTable& fairConstr) {
   // This Function Create a Simplified Network with Outputs Ordered by poList.
   // Moreover, Fairness Constraints are Strengthened by Other Fairness Constraints.
   assert (handler); assert (handler->getNtk()); assert (poList.size());
   assert (poList.size() == invConstr.size()); assert (poList.size() == fairConstr.size());
   // Copy the Original Network for Adding Additional Logic
   V3Ntk* const copyNtk = copyV3Ntk(handler->getNtk()); assert (copyNtk);
   V3NtkHandler* const copyHandler = new V3NtkHandler(0, copyNtk); assert (copyNtk);
   for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i)
      copyHandler->resetOutName(i, handler->getOutputName(i));
   // Merge Fairness Constraints
   if (fairConstr.size()) mergeFairnessConstraints(copyNtk, fairConstr);
   // Simplify the Network
   V3NetTable constrList = invConstr;
   for (uint32_t i = 0; i < fairConstr.size(); ++i)
      constrList[i].insert(constrList[i].end(), fairConstr[i].begin(), fairConstr[i].end());
   V3NtkHandler* const pNtk = elaborateSafetyNetwork(copyHandler, poList, constrList);
   assert (pNtk); assert (poList.size() == pNtk->getNtk()->getOutputSize()); delete copyHandler;
   // Update Constraints
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      uint32_t j = 0, k = 0;
      for (j = 0; j < invConstr[i].size(); ++j) invConstr[i][j] = constrList[i][k++];
      for (j = 0; j < fairConstr[i].size(); ++j) fairConstr[i][j] = constrList[i][k++];
      assert (k == constrList[i].size());
   }
   return pNtk;
}

V3NtkElaborate* const elaborateProperties(V3NtkHandler* const handler, V3StrVec& name, V3UI32Vec& prop, V3UI32Table& invc, V3UI32Table& fair, const bool& l2s, const bool& invc2Prop, const bool& safeOnly, const bool& liveOnly) {
   assert (handler); assert (handler->getNtk()); name.clear(); prop.clear(); invc.clear(); fair.clear();
   const V3PropertyMap& propList = handler->getPropertyList(); assert (propList.size());
   name.reserve(propList.size()); prop.reserve(propList.size());
   invc.reserve(propList.size()); fair.reserve(propList.size());
   V3Map<uint32_t, uint32_t>::Map pred2Idx; pred2Idx.clear();
   V3Map<uint32_t, uint32_t>::Map::const_iterator is;
   // Collect Predicates
   V3UI32Set targetNetIdSet; targetNetIdSet.clear();
   for (V3PropertyMap::const_iterator it = propList.begin(); it != propList.end(); ++it)
      it->second->getLTLFormula()->collectLeafFormula(targetNetIdSet);
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(targetNetIdSet.size());
   for (V3UI32Set::const_iterator it = targetNetIdSet.begin(); it != targetNetIdSet.end(); ++it)
      targetNets.push_back(V3NetId::makeNetId(*it));
   // Elaborate Properties
   V3NtkElaborate* const pNtk = new V3NtkElaborate(handler, targetNets); assert (pNtk);
   V3NetVec invList, invConstr, fairConstr, constr; uint32_t idx;
   for (V3PropertyMap::const_iterator it = propList.begin(); it != propList.end(); ++it) {
      V3LTLFormula* const ltlFormula = it->second->getLTLFormula(); assert (ltlFormula);
      if (safeOnly && V3_LTL_T_F == ltlFormula->getOpType(ltlFormula->getRoot())) continue;
      if (liveOnly && V3_LTL_T_F != ltlFormula->getOpType(ltlFormula->getRoot())) continue;
      // Set Property Name
      name.push_back(ltlFormula->getName());
      const bool safe = l2s || V3_LTL_T_F != ltlFormula->getOpType(ltlFormula->getRoot());
      const uint32_t pIndex = pNtk->elaborateLTLFormula(ltlFormula, l2s);
      assert ((1 + pIndex) == pNtk->getNtk()->getOutputSize());
      invList.clear(); invConstr.clear(); fairConstr.clear();
      // Elaborate Invariants
      for (uint32_t i = 0; i < it->second->getInvariantSize(); ++i) {
         const V3NetId id = pNtk->elaborateInvariants(it->second->getInvariant(i));
         if (V3NetUD != id) invList.push_back(id);
      }
      // Elaborate Invariant Constraints
      for (uint32_t i = 0; i < it->second->getInvConstrSize(); ++i) {
         constr.clear(); pNtk->elaborateConstraints(it->second->getInvConstr(i), constr);
         if (constr.size()) invConstr.insert(invConstr.end(), constr.begin(), constr.end());
      }
      if (invc2Prop && invConstr.size()) {
         pNtk->combineConstraintsToOutputs(pIndex, invConstr);
         invConstr.clear();
      }
      // Elaborate Fairness Constraints
      if (l2s) {
         for (uint32_t i = 0; i < it->second->getFairConstrSize(); ++i) {
            constr.clear(); pNtk->elaborateFairnessL2S(it->second->getFairConstr(i), constr);
            if (!constr.size()) continue; constr.push_back(pNtk->getNtk()->getOutput(pIndex));
            pNtk->getNtk()->replaceOutput(pIndex, pNtk->combineConstraints(constr));
         }
      }
      else {
         for (uint32_t i = 0; i < it->second->getFairConstrSize(); ++i) {
            constr.clear(); pNtk->elaborateConstraints(it->second->getFairConstr(i), constr);
            if (constr.size()) fairConstr.insert(fairConstr.end(), constr.begin(), constr.end());
         }
      }
      // Combine Invariants to Property Outpins
      if (invList.size()) pNtk->combineInvariantToOutputs(pIndex, invList);
      assert (prop.size() == invc.size()); assert (prop.size() == fair.size());
      // Record Property Signal
      idx = V3NetType(pNtk->getNtk()->getOutput(pIndex)).value; is = pred2Idx.find(idx);
      if (pred2Idx.end() == is) { prop.push_back(pIndex); pred2Idx.insert(make_pair(idx, pIndex)); }
      else { prop.push_back(is->second); pNtk->getNtk()->removeLastOutput(); }
      // Encode Property Type to the LSB of the Property Signal
      prop.back() = prop.back() << 1; if (!safe) prop.back() = prop.back() + 1;
      // Record Invariant Constraints
      invc.push_back(V3UI32Vec()); invc.back().clear();
      for (uint32_t i = 0; i < invConstr.size(); ++i) {
         idx = V3NetType(invConstr[i]).value; is = pred2Idx.find(idx);
         if (pred2Idx.end() == is) {
            pred2Idx.insert(make_pair(idx, pNtk->getNtk()->getOutputSize()));
            invc.back().push_back(pNtk->getNtk()->getOutputSize());
            pNtk->getNtk()->createOutput(invConstr[i]);
         }
         else invc.back().push_back(is->second);
      }
      // Record Fairness Constraints
      fair.push_back(V3UI32Vec()); fair.back().clear();
      for (uint32_t i = 0; i < fairConstr.size(); ++i) {
         idx = V3NetType(fairConstr[i]).value; is = pred2Idx.find(idx);
         if (pred2Idx.end() == is) {
            pred2Idx.insert(make_pair(idx, pNtk->getNtk()->getOutputSize()));
            fair.back().push_back(pNtk->getNtk()->getOutputSize());
            pNtk->getNtk()->createOutput(fairConstr[i]);
         }
         else fair.back().push_back(is->second);
      }
   }
   return pNtk;
}

#endif

