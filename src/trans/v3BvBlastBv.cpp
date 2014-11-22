/****************************************************************************
  FileName     [ v3BvBlastBv.cpp ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Bit-Blasting (Pseudo) Primary Ports for BV Networks. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BV_BLAST_BV_C
#define V3_BV_BLAST_BV_C

#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3BvBlastBv.h"

/* -------------------------------------------------- *\
 * Class V3BvBlastBv Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3BvBlastBv::V3BvBlastBv(V3NtkHandler* const p) : V3NtkHandler(p) {
   assert (dynamic_cast<V3BvNtk*>(p->getNtk()));
   _ntk = new V3BvNtk(*(dynamic_cast<V3BvNtk*>(p->getNtk())));
   assert (_ntk); assert (dynamic_cast<V3BvNtk*>(_ntk));
   // Perform Network Transformation
   _c2pMap.clear(); _p2cMap.clear(); performNtkTransformation();
   // Reserve Mapping Tables if Needed
   if (!V3NtkHandler::P2CMapON()) _p2cMap.clear();
   if (!V3NtkHandler::C2PMapON()) _c2pMap.clear();
}

V3BvBlastBv::~V3BvBlastBv() {
   _c2pMap.clear(); _p2cMap.clear();
}

// I/O Ancestry Functions
const string
V3BvBlastBv::getInputName(const uint32_t& index) const {
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
V3BvBlastBv::getOutputName(const uint32_t& index) const {
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
V3BvBlastBv::getInoutName(const uint32_t& index) const {
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
V3BvBlastBv::getNetName(V3NetId& id, string& name) const {
   if (V3NetUD == id) return;
   // Current Network
   if (!id.cp) {
      V3NetStrHash::const_iterator it = _netHash.find(id.id);
      if (it != _netHash.end()) { name = it->second; return; }
   }
   // Parent Networks
   if (_handler) {
      const V3NetId netId = id; id = getParentNetId(id); _handler->getNetName(id, name);
      if (_handler->getNtk()->getNetSize() <= netId.id && name.size()) 
         name += (V3AuxNameBitPrefix + v3Int2Str(getParentIndex(netId)) + V3AuxNameBitSuffix);
   }
}

const V3NetId
V3BvBlastBv::getNetFromName(const string& s) const {
   // Try the full name first
   V3StrNetHash::const_iterator it = _nameHash.find(s);
   if (it != _nameHash.end()) return it->second;
   V3StrIdxHash::const_iterator is = _outNameHash.find(s);
   if (is != _outNameHash.end() && is->second < _ntk->getOutputSize()) return _ntk->getOutput(is->second);
   if (_handler) { V3NetId id = _handler->getNetFromName(s); if (V3NetUD != id) return getCurrentNetId(id, V3NtkUD); }
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
   if (!_handler) return V3NetUD; const V3NetId id = _handler->getNetFromName(name);
   return (V3NetUD == id) ? id : getCurrentNetId(id, index);
}

const V3NetId
V3BvBlastBv::getParentNetId(const V3NetId& id) const {
   if (V3NetUD == id || !_handler || !_handler->getNtk()) return V3NetUD;
   const uint32_t size = _handler->getNtk()->getNetSize(); if (size > id.id) return id;
   if (id.id >= _ntk->getNetSize() || (id.id - size) >= _c2pMap.size()) return V3NetUD;
   return V3NetId::makeNetId(_c2pMap[id.id - size], id.cp);
}

const V3NetId
V3BvBlastBv::getCurrentNetId(const V3NetId& id, const uint32_t& index) const {
   if (V3NetUD == id || !_handler || !_handler->getNtk()) return V3NetUD;
   if (id.id >= _handler->getNtk()->getNetSize()) return V3NetUD;
   if (V3_FF < _handler->getNtk()->getGateType(id) || 1 == _handler->getNtk()->getNetWidth(id))
      return (index && (V3NtkUD != index)) ? V3NetUD : id;
   V3BvBlastBvMap::const_iterator it = _p2cMap.find(id.id); if (_p2cMap.end() == it) return V3NetUD;
   if (V3NtkUD == index) return id;  // The Net that Merges Bit-Blasted PI / PIO / FF
   assert (_ntk->getNetSize() > (index + it->second)); return V3NetId::makeNetId(it->second, id.cp);
}

// Ancestry Helper Functions
const uint32_t
V3BvBlastBv::getParentIndex(const V3NetId& id) const {
   assert (id.id >= _handler->getNtk()->getNetSize()); assert (V3_FF >= _ntk->getGateType(id));
   const V3NetId parentId = getParentNetId(id); assert (V3NetUD != parentId);
   V3BvBlastBvMap::const_iterator it = _p2cMap.find(parentId.id);
   assert (_p2cMap.end() != it); assert (id.id >= it->second);
   assert ((id.id - it->second) < _handler->getNtk()->getNetWidth(parentId));
   return id.id - it->second;
}

// Transformation Functions
void
V3BvBlastBv::performNtkTransformation() {
   assert (_ntk); assert (!_p2cMap.size()); assert (!_c2pMap.size());
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (_ntk->getNetSize() == ntk->getNetSize());
   // Clear PI / PIO / FF / PO List
   while (_ntk->getInputSize()) { _ntk->removeLastInput(); }
   while (_ntk->getInoutSize()) { _ntk->removeLastInout(); }
   while (_ntk->getLatchSize()) { _ntk->removeLastLatch(); }
   while (_ntk->getOutputSize()) { _ntk->removeLastOutput(); }
   // Bit-blasting every PI with bit-width > 1
   for (uint32_t i = 0, size; i < ntk->getInputSize(); ++i) {
      size = ntk->getNetWidth(ntk->getInput(i)); if (1 == size) continue;
      assert (_p2cMap.end() == _p2cMap.find(ntk->getInput(i).id));
      _p2cMap.insert(make_pair(ntk->getInput(i).id, _ntk->getNetSize()));
      for (uint32_t j = 0; j < size; ++j) { _c2pMap.push_back(ntk->getInput(i).id); _ntk->createNet(); }
   }
   // Bit-blasting every PIO with bit-width > 1
   for (uint32_t i = 0, size; i < ntk->getInoutSize(); ++i) {
      size = ntk->getNetWidth(ntk->getInout(i)); if (1 == size) continue;
      assert (_p2cMap.end() == _p2cMap.find(ntk->getInout(i).id));
      _p2cMap.insert(make_pair(ntk->getInout(i).id, _ntk->getNetSize()));
      for (uint32_t j = 0; j < size; ++j) { _c2pMap.push_back(ntk->getInout(i).id); _ntk->createNet(); }
   }
   // Bit-blasting every FF with bit-width > 1
   for (uint32_t i = 0, size; i < ntk->getLatchSize(); ++i) {
      size = ntk->getNetWidth(ntk->getLatch(i)); if (1 == size) continue;
      assert (_p2cMap.end() == _p2cMap.find(ntk->getLatch(i).id));
      _p2cMap.insert(make_pair(ntk->getLatch(i).id, _ntk->getNetSize()));
      for (uint32_t j = 0; j < size; ++j) { _c2pMap.push_back(ntk->getLatch(i).id); _ntk->createNet(); }
   }
   // Create Primary Inputs
   V3NetId id = V3NetId::makeNetId(ntk->getNetSize());
   V3InputVec inputs(2, V3NtkUD); uint32_t size;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      size = ntk->getNetWidth(ntk->getInput(i));
      if (1 == size) { _ntk->createInput(ntk->getInput(i)); continue; }
      mergeBitBlastedNets(id, size, ntk->getInput(i));
      for (uint32_t j = 0; j < size; ++j, ++id.id) _ntk->createInput(id);
   }
   // Create Primary Inouts
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      size = ntk->getNetWidth(ntk->getInout(i)); _ntk->freeNetId(ntk->getInout(i));
      if (1 == size) {
         inputs[0] = ntk->getInputNetId(ntk->getInout(i), 0);
         inputs[1] = ntk->getInputNetId(ntk->getInout(i), 1);
         _ntk->setInput(ntk->getInout(i), inputs); _ntk->createInout(ntk->getInout(i)); continue;
      }
      mergeBitBlastedNets(id, size, ntk->getInout(i));
      for (uint32_t j = 0; j < size; ++j, ++id.id) {
         inputs[0] = getBitBlastedNet(ntk->getInputNetId(ntk->getInout(i), 0), j);
         inputs[1] = getBitBlastedNet(ntk->getInputNetId(ntk->getInout(i), 1), j);
         _ntk->setInput(id, inputs); _ntk->createInout(id);
      }
   }
   // Create Latches
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      size = ntk->getNetWidth(ntk->getLatch(i)); _ntk->freeNetId(ntk->getLatch(i));
      if (1 == size) {
         inputs[0] = ntk->getInputNetId(ntk->getLatch(i), 0);
         inputs[1] = getBitBlastedNet(ntk->getInputNetId(ntk->getLatch(i), 1), 0);
         _ntk->setInput(ntk->getLatch(i), inputs); _ntk->createLatch(ntk->getLatch(i)); continue;
      }
      mergeBitBlastedNets(id, size, ntk->getLatch(i));
      for (uint32_t j = 0; j < size; ++j, ++id.id) {
         inputs[0] = getBitBlastedNet(ntk->getInputNetId(ntk->getLatch(i), 0), j);
         inputs[1] = getBitBlastedNet(ntk->getInputNetId(ntk->getLatch(i), 1), j);
         _ntk->setInput(id, inputs); _ntk->createLatch(id);
      }
   }
   // Bit-blasting every PO with bit-width > 1
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      size = ntk->getNetWidth(ntk->getOutput(i));
      if (1 == size) { _ntk->createOutput(ntk->getOutput(i)); continue; }
      V3BvBlastBvMap::const_iterator it = _p2cMap.find(ntk->getOutput(i).id);
      if (_p2cMap.end() != it) {
         for (uint32_t j = 0; j < size; ++j)
            _ntk->createOutput(V3NetId::makeNetId(j + it->second, ntk->getOutput(i).cp));
      }
      else {
         for (uint32_t j = 0; j < size; ++j) {
            inputs[0] = ntk->getOutput(i); inputs[1] = V3BvNtk::hashV3BusId(j, j); id = _ntk->createNet();
            _ntk->setInput(id, inputs); _ntk->createGate(BV_SLICE, id); _ntk->createOutput(id);
         }
      }
   }
}

// Transformation Helper Functions
void
V3BvBlastBv::mergeBitBlastedNets(V3NetId id, const uint32_t& size, const V3NetId& mergedId) {
   assert (id.id < _ntk->getNetSize()); assert (mergedId.id < _handler->getNtk()->getNetSize());
   assert (size > 1); assert (size == _handler->getNtk()->getNetWidth(mergedId));
   V3InputVec inputs(2, V3NtkUD); V3NetId id1;
   for (uint32_t i = 1; i < size; ++i, ++id.id) {
      if (V3NtkUD == inputs[1].value) inputs[1] = id; else inputs[0] = id;
      if (V3NtkUD == inputs[0].value) continue; id1 = _ntk->createNet(i);
      _ntk->setInput(id1, inputs); _ntk->createGate(BV_MERGE, id1); inputs[1] = id1;
   }
   assert (V3NtkUD != inputs[1].value); inputs[0] = id;
   _ntk->setInput(mergedId, inputs); _ntk->createGate(BV_MERGE, mergedId);
}

const V3NetId
V3BvBlastBv::getBitBlastedNet(const V3NetId& id, const uint32_t& index) {
   assert (id.id < _handler->getNtk()->getNetSize()); assert (index < _handler->getNtk()->getNetWidth(id));
   V3BvBlastBvMap::const_iterator it = _p2cMap.find(id.id);
   if (_p2cMap.end() != it) return V3NetId::makeNetId(index + it->second, id.cp);
   if (BV_CONST == _handler->getNtk()->getGateType(id)) {
      const V3BitVecX value = dynamic_cast<V3BvNtk*>(_handler->getNtk())->getInputConstValue(id);
      assert ('X' != value[index]); return V3NetId::makeNetId(0, '1' == value[index]);
   }
   else {
      const V3NetId netId = _ntk->createNet(); V3InputVec inputs(2, V3NetUD);
      inputs[0] = id; inputs[1] = V3BvNtk::hashV3BusId(index, index);
      _ntk->setInput(netId, inputs); _ntk->createGate(BV_SLICE, netId); return netId;
   }
}

#endif

