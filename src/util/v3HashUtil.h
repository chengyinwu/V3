/****************************************************************************
  FileName     [ v3Hash.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ V3 Hash Functions. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_HASH_H
#define V3_HASH_H

#include <string>

using namespace std;

// Robert Jenkins' 32 bit integer hash function (http://www.concentric.net/~ttwang/tech/inthash.htm)
static inline size_t V3MakeHash(size_t a) {
   a = (a + 0x7ed55d16) + (a << 12);
   a = (a ^ 0xc761c23c) ^ (a >> 19);
   a = (a + 0x165667b1) + (a << 5);
   a = (a + 0xd3a2646c) ^ (a << 9);
   a = (a + 0xfd7046c5) + (a << 3);
   a = (a ^ 0xb55a4f09) ^ (a >> 16);
   return a;
}

static inline size_t V3MakeHash(const string& s) {
   const char* data = s.data();
   size_t value = 0;
   for (int i = s.length(); i > 0; --i)
      value = (value * 131) + *data++;
   return value;
}

static inline size_t V3MakeHash(const size_t a, const size_t b) {
   return V3MakeHash(b | (a << (sizeof(size_t) << 2)));
}

#endif

