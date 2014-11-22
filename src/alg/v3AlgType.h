/****************************************************************************
  FileName     [ v3AlgType.h ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ Types for V3 Algorithms. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_TYPE_H
#define V3_ALG_TYPE_H

#include "v3Ntk.h"

// Defines for Simulation Data
typedef V3Vec<V3BitVecX>::Vec             V3SimTrace;
typedef V3Vec<V3BitVecX>::Vec             V3BvSimDataVec;
typedef V3Vec<V3BitVecS>::Vec             V3AigSimDataVec;
typedef V3Vec<V3SimTrace>::Vec            V3SimTraceVec;
typedef V3Vec<V3BvSimDataVec>::Vec        V3BvSimTraceVec;
typedef V3Vec<V3AigSimDataVec>::Vec       V3AigSimTraceVec;

// Defines for Simulation Records
typedef pair<V3NetId, uint32_t>           V3BvSimNetId;
typedef V3Vec<V3BvSimNetId>::Vec          V3BvSimNetVec;
typedef pair<V3NetId, V3BitVecX>          V3BvSimRecord;
typedef pair<V3NetId, V3BitVecS>          V3AigSimRecord;
typedef V3Vec<V3BvSimRecord>::Vec         V3BvSimRecordVec;
typedef V3Vec<V3AigSimRecord>::Vec        V3AigSimRecordVec;

#endif

