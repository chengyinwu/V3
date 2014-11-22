/****************************************************************************
  FileName     [ v3Vec.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for Vec. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_VEC_H
#define V3_ADT_VEC_H

#include <vector>

template<class T>
struct V3Vec {
   typedef std::vector<T> Vec;
};

#endif

