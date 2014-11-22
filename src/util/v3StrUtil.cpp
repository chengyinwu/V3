/****************************************************************************
  FileName     [ v3StrUtil.cpp ]
  PackageName  [ v3/src/util ]
  Synopsis     [ String Related Utility Functions. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STR_UTIL_C
#define V3_STR_UTIL_C

#include <cstdio>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "v3StrUtil.h"

// General Functions
const int
v3StrNCmp(const string& s1, const string& s2, const uint32_t& n) {
   assert(n > 0);
   if (s2.size() == 0) return -1;
   uint32_t l = s1.size();
   assert(l >= n);
   for (uint32_t i = 0; i < l; ++i) {
      if (!s2[i])
         return (i < n)? 1 : 0;
      char ch1 = (isupper(s1[i]))? tolower(s1[i]) : s1[i];
      char ch2 = (isupper(s2[i]))? tolower(s2[i]) : s2[i];
      if (ch1 != ch2)
         return (ch1 - ch2);
   }
   return (l - s2.size());
}

const bool
v3Str2Int(const string& str, int& num) {
   num = 0;
   size_t i = 0;
   int sign = 1;
   if (str[0] == '-') { sign = -1; i = 1; }
   bool valid = false;
   for (; i < str.size(); ++i) {
      if (isdigit(str[i])) {
         num *= 10;
         num += int(str[i] - '0');
         valid = true;
      }
      else return false;
   }
   num *= sign;
   return valid;
}

const bool
v3Str2UInt(const string& str, uint32_t& num) {
   int temp;
   if (!v3Str2Int(str, temp) || temp < 0) return false;
   num = (uint32_t)temp; return true;
}

const string
v3Int2Str(const int& num, const uint32_t& width) {
   if (!width) {
      char str[64];
      sprintf (str, "%d", num);
      return str;
   }
   else {
      stringstream ss;
      ss << setw(width) << setfill('0') << num;
      return ss.str();
   }
}

// Tokenization Functions
const size_t
v3StrGetTok(const string& str, string& tok, const size_t& pos, const char del) {
   size_t begin = str.find_first_not_of(del, pos);
   if (begin == string::npos) { tok = ""; return begin; }
   size_t end = str.find_first_of(del, begin);
   tok = str.substr(begin, end - begin);
   return end;
}

const string
v3Str2BVExpr(const string& expr, uint32_t& start, uint32_t& end) {
   // E.g. expr = c      --> start = V3IntMAX, end = V3IntMAX, return c
   //      expr = c[1]   --> start = 1, end = 1, return c
   //      expr = c[0:3] --> start = 0, end = 3, return c
   //      expr = c[3:0] --> start = 3, end = 0, return c
   //      expr = c[$]   --> start = V3IntMAX, end = V3IntMAX, return c
   //      expr = c[3:$] --> start = 3, end = V3IntMAX, return c
   //      expr = c[$:3] --> start = V3IntMAX, end = 3, return c
   // Return "" if expr does not match any expected expr patterns above
   // Note: c[], c[$], c[$:$] are illegal
   uint32_t startPos = V3IntMAX, midPos = V3IntMAX;
   for (uint32_t i = 0; i < expr.size(); ++i) {
      if ('[' == expr[i]) {
         if (V3IntMAX != startPos) return "";
         startPos = midPos = 1 + i;
      }
      else if (':' == expr[i]) {
         if (V3IntMAX == startPos) return ""; midPos = 1 + i;
         const string startStr = expr.substr(startPos, i - startPos);
         if ("$" == startStr) start = V3IntMAX;
         else {
            int temp; if (!v3Str2Int(startStr, temp)) return "";
            if (temp < 0) return ""; start = (uint32_t)temp;
         }
      }
      else if (']' == expr[i]) {
         if (V3IntMAX == midPos) return "";
         if ((1 + i) != expr.size()) return "";
         const string endStr = expr.substr(midPos, i - midPos);
         if ("$" == endStr) end = V3IntMAX;
         else {
            int temp; if (!v3Str2Int(endStr, temp)) return "";
            if (temp < 0) return ""; end = (uint32_t)temp;
         }
         if (startPos == midPos) start = end;
         if (V3IntMAX == start && V3IntMAX == end) return "";
         return expr.substr(0, startPos - 1);
      }
   }
   if (V3IntMAX != startPos) return "";
   else { start = end = V3IntMAX; return expr; }
}

// Trimming Functions
const string
v3StrRemoveSpaces(const string str) {
   string s = "";
   for (uint32_t i = 0; i < str.size(); ++i)
      if (!isspace(str[i]) && '\n' != str[i]) s += str[i];
   return s;
}

#endif

