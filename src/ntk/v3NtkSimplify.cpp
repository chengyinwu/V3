/****************************************************************************
  FileName     [ v3NtkSimplify.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Simplifications for V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_SIMPLIFY_C
#define V3_NTK_SIMPLIFY_C

#include "v3NtkUtil.h"
#include "v3NtkSimplify.h"

/* -------------------------------------------------- *\
 * Class V3NtkSimplify Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3NtkSimplify::V3NtkSimplify(V3NtkHandler* const handler) : V3NtkHandler(handler) {
   assert (_handler); assert (!_ntk); _c2pMap.clear(); _p2cMap.clear();
   // Duplicate Ntk
   _ntk = duplicateNtk(_handler, _p2cMap, _c2pMap); assert (_ntk);
   // Duplicate PO
   for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i) {
      V3NetId id = handler->getNtk()->getOutput(i); assert (V3NetUD != _p2cMap[id.id]);
      _ntk->createOutput(V3NetId::makeNetId(_p2cMap[id.id].id, _p2cMap[id.id].cp ^ id.cp));
   }
   // Reserve Mapping Tables if Needed
   if (!V3NtkHandler::P2CMapON()) _p2cMap.clear();
   if (!V3NtkHandler::C2PMapON()) _c2pMap.clear();
}

V3NtkSimplify::~V3NtkSimplify() {
   _c2pMap.clear(); _p2cMap.clear();
}

// Net Ancestry Functions
const V3NetId
V3NtkSimplify::getParentNetId(const V3NetId& id) const {
   if (V3NetUD == id || _c2pMap.size() <= id.id) return V3NetUD;
   return (isV3NetInverted(id) ? getV3InvertNet(_c2pMap[id.id]) : _c2pMap[id.id]);
}

const V3NetId
V3NtkSimplify::getCurrentNetId(const V3NetId& id, const uint32_t& index) const {
   if (V3NetUD == id || _p2cMap.size() <= id.id) return V3NetUD;
   return (isV3NetInverted(id) ? getV3InvertNet(_p2cMap[id.id]) : _p2cMap[id.id]);
}

#endif

