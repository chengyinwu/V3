/****************************************************************************
  FileName     [ v3VrfResult.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Verification Result Validation Checker. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_RESULT_H
#define V3_VRF_RESULT_H

#include "v3NtkHandler.h"

// Counterexample Checker
const int simulationCheckFiredResult(const V3CexTrace&, const V3NetVec&, const V3NetVec&, const bool&,
                                     const V3NtkHandler* const, const uint32_t& = 0);
const int formalCheckFiredResult(const V3CexTrace&, const V3NetVec&, const V3NetVec&, const bool&,
                                 const V3NtkHandler* const, const uint32_t& = 0);

// Simulation-Based Checker
const int cycleSimulateResult(const V3CexTrace&, const V3NetVec&, const V3NetVec&, const bool&,
                              const V3NtkHandler* const, const uint32_t& = 0);
const int expandSimulateResult(const V3CexTrace&, const V3NetVec&, const V3NetVec&, const bool&,
                               const V3NtkHandler* const, const uint32_t& = 0);

// Counterexample Checker Report Functions
void reportUnexpectedState(const uint32_t&, const uint32_t&, const V3BitVecX&, const V3BitVecX&);
void reportUnsatisfiedConstraint(const uint32_t&, const uint32_t&, const V3BitVecX&);
void reportShorterTrace(const uint32_t&, const char&);

// Inductive Invariant Checker
const int formalCheckProvenResult(const V3Ntk* const, const V3NtkHandler* const);

#endif

