/****************************************************************************
  FileName     [ v3NtkStrash.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Implementation of Structural Hashing. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_STRASH_H
#define V3_NTK_STRASH_H

#include "v3NtkUtil.h"

// Structural Hashing Sub-Functions for BV Network
const V3NetId strashBvPairTypeGate(V3BvNtk* const, const V3GateType&, const V3InputVec&, V3PortableType&);
const V3NetId strashBvReducedTypeGate(V3BvNtk* const, const V3GateType&, const V3InputVec&, V3PortableType&);
const V3NetId strashBvMuxGate(V3BvNtk* const, const V3InputVec&, V3PortableType&);
const V3NetId strashBvSliceGate(V3BvNtk* const, const V3InputVec&, V3PortableType&);
const V3NetId strashBvConstGate(V3BvNtk* const, const V3InputVec&, V3PortableType&);
void strashBvPairTypeGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);
void strashBvReducedTypeGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);
void strashBvMuxGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);
void strashBvSliceGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);
void strashBvConstGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);

// Structural Hashing Sub-Functions for AIG Network
const V3NetId strashAigNodeGate(V3AigNtk* const, const V3InputVec&, V3PortableType&);
void strashAigNodeGate(V3AigNtk* const, const V3NetId&, V3PortableType&);

// Structural Hashing Main Functions
const V3NetId strashBvGate(V3BvNtk* const, const V3GateType&, const V3InputVec&, V3PortableType&);
const V3NetId strashAigGate(V3AigNtk* const, const V3GateType&, const V3InputVec&, V3PortableType&);
void strashBvGate(V3BvNtk* const, const V3NetId& id, V3PortableType&);
void strashAigGate(V3AigNtk* const, const V3NetId&, V3PortableType&);

#endif
