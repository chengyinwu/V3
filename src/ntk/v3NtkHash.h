/****************************************************************************
  FileName     [ v3NtkHash.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Hash Tables for V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_HASH_H
#define V3_NTK_HASH_H

#include "v3Ntk.h"

// Defines for Net Hash Data Types
typedef pair<uint64_t, uint32_t>             V3MuxKey;
typedef V3PairHashFcn<uint64_t, uint32_t>    V3MuxFcn;
typedef V3PairHashEqual<uint64_t, uint32_t>  V3MuxEqual;

// Defines for Net Hash Tables
typedef V3HashMap<uint64_t,   V3NetId                      >::Hash   V3UI64Hash;
typedef V3HashMap<uint32_t,   V3NetId                      >::Hash   V3UI32Hash;
typedef V3HashMap<V3MuxKey,   V3NetId, V3MuxFcn, V3MuxEqual>::Hash   V3MuxHash;

// Helper Functions for Inputs to Hash Key Translation
inline const uint32_t V3NetType2HashKey(const V3NetType& id) { return id.value; }

inline const uint64_t V3OrderedNetType2HashKey(const V3NetType& id1, const V3NetType& id2) {
   return V3PairType(id1.value, id2.value).pair; }

inline const uint64_t V3UnorderedNetType2HashKey(const V3NetType& id1, const V3NetType& id2) {
   if (id1.value > id2.value) return V3PairType(id2.value, id1.value).pair;
   else return V3PairType(id1.value, id2.value).pair; }

inline const V3MuxKey V3MuxNetType2HashKey(const V3NetType& fId, const V3NetType& tId, const V3NetType& sId) {
   return make_pair(V3OrderedNetType2HashKey(fId, tId), V3NetType2HashKey(sId)); }

#endif

