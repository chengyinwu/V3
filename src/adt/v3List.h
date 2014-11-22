/****************************************************************************
  FileName     [ v3List.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for List. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_LIST_H
#define V3_ADT_LIST_H

#include <list>

template<class T>
struct V3List {
   typedef std::list<T> List;
};

#endif

