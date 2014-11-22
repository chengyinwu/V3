/****************************************************************************
  FileName     [ v3Misc.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Definition of Miscellaneous Types. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MISC_H
#define V3_MISC_H

#include "v3Map.h"
#include "v3Set.h"
#include "v3Vec.h"
#include "v3List.h"
#include "v3Hash.h"
#include "v3Type.h"
#include "v3Queue.h"
#include "v3Stack.h"

#include <cassert>

// Defines for General Arrays
typedef V3Vec<bool      >::Vec   V3BoolVec;
typedef V3Vec<int       >::Vec   V3IntVec;
typedef V3Vec<size_t    >::Vec   V3PtrVec;
typedef V3Vec<double    >::Vec   V3DblVec;
typedef V3Vec<string    >::Vec   V3StrVec;
typedef V3Vec<uint32_t  >::Vec   V3UI32Vec;
typedef V3Vec<uint64_t  >::Vec   V3UI64Vec;

// Defines for General Tables
typedef V3Vec<V3StrVec  >::Vec   V3StrTable;
typedef V3Vec<V3BoolVec >::Vec   V3BoolTable;
typedef V3Vec<V3UI32Vec >::Vec   V3UI32Table;

// Defines for General Sets
typedef V3Set<string    >::Set   V3StrSet;
typedef V3Set<uint32_t  >::Set   V3UI32Set;

// Defines for General Queues
typedef V3Queue<uint32_t>::Queue V3UI32Queue;

// Defines for General Stacks
typedef V3Stack<uint32_t>::Stack V3UI32Stack;

// Defines for General Compare Functions
template<class T>
struct V3UI32LessOrEq { const bool operator() (const T& i, const T& j) const { return i <= j; } };

// class V3PortableType : Portable Type for Dynamic Allocation
// NOTE : Data in _list will not be freed by the destructor !!
class V3PortableType {
   public : 
      // Constructor and Destructor
      V3PortableType(const uint32_t& s = 0, const uint32_t& f = 0) : _size(s), _shift(f) {
         _list.clear(); _list.reserve(s); _index.clear(); _index.reserve(s);
         for (uint32_t i = 0; i < s; ++i) _index.push_back(V3NtkUD);
      }
      ~V3PortableType() { _list.clear(); _index.clear(); }
      // Inline Member Functions
      inline const bool existList(const uint32_t& i) const {
         assert (i >= _shift); assert (i < (_shift + _size)); return (V3NtkUD != _index[i - _shift]); }
      inline const size_t& getList(const uint32_t& i) const {
         assert (existList(i)); return _list[_index[i - _shift]]; }
      inline void setList(const uint32_t& i, const size_t& t) {
         assert (!existList(i)); _index[i - _shift] = _list.size(); _list.push_back(t); }
      // Operator Overloads
      V3PortableType& operator = (const V3PortableType& t) {
         *(const_cast<uint32_t*>(&_size)) = t._size;
         *(const_cast<uint32_t*>(&_shift)) = t._shift;
         _list = t._list; _index = t._index; return *this;
      }
   private : 
      const uint32_t       _size;
      const uint32_t       _shift;
      V3PtrVec             _list;
      V3UI32Vec            _index;
};

#endif

