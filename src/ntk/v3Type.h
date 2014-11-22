/****************************************************************************
  FileName     [ v3Type.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Types in V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_TYPE_H
#define V3_TYPE_H

#include "v3IntType.h"

#include <string>
#include <cassert>
#include <utility>

using namespace std;

// Global Define
#define V3UI64Type(x)      ((uint64_t)(x))
#define V3UI64Pair(x, y)   ((V3UI64Type(x) << 32) | V3UI64Type(y))
#define V3UI64MSB(x)       ((uint32_t)(x >> 32))
#define V3UI64LSB(x)       ((uint32_t)x)

// Constants
const uint32_t V3NtkUD     = V3IntMAX;
const string   V3_INDENT   = "   ";

// Defines
struct V3NetId {  // 4 Bytes
   uint32_t    cp :  1;
   uint32_t    id : 31;
   static V3NetId makeNetId(uint32_t i = V3NtkUD, uint32_t c = 0) { V3NetId j; j.cp = c; j.id = i; return j; }
   V3NetId operator ~ () const { return makeNetId(id, cp ^ 1); }
   const bool operator == (const V3NetId& i) const { return cp == i.cp && id == i.id; }
   const bool operator != (const V3NetId& i) const { return !(*this == i); }
};

struct V3MiscType {  // 4 Bytes
   uint32_t    type : 6;
   uint32_t    misc : 26;
   V3MiscType(uint32_t t = 0, uint32_t m = 0) { type = t; misc = m; }
};

union V3NetType {  // 4 Bytes
   V3NetId     id;
   uint32_t    value;
   V3NetType(uint32_t i = V3NtkUD) { value = i; }
   V3NetType(const V3NetId& i) { id = i; }
};

struct V3PairType {  // 8 Bytes
   union {
      uint64_t    pair;
      uint32_t    bus[2];
   };
   V3PairType(uint64_t p) { pair = p; }
   V3PairType(uint32_t m, uint32_t l) { bus[0] = m; bus[1] = l; }
};

// Gate Types in V3 Network
enum V3GateType {
   // Internal Admitted Types
   V3_PI = 0, V3_PIO, V3_FF, V3_MODULE, AIG_NODE, AIG_FALSE,                     // PI, PIO, FF, MODULE, AIG GATES
   BV_RED_AND, BV_RED_OR, BV_RED_XOR, BV_MUX, BV_AND, BV_XOR,                    // BV_(RED, LOGIC)
   BV_ADD, BV_SUB, BV_MULT, BV_DIV, BV_MODULO, BV_SHL, BV_SHR,                   // BV_(ARITH)
   BV_MERGE, BV_EQUALITY, BV_GEQ, BV_SLICE, BV_CONST, V3_XD,                     // BV_(MODEL), BV_(COMP)
   // External Extended Types
   BV_BUF, BV_INV, BV_OR, BV_NAND, BV_NOR, BV_XNOR,                              // BV_EXTERNAL_ONLY
   BV_NEQ, BV_GREATER, BV_LEQ, BV_LESS, V3_GATE_TOTAL                            // BV_EXTERNAL_ONLY
};

// String of Gate Types in V3 Network
const string V3GateTypeStr[] = {
   // Internal Admitted Types
   "V3_PI", "V3_PIO", "V3_FF", "V3_MODULE", "AIG_NODE", "AIG_FALSE",             // PI, PIO, FF, MODULE, AIG GATES
   "BV_RED_AND", "BV_RED_OR", "BV_RED_XOR", "BV_MUX", "BV_AND", "BV_XOR",        // BV_(RED, LOGIC)
   "BV_ADD", "BV_SUB", "BV_MULT", "BV_DIV", "BV_MODULO", "BV_SHL", "BV_SHR",     // BV_(ARITH)
   "BV_MERGE", "BV_EQUALITY", "BV_GEQ", "BV_SLICE", "BV_CONST", "V3_XD",         // BV_(MODEL), BV_(COMP)
   // External Extended Types
   "BV_BUF", "BV_INV", "BV_OR", "BV_NAND", "BV_NOR", "BV_XNOR",                  // BV_EXTERNAL_ONLY
   "BV_NEQ", "BV_GREATER", "BV_LEQ", "BV_LESS"                                   // BV_EXTERNAL_ONLY
};

// V3 Automatic Renaming Prefix and Suffix
const string V3HierSeparator  = ".";
const string V3NameInvPrefix  = "~";
const string V3NameBitPrefix  = "[";
const string V3NameBitSuffix  = "]";
const string V3ExpansionName  = "@";
const string V3NetNamePrefix  = "v3_Internal_Net_";
const string V3AsyncDFFName   = "v3_Async_FF_Net_";
const string V3AsyncMuxName   = "v3_Async_MUX_Net_";

#endif

