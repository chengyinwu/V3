/****************************************************************************
  FileName     [ v3Hash.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for Hash. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_HASH_H
#define V3_ADT_HASH_H

// Defines
#define V3_USE_BOOST_HASH

#include "v3HashUtil.h"

#ifdef V3_USE_BOOST_HASH
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#else
#include <hash_map>
#include <hash_set>
#endif

template<class Key, class Data, class HashFcn = tr1::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = allocator<pair<const Key, Data> > >
struct V3HashMap {
#ifdef V3_USE_BOOST_HASH
   typedef tr1::unordered_map<Key, Data, HashFcn, EqualKey, Alloc> Hash;
#else
   typedef std::hash_map<Key, Data, HashFcn, EqualKey, Alloc> Hash;
#endif
};

template<class Key, class HashFcn = tr1::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = allocator<Key> >
struct V3HashSet {
#ifdef V3_USE_BOOST_HASH
   typedef tr1::unordered_set<Key, HashFcn, EqualKey, Alloc> Hash;
#else
   typedef std::hash_set<Key, HashFcn, EqualKey, Alloc> Hash;
#endif
};

// Extended Hash Function for Pair Key
template <class S, class T>
struct V3PairHashFcn {
   inline size_t operator() (const pair<S, T>& key) const {
      return V3MakeHash(key.first, key.second);
   }
};

template <class S, class T>
struct V3PairHashEqual {
   inline size_t operator() (const pair<S, T>& key1, const pair<S, T>& key2) const {
      return key1 == key2;
   }
};

#endif

