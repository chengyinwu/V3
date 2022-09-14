/****************************************************************************
  FileName     [ v3VrfSIM.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Constrained Random Simulation on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SIM_C
#define V3_VRF_SIM_C

#include "v3Msg.h"
#include "v3VrfSIM.h"
#include "v3VrfResult.h"
#include "v3AlgSimulate.h"

#include <cmath>
#include <ctime>
#include <iomanip>

/* -------------------------------------------------- *\
 * Class V3VrfUMC Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfSIM::V3VrfSIM(const V3NtkHandler* const handler) : V3VrfBase(handler) {
}

V3VrfSIM::~V3VrfSIM() {
}

// Verification Main Functions
void
V3VrfSIM::startVerify(const uint32_t& p) {
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p);
   
   // Consistency Check
   consistencyCheck();

   // Initialize Parameters
   const V3NetId& pId = _vrfNtk->getOutput(p); assert (V3NetUD != pId);
   const uint32_t logMaxWidth = (uint32_t)(ceil(log10(_maxDepth)));
   const string flushSpace = string(100, ' ');
   uint32_t fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);

   // Initialize Simulator
   V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(_vrfNtk)) simulator = new V3AlgBvSimulate(_handler);
   else simulator = new V3AlgAigSimulate(_handler); assert (simulator);

   // Initialize Constraint Satisfaction Value
   V3BitVecX constrValue = simulator->getSimValue(pId);
   for (uint32_t i = 0; i < constrValue.size(); ++i) constrValue.set1(i);
   V3BitVecX tempValue, curValue = constrValue; assert (curValue.all1());
   const V3NetVec constr = (p < _constr.size()) ? _constr[p] : V3NetVec();

   // Initialize Simulation Trace Data
   V3Vec<V3SimTrace>::Vec traceData; traceData.clear();
   V3SimTrace initData; initData.clear(); V3CexTrace* cex = 0;
   uint32_t patternSize = _vrfNtk->getInputSize() + _vrfNtk->getInoutSize();
   uint32_t initSize = _vrfNtk->getLatchSize();

   // Initialize Unsolved List
   V3UI32Vec unsolved; unsolved.clear(); unsolved.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i) 
      unsolved.push_back((_result[i].isCex() || _result[i].isInv()) ? V3NtkUD : 0);

   // Initialize the List of Uninitialized Latches
   V3UI32Vec uninitLatch; uninitLatch.clear();
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i)
      if (_vrfNtk->getLatch(i).id == _vrfNtk->getInputNetId(_vrfNtk->getLatch(i), 1).id) uninitLatch.push_back(i);

   // Start SIM Based Verification
   uint32_t k, x = 0, cycle = _maxDepth;
   while (true) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Shared Results
      if (_sharedBound) {
         k = _sharedBound->getBound(p); if (V3NtkUD == k) break;
         if (cycle < (k << 1)) cycle = k << 1;
      }
      // Get the List of Unsolved Properties
      if (_sharedBound) _sharedBound->getBound(unsolved);
      for (uint32_t i = 0; i < _result.size(); ++i) if (_result[i].isCex()) unsolved[i] = V3NtkUD;
      // Reset Simulator to an Initial State
      for (uint32_t i = 0; i < traceData.size(); ++i) traceData[i].clear(); traceData.clear(); traceData.reserve(cycle);
      initData.clear(); initData.reserve(_vrfNtk->getLatchSize()); curValue = constrValue;
      V3NetVec simTargets; simTargets.clear(); simTargets.reserve(unsolved.size() + constr.size());
      for (uint32_t i = 0; i < unsolved.size(); ++i) if (V3NtkUD != unsolved[i]) simTargets.push_back(_vrfNtk->getOutput(i));
      for (uint32_t i = 0; i < constr.size(); ++i) simTargets.push_back(constr[i]);
      simulator->reset(simTargets); simulator->updateNextStateValue();
      for (uint32_t i = 0; i < uninitLatch.size(); ++i) simulator->clearSource(_vrfNtk->getLatch(uninitLatch[i]), true);
      // Record Initial State
      for (uint32_t i = 0; i < initSize; ++i) initData.push_back(simulator->getSimValue(_vrfNtk->getLatch(i)));
      // Try to Find a Witness in a Number of Cycles
      for (uint32_t i = 0; i < cycle; ++i) {
         // Assign Random Input Values
         simulator->setSourceFree(V3_PI, true);
         simulator->setSourceFree(V3_PIO, true);
         // Simulate for One Cycle
         simulator->simulate();
         // Check if Constraints Satisfied
         for (uint32_t j = 0; j < constr.size(); ++j) curValue &= simulator->getSimValue(constr[j]);
         if (!curValue.exist1()) break;
         // Record Trace Data
         traceData.push_back(V3SimTrace()); traceData.back().reserve(patternSize);
         for (uint32_t j = 0; j < _vrfNtk->getInputSize(); ++j) 
            traceData.back().push_back(simulator->getSimValue(_vrfNtk->getInput(j)));
         for (uint32_t j = 0; j < _vrfNtk->getInoutSize(); ++j) 
            traceData.back().push_back(simulator->getSimValue(_vrfNtk->getInout(j)));
         // Check if Property Asserted
         for (uint32_t j = 0, idx; j < unsolved.size(); ++j) {
            if (V3NtkUD == unsolved[j]) continue;
            tempValue = simulator->getSimValue(_vrfNtk->getOutput(j)); tempValue &= curValue;
            idx = tempValue.first1(); if (tempValue.size() == idx) continue;
            assert (tempValue.exist1()); unsolved[j] = V3NtkUD;
            // Record Counterexample for Property j
            cex = new V3CexTrace(traceData.size()); assert (cex);
            if (initSize) {
               tempValue.resize(initSize, true);
               for (k = 0; k < initSize; ++k) {
                  switch (initData[k][idx]) {
                     case '1': tempValue.set1(k); break;
                     case '0': tempValue.set0(k); break;
                  }
               }
               cex->setInit(tempValue);
            }
            if (patternSize) {
               tempValue.resize(patternSize, false);
               for (uint32_t v = 0; v < traceData.size(); ++v) {
                  tempValue.clear();
                  for (k = 0; k < patternSize; ++k) {
                     switch (traceData[v][k][idx]) {
                        case '1': tempValue.set1(k); break;
                        case '0': tempValue.set0(k); break;
                     }
                  }
                  cex->pushData(tempValue);
               }
            }
            // Record the CounterExample Trace
            _result[j].setCexTrace(cex); assert (_result[j].isCex());
         }
         if (_result[p].isCex()) { fired = i; break; }
         // Update FF Next State Values
         simulator->updateNextStateValue();
      }
      if (V3NtkUD != fired) break; ++x;
      // Report Verification Progress
      if (!isIncKeepSilent() && intactON()) {
         if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << "Simulation completed under trial = " << setw(logMaxWidth) << x;
         if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != fired) Msg(MSG_IFO) << "Counter-example found at cycle = " << ++fired;
      else Msg(MSG_IFO) << "UNDECIDED at trial = " << x;
      if (usageON()) {
         gettimeofday(&curtime, NULL);
         Msg(MSG_IFO) << "  (time = " << setprecision(5) << getTimeUsed(inittime, curtime) << "  sec)" << endl;
      }
      if (profileON()) { /* Report some profiling here ... */ }
   }
}

#endif

