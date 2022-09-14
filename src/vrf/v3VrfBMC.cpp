/****************************************************************************
  FileName     [ v3VrfBMC.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Bounded Model Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_BMC_C
#define V3_VRF_BMC_C

#include "v3VrfBMC.h"
#include "v3NtkUtil.h"
#include "v3NtkExpand.h"

/* -------------------------------------------------- *\
 * Class V3VrfBMC Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfBMC::V3VrfBMC(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   // Private Data Members
   _preDepth = 0; _incDepth = 1;
}

V3VrfBMC::~V3VrfBMC() {
}

// Private Verification Main Functions
void
V3VrfBMC::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p);
   
   // Consistency Check
   consistencyCheck(); assert (!_constr.size());
   
   // Initialize Parameters
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _vrfNtk->getOutput(p); assert (V3NetUD != pId);
   const uint32_t logMaxWidth = (uint32_t)(ceil(log10(_maxDepth)));
   const string flushSpace = string(100, ' ');
   uint32_t fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);
   uint32_t lastDepth = getIncLastDepthToKeepGoing(); if (10000000 < lastDepth) lastDepth = 0;
   uint32_t boundDepth = lastDepth ? lastDepth : 0, incSize = 0;

   // Start BMC Based Verification
   V3Ntk* simpNtk = 0; V3SvrBase* solver = 0;
   while (boundDepth < _maxDepth) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
      // Check Shared Results
      if (_sharedBound) {
         const uint32_t k = _sharedBound->getBound(p); if (V3NtkUD == k) break;
         if (boundDepth < k) boundDepth = k;
      }
      // Check Shared Networks
      if (_sharedNtk) {
         V3NtkHandler* const sharedNtk = _sharedNtk->getNtk(_handler);
         if (sharedNtk) {
            setIncKeepLastReachability(true); setIncContinueOnLastSolver(false); setIncLastDepthToKeepGoing(boundDepth);
            _handler = sharedNtk; _vrfNtk = sharedNtk->getNtk(); goto vrfRestart;
         }
      }
      incSize = boundDepth;
      if (!boundDepth) boundDepth = _preDepth; boundDepth += _incDepth;
      incSize = boundDepth - incSize;
      // Expand Network and Set Initial States
      V3NtkExpand* const pNtk = new V3NtkExpand(_handler, boundDepth, true); assert (pNtk);
      V3NetId id; V3NetVec p2cMap, c2pMap; V3RepIdHash repIdHash; repIdHash.clear();
      simpNtk = duplicateNtk(pNtk, p2cMap, c2pMap); assert (simpNtk);
      // Create Outputs for the Unrolled Property Signals
      for (uint32_t i = boundDepth - incSize, j = 0; j < incSize; ++i, ++j) {
         id = pNtk->getNtk()->getOutput(p + (_vrfNtk->getOutputSize() * i));
         simpNtk->createOutput(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      }
      // Set CONST 0 to Proven Property Signals
      for (uint32_t i = 0, j = boundDepth - incSize, k = p; i < j; ++i, k += _vrfNtk->getOutputSize()) {
         id = pNtk->getNtk()->getOutput(k); id = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
         repIdHash.insert(make_pair(id.id, V3NetId::makeNetId(0, id.cp)));
      }
      if (repIdHash.size()) simpNtk->replaceFanin(repIdHash); delete pNtk; p2cMap.clear(); c2pMap.clear();
      assert (!simpNtk->getLatchSize()); assert (incSize == simpNtk->getOutputSize());
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) { delete simpNtk; break; }
      // Initialize Solver
      solver = allocSolver(getSolver(), simpNtk); assert (solver);
      for (uint32_t i = 0, k = 1 + boundDepth - incSize; i < incSize; ++i, ++k) {
         solver->addBoundedVerifyData(simpNtk->getOutput(i), 0); solver->simplify();
         solver->assumeRelease(); solver->assumeProperty(simpNtk->getOutput(i), false, 0);
         if (solver->assump_solve()) { fired = k; break; }
         solver->assertProperty(simpNtk->getOutput(i), true, 0);
         // Report Verification Progress
         if (!isIncKeepSilent() && intactON()) {
            if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
            Msg(MSG_IFO) << "Verification completed under depth = " << setw(logMaxWidth) << k;
            if (svrInfoON()) { Msg(MSG_IFO) << "  ("; solver->printInfo(); Msg(MSG_IFO) << ")"; }
            if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
         }
      }
      if (V3NtkUD != fired) break; delete solver; delete simpNtk;
      if (_sharedBound) _sharedBound->updateBound(p, boundDepth);
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != fired) Msg(MSG_IFO) << "Counter-example found at depth = " << fired;
      else Msg(MSG_IFO) << "UNDECIDED at depth = " << _maxDepth;
      if (usageON()) {
         gettimeofday(&curtime, NULL);
         Msg(MSG_IFO) << "  (time = " << setprecision(5) << getTimeUsed(inittime, curtime) << "  sec)" << endl;
      }
      if (profileON()) { /* Report some profiling here ... */ }
   }

   // Record CounterExample Trace or Invariant
   if (V3NtkUD != fired) {  // Record Counter-Example
      V3CexTrace* const cex = new V3CexTrace(fired); assert (cex);
      // Set Pattern Value
      uint32_t patternSize = _vrfNtk->getInputSize() + _vrfNtk->getInoutSize();
      V3BitVecX dataValue, patternValue(patternSize ? patternSize : 1);
      for (uint32_t i = 0, inSize = 0, ioSize = 0; i < fired; ++i) {
         patternSize = 0; patternValue.clear();
         for (uint32_t j = 0; j < _vrfNtk->getInputSize(); ++j, ++patternSize, ++inSize) {
            if (!solver->existVerifyData(simpNtk->getInput(inSize), 0)) continue;
            dataValue = solver->getDataValue(simpNtk->getInput(inSize), 0);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         for (uint32_t j = 0; j < _vrfNtk->getInoutSize(); ++j, ++patternSize, ++ioSize) {
            if (!solver->existVerifyData(simpNtk->getInout(ioSize), 0)) continue;
            dataValue = solver->getDataValue(simpNtk->getInout(ioSize), 0);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         assert (!patternSize || patternSize == patternValue.size()); cex->pushData(patternValue);
      }
      // Set Initial State Value
      if (_vrfNtk->getLatchSize()) {
         const uint32_t piSize = boundDepth * _vrfNtk->getInputSize();
         patternValue.resize(_vrfNtk->getLatchSize());
         patternValue.clear(); V3NetId id; uint32_t k = 0;
         for (uint32_t j = 0; j < _vrfNtk->getLatchSize(); ++j) {
            id = _vrfNtk->getInputNetId(_vrfNtk->getLatch(j), 1);
            if (!id.id) { if (id.cp) patternValue.set1(j); else patternValue.set0(j); }
            else {
               assert (simpNtk->getInputSize() > (piSize + k));
               if (solver->existVerifyData(simpNtk->getInput(piSize + k), 0)) {
                  dataValue = solver->getDataValue(simpNtk->getInput(piSize + k), 0);
                  if ('0' == dataValue[0]) patternValue.set0(j);
                  else if ('1' == dataValue[0]) patternValue.set1(j);
               }
               ++k;
            }
         }
         cex->setInit(patternValue);
      }
      delete solver; delete simpNtk;
      _result[p].setCexTrace(cex); assert (_result[p].isCex());
      // Check Common Results
      if (isIncVerifyUsingCurResult()) checkCommonCounterexample(p, *cex);
   }
}

#endif

