/****************************************************************************
  FileName     [ v3NtkFlatten.cpp ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Flatten Hierarchical Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_FLATTEN_C
#define V3_NTK_FLATTEN_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkFlatten.h"

/* -------------------------------------------------- *\
 * Class V3NtkFlatten Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3NtkFlatten::V3NtkFlatten(V3NtkHandler* const handler, const uint32_t& level) : V3NtkHandler(handler) {
   assert (_handler); assert (_handler->getNtk());
   // Perform Network Transformation
   _c2pMap.clear(); _hierInfo.clear();
   // Flatten Ntk
   assert (!reportInconsistentRefNtk(_handler->getNtk()));
   _ntk = flattenNtk(_handler, _c2pMap, _hierInfo, level); assert (_ntk);
   // Duplicate PO
   const V3NetVec& p2cMap = _hierInfo.getMap(0); assert (_handler == _hierInfo.getHandler(0));
   for (uint32_t i = 0; i < _handler->getNtk()->getOutputSize(); ++i) {
      V3NetId id = _handler->getNtk()->getOutput(i); assert (V3NetUD != p2cMap[id.id]);
      id = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp); _ntk->createOutput(id);
   }
   // Reserve Mapping Tables if Needed
   if (!V3NtkHandler::P2CMapON() && !V3NtkHandler::C2PMapON()) _hierInfo.clear();
   else if (!V3NtkHandler::P2CMapON()) _hierInfo.clearMap();
   else if (!V3NtkHandler::C2PMapON()) _hierInfo.clearRefId();
   if (!V3NtkHandler::C2PMapON()) _c2pMap.clear();
}

V3NtkFlatten::~V3NtkFlatten() {
   _c2pMap.clear(); _hierInfo.clear();
}

// Net Ancestry Functions
void
V3NtkFlatten::getNetName(V3NetId& id, string& name) const {
   if (V3NetUD == id) return;
   // Current Network
   if (!id.cp) {
      V3NetStrHash::const_iterator it = _netHash.find(id.id);
      if (it != _netHash.end()) { name = it->second; return; }
   }
   // Parent Networks
   if (_handler) {
      const V3NetId netId = id; id = getParentNetId(netId); if (V3NetUD == id) return;
      uint32_t pIndex = getParentIndex(netId); assert (pIndex < _hierInfo.getNtkSize());
      _hierInfo.getHandler(pIndex)->getNetName(id, name);
      if (name.size() && pIndex) {
         do {
            name = _hierInfo.getHandler(pIndex)->getNtkName() + V3AuxHierSeparator + name;
            pIndex = _hierInfo.getParentId(pIndex);
         } while (pIndex);
      }
   }
}

const V3NetId
V3NtkFlatten::getNetFromName(const string& s) const {
   // Current Network
   V3StrNetHash::const_iterator it = _nameHash.find(s);
   if (it != _nameHash.end()) return it->second;
   V3StrIdxHash::const_iterator is = _outNameHash.find(s);
   if (is != _outNameHash.end() && is->second < _ntk->getOutputSize()) return _ntk->getOutput(is->second);
   // Try dropping the "." hierarchy separators
   if (s.size() >= V3AuxHierSeparator.size()) {
      size_t pos = s.find_last_of(V3AuxHierSeparator);
      if (string::npos != pos) {
         // Submodule of Parent Networks
         string path = s.substr(0, 1 + pos - V3AuxHierSeparator.size());
         const string name = s.substr(pos + 1); pos = path.find_last_of(V3AuxHierSeparator);
         string handlerName = (string::npos == pos) ? path : path.substr(pos + 1); uint32_t i = 0;
         for (; i < _hierInfo.getNtkSize(); ++i) {
            if (handlerName != _hierInfo.getHandler(i)->getNtkName()) continue;
            // Compute the path name for a matched network in the hierarchy
            uint32_t pIndex = i; string pathName = handlerName;
            while (pIndex = _hierInfo.getParentId(pIndex))
               pathName = _hierInfo.getHandler(pIndex)->getNtkName() + V3AuxHierSeparator + pathName;
            if (path != pathName) continue;
            // Return the net if it is found
            V3NetId id = _hierInfo.getHandler(i)->getNetFromName(name);
            if (V3NetUD != id) id = getCurrentNetId(id, i);
            if (V3NetUD != id) return id;
         }
         if (i == _hierInfo.getNtkSize()) return V3NetUD;
      }
   }
   // Parent Networks
   const V3NetId id = _hierInfo.getHandler(0)->getNetFromName(s);
   return (V3NetUD == id) ? id : getCurrentNetId(id, 0);
}

const V3NetId
V3NtkFlatten::getParentNetId(const V3NetId& id) const {
   if (V3NetUD == id || _c2pMap.size() <= id.id || V3NetUD == _c2pMap[id.id]) return V3NetUD;
   return isV3NetInverted(id) ? getV3InvertNet(_c2pMap[getV3NetIndex(id)]) : _c2pMap[getV3NetIndex(id)];
}

const V3NetId
V3NtkFlatten::getCurrentNetId(const V3NetId& id, const uint32_t& handlerIndex) const {
   if (V3NetUD == id) return V3NetUD; assert (handlerIndex < _hierInfo.getNtkSize());
   const V3NetVec& p2cMap = _hierInfo.getMap(handlerIndex); if (p2cMap.size() <= id.id) return V3NetUD;
   return isV3NetInverted(id) ? getV3InvertNet(p2cMap[id.id]) : p2cMap[id.id];
}

// Ancestry Helper Functions
const uint32_t
V3NtkFlatten::getParentIndex(const V3NetId& id) const {
   for (uint32_t i = 0; i < _hierInfo.getRefSize(); ++i) 
      if (id.id < _hierInfo.getRefId(i).bus[0]) return _hierInfo.getRefId(i).bus[1];
   return (id.id < _ntk->getNetSize()) ? 0 : _hierInfo.getNtkSize();
}

#endif

