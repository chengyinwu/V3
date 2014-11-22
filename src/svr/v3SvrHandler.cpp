/****************************************************************************
  FileName     [ v3SvrHandler.cpp ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ Handler Functions for V3 Base Solver. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_HANDLER_C
#define V3_SVR_HANDLER_C

#include "v3SvrHandler.h"

// Solver Creation Functions
V3SvrBase* const allocSolver(const V3SolverType& type, const V3Ntk* const ntk, const bool& freeBound) {
   assert (type < V3_SVR_TOTAL); assert (ntk);
   switch (type) {
      case V3_SVR_MINISAT     : return new V3SvrMiniSat(ntk, freeBound);
      case V3_SVR_BOOLECTOR   : return new V3SvrBoolector(ntk, freeBound);
      default                 : return 0;
   }
}

V3SvrBase* const referenceSolver(V3SvrBase* const solver) {
   assert (solver);
   if (dynamic_cast<V3SvrMiniSat*>(solver)) return new V3SvrMiniSat(*(dynamic_cast<V3SvrMiniSat*>(solver)));
   else if (dynamic_cast<V3SvrBoolector*>(solver)) return new V3SvrBoolector(*(dynamic_cast<V3SvrBoolector*>(solver)));
   else return 0;
}

#endif

