/****************************************************************************
  FileName     [ v3BvBlastAig.cpp ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Bit Blasting of V3 BV Network to AIG Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BV_BLAST_AIG_C
#define V3_BV_BLAST_AIG_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3BvBlastAig.h"
#include "v3BvBlastAigHelper.h"

#include <cmath>

/* -------------------------------------------------- *\
 * Class V3BvBlastAig Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3BvBlastAig::V3BvBlastAig(V3NtkHandler* const p) 
   : V3NtkHandler(p, createV3Ntk(!dynamic_cast<V3BvNtk*>(p->getNtk()))) {
   assert (dynamic_cast<V3BvNtk*>(p->getNtk())); assert (!dynamic_cast<V3BvNtk*>(_ntk));
   // Perform Network Transformation
   _c2pMap.clear(); _p2cMap.clear(); performNtkTransformation();
   // Reserve Mapping Tables if Needed
   if (!V3NtkHandler::P2CMapON()) _p2cMap.clear();
   if (!V3NtkHandler::C2PMapON()) _c2pMap.clear();
}

V3BvBlastAig::~V3BvBlastAig() {
   _c2pMap.clear(); _p2cMap.clear();
}

// I/O Ancestry Functions
const string
V3BvBlastAig::getInputName(const uint32_t& index) const {
   // Current Network
   assert (_ntk); if (index >= _ntk->getInputSize()) return "";
   V3NetStrHash::const_iterator it = _netHash.find(_ntk->getInput(index).id);
   if (it != _netHash.end()) { assert (!_ntk->getInput(index).cp); return it->second; }
   // Parent Networks
   string name = "";
   if (_handler) {
      const V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
      uint32_t i = 0, pIndex = index;
      for (; i < ntk->getInputSize(); ++i)
         if (pIndex < ntk->getNetWidth(ntk->getInput(i))) break;
         else pIndex -= ntk->getNetWidth(ntk->getInput(i));
      name = _handler->getInputName(i); assert (pIndex < ntk->getNetWidth(ntk->getInput(i)));
      if (name.size() && 1 < ntk->getNetWidth(ntk->getInput(i)))
         name += (V3AuxNameBitPrefix + v3Int2Str(pIndex) + V3AuxNameBitSuffix);
   }
   return name.size() ? name : getNetNameOrFormedWithId(_ntk->getInput(index));
}

const string
V3BvBlastAig::getOutputName(const uint32_t& index) const {
   // Current Network
   assert (_ntk); if (index >= _ntk->getOutputSize()) return "";
   V3IdxStrHash::const_iterator it = _outIndexHash.find(index);
   if (it != _outIndexHash.end()) return it->second;
   // Parent Networks
   string name = "";
   if (_handler) {
      const V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
      uint32_t i = 0, pIndex = index;
      for (; i < ntk->getOutputSize(); ++i)
         if (pIndex < ntk->getNetWidth(ntk->getOutput(i))) break;
         else pIndex -= ntk->getNetWidth(ntk->getOutput(i));
      name = _handler->getOutputName(i); assert (pIndex < ntk->getNetWidth(ntk->getOutput(i)));
      if (name.size() && 1 < ntk->getNetWidth(ntk->getOutput(i)))
         name += (V3AuxNameBitPrefix + v3Int2Str(pIndex) + V3AuxNameBitSuffix);
   }
   return name;
}

// Net Ancestry Functions
const string
V3BvBlastAig::getInoutName(const uint32_t& index) const {
   // Current Network
   assert (_ntk); if (index >= _ntk->getInoutSize()) return "";
   V3NetStrHash::const_iterator it = _netHash.find(_ntk->getInout(index).id);
   if (it != _netHash.end()) { assert (!_ntk->getInout(index).cp); return it->second; }
   // Parent Networks
   string name = "";
   if (_handler) {
      const V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
      uint32_t i = 0, pIndex = index;
      for (; i < ntk->getInoutSize(); ++i)
         if (pIndex < ntk->getNetWidth(ntk->getInout(i))) break;
         else pIndex -= ntk->getNetWidth(ntk->getInout(i));
      name = _handler->getInoutName(i); assert (pIndex < ntk->getNetWidth(ntk->getInout(i)));
      if (name.size() && 1 < ntk->getNetWidth(ntk->getInout(i)))
         name += (V3AuxNameBitPrefix + v3Int2Str(pIndex) + V3AuxNameBitSuffix);
   }
   return name.size() ? name : getNetNameOrFormedWithId(_ntk->getInout(index));
}

void
V3BvBlastAig::getNetName(V3NetId& id, string& name) const {
   if (V3NetUD == id) return;
   // Current Network
   if (!id.cp) {
      V3NetStrHash::const_iterator it = _netHash.find(id.id);
      if (it != _netHash.end()) { name = it->second; return; }
   }
   // Parent Networks
   if (_handler) {
      const V3NetId netId = id; id = getParentNetId(id); _handler->getNetName(id, name);
      if (name.size() && 1 < _handler->getNtk()->getNetWidth(getParentNetId(netId)))
         name += (V3AuxNameBitPrefix + v3Int2Str(getParentIndex(netId)) + V3AuxNameBitSuffix);
   }
}

const V3NetId
V3BvBlastAig::getNetFromName(const string& s) const {
   // Try the full name first
   V3NetId id = V3NtkHandler::getNetFromName(s); if (V3NetUD != id) return id;
   // Try dropping the "[index]" suffix
   if (s.size() < V3AuxNameBitSuffix.size()) return V3NetUD;
   if (s.compare(s.size() - V3AuxNameBitSuffix.size(), V3AuxNameBitSuffix.size(), V3AuxNameBitSuffix)) return V3NetUD;
   const size_t pos = s.find_last_of(V3AuxNameBitPrefix); if (string::npos == pos) return V3NetUD;
   const string name = s.substr(0, 1 + pos - V3AuxNameBitPrefix.size()); if (!name.size()) return V3NetUD; int index;
   if (!v3Str2Int(s.substr(pos + 1, s.size() - pos - V3AuxNameBitSuffix.size()), index) || index < 0) return V3NetUD;
   // Current Network
   if (!index) {
      V3StrNetHash::const_iterator it = _nameHash.find(name);
      if (it != _nameHash.end()) return it->second;
      V3StrIdxHash::const_iterator is = _outNameHash.find(name);
      if (is != _outNameHash.end() && is->second < _ntk->getOutputSize()) return _ntk->getOutput(is->second);
   }
   // Parent Networks
   if (!_handler) return V3NetUD; id = _handler->getNetFromName(name);
   return (V3NetUD == id) ? id : getCurrentNetId(id, index);
}

const V3NetId
V3BvBlastAig::getParentNetId(const V3NetId& id) const {
   if (V3NetUD == id || _c2pMap.size() <= id.id || V3NetUD == _c2pMap[id.id]) return V3NetUD;
   return isV3NetInverted(id) ? getV3InvertNet(_c2pMap[getV3NetIndex(id)]) : _c2pMap[getV3NetIndex(id)];
}

const V3NetId
V3BvBlastAig::getCurrentNetId(const V3NetId& id, const uint32_t& index) const {
   if (V3NetUD == id || _p2cMap.size() <= id.id || _handler->getNtk()->getNetWidth(id) <= index) return V3NetUD;
   return isV3NetInverted(id) ? getV3InvertNet(_p2cMap[getV3NetIndex(id)][index]) : _p2cMap[getV3NetIndex(id)][index];
}

// Ancestry Helper Functions
const uint32_t
V3BvBlastAig::getParentIndex(const V3NetId& id) const {
   const V3NetId parentId = getParentNetId(id); assert (V3NetUD != parentId);
   uint32_t i = 0;
   for (; i < _p2cMap[parentId.id].size(); ++i) if (id.id == _p2cMap[parentId.id][i].id) break;
   assert (i != _p2cMap[parentId.id].size()); return i;
}

// Transformation Functions
void
V3BvBlastAig::performNtkTransformation() {
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(_handler->getNtk()); assert (ntk);
   const uint32_t parentNets = ntk->getNetSize(); assert (parentNets);
   // Initialize Mapper From Parent (Index) to Current (V3NetId) Ntk for the Base Frame
   _p2cMap.resize(parentNets); assert (parentNets == _p2cMap.size()); _c2pMap.clear();
   for (uint32_t i = 0; i < parentNets; ++i) _p2cMap[i].clear(); _p2cMap[0].push_back(V3NetId::makeNetId(0));
   // Compute DFS Order for Transformation
   V3NetVec orderMap; orderMap.clear(); dfsNtkForGeneralOrder(ntk, orderMap);
   assert (orderMap.size() <= parentNets); assert (!orderMap[0].id);
   // Bit-Blast PI / PIO / Latch in Consistent Order
   uint32_t i = 1;
   for (uint32_t j = i + ntk->getInputSize(); i < j; ++i) {
      assert (V3_PI == ntk->getGateType(orderMap[i])); assert (!_p2cMap[orderMap[i].id].size());
      _p2cMap[orderMap[i].id].reserve(ntk->getNetWidth(orderMap[i]));
      for (uint32_t k = 0; k < ntk->getNetWidth(orderMap[i]); ++k) {
         _p2cMap[orderMap[i].id].push_back(_ntk->createNet());
         _ntk->createInput(_p2cMap[orderMap[i].id].back());
      }
   }
   for (uint32_t j = i + ntk->getInoutSize(); i < j; ++i) {
      assert (V3_PIO == ntk->getGateType(orderMap[i])); assert (!_p2cMap[orderMap[i].id].size());
      _p2cMap[orderMap[i].id].reserve(ntk->getNetWidth(orderMap[i]));
      for (uint32_t k = 0; k < ntk->getNetWidth(orderMap[i]); ++k) {
         _p2cMap[orderMap[i].id].push_back(_ntk->createNet());
         _ntk->createInout(_p2cMap[orderMap[i].id].back());
      }
   }
   for (uint32_t j = i + ntk->getLatchSize(); i < j; ++i) {
      assert (V3_FF == ntk->getGateType(orderMap[i])); assert (!_p2cMap[orderMap[i].id].size());
      _p2cMap[orderMap[i].id].reserve(ntk->getNetWidth(orderMap[i]));
      for (uint32_t k = 0; k < ntk->getNetWidth(orderMap[i]); ++k) 
         _p2cMap[orderMap[i].id].push_back(_ntk->createNet());
   }
   // Transform BV Types to AIG from DFS Order
   V3BoolVec moduleList(ntk->getModuleSize(), false); V3GateType type; 
   for (; i < orderMap.size(); ++i) {
      type = ntk->getGateType(orderMap[i]); assert (V3_XD > type);
      assert (V3_MODULE == type || AIG_FALSE < type);
      if (isV3PairType(type)) {
         switch (type) {
            case BV_AND       : bitBlast_AND(ntk, orderMap[i]);      break;
            case BV_XOR       : bitBlast_XOR(ntk, orderMap[i]);      break;
            case BV_ADD       : bitBlast_ADD(ntk, orderMap[i]);      break;
            case BV_SUB       : bitBlast_SUB(ntk, orderMap[i]);      break;
            case BV_SHL       : bitBlast_SHL(ntk, orderMap[i]);      break;
            case BV_SHR       : bitBlast_SHR(ntk, orderMap[i]);      break;
            case BV_MERGE     : bitBlast_MERGE(ntk, orderMap[i]);    break;
            case BV_EQUALITY  : bitBlast_EQUALITY(ntk, orderMap[i]); break;
            case BV_GEQ       : bitBlast_GEQ(ntk, orderMap[i]);      break;
            case BV_MULT      : bitBlast_MULT(ntk, orderMap[i]);     break;
            case BV_DIV       : bitBlast_DIV(ntk, orderMap[i]);      break;
            default           : bitBlast_MODULO(ntk, orderMap[i]);   break;
         }
      }
      else if (isV3ReducedType(type)) {
         switch (type) {
            case BV_RED_AND   : bitBlast_RED_AND(ntk, orderMap[i]);  break;
            case BV_RED_OR    : bitBlast_RED_OR(ntk, orderMap[i]);   break;
            default           : bitBlast_RED_XOR(ntk, orderMap[i]);  break;
         }
      }
      else if (BV_MUX == type) bitBlast_MUX(ntk, orderMap[i]);
      else if (BV_SLICE == type) bitBlast_SLICE(ntk, orderMap[i]);
      else if (BV_CONST == type) bitBlast_CONST(ntk, orderMap[i]);
      else {
         assert (V3_MODULE == type); if (_p2cMap[orderMap[i].id].size()) continue;
         moduleList[V3NetType(ntk->getInputNetId(orderMap[i], 0)).value] = true;
         for (uint32_t k = 0; k < ntk->getNetWidth(orderMap[i]); ++k) 
            _p2cMap[orderMap[i].id].push_back(_ntk->createNet());
      }
   }
   // Construct DFF in Consistent Order
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      assert (ntk->getNetWidth(ntk->getLatch(i)) == _p2cMap[ntk->getLatch(i).id].size());
      bitBlast_FF(ntk, ntk->getLatch(i));
   }
   // Construct PO in Consistent Order
   V3NetId id;
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      assert (ntk->getNetWidth(ntk->getOutput(i)) == _p2cMap[ntk->getOutput(i).id].size());
      for (uint32_t j = 0; j < _p2cMap[ntk->getOutput(i).id].size(); ++j) {
         id = _p2cMap[ntk->getOutput(i).id][j]; assert (V3NetUD != id);
         if (ntk->getOutput(i).cp) id.cp ^= 1; _ntk->createOutput(id);
      }
   }
   // Construct Module Instance in Consistent Order
   V3NetVec cInputs, cOutputs; cInputs.clear(); cOutputs.clear();
   for (i = 0; i < moduleList.size(); ++i) {
      if (!moduleList[i]) continue;
      const V3NetVec& pOutputs = ntk->getModule(i)->getOutputList();
      for (uint32_t j = 0; j < pOutputs.size(); ++j) {
         if (!_p2cMap[pOutputs[j].id].size()) {  // Create Floating Nets for Floating Module Outputs
            for (uint32_t k = 0; k < ntk->getNetWidth(pOutputs[j]); ++k) {
               _p2cMap[pOutputs[j].id].push_back(_ntk->createNet(ntk->getNetWidth(pOutputs[j])));
               assert (_c2pMap.size() == _p2cMap[pOutputs[j].id].back().id); _c2pMap.push_back(pOutputs[j]);
            }
         }
         for (uint32_t k = 0; k < ntk->getNetWidth(pOutputs[j]); ++k) 
            cOutputs.push_back(V3NetId::makeNetId(_p2cMap[pOutputs[j].id][k].id, 
                                                  _p2cMap[pOutputs[j].id][k].cp ^ pOutputs[j].cp));
      }
      const V3NetVec& pInputs = ntk->getModule(i)->getInputList();
      for (uint32_t j = 0; j < pInputs.size(); ++j) {
         assert (_p2cMap[pInputs[j].id].size() == ntk->getNetWidth(pInputs[i]));
         for (uint32_t k = 0; k < ntk->getNetWidth(pInputs[j]); ++k) 
            cInputs.push_back(V3NetId::makeNetId(_p2cMap[pInputs[j].id][k].id, 
                                                 _p2cMap[pInputs[j].id][k].cp ^ pInputs[j].cp));
      }
      // Call Utility Function  (Slower, but more simple and safe)
      createModule(_ntk, cInputs, cOutputs, ntk->getModule(i)->getNtkRef(), ntk->getModule(i)->isNtkRefBlackBoxed());
      cInputs.clear(); cOutputs.clear();
   }
   // Construct Clock
   if (V3NetUD != ntk->getClock()) {
      if (!_p2cMap[ntk->getClock().id].size()) _p2cMap[ntk->getClock().id].push_back(_ntk->createNet());
      V3NetId clockId = _p2cMap[ntk->getClock().id][0]; assert (clockId.id < _ntk->getNetSize());
      if (ntk->getClock().cp) clockId = ~clockId; _ntk->createClock(clockId);
   }
   // Set c2pMap from p2cMap
   i = _p2cMap.size(); _c2pMap = V3NetVec(_ntk->getNetSize(), V3NetUD);
   id = V3NetId::makeNetId(_p2cMap.size() - 1);
   while (i--) {
      for (uint32_t j = 0; j < _p2cMap[i].size(); ++j) _c2pMap[_p2cMap[i][j].id] = _p2cMap[i][j].cp ? ~id : id;
      --id.id;
   }
}

// Transformation Helper Functions
void
V3BvBlastAig::bitBlast_FF(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (V3_FF == ntk->getGateType(id));
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id2));
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id));
   assert (ntk->getNetWidth(id) == _p2cMap[id.id].size());
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;
   
   V3InputVec input(2, V3NetUD); assert (input.size() == 2);
   for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) {
      input[0] = id1Vec[i]; input[1] = id2Vec[i];
      _ntk->setInput(_p2cMap[id.id][i], input); _ntk->createLatch(_p2cMap[id.id][i]);
   }
}

void
V3BvBlastAig::bitBlast_RED_AND(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_RED_AND == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size());
   assert (1 == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(1);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   
   _p2cMap[id.id].push_back(and_red(_ntk, id1Vec));
}

void
V3BvBlastAig::bitBlast_RED_OR(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_RED_OR == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size());
   assert (1 == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(1);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   
   _p2cMap[id.id].push_back(or_red(_ntk, id1Vec));
}

void
V3BvBlastAig::bitBlast_RED_XOR(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_RED_XOR == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size());
   assert (1 == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(1);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   
   _p2cMap[id.id].push_back(xor_red(_ntk, id1Vec));
}

void
V3BvBlastAig::bitBlast_AND(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_AND == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;
   
   for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(and_2(_ntk, id1Vec[i], id2Vec[i]));
}

void
V3BvBlastAig::bitBlast_XOR(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_XOR == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;
   
   for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(xor_2(_ntk, id1Vec[i], id2Vec[i]));
}

void
V3BvBlastAig::bitBlast_ADD(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_ADD == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   const uint32_t width = ntk->getNetWidth(id); V3NetId carry = V3NetId::makeNetId(0);
   _p2cMap[id.id].push_back(xor_2(_ntk, id1Vec[0], id2Vec[0]));
   if (width > 1) carry = carry_2_woc(_ntk, id1Vec[0], id2Vec[0]);
   for (uint32_t i = 1; i < width; ++i) {
      _p2cMap[id.id].push_back(xor_3(_ntk, id1Vec[i], id2Vec[i], carry));
      if (i < width - 1) carry = carry_2(_ntk, id1Vec[i], id2Vec[i], carry);
   }
}

void
V3BvBlastAig::bitBlast_SUB(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_SUB == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   const uint32_t width = ntk->getNetWidth(id); V3NetId borrow = V3NetId::makeNetId(0);
   _p2cMap[id.id].push_back(xor_2(_ntk, id1Vec[0], id2Vec[0]));
   if (width > 1) borrow = borrow_2_woB(_ntk, id1Vec[0], id2Vec[0]);
   for (uint32_t i = 1; i < width; ++i) {
      _p2cMap[id.id].push_back(xor_3(_ntk, id1Vec[i], id2Vec[i], borrow));
      if (i < width - 1) borrow = borrow_2(_ntk, id1Vec[i], id2Vec[i], borrow);
   }
}

void
V3BvBlastAig::bitBlast_MULT(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_MULT == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   const uint32_t width = ntk->getNetWidth(id); assert (width);
   V3NetId carry = V3NetUD; V3NetVec prevResult(width);
   // 1. AND results for corresponding ai * bj
   _p2cMap[id.id].push_back(and_2(_ntk, id1Vec[0], id2Vec[0]));
   for (uint32_t i = 0; i < width; ++i)
      for (uint32_t j = (i == 0) ? i + 1 : i; j < width; ++j) 
         carry = and_2(_ntk, id1Vec[j - i], id2Vec[i]);
   assert (V3NetUD == carry || carry.id == (_p2cMap[id.id][0].id + ((width * (width + 1)) >> 1) - 1));
   // 2. Adders for sum_out and carry
   for (uint32_t i = 1; i < width; ++i) { prevResult[i].id = _p2cMap[id.id][0].id + i; prevResult[i].cp = false; }
   V3NetId out_aig, baseId = _p2cMap[id.id][0]; baseId.id += width; assert (!baseId.cp);
   for (uint32_t i = 1; i < width; ++i) {
      _p2cMap[id.id].push_back(xor_2(_ntk, prevResult[i], baseId));
      if (i < width - 1) carry = carry_2_woc(_ntk, prevResult[i], baseId);
      ++baseId.id; assert (!baseId.cp);
      for (uint32_t j = i + 1; j < width; ++j) {
         out_aig = xor_3(_ntk, prevResult[j], baseId, carry);
         if (j < width - 1) carry = carry_2(_ntk, prevResult[j], baseId, carry);
         prevResult[j] = out_aig; ++baseId.id; assert (!baseId.cp);
      }
   }
   assert (baseId.id == (_p2cMap[id.id][0].id + (width * (width + 1) >> 1)));
}

void
V3BvBlastAig::bitBlast_DIV(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_DIV == ntk->getGateType(id));
   assert (!_p2cMap[id.id].size()); bitBlast_DIV_MODULO(ntk, id, false);
}

void
V3BvBlastAig::bitBlast_MODULO(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_MODULO == ntk->getGateType(id));
   assert (!_p2cMap[id.id].size()); bitBlast_DIV_MODULO(ntk, id, true);
}

void
V3BvBlastAig::bitBlast_SHL(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_SHL == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   // Constant Shift
   if (BV_CONST == ntk->getGateType(id2) && (sizeof(int) << 3) > ntk->getNetWidth(id2)) {
      const V3BitVecX value = ntk->getInputConstValue(id2);
      if (value.size() == value.firstX()) {
         uint32_t shift = value.value(); if (shift > ntk->getNetWidth(id)) shift = ntk->getNetWidth(id);
         for (uint32_t i = 0; i < shift; ++i) _p2cMap[id.id].push_back(V3NetId::makeNetId(0));
         for (uint32_t i = shift; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(id1Vec[i - shift]);
         return;
      }
   }
   // Non-deterministic Shift
   const uint32_t width = ntk->getNetWidth(id), bWidth = ntk->getNetWidth(id2);
   uint32_t shift_width = (int)ceil(log2(width + 1));
   shift_width = (shift_width < bWidth) ? shift_width : bWidth;
   uint32_t j, pow_shift;
   for (uint32_t i = 0; i < width; ++i) _p2cMap[id.id].push_back(id1Vec[i]);
   for (uint32_t i = 0; i < shift_width; ++i) {
      pow_shift = (uint32_t)pow(2.00, (int)i);  // Sub-Shift Process : a << pow_shift
      j = width;
      while (j-- > pow_shift)
         _p2cMap[id.id][j] = select(_ntk, _p2cMap[id.id][j - pow_shift], _p2cMap[id.id][j], id2Vec[i]);
      j = (pow_shift < width) ? pow_shift : width;
      while (j--) _p2cMap[id.id][j] = and_2(_ntk, _p2cMap[id.id][j], ~id2Vec[i]);
   }
}

void
V3BvBlastAig::bitBlast_SHR(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_SHR == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   // Constant Shift
   if (BV_CONST == ntk->getGateType(id2) && (sizeof(int) << 3) > ntk->getNetWidth(id2)) {
      const V3BitVecX value = ntk->getInputConstValue(id2);
      if (value.size() == value.firstX()) {
         const uint32_t shift = value.value();
         for (uint32_t i = shift; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(id1Vec[i]);
         while (ntk->getNetWidth(id) > _p2cMap[id.id].size()) _p2cMap[id.id].push_back(V3NetId::makeNetId(0));
         return;
      }
   }
   // Non-deterministic Shift
   const uint32_t width = ntk->getNetWidth(id), bWidth = ntk->getNetWidth(id2);
   uint32_t shift_width = (int)ceil(log2(width + 1));
   shift_width = (shift_width < bWidth) ? shift_width : bWidth;
   uint32_t pow_shift;
   for (uint32_t i = 0; i < width; ++i) _p2cMap[id.id].push_back(id1Vec[i]);
   for (uint32_t i = 0; i < shift_width; ++i) {
      pow_shift = (uint32_t)pow(2.00, (int)i);  // Sub-Shift Process : a >> pow_shift
      if (width > pow_shift)
         for (uint32_t j = pow_shift; j < width; ++j) 
            _p2cMap[id.id][j - pow_shift] = select(_ntk, _p2cMap[id.id][j], _p2cMap[id.id][j - pow_shift], id2Vec[i]);
      for (uint32_t j = 0; j < pow_shift; ++j) 
         _p2cMap[id.id][width + j - pow_shift] = and_2(_ntk, _p2cMap[id.id][width + j - pow_shift], ~id2Vec[i]);
   }
}

void
V3BvBlastAig::bitBlast_MERGE(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_MERGE == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert ((ntk->getNetWidth(id1) + ntk->getNetWidth(id2)) == ntk->getNetWidth(id));
   _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   for (uint32_t i = 0; i < ntk->getNetWidth(id2); ++i) _p2cMap[id.id].push_back(id2Vec[i]);
   for (uint32_t i = 0; i < ntk->getNetWidth(id1); ++i) _p2cMap[id.id].push_back(id1Vec[i]);
}

void
V3BvBlastAig::bitBlast_EQUALITY(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_EQUALITY == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (1 == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(1);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   V3NetVec auxNet; auxNet.clear(); auxNet.reserve(ntk->getNetWidth(id1));
   for (uint32_t i = 0; i < ntk->getNetWidth(id1); ++i) 
      auxNet.push_back(xnor_2(_ntk, id1Vec[i], id2Vec[i]));
   _p2cMap[id.id].push_back(and_red(_ntk, auxNet));
}

void
V3BvBlastAig::bitBlast_GEQ(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_GEQ == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (1 == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(1);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   V3NetVec eqNet; eqNet.clear(); eqNet.reserve(ntk->getNetWidth(id1) - 1);
   V3NetVec cpNet; cpNet.clear(); cpNet.reserve(ntk->getNetWidth(id1));
   cpNet.push_back(geq_2(_ntk, id1Vec[0], id2Vec[0]));
   for (uint32_t i = 1; i < ntk->getNetWidth(id1); ++i) {
      eqNet.push_back(xnor_2(_ntk, id1Vec[i], id2Vec[i]));
      cpNet.push_back(greater_2(_ntk, id1Vec[i], id2Vec[i]));
   }
   _p2cMap[id.id].push_back(compare_generate(_ntk, eqNet, cpNet));
}

void
V3BvBlastAig::bitBlast_MUX(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_MUX == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId fId = ntk->getInputNetId(id, 0); assert (fId.id < ntk->getNetSize());
   const V3NetId tId = ntk->getInputNetId(id, 1); assert (tId.id < ntk->getNetSize());
   const V3NetId sId = ntk->getInputNetId(id, 2); assert (sId.id < ntk->getNetSize());
   assert (ntk->getNetWidth(fId) == _p2cMap[fId.id].size()); assert (ntk->getNetWidth(tId) == _p2cMap[tId.id].size());
   assert (ntk->getNetWidth(sId) == _p2cMap[sId.id].size()); assert (1 == ntk->getNetWidth(sId));
   assert (ntk->getNetWidth(fId) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec fVec = _p2cMap[fId.id]; if (fId.cp) for (uint32_t i = 0; i < fVec.size(); ++i) fVec[i].cp ^= 1;
   V3NetVec tVec = _p2cMap[tId.id]; if (tId.cp) for (uint32_t i = 0; i < tVec.size(); ++i) tVec[i].cp ^= 1;
   V3NetVec sVec = _p2cMap[sId.id]; if (sId.cp) for (uint32_t i = 0; i < sVec.size(); ++i) sVec[i].cp ^= 1;

   for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i)
      _p2cMap[id.id].push_back(mux(_ntk, fVec[i], tVec[i], sVec[0]));
}

void
V3BvBlastAig::bitBlast_SLICE(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_SLICE == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   if (ntk->getNetWidth(id1) != _p2cMap[id1.id].size()) {
      Msg(MSG_DBG) << "Prev = " << _handler->getNetNameOrFormedWithId(id1) 
                   << ", expr = " << _handler->getNetExpression(id1) << endl;
      Msg(MSG_DBG) << "Cur = " << _handler->getNetNameOrFormedWithId(id) 
                   << ", expr = " << _handler->getNetExpression(id) << endl;
   }
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size());
   const uint32_t msb = ntk->getInputSliceBit(id, true), lsb = ntk->getInputSliceBit(id, false);
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;

   if (msb >= lsb) {
      assert (msb < ntk->getNetWidth(id1)); assert ((1 + msb - lsb) == ntk->getNetWidth(id));
      for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(id1Vec[lsb + i]);
   }
   else {
      assert (lsb < ntk->getNetWidth(id1)); assert ((1 + lsb - msb) == ntk->getNetWidth(id));
      for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) _p2cMap[id.id].push_back(id1Vec[lsb - i]);
   }
}

void
V3BvBlastAig::bitBlast_CONST(const V3BvNtk* const ntk, const V3NetId& id) {
   assert (ntk); assert (BV_CONST == ntk->getGateType(id)); assert (!_p2cMap[id.id].size());
   const V3BitVecX value = ntk->getInputConstValue(V3NetId::makeNetId(id.id));
   assert (value.size() == ntk->getNetWidth(id)); _p2cMap.reserve(ntk->getNetWidth(id));

   V3NetId zeroId = V3NetId::makeNetId(0); V3NetId oneId = ~zeroId;
   for (uint32_t i = 0; i < ntk->getNetWidth(id); ++i) {
      if ('X' == value[i]) Msg(MSG_WAR) << "Transform \'X\' into \'0\' in AIG Network !!" << endl;
      _p2cMap[id.id].push_back(('1' == value[i]) ? oneId : zeroId);
   }
}

// Divider / Modular Helper Functions
void
V3BvBlastAig::bitBlast_DIV_MODULO(const V3BvNtk* const ntk, const V3NetId& id, const bool& setRemainder) {
   const V3NetId id1 = ntk->getInputNetId(id, 0); assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = ntk->getInputNetId(id, 1); assert (id2.id < ntk->getNetSize());
   assert (ntk->getNetWidth(id1) == _p2cMap[id1.id].size()); assert (ntk->getNetWidth(id2) == _p2cMap[id2.id].size());
   assert (ntk->getNetWidth(id1) == ntk->getNetWidth(id)); _p2cMap[id.id].reserve(ntk->getNetWidth(id));
   V3NetVec id1Vec = _p2cMap[id1.id]; if (id1.cp) for (uint32_t i = 0; i < id1Vec.size(); ++i) id1Vec[i].cp ^= 1;
   V3NetVec id2Vec = _p2cMap[id2.id]; if (id2.cp) for (uint32_t i = 0; i < id2Vec.size(); ++i) id2Vec[i].cp ^= 1;

   const uint32_t width = ntk->getNetWidth(id); V3NetId borrow;
   V3NetVec sVec(width), qVec(width), rVec(width); uint32_t i, j;
   for (i = 0; i < width; ++i) rVec[i] = V3NetId::makeNetId(0);
   _p2cMap[id.id] = V3NetVec(width, V3NetUD); i = width;
   while (i--) {
      // Set current dividen r = {(r << 1), a[i]}
      j = width; while (--j) rVec[j] = rVec[j - 1]; rVec[0] = id1Vec[i];
      // Compute s = r - id2 and the final borrow
      sVec[0] = xor_2(_ntk, rVec[0], id2Vec[0]);
      borrow = borrow_2_woB(_ntk, rVec[0], id2Vec[0]);
      for (j = 1; j < width; ++j) {
         sVec[j] = xor_3(_ntk, rVec[j], id2Vec[j], borrow);
         borrow = borrow_2(_ntk, rVec[j], id2Vec[j], borrow);
      }
      // Set remainder = borrow ? rVec : sVec, and quotient = !borrow
      _p2cMap[id.id][i] = ~borrow;
      for (j = 0; j < width; ++j) rVec[j] = mux(_ntk, sVec[j], rVec[j], borrow);
   }
   if (setRemainder) for (i = 0; i < width; ++i) _p2cMap[id.id][i] = rVec[i];
}

#endif

