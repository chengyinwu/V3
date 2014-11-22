/****************************************************************************
  FileName     [ v3NtkInput.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Base Input Handler for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_INPUT_C
#define V3_NTK_INPUT_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3NtkInput.h"

/* -------------------------------------------------- *\
 * Class V3Parser Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3NtkInput::V3NtkInput(const bool& isAig, const string& name) : V3NtkHandler(0, createV3Ntk(!isAig)) {
   assert (_ntk); if (name.size()) setNtkName(name);
   _nameHash.insert(make_pair("0", V3NetId::makeNetId(0))); _netHash.insert(make_pair(0, "0"));
}

V3NtkInput::~V3NtkInput() {
}

// Net Ancestry Functions
const V3NetId
V3NtkInput::createNet(const string& netName, uint32_t width) {
   assert (width);
   V3NetId id = (netName.size()) ? getNetId(netName) : V3NetUD;
   if (id == V3NetUD) {
      id = _ntk->createNet(width); assert (V3NetUD != id);
      if (netName.size()) {
         _nameHash.insert(make_pair(netName, id));
         _netHash.insert(make_pair(id.id, netName));
      }
      return id;
   }
   else if (width == _ntk->getNetWidth(id)) return id;
   Msg(MSG_ERR) << "Existing net \"" << netName << "\" has width = " << _ntk->getNetWidth(id) << " != " << width << endl;
   return V3NetUD;
}

const V3NetId
V3NtkInput::getNetId(const string& netName) const {
   V3StrNetHash::const_iterator it = _nameHash.find(netName);
   return (it != _nameHash.end()) ? it->second : V3NetUD;
}

// Extended Helper Functions
void
V3NtkInput::removePrefixNetName(V3Str2BoolFuncPtr toRemove) {
   assert (_nameHash.size() == _netHash.size());
   V3NetStrHash::iterator it = _netHash.begin();
   while (_netHash.end() != it) {
      if (!toRemove(it->second)) ++it;
      else {
         assert (_nameHash.end() != _nameHash.find(it->second));
         assert (it->first == _nameHash.find(it->second)->second.id);
         _nameHash.erase(_nameHash.find(it->second)); _netHash.erase(it++);
      }
   }
   assert (_nameHash.size() == _netHash.size());
}

void
V3NtkInput::renderFreeNetAsInput() {
   V3NetId id = V3NetId::makeNetId(1); assert (!_ntk->getInputSize());
   for (uint32_t i = 1; i < _ntk->getNetSize(); ++i, ++id.id) {
      if (V3_PI != _ntk->getGateType(id)) continue;
      assert (i == id.id); _ntk->createInput(id);
   }
}

#endif

