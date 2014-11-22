/****************************************************************************
  FileName     [ v3NtkStrash.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Implementation of Structural Hashing. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_STRASH_C
#define V3_NTK_STRASH_C

#include "v3NtkHash.h"
#include "v3NtkStrash.h"

// Structural Hashing Sub-Functions for BV Network
const V3NetId strashBvPairTypeGate(V3BvNtk* const ntk, const V3GateType& type, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (isV3PairType(type)); assert (2 == inputs.size());
   const uint64_t hashKey = isV3ExchangableType(type) ? V3UnorderedNetType2HashKey(inputs[0], inputs[1]) 
                                                      : V3OrderedNetType2HashKey(inputs[0], inputs[1]);
   if (netHash.existList(type)) {
      V3UI64Hash* hashTable = (V3UI64Hash*)(netHash.getList(type)); assert (hashTable);
      V3UI64Hash::const_iterator it = hashTable->find(hashKey); if (hashTable->end() != it) return it->second;
   }
   return V3NetUD;
}

const V3NetId strashBvReducedTypeGate(V3BvNtk* const ntk, const V3GateType& type, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (isV3ReducedType(type)); assert (1 == inputs.size());
   const uint32_t hashKey = V3NetType2HashKey(inputs[0]);
   if (netHash.existList(type)) {
      V3UI32Hash* hashTable = (V3UI32Hash*)(netHash.getList(type)); assert (hashTable);
      V3UI32Hash::const_iterator it = hashTable->find(hashKey); if (hashTable->end() != it) return it->second;
   }
   return V3NetUD;
}

const V3NetId strashBvMuxGate(V3BvNtk* const ntk, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (3 == inputs.size());
   const V3MuxKey hashKey = V3MuxNetType2HashKey(inputs[0], inputs[1], inputs[2]);
   if (netHash.existList(BV_MUX)) {
      V3MuxHash* hashTable = (V3MuxHash*)(netHash.getList(BV_MUX)); assert (hashTable);
      V3MuxHash::const_iterator it = hashTable->find(hashKey); if (hashTable->end() != it) return it->second;
   }
   return V3NetUD;
}

const V3NetId strashBvSliceGate(V3BvNtk* const ntk, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (2 == inputs.size());
   const uint64_t hashKey = V3OrderedNetType2HashKey(inputs[0], inputs[1]);
   if (netHash.existList(BV_SLICE)) {
      V3UI64Hash* hashTable = (V3UI64Hash*)(netHash.getList(BV_SLICE)); assert (hashTable);
      V3UI64Hash::const_iterator it = hashTable->find(hashKey); if (hashTable->end() != it) return it->second;
   }
   return V3NetUD;
}

const V3NetId strashBvConstGate(V3BvNtk* const ntk, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (1 == inputs.size());
   const uint32_t hashKey = V3NetType2HashKey(inputs[0]);
   V3UI32Hash* hashTable = 0;
   if (netHash.existList(BV_CONST)) {
      hashTable = (V3UI32Hash*)(netHash.getList(BV_CONST));
      assert (hashTable);
   }
   else {  // Create Hash Table for CONST
      hashTable = new V3UI32Hash(); assert (hashTable);
      netHash.setList(BV_CONST, (size_t)hashTable);
      hashTable->insert(make_pair(V3NetType2HashKey(0), V3NetId::makeNetId(0)));
   }
   V3UI32Hash::const_iterator it = hashTable->find(hashKey);
   if (hashTable->end() != it) return it->second; return V3NetUD;
}

void strashBvPairTypeGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (2 == ntk->getInputNetSize(id));
   const V3GateType& type = ntk->getGateType(id); assert (isV3PairType(type));
   const uint64_t hashKey = isV3ExchangableType(type) ? 
                            V3UnorderedNetType2HashKey(ntk->getInputNetId(id, 0), ntk->getInputNetId(id, 1)) : 
                            V3OrderedNetType2HashKey(ntk->getInputNetId(id, 0), ntk->getInputNetId(id, 1));
   V3UI64Hash* hashTable = 0;
   if (netHash.existList(type)) hashTable = (V3UI64Hash*)(netHash.getList(type));
   else { hashTable = new V3UI64Hash(); netHash.setList(type, (size_t)hashTable); }
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

void strashBvReducedTypeGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (1 == ntk->getInputNetSize(id));
   const V3GateType& type = ntk->getGateType(id); assert (isV3ReducedType(type));
   const uint32_t hashKey = V3NetType2HashKey(ntk->getInputNetId(id, 0));
   V3UI32Hash* hashTable = 0;
   if (netHash.existList(type)) hashTable = (V3UI32Hash*)(netHash.getList(type));
   else { hashTable = new V3UI32Hash(); netHash.setList(type, (size_t)hashTable); }
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

void strashBvMuxGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (3 == ntk->getInputNetSize(id));
   assert (BV_MUX == ntk->getGateType(id));
   const V3MuxKey hashKey = V3MuxNetType2HashKey(ntk->getInputNetId(id, 0), 
                                                 ntk->getInputNetId(id, 1), ntk->getInputNetId(id, 2));
   V3MuxHash* hashTable = 0;
   if (netHash.existList(BV_MUX)) hashTable = (V3MuxHash*)(netHash.getList(BV_MUX));
   else { hashTable = new V3MuxHash(); netHash.setList(BV_MUX, (size_t)hashTable); }
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

void strashBvSliceGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (2 == ntk->getInputNetSize(id));
   assert (BV_SLICE == ntk->getGateType(id));
   const uint64_t hashKey = V3OrderedNetType2HashKey(ntk->getInputNetId(id, 0), ntk->getInputNetId(id, 1));
   V3UI64Hash* hashTable = 0;
   if (netHash.existList(BV_SLICE)) hashTable = (V3UI64Hash*)(netHash.getList(BV_SLICE));
   else { hashTable = new V3UI64Hash(); netHash.setList(BV_SLICE, (size_t)hashTable); }
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

void strashBvConstGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (1 == ntk->getInputNetSize(id));
   assert (BV_CONST == ntk->getGateType(id));
   const uint32_t hashKey = V3NetType2HashKey(ntk->getInputNetId(id, 0));
   assert (netHash.existList(BV_CONST));  // CONST Hash Table Must Exists !!!
   V3UI32Hash* hashTable = (V3UI32Hash*)(netHash.getList(BV_CONST));
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

// Structural Hashing Sub-Functions for AIG Network
const V3NetId strashAigNodeGate(V3AigNtk* const ntk, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (2 == inputs.size());
   const uint64_t hashKey = V3UnorderedNetType2HashKey(inputs[0], inputs[1]);
   if (netHash.existList(AIG_NODE)) {
      V3UI64Hash* hashTable = (V3UI64Hash*)(netHash.getList(AIG_NODE)); assert (hashTable);
      V3UI64Hash::const_iterator it = hashTable->find(hashKey); if (hashTable->end() != it) return it->second;
   }
   return V3NetUD;
}

void strashAigNodeGate(V3AigNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize()); assert (2 == ntk->getInputNetSize(id));
   const V3GateType& type = ntk->getGateType(id); assert (AIG_NODE == type);
   const uint64_t hashKey = V3UnorderedNetType2HashKey(ntk->getInputNetId(id, 0), ntk->getInputNetId(id, 1));
   V3UI64Hash* hashTable = 0;
   if (netHash.existList(AIG_NODE)) hashTable = (V3UI64Hash*)(netHash.getList(AIG_NODE));
   else { hashTable = new V3UI64Hash(); netHash.setList(AIG_NODE, (size_t)hashTable); }
   assert (hashTable); assert (hashTable->end() == hashTable->find(hashKey));
   hashTable->insert(make_pair(hashKey, id)); assert (hashTable->end() != hashTable->find(hashKey));
}

// Structural Hashing Main Functions
const V3NetId strashBvGate(V3BvNtk* const ntk, const V3GateType& type, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (V3_XD > type); assert (AIG_FALSE < type);
   if (isV3PairType(type)) return strashBvPairTypeGate(ntk, type, inputs, netHash);
   else if (isV3ReducedType(type)) return strashBvReducedTypeGate(ntk, type, inputs, netHash);
   else if (BV_MUX == type) return strashBvMuxGate(ntk, inputs, netHash);
   else if (BV_SLICE == type) return strashBvSliceGate(ntk, inputs, netHash);
   assert (BV_CONST == type); return strashBvConstGate(ntk, inputs, netHash);
}

const V3NetId strashAigGate(V3AigNtk* const ntk, const V3GateType& type, const V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); if (AIG_FALSE == type) return V3NetUD;
   assert (AIG_NODE == type); return strashAigNodeGate(ntk, inputs, netHash);
}

void strashBvGate(V3BvNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize());
   const V3GateType& type = ntk->getGateType(id); assert (V3_XD > type); assert (AIG_FALSE < type);
   if (isV3PairType(type)) return strashBvPairTypeGate(ntk, id, netHash);
   else if (isV3ReducedType(type)) return strashBvReducedTypeGate(ntk, id, netHash);
   else if (BV_MUX == type) return strashBvMuxGate(ntk, id, netHash);
   else if (BV_SLICE == type) return strashBvSliceGate(ntk, id, netHash);
   assert (BV_CONST == type); return strashBvConstGate(ntk, id, netHash);
}

void strashAigGate(V3AigNtk* const ntk, const V3NetId& id, V3PortableType& netHash) {
   assert (ntk); assert (id.id < ntk->getNetSize());
   const V3GateType& type = ntk->getGateType(id);
   if (AIG_FALSE != type) strashAigNodeGate(ntk, id, netHash);
}

#endif

