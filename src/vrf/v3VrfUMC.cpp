/****************************************************************************
  FileName     [ v3VrfUMC.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Unbounded Model Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_UMC_C
#define V3_VRF_UMC_C

#include "v3Msg.h"
#include "v3VrfUMC.h"
#include "v3VrfResult.h"

#include <cmath>
#include <ctime>
#include <iomanip>

/* -------------------------------------------------- *\
 * Class V3VrfUMC Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfUMC::V3VrfUMC(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   // Private Data Members
   _preDepth = 0; _incDepth = 1; _umcAttr = 0; _solver = 0;
   // Uniqueness Constraints
   _uniList.clear(); _uniLatch.clear(); _uniAssump.clear();
}

V3VrfUMC::~V3VrfUMC() {
   // Private Data Members
   if (_solver) delete _solver; _solver = 0;
   // Uniqueness Constraints
   for (uint32_t i = 0; i < _uniAssump.size(); ++i) _uniAssump[i].clear();
   _uniAssump.clear(); _uniList.clear(); _uniLatch.clear();
}

// Private Verification Main Functions
/* ---------------------------------------------------------------------------------------------------- *\
isIncKeepLastReachability(): Keep uniqueness constraints if solver is kept.
\* ---------------------------------------------------------------------------------------------------- */
void
V3VrfUMC::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p);
   
   // Consistency Check
   consistencyCheck(); assert (!_constr.size());
   
   // Initialize Solver
   if (_solver) { if (isIncContinueOnLastSolver()) _solver->update(); else { delete _solver; _solver = 0; } }
   if (!_solver) { _solver = allocSolver(getSolver(), _vrfNtk); assert (_solver->totalSolves() == 0); }

   // Initialize Uniqueness Constraint Data Members
   if (!isIncContinueOnLastSolver() || !isIncKeepLastReachability()) {
      for (uint32_t i = 0; i < _uniAssump.size(); ++i) _uniAssump[i].clear();
      _uniAssump.clear(); _uniList.clear(); _uniLatch.clear();
   }
   
   // Initialize Parameters
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _vrfNtk->getOutput(p); assert (V3NetUD != pId);
   const uint32_t logMaxWidth = (uint32_t)(ceil(log10(_maxDepth)));
   const string flushSpace = string(100, ' ');
   uint32_t proved = V3NtkUD, fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);
   uint32_t lastDepth = getIncLastDepthToKeepGoing(); if (10000000 < lastDepth) lastDepth = 0;
   uint32_t boundDepth = (lastDepth ? (lastDepth + _incDepth) : (_preDepth ? _preDepth : _incDepth));

   // Initialize Solver Data
   V3PtrVec pFormula; pFormula.clear();
   pFormula.reserve((_preDepth > _incDepth) ? _preDepth : _incDepth);
   
   // Initialize Uniqueness Data
   if (isUniqueness() && _uniLatch.size() != _vrfNtk->getLatchSize()) _uniLatch = V3UI32Vec(_vrfNtk->getLatchSize(), 0);

   // Start UMC Based Verification
   uint32_t i = 0, k = 0;
   for (uint32_t j = _maxDepth; i < j; ++i) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
      // Check Shared Results
      if (_sharedBound && (i >= k)) {
         k = _sharedBound->getBound(p); if (V3NtkUD == k) break;
         if (boundDepth <= k) { boundDepth = 1 + k; lastDepth += _incDepth; if (k < lastDepth) lastDepth = k; }
      }
      // Check Shared Networks
      if (_sharedNtk) {
         V3NtkHandler* const sharedNtk = _sharedNtk->getNtk(_handler);
         if (sharedNtk) {
            setIncKeepLastReachability(true); setIncContinueOnLastSolver(false); setIncLastDepthToKeepGoing(i);
            _handler = sharedNtk; _vrfNtk = sharedNtk->getNtk(); goto vrfRestart;
         }
      }
      // Add One Frame COI to SAT Solver
      _solver->addBoundedVerifyData(pId, i);
      // Check Shared Invariants
      if (_sharedInv) _sharedInv->pushInv(_solver, _vrfNtk, i);
      // Add Uniqueness Constraints
      if (isUniqueness()) {
         // Push New V3SvrDataVec for New Depth
         for (uint32_t u = _uniAssump.size(); u <= i; ++u)
            _uniAssump.push_back(V3SvrDataVec(_uniList.size(), V3NtkUD));
         // Complete Uniqueness Constraints in New COI
         for (uint32_t u = 0; u < _uniList.size(); ++u)
            for (uint32_t d = _uniList[u]; d <= i; ++d) addUniqueness(d, u);
      }
      if (i < lastDepth) continue;
      // Put Property Formula into List
      pFormula.push_back(_solver->getFormula(pId, i));
      // Perform Induction Check
      if (!isFireOnly() && (1 == pFormula.size())) {
         // Add assume for assumption solve only
         _solver->assumeRelease(); _solver->assumeProperty(pId, false, i);
         for (uint32_t x = 0; x < i; ++x) _solver->assumeProperty(pId, true, x);
         // Assumption Solve : If UNSAT, proved!
         while (true) {
            _solver->simplify();
            if (!_solver->assump_solve()) {
               if (!isProveOnly()) { proved = i; break; }
               if (_sharedBound) {
                  if (i <= _sharedBound->getBound(p)) { proved = i; break; } sleep(5);
                  if (i <= _sharedBound->getBound(p)) { proved = i; break; } return;
               }
            }
            else {
               if (!isUniqueness()) break;
               // Analyze Counterexample for New Uniqueness Bounds
               // Solve the Inductive Case Again Under New Constraints
               if (!generateAndAddUniquenessBound(i)) break;
            }
         }
         if (V3NtkUD != proved) break;
         // Compute New Latches for Uniqueness in the Next Iteration
         if (isUniqueness()) computeLatchBoundsForUniqueness(i);
      }
      // Perform BMC Check
      if (!isProveOnly()) {
         // Check if the bound is achieved
         if (k > i) { pFormula.clear(); continue; }
         if ((1 + i) < boundDepth) continue; assert ((1 + i) == boundDepth);
         V3SvrData pFormulaData = 0; assert ((1 + i) >= pFormula.size());
         // Add assume for assumption solve only
         _solver->assumeRelease(); assert (pFormula.size());
         if (1 == pFormula.size()) _solver->assumeProperty(pId, false, i);
         else {
            pFormulaData = _solver->setImplyUnion(pFormula);
            assert (pFormulaData); _solver->assumeProperty(pFormulaData);
         }
         if (i >= pFormula.size()) 
            for (uint32_t x = 0; x <= (i - pFormula.size()); ++x) _solver->assumeProperty(pId, true, x);
         // Assumption Solve : If SAT, disproved!
         if (!isProveOnly()) {
            if (!_sharedBound || (i >= _sharedBound->getBound(p))) {
               // Conjunction with initial condition
               _solver->assumeInit(); _solver->simplify();
               if (_solver->assump_solve()) {
                  for (uint32_t x = 0; x < pFormula.size(); ++x)
                     if (_solver->getDataValue(pFormula[x])) { fired = (1 + i + x - pFormula.size()); break; }
                  assert (V3NtkUD != fired); break;
               }
               if (_sharedBound) _sharedBound->updateBound(p, 1 + i);
            }
         }
         // Increase Depth for Next BMC
         if (isDynamicInc()) {
            boundDepth += _incDepth;
         }
         else boundDepth += _incDepth;
         // Add assert back to the property
         if (1 < pFormula.size()) { assert (pFormulaData); _solver->assertProperty(pFormulaData, true); }
      }
      pFormula.clear();

      // Report Verification Progress
      if (!isIncKeepSilent() && intactON()) {
         if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << "Verification completed under depth = " << setw(logMaxWidth) << (i + 1);
         if (svrInfoON()) { Msg(MSG_IFO) << "  ("; _solver->printInfo(); Msg(MSG_IFO) << ")"; }
         if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != proved) Msg(MSG_IFO) << "Inductive Invariant found at depth = " << ++proved;
      else if (V3NtkUD != fired) Msg(MSG_IFO) << "Counter-example found at depth = " << ++fired;
      else Msg(MSG_IFO) << "UNDECIDED at depth = " << _maxDepth;
      if (usageON()) {
         gettimeofday(&curtime, NULL);
         Msg(MSG_IFO) << "  (time = " << setprecision(5) << getTimeUsed(inittime, curtime) << "  sec)" << endl;
      }
      if (profileON()) { /* Report some profiling here ... */ }
   }
   else { if (V3NtkUD != proved) ++proved; else if (V3NtkUD != fired) ++fired; }

   // Record CounterExample Trace or Invariant
   if (V3NtkUD != fired) {  // Record Counter-Example
      V3CexTrace* const cex = new V3CexTrace(fired); assert (cex);
      // Set Pattern Value
      uint32_t patternSize = _vrfNtk->getInputSize() + _vrfNtk->getInoutSize();
      V3BitVecX dataValue, patternValue(patternSize ? patternSize : 1);
      for (i = 0; i < fired; ++i) {
         patternSize = 0; patternValue.clear();
         for (uint32_t j = 0; j < _vrfNtk->getInputSize(); ++j, ++patternSize) {
            if (!_solver->existVerifyData(_vrfNtk->getInput(j), i)) continue;
            dataValue = _solver->getDataValue(_vrfNtk->getInput(j), i);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         for (uint32_t j = 0; j < _vrfNtk->getInoutSize(); ++j, ++patternSize) {
            if (!_solver->existVerifyData(_vrfNtk->getInout(j), i)) continue;
            dataValue = _solver->getDataValue(_vrfNtk->getInout(j), i);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         assert (_solver->existVerifyData(pId, i));
         assert (!patternSize || patternSize == patternValue.size()); cex->pushData(patternValue);
      }
      // Set Initial State Value
      if (_vrfNtk->getLatchSize()) {
         patternValue.resize(_vrfNtk->getLatchSize());
         patternValue.clear(); V3NetId id;
         for (uint32_t j = 0; j < _vrfNtk->getLatchSize(); ++j) {
            if (_solver->existVerifyData(_vrfNtk->getLatch(j), 0)) {
               dataValue = _solver->getDataValue(_vrfNtk->getLatch(j), 0);
               if ('0' == dataValue[0]) patternValue.set0(j);
               else if ('1' == dataValue[0]) patternValue.set1(j);
            }
            else {
               id = _vrfNtk->getInputNetId(_vrfNtk->getLatch(j), 1);
               if (id.id) { assert (id == _vrfNtk->getLatch(j)); continue; }
               if (id.cp) patternValue.set1(j); else patternValue.set0(j);
            }
         }
         cex->setInit(patternValue);
      }
      _result[p].setCexTrace(cex); assert (_result[p].isCex());
      // Check Common Results
      if (isIncVerifyUsingCurResult()) checkCommonCounterexample(p, *cex);
   }
   else if (V3NtkUD != proved) {  // Record Inductive Invariant
      _result[p].setIndInv(_vrfNtk); assert (_result[p].isInv());
   }
}

void
V3VrfUMC::addUniqueness(const uint32_t& depth, const uint32_t& uIndex, const bool& force) {
   assert (_solver); assert (uIndex < _uniList.size());
   const uint32_t bound = _uniList[uIndex]; assert (depth >= bound);
   const uint32_t eqDepth = depth - bound;
   // Add Uniqueness Constraints for [eqDepth, depth]
   assert (_uniAssump.size() > eqDepth);
   assert (_uniAssump[eqDepth].size() > uIndex);
   assert (!force || (V3NtkUD == _uniAssump[eqDepth][uIndex]));
   V3NetId id; V3SvrDataVec eq(3), assump; assump.clear();
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
      id = _vrfNtk->getLatch(i); assert (id.id < _vrfNtk->getNetSize());
      if (!force && (depth < _uniLatch[i])) continue;  // Constraint Computed
      if (!_solver->existVerifyData(id, eqDepth)) continue;
      if (!_solver->existVerifyData(id, depth)) continue;
      assump.push_back(_solver->reserveFormula());
      eq[0] = _solver->getNegFormula(assump.back());
      eq[1] = _solver->getFormula(id, eqDepth);
      eq[2] = _solver->getFormula(id, depth);
      _solver->assertImplyUnion(eq);
      eq[1] = _solver->getFormula(~id, eqDepth);
      eq[2] = _solver->getFormula(~id, depth);
      _solver->assertImplyUnion(eq);
   }
   if (assump.size()) {
      if (V3NtkUD != _uniAssump[eqDepth][uIndex]) assump.push_back(_uniAssump[eqDepth][uIndex]);
      _uniAssump[eqDepth][uIndex] = _solver->setImplyUnion(assump);
   }
   if (V3NtkUD != _uniAssump[eqDepth][uIndex]) _solver->assumeProperty(_uniAssump[eqDepth][uIndex]);
}

