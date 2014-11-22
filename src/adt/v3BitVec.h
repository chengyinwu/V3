/****************************************************************************
  FileName     [ v3BitVec.h ]
  PackageName  [ v3/src/adt ]
  Synopsis     [ Bit-Vector Data Structure Implementation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BIT_VEC_H
#define V3_BIT_VEC_H

#include "v3IntType.h"

#include <iostream>
#include <cstdlib>

using namespace std;

class V3BitVec
{
   public : 
      // Constructors and Destructor
      V3BitVec(const unsigned& = 8);
      V3BitVec(const char*);
      V3BitVec(const V3BitVec&);
      ~V3BitVec();
      // Member Functions
      inline const unsigned size() const { return _size; }
      void set1 (const unsigned&);
      void set0 (const unsigned&);
      const bool all1 () const;
      const bool all0 () const;
      const unsigned ones() const;
      const unsigned value() const;
      void clear();
      void random();
      const string toExp() const;
      const string regEx() const;
      void copy(V3BitVec&) const;
      void resize(const unsigned&, const bool& = true);
      // Operator Overloads
      const bool operator[] (const unsigned&) const;
      V3BitVec& operator &= (const V3BitVec&);
      V3BitVec& operator |= (const V3BitVec&);
      V3BitVec& operator =  (const V3BitVec&);
      const bool operator == (const V3BitVec&) const;
      friend ostream& operator << (ostream&, const V3BitVec&);
   private : 
      void set(const unsigned&, const bool& = false);
      void set(const char*);
      unsigned          _size;
      unsigned char*    _data;
};

// V3BitVec with Don't Care Supports (00 for X, 01 for 0, 10 for 1, 11 illegal)
class V3BitVecS;
class V3BitVecX
{
   public : 
      friend class V3BitVecS;
      // Constructors and Destructors
      V3BitVecX(const unsigned& = 8);
      V3BitVecX(const V3BitVec&);
      V3BitVecX(const V3BitVecS&);
      V3BitVecX(const V3BitVecX&);
      ~V3BitVecX();
      // Inline Member Functions
      inline const unsigned size() const { return _size; }
      void set1 (const unsigned&);
      void set0 (const unsigned&);
      void setX (const unsigned&);
      void restrict0();
      const bool all1 () const;
      const bool all0 () const;
      const bool allX () const;
      const bool exist1 () const;
      const bool exist0 () const;
      const unsigned value() const;
      const unsigned first1() const;
      const unsigned first0() const;
      const unsigned firstX() const;
      const unsigned dcBits() const;
      void clear();
      void random();
      const string toExp() const;
      const string regEx() const;
      void copy(V3BitVecX&) const;
      void resize(const unsigned&, const bool& = true);
      // Logic Operators
      V3BitVecX& operator &= (const V3BitVecX&);
      V3BitVecX& operator |= (const V3BitVecX&);
      V3BitVecX& operator ^= (const V3BitVecX&);
      // Arithmetic Operators
      V3BitVecX& operator += (const V3BitVecX&);
      V3BitVecX& operator -= (const V3BitVecX&);
      V3BitVecX& operator *= (const V3BitVecX&);
      V3BitVecX& operator /= (const V3BitVecX&);
      V3BitVecX& operator %= (const V3BitVecX&);
      // Shifter Operators
      V3BitVecX& operator >>= (const V3BitVecX&);
      V3BitVecX& operator <<= (const V3BitVecX&);
      // Symbolic Comparative Operators
      const bool operator == (const V3BitVecX&) const;
      const bool operator != (const V3BitVecX&) const;
      // Assignments
      V3BitVecX& operator = (const V3BitVecX&);
      const char operator[] (const unsigned&) const;
      friend ostream& operator << (ostream&, const V3BitVecX&);
      // Ternary Comparative Operators
      const V3BitVecX bv_equal(const V3BitVecX&) const;
      const V3BitVecX bv_geq(const V3BitVecX&) const;
      const V3BitVecX bv_leq(const V3BitVecX&) const;
      const V3BitVecX bv_gt(const V3BitVecX&) const;
      const V3BitVecX bv_lt(const V3BitVecX&) const;
      // Bit-wise Operators
      const V3BitVecX operator ~ () const;
      const V3BitVecX bv_red_and() const;
      const V3BitVecX bv_red_or() const;
      const V3BitVecX bv_red_xor() const;
      // Partial Manipulating Operators
      const V3BitVecX bv_slice(const unsigned&, const unsigned&) const;
      const V3BitVecX bv_concat(const V3BitVecX&) const;  // {a = this, b};
      const V3BitVecX bv_select(const V3BitVecX&, const V3BitVecX&) const;  // (s = this);
      // Special Operators
      const bool bv_intersect(const V3BitVecX&) const;
      const bool bv_cover(const V3BitVecX&) const;
   private : 
      void set(const unsigned&, const bool& = false);
      void set_value(const unsigned&, const char&);
      // Bit Operators
      const char op_inv(const char&) const;
      const char op_and(const char&, const char&) const;
      const char op_or(const char&, const char&) const;
      const char op_xor(const char&, const char&) const;
      const char op_add(const char&, const char&, const char&) const;
      const char op_carry(const char&, const char&, const char&) const;
      const bool op_eq(const char&, const char&) const;
      const bool op_ge(const char&, const char&) const;
      const bool op_le(const char&, const char&) const;
      // Ternary Comparative Operators
      const char op_bv_equal(const V3BitVecX&) const;
      const char op_bv_geq(const V3BitVecX&) const;
      const char op_bv_leq(const V3BitVecX&) const;
      // Divider / Modular Operators
      void bv_divided_by(const V3BitVecX&, const bool&);
      // Data Members
      unsigned          _size;
      unsigned char*    _data0;
      unsigned char*    _data1;
};

// Simplified V3BitVecX for Only AND, INV Usage
class V3BitVecS
{
   public : 
      friend class V3BitVecX;
      // Constructors and Destructors
      V3BitVecS(const uint64_t&, const uint64_t&);
      V3BitVecS(const V3BitVecS&);
      V3BitVecS(const V3BitVecX&);
      V3BitVecS();
      ~V3BitVecS();
      // Inline Member Functions
      inline void setZeros(const uint64_t& v) { _data0 = v; }
      inline void setOnes(const uint64_t& v) { _data1 = v; }
      inline void set0(const unsigned& i) { _data0 |= (1ul << i); _data1 &= ~(1ul << i); }
      inline void set1(const unsigned& i) { _data0 &= ~(1ul << i); _data1 |= (1ul << i); }
      inline void setX(const unsigned& i) { _data0 &= ~(1ul << i); _data1 &= ~(1ul << i); }
      inline void clear() { _data0 = _data1 = 0; }
      inline void random() { _data0 = rand(); _data1 = ~_data0; }
      inline const bool allX() const { return !_data0 && !_data1; }
      // Operators Overloads
      inline const V3BitVecS operator ~ () const { const V3BitVecS v(_data1, _data0); return v; }
      inline V3BitVecS& operator &= (const V3BitVecS& v) { _data0 |= v._data0; _data1 &= v._data1; return *this; }
      inline const bool operator == (const V3BitVecS& v) const { return (_data0 == v._data0) && (_data1 == v._data1); }
      inline const bool operator != (const V3BitVecS& v) const { return !(*this == v); }
      const char operator[] (const unsigned&) const;
      V3BitVecS& operator ^= (const V3BitVecS&);
      friend ostream& operator << (ostream&, const V3BitVecS&);
      // Special Operators
      void bv_and(const V3BitVecS&, const bool&, const V3BitVecS&, const bool&);
      inline const bool bv_cover(const V3BitVecS& v) const { return !((_data0 & ~v._data0) | (_data1 & ~v._data1)); }
      inline const bool bv_full() const { return (_data0 == 0ul || _data0 == ~0ul) && (_data1 == 0ul || _data1 == ~0ul); }
   private : 
      uint64_t    _data0;
      uint64_t    _data1;
};

#endif

