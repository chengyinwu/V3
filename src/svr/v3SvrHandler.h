/****************************************************************************
  FileName     [ v3SvrHandler.h ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ Handler Functions for V3 Base Solver. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_HANDLER_H
#define V3_SVR_HANDLER_H

#include "v3SvrMiniSat.h"
#include "v3SvrBoolector.h"

// Solver Type Enumerations
enum V3SolverType
{
   V3_SVR_MINISAT    = 0,
   V3_SVR_BOOLECTOR  = 1,
   V3_SVR_TOTAL      = 2
};

const string V3SolverTypeStr[] = 
{
   "MINISAT", "BOOLECTOR"
};

// Solver Creation Functions
V3SvrBase* const allocSolver(const V3SolverType&, const V3Ntk* const, const bool& = false);
V3SvrBase* const referenceSolver(V3SvrBase* const);

#endif

