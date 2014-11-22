/****************************************************************************
  FileName     [ v3VrfBase.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Base Class for Verification on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_BASE_C
#define V3_VRF_BASE_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3VrfBase.h"
#include "v3VrfResult.h"
#include "v3AlgSimulate.h"

/* -------------------------------------------------- *\
 * Class V3VrfBase Implementations
\* -------------------------------------------------- */
// Static Member Initialization
unsigned char   V3VrfBase::_extVerbosity        = 0;
V3SolverType    V3VrfBase::_extSolverType       = V3_SVR_TOTAL;

// Constructor and Destructor
V3VrfBase::V3VrfBase(const V3NtkHandler* const handler) {
   _handler = const_cast<V3NtkHandler*>(handler); assert (_handler);
   _vrfNtk = handler->getNtk(); assert (_vrfNtk); assert (_vrfNtk->getOutputSize());
   // Private Data Members
   _constr.clear(); _maxTime = V3DblMAX; _maxDepth = 100; _solverType = V3_SVR_TOTAL;
   // Private Members for Verification Results
   _result = V3VrfResultVec(_vrfNtk->getOutputSize());
   if (_vrfNtk->getModuleSize())
      Msg(MSG_WAR) << "Module Instances will be Treated as Black-Boxes in Model Checking !!" << endl;
   // Private Members for Shared Information
   _sharedBound = 0; _sharedInv = 0; _sharedNtk = 0; _sharedMem = 0;
   // Private Members for Incremental Verification
   _incKeepSilent = 0; _incKeepLastReachability = 0; _incContinueOnLastSolver = 0;
   _incVerifyUsingCurResult = 0; _incLastDepthToKeepGoing = 0;
}

V3VrfBase::~V3VrfBase() {
   _constr.clear(); _result.clear();
   if (_sharedNtk) _sharedNtk->releaseNtk(_handler);
}

// Constraints Setting Functions
void
V3VrfBase::setConstraint(const V3NetVec& constrList, const uint32_t& p) {
   if (p >= _constr.size()) _constr.resize(1 + p);
   _constr[p].clear(); _constr[p].reserve(constrList.size());
   for (uint32_t i = 0; i < constrList.size(); ++i) {
      assert (constrList[i].id < _vrfNtk->getNetSize());
      _constr[p].push_back(constrList[i]);
   }
}

// Verification Main Functions
void
V3VrfBase::verifyInOrder() {
   for (uint32_t i = 0; i < _result.size(); ++i)
      if (!_result[i].isCex() && !_result[i].isInv()) verifyProperty(i);
}

void
V3VrfBase::verifyProperty(const uint32_t& p) {
   assert (p < _result.size()); startVerify(p);
}

// Interactive Verbosity Setting Functions
void
V3VrfBase::printReportSettings() {
   if (1ul  & _extVerbosity) {
      Msg(MSG_IFO) << "-" << ((2ul  & _extVerbosity) ? "NO" : "") << "Interactive ";
      if (4ul  & _extVerbosity) Msg(MSG_IFO) << "-Endline ";
      if (8ul  & _extVerbosity) Msg(MSG_IFO) << "-Solver ";
      if (16ul & _extVerbosity) Msg(MSG_IFO) << "-Usage ";
      if (32ul & _extVerbosity) Msg(MSG_IFO) << "-Profile ";
   }
}

// Solver List Setting Functions
void
V3VrfBase::printSolverSettings() {
   Msg(MSG_IFO) << V3SolverTypeStr[getDefaultSolver()];
}

// Private Verification Main Functions
void
V3VrfBase::consistencyCheck() const {
   for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i) { assert (1 == _vrfNtk->getNetWidth(_vrfNtk->getInput(i))); }
   for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i) { assert (1 == _vrfNtk->getNetWidth(_vrfNtk->getInout(i))); }
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) { assert (1 == _vrfNtk->getNetWidth(_vrfNtk->getLatch(i)));
      const V3NetId id = _vrfNtk->getInputNetId(_vrfNtk->getLatch(i), 1); assert (!id.id || id == _vrfNtk->getLatch(i)); }
}

void
V3VrfBase::startVerify(const uint32_t& p) {
}

