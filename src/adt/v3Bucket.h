/****************************************************************************
  FileName     [ v3Bucket.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Simple Bucket List Implementation ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BUCKET_H
#define V3_BUCKET_H

#include "v3Misc.h"

#include <cassert>

using namespace std;

// class V3BucketList: General Bucket List
template<class T>
class V3BucketList {
   typedef typename V3Stack<T>::Stack     Bucket;
   typedef typename V3Vec<Bucket>::Vec    BucketList;
   public : 
      V3BucketList(const uint32_t& size = 0) { _size = 0; _bestIndex = 0; _elements.clear(); init(size); }
      ~V3BucketList();
      void add(const uint32_t&, const T&);
      bool pop(uint32_t&, T&);
      void clear();
      void init(const uint32_t&);
      inline const uint32_t size() const { return _size; }
      inline const uint32_t buckets() const { return _elements.size(); }
   private : 
      BucketList     _elements;
      uint32_t       _bestIndex;
      uint32_t       _size;
};

template<class T>
V3BucketList<T>::~V3BucketList() {
   for (uint32_t i = 0; i < _elements.size(); ++i)
      while (!_elements[i].empty()) _elements[i].pop();
   _elements.clear();
}

template<class T> void
V3BucketList<T>::add(const uint32_t& index, const T& t) {
   assert (index < _elements.size());
   if (index < _bestIndex) _bestIndex = index;
   _elements[index].push(t); ++_size;
   assert (_elements[_bestIndex].size());
}

template<class T> bool
V3BucketList<T>::pop(uint32_t& index, T& t) {
   if (_bestIndex < _elements.size()) index = _bestIndex;
   else return false;
   if (_elements[_bestIndex].size() == 1) 
      for (_bestIndex = index + 1; _bestIndex < _elements.size(); ++_bestIndex) 
         if (_elements[_bestIndex].size()) break;
   t = _elements[index].top(); _elements[index].pop(); --_size;
   assert (_elements[index].size() || index < _bestIndex);
   return true;
}

template<class T> void
V3BucketList<T>::clear() {
   for (uint32_t i = 0; i < _elements.size(); ++i) 
      while (!_elements[i].empty()) _elements[i].pop();
   _size = 0; _bestIndex = _elements.size();
}

template<class T> void
V3BucketList<T>::init(const uint32_t& size) {
   clear(); if (!size) return; _elements.reserve(size);
   for (uint32_t i = 0; i < size; ++i) {
      _elements.push_back(Bucket());
      while (!_elements[i].empty()) _elements[i].pop();
   }
   _bestIndex = _elements.size();
}

/*
// class V3IncBucketList: Increasing ONLY Bucket List
template<class T>
class V3IncBucketList {
   typedef typename V3Stack<T>::Stack     Bucket;
   typedef typename V3Vec<Bucket>::Vec    BucketList;
   public : 
      V3IncBucketList(const uint32_t& size = 0) { _size = 0; _bestIndex = 0; _elements.clear(); init(size); }
      ~V3IncBucketList();
      void add(const uint32_t&, const T&);
      bool pop(uint32_t&, T&);
      void clear();
      void init(const uint32_t&);
      inline const uint32_t size() const { return _size; }
      inline const uint32_t buckets() const { return _elements.size(); }
   private : 
      BucketList     _elements;
      uint32_t       _bestIndex;
      uint32_t       _size;
};

template<class T>
V3IncBucketList<T>::~V3IncBucketList() {
   for (uint32_t i = 0; i < _elements.size(); ++i)
      while (!_elements[i].empty()) _elements[i].pop();
   _elements.clear();
}

template<class T> void
V3IncBucketList<T>::add(const uint32_t& index, const T& t) {
   assert (index < _elements.size());
   assert (!_size || index > _bestIndex);  // Increasing
   _elements[index].push(t); ++_size;
}

template<class T> bool
V3IncBucketList<T>::pop(uint32_t& index, T& t) {
   if (!_size) return false; --_size;
   while (!_elements[_bestIndex].size()) ++_bestIndex;
   index = _bestIndex; assert (index < _elements.size());
   t = _elements[index].top(); _elements[index].pop();
   return true;
}

template<class T> void
V3IncBucketList<T>::clear() {
   if (_size) {
      for (uint32_t i = _bestIndex; i < _elements.size(); ++i) 
         while (!_elements[i].empty()) _elements[i].pop();
      _size = 0;
   }
   _bestIndex = 0;
}

template<class T> void
V3IncBucketList<T>::init(const uint32_t& size) {
   clear(); if (!size) return; _elements.reserve(size);
   for (uint32_t i = 0; i < size; ++i) {
      _elements.push_back(Bucket());
      while (!_elements[i].empty()) _elements[i].pop();
   }
}
*/

// class V3IncBucketList: Increasing ONLY Bucket List
template<class T>
class V3IncBucketList {
   typedef typename V3Vec<T>::Vec         Bucket;
   typedef pair<uint32_t, Bucket>         BucketId;
   typedef typename V3Vec<BucketId>::Vec  BucketList;
   public : 
      V3IncBucketList(const uint32_t& size = 0) { _size = 0; _bestIndex = 0; _elements.clear(); init(size); }
      ~V3IncBucketList();
      void add(const uint32_t&, const T&);
      void add(const V3UI32Vec&, const Bucket&);
      bool pop(uint32_t&, T&);
      void clear();
      void init(const uint32_t&);
      inline const uint32_t size() const { return _size; }
      inline const uint32_t buckets() const { return _elements.size(); }
   private : 
      BucketList     _elements;
      uint32_t       _bestIndex;
      uint32_t       _size;
};

template<class T>
V3IncBucketList<T>::~V3IncBucketList() {
   for (uint32_t i = 0; i < _elements.size(); ++i) _elements[i].second.clear();
   _elements.clear();
}

template<class T> void
V3IncBucketList<T>::add(const uint32_t& index, const T& t) {
   assert (index < _elements.size());
   assert (!_size || index >= _bestIndex);  // Increasing
   _elements[index].second.push_back(t); ++_size;
}

template<class T> void
V3IncBucketList<T>::add(const V3UI32Vec& iList, const Bucket& tList) {
   assert (iList.size() == tList.size());
   assert (!_bestIndex);
   for (uint32_t i = 0; i < tList.size(); ++i) {
      assert (iList[i] < _elements.size());
      _elements[iList[i]].second.push_back(tList[i]);
   }
   _size += tList.size();
}

template<class T> bool
V3IncBucketList<T>::pop(uint32_t& index, T& t) {
   if (!_size) return false; --_size;
   while (_elements[_bestIndex].first == _elements[_bestIndex].second.size()) ++_bestIndex;
   index = _bestIndex; assert (index < _elements.size());
   t = _elements[index].second[_elements[index].first];
   ++_elements[index].first; return true;
}

template<class T> void
V3IncBucketList<T>::clear() {
   for (uint32_t i = _bestIndex; i < _elements.size(); ++i) {
      _elements[i].first = 0; _elements[i].second.clear();
   }
   _size = 0; _bestIndex = 0;
}

template<class T> void
V3IncBucketList<T>::init(const uint32_t& size) {
   clear(); _elements.clear(); if (!size) return;
   _elements.reserve(size);
   for (uint32_t i = 0; i < size; ++i)
      _elements.push_back(make_pair(0, Bucket()));
}

#endif

