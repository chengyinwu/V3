/****************************************************************************
  FileName     [ v3Set.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for Set. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_SET_H
#define V3_ADT_SET_H

#include <set>

template<class Key, class Compare = std::less<Key>, class Alloc = std::allocator<Key> >
struct V3Set {
   typedef std::set<Key, Compare, Alloc> Set;
};

#endif

