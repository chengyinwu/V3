/****************************************************************************
  FileName     [ v3Map.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for Map. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_MAP_H
#define V3_ADT_MAP_H

#include <map>

template<class Key, class Data, class Compare = std::less<Key>, class Alloc = std::allocator<std::pair<const Key, Data> > >
struct V3Map {
   typedef std::map<Key, Data, Compare, Alloc> Map;
};

#endif

