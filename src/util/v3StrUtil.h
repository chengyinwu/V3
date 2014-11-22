/****************************************************************************
  FileName     [ v3StrUtil.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ String Related Utility Functions. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STR_UTIL_H
#define V3_STR_UTIL_H

#include <string>
#include <cstring>

#include "v3IntType.h"

using namespace std;

// General Functions
const int v3StrNCmp(const string&, const string&, const uint32_t&);
const bool v3Str2Int(const string&, int&);
const bool v3Str2UInt(const string&, uint32_t&);
const string v3Int2Str(const int&, const uint32_t& = 0);

// Tokenization Functions
const size_t v3StrGetTok(const string&, string&, const size_t& = 0, const char = ' ');
const string v3Str2BVExpr(const string&, uint32_t&, uint32_t&);

// Trimming Functions
const string v3StrRemoveSpaces(const string);

#endif

