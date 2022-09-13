/****************************************************************************
  FileName     [ v3NtkRewrite.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Implementation of Rule-based Rewriting. ]
  Author       [ Mao-Kai (Nanny) Huang, Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_REWRITE_H
#define V3_NTK_REWRITE_H

#include "v3NtkUtil.h"

// Rewrite Sub-Functions
const bool rewrite_AND     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_XOR     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_ADD     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_SUB     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_MULT    (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_DIV     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_MODULO  (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_SHL     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_SHR     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_MERGE   (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_EQUALITY(V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_GEQ     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_RED_AND (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_RED_OR  (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_RED_XOR (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_MUX     (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_SLICE   (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_CONST   (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewrite_AIG_NODE(V3AigNtk* const, V3GateType&, V3InputVec&, V3PortableType&);

// Rewrite Main Functions
const bool rewriteBvGate   (V3BvNtk * const, V3GateType&, V3InputVec&, V3PortableType&);
const bool rewriteAigGate  (V3AigNtk* const, V3GateType&, V3InputVec&, V3PortableType&);

#endif

