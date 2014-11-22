/****************************************************************************
  FileName     [ v3Queue.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Abstract Data Type for Queue. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ADT_QUEUE_H
#define V3_ADT_QUEUE_H

#include <queue>

template<class T>
struct V3Queue {
   typedef std::queue<T> Queue;
};

#endif