void
V3VrfUMC::computeLatchBoundsForUniqueness(const uint32_t& depth) {
   assert (_solver); assert (_uniAssump.size() > depth);
   assert (_uniLatch.size() == _vrfNtk->getLatchSize());
   // Update Upper Bound of Depth for the Existence of Latches
   for (uint32_t i = 0; i < _uniLatch.size(); ++i)
      while (_solver->existVerifyData(_vrfNtk->getLatch(i), _uniLatch[i])) { ++_uniLatch[i]; }
}

const bool
V3VrfUMC::generateAndAddUniquenessBound(const uint32_t& depth) {
   assert (_solver); if (!depth) return false;
   // Record State Values on Current Counterexample
   V3Vec<V3BitVecX>::Vec stateVec(1 + depth); V3NetId id;
   for (uint32_t i = 0; i <= depth; ++i) {
      for (uint32_t j = 0; j < _vrfNtk->getLatchSize(); ++j) {
         id = _vrfNtk->getLatch(j); assert (id.id < _vrfNtk->getNetSize());
         if (!j) {
            if (!_solver->existVerifyData(id, i)) stateVec[i].resize(1);
            else stateVec[i] = _solver->getDataValue(id, i);
         }
         else stateVec[i] = stateVec[i].bv_concat((_solver->existVerifyData(id, i)) ? 
                                                  _solver->getDataValue(id, i) : V3BitVecX(1));
      }
   }
   // Check Repeated States
   V3UI32Set newUniBound; newUniBound.clear();
   for (uint32_t i = 0; i < stateVec.size(); ++i)
      for (uint32_t j = 1 + i; j < stateVec.size(); ++j)
         if (stateVec[i].bv_intersect(stateVec[j])) newUniBound.insert(j - i);
   if (!newUniBound.size()) return false;
   // Add New Uniqueness Constraints
   for (V3UI32Set::const_iterator it = newUniBound.begin(); it != newUniBound.end(); ++it) {
      uint32_t i = 0, j; for (; i < _uniAssump.size(); ++i) _uniAssump[i].push_back(V3NtkUD);
      i = depth; j = _uniList.size(); _uniList.push_back(*it); while (*it <= i) addUniqueness(i--, j, true);
   }
   return true;
}

#endif