// Functions for Checking Common Results
void
V3VrfBase::checkCommonCounterexample(const uint32_t& p, const V3CexTrace& cex) {
   assert (_result[p].isCex()); assert (cex.getTraceSize());
   // Collect Unsolved Properties
   V3UI32Vec unsolved; unsolved.clear(); unsolved.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i)
      if (!(_result[i].isCex() || _result[i].isInv())) unsolved.push_back(i);
   if (!unsolved.size()) return; assert (!_constr.size());
   V3UI32Vec firedIndex, firedDepth; firedIndex.clear(); firedDepth.clear();
   // Check Same Property
   const V3NetId pId = _vrfNtk->getOutput(p);
   for (uint32_t i = 0; i < unsolved.size(); ++i) {
      if (pId != _vrfNtk->getOutput(unsolved[i])) continue;
      firedIndex.push_back(unsolved[i]); firedDepth.push_back(cex.getTraceSize());
      unsolved.erase(unsolved.begin() + i); --i;
   }
   // Create Simulator
   V3AlgSimulate* simulator = 0; V3BitVecX pattern;
   if (dynamic_cast<V3BvNtk*>(_vrfNtk)) simulator = new V3AlgBvSimulate(_handler);
   else simulator = new V3AlgAigSimulate(_handler); assert (simulator);
   // For each time-frame, set pattern from counter-example
   for (uint32_t i = 0, k; i < cex.getTraceSize(); ++i) {
      if (!unsolved.size()) break;
      // Update FF Next State Values
      simulator->updateNextStateValue();
      // Set Initial State Values
      if (!i && cex.getInit()) {
         V3BitVecX* const initValue = cex.getInit(); k = 0;
         for (uint32_t j = 0; j < _vrfNtk->getLatchSize(); ++j, ++k)
            simulator->setSource(_vrfNtk->getLatch(j), initValue->bv_slice(k, k));
      }
      // Set PI Values
      if (cex.getTraceDataSize()) pattern = cex.getData(i); k = 0;
      for (uint32_t j = 0; j < _vrfNtk->getInputSize(); ++j, ++k)
         simulator->setSource(_vrfNtk->getInput(j), pattern.bv_slice(k, k));
      for (uint32_t j = 0; j < _vrfNtk->getInoutSize(); ++j, ++k)
         simulator->setSource(_vrfNtk->getInout(j), pattern.bv_slice(k, k));
      // Simulate Ntk for a Cycle
      simulator->simulate();
      // Check Property Assertion
      for (uint32_t j = 0; j < unsolved.size(); ++j) {
         if ('1' == simulator->getSimValue(_vrfNtk->getOutput(unsolved[j])).bv_slice(0, 0)[0]) {
            firedIndex.push_back(unsolved[j]); firedDepth.push_back(1 + i);
            unsolved.erase(unsolved.begin() + j); --j;
         }
      }
   }
   delete simulator; simulator = 0; assert (firedIndex.size() == firedDepth.size());
   // Set Sub-Traces for Cex to Fired Properties
   for (uint32_t i = 0; i < firedIndex.size(); ++i) {
      V3CexTrace* const subTrace = new V3CexTrace(firedDepth[i]); assert (subTrace);
      if (cex.getTraceDataSize()) for (uint32_t j = 0; j < firedDepth[i]; ++j) subTrace->pushData(cex.getData(j));
      if (cex.getInit()) subTrace->setInit(*(cex.getInit())); _result[firedIndex[i]].setCexTrace(subTrace);
   }
}

void
V3VrfBase::checkCommonProof(const uint32_t& p, const V3NetTable& invList, const bool& checkInitial) {
   assert (_result[p].isInv());
   // Collect Unsolved Properties
   V3UI32Vec unsolved; unsolved.clear(); unsolved.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i)
      if (!(_result[i].isCex() || _result[i].isInv())) unsolved.push_back(i);
   // Check Same Property
   const V3NetId pId = _vrfNtk->getOutput(p);
   for (uint32_t i = 0; i < unsolved.size(); ++i) {
      if (pId != _vrfNtk->getOutput(unsolved[i])) continue;
      _result[unsolved[i]].setIndInv(_vrfNtk);
      unsolved.erase(unsolved.begin() + i); --i;
   }
   if (!unsolved.size()) return; assert (!_constr.size());
   // Create Solver
   V3SvrBase* const solver = allocSolver(getSolver(), _vrfNtk); assert (solver);
   V3SvrDataVec formula; formula.clear(); V3NetId id;
   // Set Inductive Invariant
   for (uint32_t i = 0; i < invList.size(); ++i) {
      formula.clear(); formula.reserve(invList[i].size());
      for (uint32_t j = 0; j < invList[i].size(); ++j) {
         id = invList[i][j]; assert (_vrfNtk->getLatchSize() > id.id);
         if (!solver->existVerifyData(_vrfNtk->getLatch(id.id), 0)) 
            solver->addBoundedVerifyData(_vrfNtk->getLatch(id.id), 0);
         assert (solver->existVerifyData(_vrfNtk->getLatch(id.id), 0));
         formula.push_back(solver->getFormula(_vrfNtk->getLatch(id.id), 0, 0));
         if (!id.cp) formula.back() = solver->getNegFormula(formula.back());
      }
      solver->assertImplyUnion(formula);
   }
   if (checkInitial) {
      solver->assumeRelease(); solver->assumeInit();
      if (solver->assump_solve()) { delete solver; return; }
   }
   // Check if the bad State Signal is Inconsistent with the Inductive Invariant
   for (uint32_t i = 0, j = 0; i < unsolved.size(); ++i) {
      solver->assumeRelease();
      if (!solver->existVerifyData(_vrfNtk->getOutput(unsolved[i]), 0))
         solver->addBoundedVerifyData(_vrfNtk->getOutput(unsolved[i]), 0);
      assert (solver->existVerifyData(_vrfNtk->getOutput(unsolved[i]), 0));
      solver->assumeProperty(_vrfNtk->getOutput(unsolved[i]), false, 0);
      if (!solver->assump_solve()) { _result[unsolved[i]].setIndInv(_vrfNtk); j = 0; }
      else if (++j > 100) break;  // Avoid Spending Too Much Effort on the Check
   }
   delete solver;
}

#endif

