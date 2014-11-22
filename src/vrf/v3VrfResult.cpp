/****************************************************************************
  FileName     [ v3VrfResult.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Verification Result Validation Checker. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_RESULT_C
#define V3_VRF_RESULT_C

#include "v3Msg.h"
#include "v3VrfBase.h"
#include "v3VrfResult.h"
#include "v3AlgSimulate.h"

#include <iomanip>

// Counterexample Checker
const int simulationCheckFiredResult(const V3CexTrace& cex, const V3NetVec& constrList, const V3NetVec& fairList, const bool& safe, const V3NtkHandler* const handler, const uint32_t& index) {
   return cycleSimulateResult(cex, constrList, fairList, safe, handler, index);
   //return expandSimulateResult(cex, handler);
}

const int formalCheckFiredResult(const V3CexTrace& cex, const V3NetVec& constrList, const V3NetVec& fairList, const bool& safe, const V3NtkHandler* const handler, const uint32_t& index) {
   return cycleSimulateResult(cex, constrList, fairList, safe, handler, index);
}

// Simulation-Based Checker
const int cycleSimulateResult(const V3CexTrace& cex, const V3NetVec& constrList, const V3NetVec& fairList, const bool& safe, const V3NtkHandler* const handler, const uint32_t& index) {
   // Check Counterexample Directly on Input Ntk
   assert (cex.getTraceSize()); assert (handler); assert (!safe || !fairList.size());
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Create Simulator
   V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(ntk)) simulator = new V3AlgBvSimulate(handler);
   else simulator = new V3AlgAigSimulate(handler); assert (simulator);
   // Add Constraints to Simulator
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(constrList.size() + fairList.size() + 1);
   for (uint32_t i = 0; i < constrList.size(); ++i) targetNets.push_back(constrList[i]);
   for (uint32_t i = 0; i < fairList.size(); ++i) targetNets.push_back(fairList[i]);
   targetNets.push_back(ntk->getOutput(index)); simulator->reset(targetNets);
   // Storage for Liveness Properties
   V3Vec<V3BitVec>::Vec fairVec(fairList.size() ? fairList.size() : 1, V3BitVec(cex.getTraceSize()));
   V3Vec<V3BitVecX>::Vec stateVec; stateVec.clear();
   // For each time-frame, set pattern from counter-example
   V3BitVecX pattern, value;
   for (uint32_t i = 0, k; i < cex.getTraceSize(); ++i) {
      // Update FF Next State Values
      simulator->updateNextStateValue();
      // Set Initial State Values
      if (!i && cex.getInit()) {
         V3BitVecX* const initValue = cex.getInit(); k = 0;
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
            pattern = simulator->getSimValue(ntk->getLatch(j)).bv_slice(ntk->getNetWidth(ntk->getLatch(j)) - 1, 0);
            value = initValue->bv_slice(k + ntk->getNetWidth(ntk->getLatch(j)) - 1, k);
            if (!pattern.bv_cover(value)) { reportUnexpectedState(1, j, value, pattern); return -1; }
            simulator->setSource(ntk->getLatch(j), value);
            k += ntk->getNetWidth(ntk->getLatch(j)); assert (k <= initValue->size());
         }
      }
      // Set PI Values
      if (cex.getTraceDataSize()) pattern = cex.getData(i); k = 0;
      for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
         simulator->setSource(ntk->getInput(j), pattern.bv_slice(k + ntk->getNetWidth(ntk->getInput(j)) - 1, k));
         k += ntk->getNetWidth(ntk->getInput(j)); assert (k <= pattern.size());
      }
      for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
         simulator->setSource(ntk->getInout(j), pattern.bv_slice(k + ntk->getNetWidth(ntk->getInout(j)) - 1, k));
         k += ntk->getNetWidth(ntk->getInout(j)); assert (k <= pattern.size());
      }
      // Simulate Ntk for a Cycle
      simulator->simulate();
      // Check Invariant Constraints
      for (uint32_t j = 0; j < constrList.size(); ++j) {
         assert (1 == ntk->getNetWidth(constrList[j]));
         value = simulator->getSimValue(constrList[j]).bv_slice(0, 0);
         if ('1' != value[0]) { reportUnsatisfiedConstraint(1 + i, j, value); return -1; }
      }
      // Check Property Assertion
      value = simulator->getSimValue(ntk->getOutput(index)).bv_slice(0, 0);
      if (safe) { if ((i < (cex.getTraceSize() - 1)) && ('1' == value[0])) reportShorterTrace(1 + i, value[0]); }
      else if ('1' != value[0]) {
         Msg(MSG_IFO) << "Cycle = " << i << ", Root Constraint has unexpected value = " << value << endl; return -1; }
      // Record Data for Liveness Checking
      if (!safe) {
         for (uint32_t j = 0; j < fairList.size(); ++j) {
            assert (1 == ntk->getNetWidth(fairList[j]));
            value = simulator->getSimValue(fairList[j]).bv_slice(0, 0);
            if ('1' == value[0]) fairVec[j].set1(i);
         }
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
            const uint32_t width = ntk->getNetWidth(ntk->getLatch(j));
            if (!j) value = simulator->getSimValue(ntk->getLatch(j)).bv_slice(width - 1, 0);
            else value = value.bv_concat(simulator->getSimValue(ntk->getLatch(j)).bv_slice(width - 1, 0));
         }
         stateVec.push_back(value);
      }
   }
   delete simulator; simulator = 0;
   if (!safe) {
      // Check the Existence of Loop
      uint32_t looped = stateVec.size() - 1; assert (stateVec.size());
      while (looped--) {
         if (stateVec.back() != stateVec[looped]) continue;
         // Check Fairness Constraints
         uint32_t i = 0, j;
         for (; i < fairList.size(); ++i) {
            for (j = looped; j < stateVec.size(); ++j) if (fairVec[i][j]) break;
            if (stateVec.size() == j) break;
         }
         if (fairList.size() == i) return 1;
      }
      return -1;
   }
   else return ('1' == value[0]) ? 1 : -1;
}

const int expandSimulateResult(const V3CexTrace& cex, const V3NetVec& constrList, const V3NetVec& fairList, const bool& safe, const V3NtkHandler* const handler) {
   // Check Counterexample on Time-Frame Expanded Input Ntk
   assert (cex.getTraceSize()); assert (handler); assert (!safe || !fairList.size());
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   return 0;
}

// Counterexample Checker Report Functions
void reportUnexpectedState(const uint32_t& cycle, const uint32_t& index, const V3BitVecX& v1, const V3BitVecX& v2) {
   Msg(MSG_IFO) << "Cycle = " << cycle << ", DFF(" << index << ") has unexpected trace value = " << v1
                << ", expected simulation value = " << v2 << endl;
}

void reportUnsatisfiedConstraint(const uint32_t& cycle, const uint32_t& index, const V3BitVecX& v) {
   Msg(MSG_IFO) << "Cycle = " << cycle << ", Constraint " << index << " has unexpected value = " << v << endl;
}

void reportShorterTrace(const uint32_t& cycle, const char& value) {
   Msg(MSG_IFO) << "A shorter trace found @ Cycle = " << cycle << ", assertion = " << value << endl;
}

// Inductive Invariant Checker
const int formalCheckProvenResult(const V3Ntk* const inv, const V3NtkHandler* const handler) {
   assert (inv); assert (handler);
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Inductive Invariant Recording has not implemented yet.
   // After that, we can complete this check.
   return 0;
}

#endif

