/****************************************************************************
  FileName     [ v3NumUtil.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ V3 Numerical Utility Functions. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NUM_UTIL_H
#define V3_NUM_UTIL_H

using namespace std;

template<class T>
inline bool V3SafeAdd(const T a, const T b, T& c) {
   // Perform c = a + b and return whether it overflows
   // NOTE: class T should overload '+', '=', '-', ...
   c = a + b;
   return (a == (c - b)) && (b == (c - a));
}

template<class T>
inline bool V3SafeAdd(T& a, const T b) {
   // Perform a += b and return whether it overflows
   // NOTE: class T should overload '+', '=', '-', ...
   const T c(a); assert (a == c); a += b;
   return (c == (a - b)) && (b == (a - c));
}

template<class T>
inline bool V3SafeMult(const T a, const T b, T& c) {
   // Perform c = a * b and return whether it overflows
   // NOTE: class T should overload '*', '=', '/', ...
   c = a * b;
   return (!a || !b) || ((a == (c / b)) && (b == (c / a)));
}

template<class T>
inline bool V3SafeMult(T& a, const T b) {
   // Perform a *= b and return whether it overflows
   // NOTE: class T should overload '*', '=', '/', ...
   const T c(a); assert (a == c); a *= b;
   return (!a || !b) || ((c == (a / b)) && (b == (a / c)));
}

#endif

