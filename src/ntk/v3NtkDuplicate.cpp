/****************************************************************************
  FileName     [ v3NtkDuplicate.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Generic Functions for V3 Ntk Duplication. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_DUPLICATE_C
#define V3_NTK_DUPLICATE_C

#include "v3Msg.h"
#include "v3NtkStrash.h"
#include "v3NtkRewrite.h"

//#define V3_NTK_DUPLICATE_DEBUG  // Useful in identifying some combinational loops

// General Duplication Helper Functions for V3 Ntk
void duplicateGeneralInit(V3Ntk* const ntk, V3NetVec& p2cMap, V3NetVec& orderMap, const bool& reduceON) {
   assert (ntk);
   // Compute General DFS Order for Duplication
   dfsNtkForGeneralOrder(ntk, orderMap, V3NetVec(), !reduceON);
   // Initialize Mapper From Parent (Index) to Current (V3NetId) Ntk
   p2cMap.clear(); p2cMap.reserve(ntk->getNetSize()); p2cMap.push_back(V3NetId::makeNetId(0));
   for (uint32_t i = 1, j = ntk->getNetSize(); i < j; ++i) p2cMap.push_back(V3NetUD);
}

void duplicateReductionInit(V3Ntk* const ntk, const V3NetVec& targetNets, V3NetVec& p2cMap, V3NetVec& orderMap) {
   assert (ntk); assert (targetNets.size());
   // Compute Reduced DFS Order for Duplication
   dfsNtkForReductionOrder(ntk, orderMap, targetNets);
   // Initialize Mapper From Parent (Index) to Current (V3NetId) Ntk
   p2cMap.clear(); p2cMap.reserve(ntk->getNetSize()); p2cMap.push_back(V3NetId::makeNetId(0));
   for (uint32_t i = 1, j = ntk->getNetSize(); i < j; ++i) p2cMap.push_back(V3NetUD);
}

const V3NetId duplicateNet(V3Ntk* const ntk, V3Ntk* const pNtk, const V3NetId& pId, V3NetVec& p2cMap, V3NetVec& c2pMap) {
   assert (ntk); assert (pNtk); assert (pId.id < pNtk->getNetSize());
   assert (pId.id < p2cMap.size()); assert (V3NetUD == p2cMap[pId.id]);
   assert (c2pMap.size() == ntk->getNetSize()); c2pMap.push_back(pId);
   p2cMap[pId.id] = ntk->createNet(pNtk->getNetWidth(pId));
   assert (V3NetUD != p2cMap[pId.id]); return p2cMap[pId.id];
}

void duplicateInputNets(V3NtkHandler* const handler, V3Ntk* const ntk, V3NetVec& p2cMap, V3NetVec& c2pMap, V3NetVec& orderMap) {
   assert (handler); assert (ntk); V3Ntk* const pNtk = handler->getNtk(); assert (pNtk);
   assert ((bool)(dynamic_cast<V3BvNtk*>(pNtk)) == (bool)(dynamic_cast<V3BvNtk*>(ntk)));
   assert (p2cMap.size() == pNtk->getNetSize()); assert (c2pMap.size());
   // Create IO Nets
   uint32_t i = 1;
   for (uint32_t j = i + pNtk->getInputSize(); i < j; ++i) {
      assert (i < orderMap.size()); assert (V3_PI == pNtk->getGateType(orderMap[i]));
      assert (V3NetUD == p2cMap[orderMap[i].id]); duplicateNet(ntk, pNtk, orderMap[i], p2cMap, c2pMap);
   }
   for (uint32_t j = i + pNtk->getInoutSize(); i < j; ++i) {
      assert (i < orderMap.size()); assert (V3_PIO == pNtk->getGateType(orderMap[i]));
      assert (V3NetUD == p2cMap[orderMap[i].id]); duplicateNet(ntk, pNtk, orderMap[i], p2cMap, c2pMap);
   }
}

void duplicateInputGates(V3NtkHandler* const handler, V3Ntk* const ntk, V3NetVec& p2cMap, V3NetVec& c2pMap) {
   assert (handler); assert (ntk); V3Ntk* const pNtk = handler->getNtk(); assert (pNtk);
   assert ((bool)(dynamic_cast<V3BvNtk*>(pNtk)) == (bool)(dynamic_cast<V3BvNtk*>(ntk)));
   assert (p2cMap.size() == pNtk->getNetSize()); assert (c2pMap.size());
   // Construct Primary Input / Inout
   for (uint32_t i = 0; i < pNtk->getInputSize(); ++i) {
      assert (V3_PI == pNtk->getGateType(pNtk->getInput(i)));
      assert (V3NetUD != p2cMap[pNtk->getInput(i).id]);
      ntk->createInput(p2cMap[pNtk->getInput(i).id]);
   }
   V3InputVec inputs; inputs.clear(); inputs.reserve(2); V3NetId id;
   for (uint32_t i = 0; i < pNtk->getInoutSize(); ++i) {
      assert (V3_PIO == pNtk->getGateType(pNtk->getInout(i)));
      assert (V3NetUD != p2cMap[pNtk->getInout(i).id]);
      id = pNtk->getInputNetId(pNtk->getInout(i), 0); assert (V3NetUD != id);
      inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      id = pNtk->getInputNetId(pNtk->getInout(i), 1); assert (V3NetUD != id);
      inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      id = p2cMap[pNtk->getInout(i).id]; ntk->setInput(id, inputs); inputs.clear(); ntk->createInout(id);
   }
   // Construct Clock
   if (V3NetUD != pNtk->getClock()) {
      if (V3NetUD == p2cMap[pNtk->getClock().id]) {
         p2cMap[pNtk->getClock().id] = ntk->createNet();
         c2pMap.push_back(pNtk->getClock());
      }
      id = p2cMap[pNtk->getClock().id]; assert (id.id < ntk->getNetSize());
      ntk->createClock(pNtk->getClock().cp ? ~id : id);
   }
}

void duplicateNtk(V3NtkHandler* const handler, V3Ntk* const ntk, V3NetVec& p2cMap, V3NetVec& c2pMap, V3NetVec& orderMap, V3PortableType& netHash, V3NtkHierInfo& hierInfo, const uint32_t& flattenLevel) {
   assert (handler); assert (ntk); V3Ntk* const pNtk = handler->getNtk(); assert (pNtk);
   assert ((bool)(dynamic_cast<V3BvNtk*>(pNtk)) == (bool)(dynamic_cast<V3BvNtk*>(ntk)));
   const uint32_t ntkIndex = hierInfo.getNtkSize() - 1;
   // Module Instance Data Storage
   V3BoolVec moduleList(pNtk->getModuleSize(), false);
   V3NetVec orderMap2, cInputs, cOutputs;
   V3RepIdHash repIdHash; repIdHash.clear();
   // Net and Gate Type Data Storage
   V3InputVec inputs; inputs.clear(); inputs.reserve(3);
   V3GateType type; uint32_t inSize; V3NetId id;
   // Create Latch Nets
   uint32_t i = 1 + pNtk->getInputSize() + pNtk->getInoutSize();
   for (; i < orderMap.size(); ++i) {
      assert (i < orderMap.size());
      if (V3_FF != pNtk->getGateType(orderMap[i])) break;
      if (V3NetUD != p2cMap[orderMap[i].id]) continue;
      duplicateNet(ntk, pNtk, orderMap[i], p2cMap, c2pMap);
   }
   // Construct Gates in General Order
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(ntk);
   for (; i < orderMap.size(); ++i) {
      if (V3NetUD != p2cMap[orderMap[i].id]) continue;
      type = pNtk->getGateType(orderMap[i]); assert (V3_XD > type);
      if (V3_PI == type) duplicateNet(ntk, pNtk, orderMap[i], p2cMap, c2pMap);
      else if (V3_MODULE == type) {
         duplicateNet(ntk, pNtk, orderMap[i], p2cMap, c2pMap);
         moduleList[V3NetType(pNtk->getInputNetId(orderMap[i], 0)).value] = true;
      }
      else if (bvNtk) {
         if (BV_SLICE == type) {
            id = pNtk->getInputNetId(orderMap[i], 0); assert (V3NetUD != id); assert (V3NetUD != p2cMap[id.id]);
            inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
            inputs.push_back(pNtk->getInputNetId(orderMap[i], 1));
         }
         else if (BV_CONST == type) inputs.push_back(pNtk->getInputNetId(orderMap[i], 0));
         else {
            inSize = (isV3PairType(type)) ? 2 : (BV_MUX == type) ? 3 : 1;
            for (uint32_t j = 0; j < inSize; ++j) {
               id = pNtk->getInputNetId(orderMap[i], j); assert (V3NetUD != id); assert (V3NetUD != p2cMap[id.id]);
               inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
            }
         }
         id = p2cMap[orderMap[i].id] = elaborateBvGate(bvNtk, type, inputs, netHash);
         assert (pNtk->getNetWidth(orderMap[i]) == bvNtk->getNetWidth(id)); inputs.clear();
         while (c2pMap.size() < ntk->getNetSize()) c2pMap.push_back(V3NetUD);
         assert (c2pMap.size() == ntk->getNetSize()); assert (id.id < c2pMap.size());
         if (V3NetUD == c2pMap[id.id]) c2pMap[id.id] = V3NetId::makeNetId(orderMap[i].id, id.cp);
      }
      else {
         if (AIG_NODE == type) {
            id = pNtk->getInputNetId(orderMap[i], 0); assert (V3NetUD != id); assert (V3NetUD != p2cMap[id.id]);
            inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
            id = pNtk->getInputNetId(orderMap[i], 1); assert (V3NetUD != id); assert (V3NetUD != p2cMap[id.id]);
            inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
         }
         id = p2cMap[orderMap[i].id] = elaborateAigGate(ntk, type, inputs, netHash); inputs.clear();
         while (c2pMap.size() < ntk->getNetSize()) c2pMap.push_back(V3NetUD);
         assert (c2pMap.size() == ntk->getNetSize()); assert (id.id < c2pMap.size());
         if (V3NetUD == c2pMap[id.id]) c2pMap[id.id] = V3NetId::makeNetId(orderMap[i].id, id.cp);
      }
      // Assert All Parent Nets are Duplicated
      assert (V3NetUD != p2cMap[orderMap[i].id]);
   }
   // Construct DFF
   for (i = 0; i < pNtk->getLatchSize(); ++i) {
      assert (V3_FF == pNtk->getGateType(pNtk->getLatch(i)));
      if (V3NetUD == p2cMap[pNtk->getLatch(i).id]) continue;
      if (V3_FF == ntk->getGateType(p2cMap[pNtk->getLatch(i).id])) continue;
      assert (V3_PI == ntk->getGateType(p2cMap[pNtk->getLatch(i).id]));
      id = pNtk->getInputNetId(pNtk->getLatch(i), 0); assert (V3NetUD != id);
      inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      id = pNtk->getInputNetId(pNtk->getLatch(i), 1); assert (V3NetUD != id);
      inputs.push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      id = p2cMap[pNtk->getLatch(i).id]; ntk->setInput(id, inputs); inputs.clear(); ntk->createLatch(id);
   }
   // Duplicate Module Instances
   if (!flattenLevel) {
      for (uint32_t i = 0; i < moduleList.size(); ++i) {
         if (!moduleList[i]) continue;  // Not in COI
         V3NtkModule* const module = pNtk->getModule(i); assert (module);
         V3Ntk* const moduleNtk = module->getNtkRef()->getNtk(); assert (moduleNtk);
         // Set Inputs for the Module Instance
         const V3NetVec& pInputs = module->getInputList();
         assert (moduleNtk->getInputSize() == pInputs.size());
         for (uint32_t j = 0; j < pInputs.size(); ++j) {
            assert (V3NetUD != p2cMap[pInputs[j].id]); assert (!moduleNtk->getInput(j).cp);
            cInputs.push_back(p2cMap[pInputs[j].id]);
         }
         // Set Outputs for the Module Instance
         const V3NetVec& pOutputs = module->getOutputList();
         assert (moduleNtk->getOutputSize() == pOutputs.size());
         for (uint32_t j = 0; j < pOutputs.size(); ++j) {
            if (V3NetUD == p2cMap[pOutputs[j].id]) {
               id = duplicateNet(ntk, pNtk, pOutputs[j], p2cMap, c2pMap);
               assert (id == p2cMap[pOutputs[j].id]); cOutputs.push_back(id);
            }
            else cOutputs.push_back(pOutputs[j].cp ? ~(p2cMap[pOutputs[j].id]) : p2cMap[pOutputs[j].id]);
         }
         createModule(ntk, cInputs, cOutputs, module->getNtkRef(), module->isNtkRefBlackBoxed());
         cInputs.clear(); cOutputs.clear();
      }
   }
   // Set Hierarchical Info
   hierInfo.pushRefId(c2pMap.size(), ntkIndex);
   assert (ntkIndex < hierInfo.getNtkSize());
   assert (handler == hierInfo.getHandler(ntkIndex));
   // Flatten Module Instance
   if (flattenLevel) {
      for (uint32_t i = 0; i < moduleList.size(); ++i) {
         if (!moduleList[i]) continue;  // Not in COI
         V3NtkModule* const module = pNtk->getModule(i); assert (module);
         V3Ntk* const moduleNtk = module->getNtkRef()->getNtk(); assert (moduleNtk);
         if (module->isNtkRefBlackBoxed()) {
            Msg(MSG_WAR) << "Omit Blackboxed Module \"" << module->getNtkRef()->getNtkName() << "\""
               << " in Ntk Flatten !!" << endl; continue; }
         // Create New Hierarchical Info
         hierInfo.pushNtk(module->getNtkRef(), ntkIndex);
         V3NetVec& p2cMap2 = hierInfo.getMap(hierInfo.getNtkSize() - 1);
         // Construct Mapping Tables
         duplicateGeneralInit(moduleNtk, p2cMap2, orderMap2, V3NtkHandler::reduceON());
         // Set Inputs for the Module Instance
         const V3NetVec& pInputs = module->getInputList();
         assert (moduleNtk->getInputSize() == pInputs.size());
         for (uint32_t j = 0; j < pInputs.size(); ++j) {
            assert (V3NetUD != p2cMap[pInputs[j].id]); assert (!moduleNtk->getInput(j).cp);
            assert (V3NetUD == p2cMap2[moduleNtk->getInput(j).id]);
            p2cMap2[moduleNtk->getInput(j).id] = p2cMap[pInputs[j].id];
         }
         // Recursively Duplicate Module Instance
         duplicateNtk(module->getNtkRef(), ntk, p2cMap2, c2pMap, orderMap2, netHash, hierInfo, flattenLevel - 1);
         // Set Outputs of Module Instance to p2cMap
         const V3NetVec& pOutputs = module->getOutputList();
         assert (moduleNtk->getOutputSize() == pOutputs.size());
         for (uint32_t j = 0; j < pOutputs.size(); ++j) {
            if (V3NetUD == p2cMap[pOutputs[j].id]) continue;  // Fanout Free Instance PO
            assert (V3_PI == ntk->getGateType(p2cMap[pOutputs[j].id]));
            id = p2cMap2[moduleNtk->getOutput(j).id]; assert (id.id < c2pMap.size());
            // Replace Module Outputs with Outputs of Module Instance
            assert (repIdHash.end() == repIdHash.find(p2cMap[pOutputs[j].id].id));
            id = V3NetId::makeNetId(id.id, pOutputs[j].cp ^ moduleNtk->getOutput(j).cp ^ id.cp);
            repIdHash.insert(make_pair(p2cMap[pOutputs[j].id].id, id)); p2cMap[pOutputs[j].id] = id;
         }
      }
      // Replace Fanin Nets
      if (repIdHash.size()) ntk->replaceFanin(repIdHash);
   }
}

// General Gate Type Elaboration Functions for V3 Ntk
const V3NetId elaborateAigGate(V3AigNtk* const ntk, const V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (AIG_NODE == type || AIG_FALSE == type);
   V3NetId id = V3NetUD; V3GateType gateType = type;
   // Perform Strashing and Rewriting
   const bool cpId = rewriteAigGate(ntk, gateType, inputs, netHash);
   if (V3_PI == gateType) id = inputs[0].id;
   else {  // Construct AIG Gate
      if (AIG_NODE == gateType) {
         assert (2 == inputs.size()); id = ntk->createNet(1);
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createGate(gateType, id);
         if (V3NtkHandler::strashON()) strashAigNodeGate(ntk, id, netHash);
      }
      else {
         assert (AIG_FALSE == gateType); assert (!inputs.size());
         id = V3NetId::makeNetId(0);
      }
      assert (gateType == ntk->getGateType(id)); assert (!id.cp);
   }
   assert (id.id < ntk->getNetSize()); return (cpId ? ~id : id);
}

const V3NetId elaborateBvGate(V3BvNtk* const ntk, const V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (V3_XD > type); assert (AIG_FALSE < type);
   V3NetId id = V3NetUD; V3GateType gateType = type;
   // Perform Strashing and Rewriting
   const uint32_t inputSize = inputs.size();
   const bool cpId = rewriteBvGate(ntk, gateType, inputs, netHash);
   if (V3_PI == gateType) id = inputs[0].id;
   else {  // Construct BV Gate
      if (isV3PairType(gateType)) {
         assert (2 == inputs.size());
         id = ntk->createNet((BV_MERGE == gateType) ? 
                             (ntk->getNetWidth(inputs[0].id) + ntk->getNetWidth(inputs[1].id)) : 
                             (gateType < BV_EQUALITY) ? ntk->getNetWidth(inputs[0].id) : 1);
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createGate(gateType, id);
         if (V3NtkHandler::strashON()) strashBvPairTypeGate(ntk, id, netHash);
      }
      else if (isV3ReducedType(gateType)) {
         assert (1 == inputs.size()); id = ntk->createNet(1);
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createGate(gateType, id);
         if (V3NtkHandler::strashON()) strashBvReducedTypeGate(ntk, id, netHash);
      }
      else if (BV_MUX == gateType) {
         assert (3 == inputs.size()); id = ntk->createNet(ntk->getNetWidth(inputs[0].id));
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createGate(gateType, id);
         if (V3NtkHandler::strashON()) strashBvMuxGate(ntk, id, netHash);
      }
      else if (BV_SLICE == gateType) {
         assert (2 == inputs.size()); id = ntk->createNet(ntk->getSliceWidth(inputs[1].value));
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createGate(gateType, id);
         if (V3NtkHandler::strashON()) strashBvSliceGate(ntk, id, netHash);
      }
      else {
         assert (BV_CONST == gateType); assert (1 == inputs.size());
         id = ntk->createNet(ntk->getConstWidth(inputs[0].value));
         assert (id.id < ntk->getNetSize()); ntk->setInput(id, inputs); ntk->createConst(id);
         if (V3NtkHandler::strashON()) strashBvConstGate(ntk, id, netHash);
      }
      assert (gateType == ntk->getGateType(id)); assert (!id.cp);
   }
   if (inputSize != inputs.size()) inputs = V3InputVec(inputSize, 0);
   assert (id.id < ntk->getNetSize()); return (cpId ? ~id : id);
}

const V3NetId elaborateAigAndOrAndGate(V3AigNtk* const ntk, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (4 == inputs.size());
   V3InputVec andInputs(2); V3GateType type; V3NetId id1, id2;
   andInputs[0] = inputs[0]; andInputs[1] = inputs[1];
   type = AIG_NODE; id1 = elaborateAigGate(ntk, type, andInputs, netHash);
   andInputs[0] = inputs[2]; andInputs[1] = inputs[3];
   type = AIG_NODE; id2 = elaborateAigGate(ntk, type, andInputs, netHash);
   andInputs[0] = ~id1; andInputs[1] = ~id2; type = AIG_NODE;
   return ~elaborateAigGate(ntk, type, andInputs, netHash);
}

// General Duplication Functions for V3 Ntk  (Note that PO will NOT be created !!)
V3Ntk* const duplicateNtk(V3NtkHandler* const handler, V3NetVec& p2cMap, V3NetVec& c2pMap) {
   assert (handler); assert (handler->getNtk());
   // Initialize Mapping Tables
   V3NetVec orderMap; c2pMap.clear();
   duplicateGeneralInit(handler->getNtk(), p2cMap, orderMap, V3NtkHandler::reduceON());
   assert (orderMap.size()); assert (!orderMap[0].id); c2pMap.push_back(orderMap[0]);
   // Initialize Net Hash Tables
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(handler->getNtk());
   V3PortableType netHash = V3PortableType((isBvNtk ? (V3_XD - BV_RED_AND) : 1), (isBvNtk ? BV_RED_AND : AIG_NODE));
   // Create Duplicate Ntk
   V3Ntk* const ntk = createV3Ntk(dynamic_cast<V3BvNtk*>(handler->getNtk())); assert (ntk);
   duplicateInputNets(handler, ntk, p2cMap, c2pMap, orderMap);
   V3NtkHierInfo hierInfo; hierInfo.clear(); hierInfo.pushNtk(handler, 0);
   duplicateNtk(handler, ntk, p2cMap, c2pMap, orderMap, netHash, hierInfo, 0);
   duplicateInputGates(handler, ntk, p2cMap, c2pMap); return ntk;
}

V3Ntk* const elaborateNtk(V3NtkHandler* const handler, const V3NetVec& targetNets, V3NetVec& p2cMap, V3NetVec& c2pMap, V3PortableType& netHash) {
   assert (handler); assert (handler->getNtk());
   // Initialize Mapping Tables
   V3NetVec orderMap; c2pMap.clear();
   duplicateReductionInit(handler->getNtk(), targetNets, p2cMap, orderMap);
   assert (orderMap.size()); assert (!orderMap[0].id); c2pMap.push_back(orderMap[0]);
   // Initialize Net Hash Tables
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(handler->getNtk());
   netHash = V3PortableType((isBvNtk ? (V3_XD - BV_RED_AND) : 1), (isBvNtk ? BV_RED_AND : AIG_NODE));
   // Create Duplicate Ntk
   V3Ntk* const ntk = createV3Ntk(dynamic_cast<V3BvNtk*>(handler->getNtk())); assert (ntk);
   duplicateInputNets(handler, ntk, p2cMap, c2pMap, orderMap);
   V3NtkHierInfo hierInfo; hierInfo.clear(); hierInfo.pushNtk(handler, 0);
   duplicateNtk(handler, ntk, p2cMap, c2pMap, orderMap, netHash, hierInfo, 0);
   duplicateInputGates(handler, ntk, p2cMap, c2pMap); return ntk;
}

V3Ntk* const flattenNtk(V3NtkHandler* const handler, V3NetVec& c2pMap, V3NtkHierInfo& hierInfo, const uint32_t& flattenLevel) {
   assert (handler); assert (handler->getNtk()); assert (flattenLevel); hierInfo.clear();
   // Push Top Ntk into the Hierarchy
   hierInfo.clear(); hierInfo.pushNtk(handler, 0);
   // Initialize Mapping Tables
   V3NetVec orderMap; c2pMap.clear();
   duplicateGeneralInit(handler->getNtk(), hierInfo.getMap(0), orderMap, V3NtkHandler::reduceON());
   assert (orderMap.size()); assert (!orderMap[0].id); c2pMap.push_back(orderMap[0]);
   // Initialize Net Hash Tables
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(handler->getNtk());
   V3PortableType netHash = V3PortableType((isBvNtk ? (V3_XD - BV_RED_AND) : 1), (isBvNtk ? BV_RED_AND : AIG_NODE));
   // Create Duplicate Ntk
   V3Ntk* const ntk = createV3Ntk(dynamic_cast<V3BvNtk*>(handler->getNtk())); assert (ntk);
   duplicateInputNets(handler, ntk, hierInfo.getMap(0), c2pMap, orderMap);
   duplicateNtk(handler, ntk, hierInfo.getMap(0), c2pMap, orderMap, netHash, hierInfo, flattenLevel);
   duplicateInputGates(handler, ntk, hierInfo.getMap(0), c2pMap); return ntk;
}

V3Ntk* const attachToNtk(V3NtkHandler* const handler, V3Ntk* const ntk, const V3NetVec& targetNets, V3NetVec& p2cMap, V3NetVec& c2pMap, V3PortableType& netHash) {
   assert (handler); assert (handler->getNtk()); assert (ntk);
   assert (p2cMap.size() == handler->getNtk()->getNetSize()); assert (c2pMap.size());
   assert ((bool)(dynamic_cast<V3BvNtk*>(handler->getNtk())) == (bool)(dynamic_cast<V3BvNtk*>(ntk)));
   // Compute Reduced DFS Order for Duplication
   V3NetVec orderMap; dfsNtkForReductionOrder(handler->getNtk(), orderMap, targetNets);
   // Attach to Existing Ntk
   V3NtkHierInfo hierInfo; hierInfo.clear(); hierInfo.pushNtk(handler, 0);
   duplicateNtk(handler, ntk, p2cMap, c2pMap, orderMap, netHash, hierInfo, 0); return ntk;
}

#endif

