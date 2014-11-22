/****************************************************************************
  FileName     [ v3VrfMPDR.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Monolithic Property Directed Reachability on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_MPDR_C
#define V3_VRF_MPDR_C

#include "v3Msg.h"
#include "v3Bucket.h"
#include "v3VrfMPDR.h"

#include "v3NtkUtil.h"

#include <iomanip>

//#define V3_MPDR_USE_PROPAGATE_BACKWARD
#define V3_MPDR_USE_PROPAGATE_LOW_COST

/* -------------------------------------------------- *\
 * Class V3MPDRFrame Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3MPDRFrame::V3MPDRFrame(const V3SvrData& a) : _activator(a) {
   _cubeList.clear();
}

V3MPDRFrame::~V3MPDRFrame() {
   for (V3MPDRCubeList::iterator it = _cubeList.begin(); it != _cubeList.end(); ++it) delete *it;
   _cubeList.clear();
}

// Retrieval Functions
const bool
V3MPDRFrame::pushCube(V3MPDRCube* const c) {
   const uint32_t cubeSize = _cubeList.size(); _cubeList.insert(c);
   return cubeSize != _cubeList.size();
}

// Cube Containment Functions
const bool
V3MPDRFrame::subsumes(const V3MPDRCube* const cube) const {
   // This function checks whether cube is subsumed by any cube in this frame.
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = ~(cube->getSignature());
   V3MPDRCubeList::const_reverse_iterator it = _cubeList.rbegin();
   for (; it != _cubeList.rend(); ++it) {
      const V3NetVec& state = (*it)->getState();
      // Early Return
      if (!cubeState.size() || cubeState[0].id > state[0].id) return false;
      if (cubeState.size() < state.size()) continue;
      if (cubeSignature & (*it)->getSignature()) continue;
      // General Check
      uint32_t j = 0, k = 0;
      while (j < cubeState.size() && k < state.size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (state[k].id > state[k - 1].id));
         if (cubeState[j].id > state[k].id) { assert (j >= k); if (j == k) return false; break; }
         else if (cubeState[j].id < state[k].id) ++j;
         else { if (cubeState[j].cp ^ state[k].cp) break; ++j; ++k; }
      }
      if (k == state.size()) return true;
   }
   return false;
}

void
V3MPDRFrame::removeSubsumed(const V3MPDRCube* const cube) {
   // This function checks whether there's any existing cube in this frame subsumed by cube.
   // If such cube is found, remove it from _cubeList
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3MPDRCubeList::iterator it = _cubeList.begin();
   while (it != _cubeList.end()) {
      const V3NetVec& state = (*it)->getState();
      // Early Return
      if (cubeState[0].id < state[0].id) return;
      if (state.size() < cubeState.size()) { ++it; continue; }
      if (cubeSignature & ~((*it)->getSignature())) { ++it; continue; }
      // General Check
      uint32_t j = 0, k = 0;
      while (j < cubeState.size() && k < state.size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (state[k].id > state[k - 1].id));
         if (cubeState[j].id < state[k].id) { assert (j <= k); if (j == k) return; break; }
         else if (cubeState[j].id > state[k].id) ++k;
         else { if (cubeState[j].cp ^ state[k].cp) break; ++j; ++k; }
      }
      if (j != cubeState.size()) ++it;
      else { delete *it; _cubeList.erase(it++); }
   }
}

void
V3MPDRFrame::removeSubsumed(const V3MPDRCube* const cube, const V3MPDRCubeList::const_iterator& ix) {
   // This function checks whether there's any existing cube in this frame subsumed by cube.
   // If such cube is found, remove it from _cubeList
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3MPDRCubeList::iterator it = _cubeList.begin();
   while (it != ix) {
      const V3NetVec& state = (*it)->getState();
      // Early Return
      assert (cubeState[0].id >= state[0].id);
      if (state.size() < cubeState.size()) { ++it; continue; }
      if (cubeSignature & ~((*it)->getSignature())) { ++it; continue; }
      // General Check
      uint32_t j = 0, k = 0;
      while (j < cubeState.size() && k < state.size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (state[k].id > state[k - 1].id));
         if (cubeState[j].id < state[k].id) { assert (j < k); break; }
         else if (cubeState[j].id > state[k].id) ++k;
         else { if (cubeState[j].cp ^ state[k].cp) break; ++j; ++k; }
      }
      if (j != cubeState.size()) ++it;
      else { delete *it; _cubeList.erase(it++); }
   }
}

void
V3MPDRFrame::removeSelfSubsumed() {
   // This function checks whether some cubes in this frame can be subsumed by the other cube
   // Remove all cubes that are subsumed by the cube in this frame from _cubeList
   V3MPDRCubeList::const_iterator ix;
   uint32_t candidates = 1;
   while (candidates <= _cubeList.size()) {
      ix = _cubeList.begin(); for (uint32_t i = candidates; i < _cubeList.size(); ++i) ++ix;
      removeSubsumed(*ix, ix); ++candidates;
   }
}

/* -------------------------------------------------- *\
 * Class V3VrfMPDR Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfMPDR::V3VrfMPDR(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   // Private Data Members
   _pdrFrame.clear(); _pdrBad = 0; _pdrSize = 0; _pdrAttr = 0;
   // Recycle Members
   _pdrBackup.clear(); _pdrRecycle = V3NtkUD; _pdrActCount = 0;
   // Private Engines
   _pdrSvr = 0; _pdrSim = 0; _pdrGen = 0;
   // Private Tables
   _pdrInitConst.clear(); _pdrInitValue.clear();
   // Extended Data Members
   _pdrPriority.clear();
   // Statistics
   if (profileON()) {
      _totalStat     = new V3Stat("TOTAL");
      _initSvrStat   = new V3Stat("SVR INIT",    _totalStat);
      _solveStat     = new V3Stat("SVR SOLVE",   _totalStat);
      _generalStat   = new V3Stat("GENERALIZE",  _totalStat);
      _propagateStat = new V3Stat("PROPAGATION", _totalStat);
      _ternaryStat   = new V3Stat("TERNARY SIM", _totalStat);
   }
}

V3VrfMPDR::~V3VrfMPDR() {
   // Private Data Members
   for (uint32_t i = 0; i < _pdrFrame.size(); ++i) delete _pdrFrame[i]; _pdrFrame.clear();
   if (_pdrBad) delete _pdrBad; _pdrBad = 0;
   // Recycle Members
   for (uint32_t i = 0; i < _pdrBackup.size(); ++i) delete _pdrBackup[i]; _pdrBackup.clear();
   // Private Engines
   if (_pdrSvr) delete _pdrSvr; _pdrSvr = 0;
   if (_pdrGen) delete _pdrGen; _pdrGen = 0;
   // Private Tables
   _pdrInitConst.clear(); _pdrInitValue.clear();
   // Extended Data Members
   _pdrPriority.clear();
   // Statistics
   if (profileON()) {
      if (_totalStat    ) delete _totalStat;
      if (_initSvrStat  ) delete _initSvrStat;
      if (_solveStat    ) delete _solveStat;
      if (_generalStat  ) delete _generalStat;
      if (_propagateStat) delete _propagateStat;
      if (_ternaryStat  ) delete _ternaryStat;
   }
}

// Verification Main Functions
/* ---------------------------------------------------------------------------------------------------- *\
isIncKeepLastReachability(): If the last result is unsat, put the inductive invariant into the last frame.
isIncContinueOnLastSolver(): Reset the solver.
\* ---------------------------------------------------------------------------------------------------- */
void
V3VrfMPDR::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p); if (profileON()) _totalStat->start();
   
   // Consistency Check
   consistencyCheck(); assert (!_constr.size());
   if (!reportUnsupportedInitialState()) return;
   
   // Initialize Backup Frames
   for (uint32_t i = 0; i < _pdrBackup.size(); ++i) delete _pdrBackup[i]; _pdrBackup.clear();
   if (_pdrFrame.size()) {
      if (isIncKeepLastReachability()) {
         // Backup frames in the order: ..., 2, 1, INF
         assert (_pdrFrame.size() > 1); _pdrBackup.reserve(_pdrFrame.size() - 1);
         for (uint32_t i = _pdrFrame.size() - 2; i > 0; --i) _pdrBackup.push_back(_pdrFrame[i]);
         _pdrBackup.push_back(_pdrFrame.back()); delete _pdrFrame[0];
      }
      else { for (uint32_t i = 0; i < _pdrFrame.size(); ++i) delete _pdrFrame[i]; } _pdrFrame.clear();
   }

   // Initialize Other Members
   if (!isIncKeepLastReachability()) _pdrPriority.clear(); _pdrActCount = 0;
   if (_pdrBad) delete _pdrBad; _pdrBad = 0; if (_pdrGen) delete _pdrGen; _pdrGen = 0;
   if (dynamic_cast<V3BvNtk*>(_vrfNtk)) {
      _pdrGen = new V3AlgBvGeneralize(_handler); assert (_pdrGen);
      _pdrSim = dynamic_cast<V3AlgBvSimulate*>(_pdrGen); assert (_pdrSim);
   }
   else {
      _pdrGen = new V3AlgAigGeneralize(_handler); assert (_pdrGen);
      _pdrSim = dynamic_cast<V3AlgAigSimulate*>(_pdrGen); assert (_pdrSim);
   }
   V3NetVec simTargets(1, _vrfNtk->getOutput(p)); _pdrSim->reset(simTargets);

   // Initialize Pattern Input Size
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _vrfNtk->getOutput(p); assert (V3NetUD != pId);
   _pdrSize = _vrfNtk->getInputSize() + _vrfNtk->getInoutSize();

   // Initialize Parameters
   const string flushSpace = string(100, ' ');
   uint32_t proved = V3NtkUD, fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);
   
   // Initialize Signal Priority List
   if (_pdrPriority.size() != _vrfNtk->getLatchSize()) _pdrPriority.resize(_vrfNtk->getLatchSize(), 0);

   // Initialize Solver
   if (_pdrSvr && !isIncContinueOnLastSolver()) { delete _pdrSvr; _pdrSvr = 0; } initializeSolver();

   // Initialize Bad Cube
   _pdrBad = new V3MPDRCube(0); assert (_pdrBad); _pdrBad->setState(V3NetVec(1, pId));

   // Initialize Frame 0
   if (_vrfNtk->getLatchSize()) _pdrFrame.push_back(new V3MPDRFrame(_pdrSvr->setImplyInit()));  // R0 = I0
   else _pdrFrame.push_back(new V3MPDRFrame(_pdrSvr->reserveFormula()));
   assert (_pdrFrame.back()->getActivator()); assert (_pdrFrame.size() == 1);

   // Initialize Frame INF
   if (_pdrBackup.size()) { _pdrFrame.push_back(_pdrBackup.back()); _pdrBackup.pop_back(); addFrameInfoToSolver(1); }
   else _pdrFrame.push_back(new V3MPDRFrame(_pdrSvr->reserveFormula()));
   assert (_pdrFrame.back()->getActivator()); assert (_pdrFrame.size() == 2);

   // Check Shared Invariants
   if (_sharedInv) {
      V3NetTable sharedInv; _sharedInv->getInv(sharedInv);
      for (uint32_t i = 0; i < sharedInv.size(); ++i) {
         V3MPDRCube* const inv = new V3MPDRCube(0); assert (inv);
         inv->setState(sharedInv[i]); addBlockedCube(make_pair(getPDRFrame(), inv));
      }
   }

   // Continue on the Last Depth
   while (_pdrBackup.size() && (getIncLastDepthToKeepGoing() > getPDRFrame())) {
      _pdrFrame.push_back(_pdrFrame.back());  // Keep frame INF the last frame
      _pdrFrame[_pdrFrame.size() - 2] = _pdrBackup.back(); _pdrBackup.pop_back();
      addFrameInfoToSolver(_pdrFrame.size() - 2);
   }

   // Start PDR Based Verification
   V3MPDRCube* badCube = 0;
   while (true) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Shared Results
      if (_sharedBound && (V3NtkUD == _sharedBound->getBound(p))) break;
      // Check Shared Networks
      if (_sharedNtk) {
         V3NtkHandler* const sharedNtk = _sharedNtk->getNtk(_handler);
         if (sharedNtk) {
            setIncKeepLastReachability(true); setIncContinueOnLastSolver(false); setIncLastDepthToKeepGoing(getPDRDepth());
            _handler = sharedNtk; _vrfNtk = sharedNtk->getNtk(); goto vrfRestart;
         }
      }
      // Find a Bad Cube as Initial Proof Obligation
      badCube = getInitialObligation();  // SAT(R ^ T ^ !p)
      if (!badCube) {
         if (!isIncKeepSilent() && intactON()) {
            if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
            Msg(MSG_IFO) << setw(3) << left << getPDRDepth() << " :";
            const uint32_t j = (_pdrFrame.size() > 25) ? _pdrFrame.size() - 25 : 0; if (j) Msg(MSG_IFO) << " ...";
            for (uint32_t i = j; i < _pdrFrame.size(); ++i) 
               Msg(MSG_IFO) << " " << _pdrFrame[i]->getCubeList().size();
            if (svrInfoON()) { Msg(MSG_IFO) << "  ("; _pdrSvr->printInfo(); Msg(MSG_IFO) << ")"; }
            Msg(MSG_IFO) << endl;  // Always Endline At the End of Each Frame
         }
         if (_sharedBound) _sharedBound->updateBound(p, getPDRFrame());
         // Push New Frame
         _pdrFrame.push_back(_pdrFrame.back());  // Renders F Infinity to be the last in _pdrFrame
         if (_pdrBackup.size()) {
            _pdrFrame[_pdrFrame.size() - 2] = _pdrBackup.back(); _pdrBackup.pop_back();
            addFrameInfoToSolver(_pdrFrame.size() - 2);
         }
         else _pdrFrame[_pdrFrame.size() - 2] = new V3MPDRFrame(_pdrSvr->reserveFormula());  // New Frame
         if (propagateCubes()) { proved = getPDRDepth(); break; }
         if (_maxDepth <= (getPDRFrame() - 1)) break;
      }
      else {
         badCube = recursiveBlockCube(badCube);
         if (badCube) { fired = getPDRDepth(); break; }
         // Interactively Show the Number of Bad Cubes in Frames
         if (!isIncKeepSilent() && intactON()) {
            if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
            Msg(MSG_IFO) << setw(3) << left << getPDRDepth() << " :";
            const uint32_t j = (_pdrFrame.size() > 25) ? _pdrFrame.size() - 25 : 0; if (j) Msg(MSG_IFO) << " ...";
            for (uint32_t i = j; i < _pdrFrame.size(); ++i) 
               Msg(MSG_IFO) << " " << _pdrFrame[i]->getCubeList().size();
            if (svrInfoON()) { Msg(MSG_IFO) << "  ("; _pdrSvr->printInfo(); Msg(MSG_IFO) << ")"; }
            if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
         }
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
      if (profileON()) {
         _totalStat->end();
         Msg(MSG_IFO) << *_initSvrStat << endl;
         Msg(MSG_IFO) << *_solveStat << endl;
         Msg(MSG_IFO) << *_generalStat << endl;
         Msg(MSG_IFO) << *_propagateStat << endl;
         Msg(MSG_IFO) << *_ternaryStat << endl;
         Msg(MSG_IFO) << *_totalStat << endl;
      }
   }

   // Record CounterExample Trace or Invariant
   if (V3NtkUD != fired) {  // Record Counter-Example
      // Compute PatternCount
      const V3MPDRCube* traceCube = badCube; assert (traceCube); assert (existInitial(traceCube->getState()));
      uint32_t patternCount = 0; while (_pdrBad != traceCube) { traceCube = traceCube->getNextCube(); ++patternCount; }
      V3CexTrace* const cex = new V3CexTrace(patternCount); assert (cex);
      _result[p].setCexTrace(cex); assert (_result[p].isCex());
      // Set Pattern Value
      traceCube = badCube; assert (traceCube); assert (existInitial(traceCube->getState()));
      while (_pdrBad != traceCube) {
         if (_pdrSize) cex->pushData(traceCube->getInputData());
         traceCube = traceCube->getNextCube(); assert (traceCube);
      }
      // Set Initial State Value
      if (_pdrInitValue.size()) {
         V3BitVecX initValue(_pdrInitValue.size());
         for (uint32_t i = 0; i < badCube->getState().size(); ++i) {
            assert (initValue.size() > badCube->getState()[i].id);
            if (badCube->getState()[i].cp) initValue.set0(badCube->getState()[i].id);
            else initValue.set1(badCube->getState()[i].id);
         }
         for (uint32_t i = 0; i < _pdrInitValue.size(); ++i)
            if (_pdrInitConst[i]) { if (_pdrInitValue[i]) initValue.set0(i); else initValue.set1(i); }
         cex->setInit(initValue);
      }
      // Delete Cubes on the Trace
      const V3MPDRCube* lastCube; traceCube = badCube;
      while (_pdrBad != traceCube) { lastCube = traceCube->getNextCube(); delete traceCube; traceCube = lastCube; }
      // Check Common Results
      if (isIncVerifyUsingCurResult()) checkCommonCounterexample(p, *cex);
   }
   else if (V3NtkUD != proved) {  // Record Inductive Invariant
      _result[p].setIndInv(_vrfNtk); assert (_result[p].isInv());
      // Put the Inductive Invariant to Frame INF
      uint32_t f = 1; for (; f < getPDRDepth(); ++f) if (!_pdrFrame[f]->getCubeList().size()) break;
      assert (f < getPDRDepth());
      for (uint32_t i = 1 + f; i < getPDRFrame(); ++i) {
         const V3MPDRCubeList& cubeList = _pdrFrame[i]->getCubeList(); V3MPDRCubeList::const_iterator it;
         for (it = cubeList.begin(); it != cubeList.end(); ++it) addBlockedCube(make_pair(getPDRFrame(), *it));
         _pdrFrame[i]->clearCubeList(); delete _pdrFrame[i];
      }
      // Remove Empty Frames
      _pdrFrame.back()->removeSelfSubsumed();
      _pdrFrame[f] = _pdrFrame.back(); while ((1 + f) != _pdrFrame.size()) _pdrFrame.pop_back();
      // Check Common Results
      if (isIncVerifyUsingCurResult()) {
         const V3MPDRCubeList& invCubeList = _pdrFrame.back()->getCubeList();
         V3NetTable invList; invList.clear(); invList.reserve(invCubeList.size());
         for (V3MPDRCubeList::const_iterator it = invCubeList.begin(); it != invCubeList.end(); ++it)
            invList.push_back((*it)->getState()); checkCommonProof(p, invList, false);
      }
   }
}

// PDR Initialization Functions
void
V3VrfMPDR::initializeSolver() {
   if (profileON()) _initSvrStat->start();
   const bool isNewSolver = !_pdrSvr;
   if (!_pdrSvr) { _pdrSvr = allocSolver(getSolver(), _vrfNtk); assert (_pdrSvr->totalSolves() == 0); }
   else _pdrSvr->reset(); _pdrActCount = _pdrRecycle;
   // Set Initial State to Solver
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _pdrSvr->addBoundedVerifyData(_vrfNtk->getLatch(i), 0);
   if (isNewSolver) _pdrSvr->simplify();
   if (profileON()) _initSvrStat->end();
}

void
V3VrfMPDR::addCubeToSolver(const V3NetVec& state, const uint32_t& depth) {
   assert (state.size()); assert (depth < 2); assert (_pdrSvr);
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _vrfNtk->getLatchSize());
      _pdrSvr->addBoundedVerifyData(_vrfNtk->getLatch(state[i].id), depth);
   }
}

void
V3VrfMPDR::addFrameInfoToSolver(const uint32_t& f) {
   assert (f); assert (f < _pdrFrame.size()); _pdrFrame[f]->setActivator(_pdrSvr->reserveFormula());
   const V3MPDRCubeList& cubeList = _pdrFrame[f]->getCubeList();
   V3SvrDataVec formula; formula.clear(); size_t fId;
   for (V3MPDRCubeList::const_reverse_iterator it = cubeList.rbegin(); it != cubeList.rend(); ++it) {
      addCubeToSolver((*it)->getState(), 0); formula.reserve(1 + (*it)->getState().size());
      formula.push_back(_pdrSvr->getNegFormula(_pdrFrame[f]->getActivator()));
      for (uint32_t j = 0; j < (*it)->getState().size(); ++j) {
         fId = _pdrSvr->getFormula(_vrfNtk->getLatch((*it)->getState()[j].id), 0);
         formula.push_back((*it)->getState()[j].cp ? fId : _pdrSvr->getNegFormula(fId));
      }
      _pdrSvr->assertProperty(_pdrSvr->setImplyUnion(formula)); formula.clear();
   }
}

void
V3VrfMPDR::recycleSolver() {
   // Initialize Solver
   assert (!_pdrActCount); initializeSolver();
   // Initialize Frames
   _pdrFrame[0]->setActivator(_pdrSvr->setImplyInit());
   for (uint32_t i = 1; i < _pdrFrame.size(); ++i) addFrameInfoToSolver(i);
   // Check Shared Invariants
   if (_sharedInv) {
      V3NetTable sharedInv; _sharedInv->getInv(sharedInv);
      for (uint32_t i = 0; i < sharedInv.size(); ++i) {
         V3MPDRCube* const inv = new V3MPDRCube(0); assert (inv);
         inv->setState(sharedInv[i]); addBlockedCube(make_pair(getPDRFrame(), inv));
      }
   }
   _pdrSvr->simplify();
}

// PDR Main Functions
V3MPDRCube* const
V3VrfMPDR::getInitialObligation() {  // If SAT(R ^ T ^ !p)
   assert (_pdrBad); _pdrSvr->assumeRelease();
   assumeReachability(getPDRDepth());  // Assume R
   const V3NetVec& state = _pdrBad->getState(); assert (1 == state.size());
   _pdrSvr->addBoundedVerifyData(state[0], 0);
   _pdrSvr->assumeProperty(_pdrSvr->getFormula(state[0], 0));
   if (profileON()) _solveStat->start();
   _pdrSvr->simplify();
   const bool result = _pdrSvr->assump_solve();
   if (profileON()) _solveStat->end();
   if (!result) return 0;
   V3MPDRCube* const cube = extractModel(_pdrBad, getPDRDepth());
   assert (cube); return cube;
}

V3MPDRCube* const
V3VrfMPDR::recursiveBlockCube(V3MPDRCube* const badCube) {
   // Create a Queue for Blocking Cubes
   V3BucketList<V3MPDRCube*> badQueue(getPDRFrame());
   assert (badCube); badQueue.add(getPDRDepth(), badCube);
   // Block Cubes from the Queue
   V3MPDRTimedCube baseCube, generalizedCube;
   while (badQueue.pop(baseCube.first, baseCube.second)) {
      assert (baseCube.first < getPDRFrame());
      if (!baseCube.first) {
         // Clear All Cubes in badQueue before Return
         V3Set<const V3MPDRCube*>::Set traceCubes; traceCubes.clear();
         const V3MPDRCube* traceCube = baseCube.second;
         while (true) {
            traceCubes.insert(traceCube); if (_pdrBad == traceCube) break;
            traceCube = traceCube->getNextCube();
         }
         while (badQueue.pop(generalizedCube.first, generalizedCube.second)) {
            if (traceCubes.end() == traceCubes.find(generalizedCube.second)) delete generalizedCube.second;
         }
         return baseCube.second;  // A Cube cannot be blocked by R0 --> Cex
      }
      if (!isBlocked(baseCube)) {
         assert (!existInitial(baseCube.second->getState()));
         // Check Reachability : SAT (R ^ ~cube ^ T ^ cube')
         if (checkReachability(baseCube.first, baseCube.second->getState())) {  // SAT, Not Blocked Yet
            if (profileON()) _ternaryStat->start();
            generalizedCube.second = extractModel(baseCube.second, baseCube.first - 1);
            if (profileON()) _ternaryStat->end();
            badQueue.add(baseCube.first - 1, generalizedCube.second);  // This Cube should be blocked in previous frame
            badQueue.add(baseCube.first, baseCube.second);  // This Cube has not yet been blocked (postpone to future)
         }
         else {  // UNSAT, Blocked
            bool satGen = true;
            while (true) {
               if (profileON()) _generalStat->start();
               generalizedCube.first = baseCube.first;
               generalizedCube.second = new V3MPDRCube(*(baseCube.second));
               generalization(generalizedCube);  // Generalization
               if (profileON()) _generalStat->end();
               addBlockedCube(generalizedCube);  // Record this Cube that is bad and to be blocked
               // I found Niklas Een has modified the original IWLS paper and alter the line below
               // However, I suggest the alteration restricts PDR (i.e. fold-free) and does not help
               if (satGen && (baseCube.first < getPDRDepth()) && (generalizedCube.first < getPDRFrame()))
                  badQueue.add(baseCube.first + 1, baseCube.second);
               //if ((baseCube.first < getPDRDepth()) && (generalizedCube.first < getPDRDepth()))
               //   badQueue.add(generalizedCube.first + 1, baseCube.second);
               if (!isForwardSATGen() || getPDRDepth() <= generalizedCube.first) break;
               baseCube.second = forwardModel(generalizedCube.second); if (!baseCube.second) break;
               baseCube.first = baseCube.first + 1; satGen = false;
               if (checkReachability(baseCube.first, baseCube.second->getState())) break;
            }
         }
      }
      //else delete baseCube.second;
   }
   return 0;
}

#ifdef V3_MPDR_USE_PROPAGATE_LOW_COST
const bool
V3VrfMPDR::propagateCubes() {
   if (profileON()) _propagateStat->start();
   // Check Each Frame if some Cubes can be Further Propagated
   for (uint32_t i = 1; i < getPDRDepth(); ++i) {
      const V3MPDRCubeList& cubeList = _pdrFrame[i]->getCubeList();
#ifdef V3_MPDR_USE_PROPAGATE_BACKWARD
      // Backward Version  (Check from rbegin() to rend())
      uint32_t candidates = 1; V3MPDRCubeList::const_iterator it;
      while (candidates <= cubeList.size()) {
         it = cubeList.begin(); for (uint32_t j = candidates; j < cubeList.size(); ++j) ++it;
#else
      // Forward Version  (Check from begin() to end())
      V3MPDRCubeList::const_iterator it = cubeList.begin();
      while (it != cubeList.end()) {
#endif
         // Check if this cube can be pushed forward (closer to All Frame)
         if (!checkReachability(i + 1, (*it)->getState(), false)) {
            V3MPDRTimedCube cube = make_pair(i + 1, new V3MPDRCube(*(*it)));
            // Remove Cubes in the Next Frame that can be Subsumed by the cube
            removeFromProof(cube); _pdrFrame[i + 1]->removeSubsumed(cube.second);
            // Remove cubes in this frame that can be subsumed by the cube
            _pdrFrame[i]->removeSubsumed(cube.second, ++it);
            // Block this cube again at higher frames
            addBlockedCube(cube);
         }
         else {
            // Remove cubes in this frame that can be subsumed by the cube
            _pdrFrame[i]->removeSubsumed(*it, it);
#ifdef V3_MPDR_USE_PROPAGATE_BACKWARD
            // Backward Version  (Check from rbegin() to rend())
            ++candidates;
#else
            // Forward Version  (Check from begin() to end())
            ++it;
#endif
         }
      }
      // Check if Any Remaining Cubes in this Frame can be Subsumed
      _pdrFrame[i]->removeSelfSubsumed();
      /*
      // Debug : Check Real Containment
      for (uint32_t j = i + 1; j < _pdrFrame.size(); ++j) {
         it = _pdrFrame[j]->getCubeList().begin();
         while (it != _pdrFrame[j]->getCubeList().end()) {
            assert (!checkReachability(i, (*it)->getState()));
            ++it;
         }
      }
      */
      // Check Inductive Invariant
      if (!_pdrFrame[i]->getCubeList().size()) {
         if (profileON()) _propagateStat->end();
         return true;
      }
   }
   // Check if Any Remaining Cubes in the Latest Frame can be Subsumed
   _pdrFrame[getPDRDepth()]->removeSelfSubsumed(); //_pdrFrame[getPDRFrame()]->removeSelfSubsumed();
   /*
   // Debug : Check Real Containment
   for (uint32_t j = getPDRDepth() + 1; j < _pdrFrame.size(); ++j) {
      V3MPDRCubeList::const_iterator it = _pdrFrame[j]->getCubeList().begin();
      while (it != _pdrFrame[j]->getCubeList().end()) {
         assert (!checkReachability(getPDRDepth(), (*it)->getState()));
         ++it;
      }
   }
   */
   if (profileON()) _propagateStat->end();
   return false;
}
#else
const bool
V3VrfMPDR::propagateCubes() {
   if (profileON()) _propagateStat->start();
   // Check Each Frame if some Cubes can be Further Propagated
   for (uint32_t i = 1; i < getPDRDepth(); ++i) {
      const V3MPDRCubeList& cubeList = _pdrFrame[i]->getCubeList();
#ifdef V3_MPDR_USE_PROPAGATE_BACKWARD
      // Backward Version  (Check from rbegin() to rend())
      uint32_t candidates = 1; V3MPDRCubeList::const_iterator it;
      while (candidates <= cubeList.size()) {
         it = cubeList.begin(); for (uint32_t j = candidates; j < cubeList.size(); ++j) ++it;
#else
      // Forward Version  (Check from begin() to end())
      V3MPDRCubeList::const_iterator it = cubeList.begin();
      while (it != cubeList.end()) {
#endif
         // Check if this cube can be pushed forward (closer to All Frame)
         if (!checkReachability(i + 1, (*it)->getState(), false)) {
            V3MPDRTimedCube cube = make_pair(i + 1, new V3MPDRCube(*(*it)));
            // Block this cube again at higher frames
            removeFromProof(cube); addBlockedCube(cube);
            // Remove blocked cubes in lower frames that can be subsumed by the cube
            for (uint32_t j = 1; j < i; ++j) _pdrFrame[j]->removeSubsumed(cube.second);
            // Remove Cubes in this Frame that can be Subsumed by the cube
            _pdrFrame[i]->removeSubsumed(cube.second, ++it);
            // NOTE: Need not to recover iterator after subsumption (set.erase)
            //       the iterator it will point to the next candidate
         }
         else {
            // Remove blocked cubes in lower frames that can be subsumed by the cube
            for (uint32_t j = 1; j < i; ++j) _pdrFrame[j]->removeSubsumed(*it);
#ifdef V3_MPDR_USE_PROPAGATE_BACKWARD
            // Backward Version  (Check from rbegin() to rend())
            ++candidates;
#else
            // Forward Version  (Check from begin() to end())
            ++it;
#endif
         }
      }
      // Check if Any Remaining Cubes in this Frame can be Subsumed
      _pdrFrame[i]->removeSelfSubsumed();
      // Check Inductive Invariant
      for (uint32_t j = 1; j <= i; ++j) {
         if (!_pdrFrame[j]->getCubeList().size()) {
            if (profileON()) _propagateStat->end();
            return true;
         }
      }
   }
   // Check if Any Remaining Cubes in these Frames can be Subsumed
   _pdrFrame[getPDRDepth()]->removeSelfSubsumed(); _pdrFrame[getPDRFrame()]->removeSelfSubsumed();
   if (profileON()) _propagateStat->end();
   return false;
}
#endif

// PDR Auxiliary Functions
const bool
V3VrfMPDR::checkReachability(const uint32_t& frame, const V3NetVec& cubeState, const bool& extend) {
   assert (frame > 0); assert (frame <= getPDRFrame());
   // Check if Recycle is Triggered
   if (!_pdrActCount) recycleSolver();
   // Assume R
   _pdrSvr->assumeRelease(); assumeReachability((frame == getPDRFrame()) ? frame : frame - 1);
   // Assume cube'
   addCubeToSolver(cubeState, 1);
   for (uint32_t i = 0; i < cubeState.size(); ++i)
      _pdrSvr->assumeProperty(_pdrSvr->getFormula(_vrfNtk->getLatch(cubeState[i].id), 1), cubeState[i].cp);
   if (extend) {
      // Assume ~cube
      addCubeToSolver(cubeState, 0);
      V3SvrDataVec blockCube; blockCube.clear(); blockCube.reserve(cubeState.size()); size_t fId;
      for (uint32_t i = 0; i < cubeState.size(); ++i) {
         fId = _pdrSvr->getFormula(_vrfNtk->getLatch(cubeState[i].id), 0);
         blockCube.push_back(cubeState[i].cp ? fId : _pdrSvr->getNegFormula(fId));
      }
      _pdrSvrData = _pdrSvr->setImplyUnion(blockCube);
      assert (_pdrSvrData); _pdrSvr->assumeProperty(_pdrSvrData);
      // Check Reachability by SAT Calling
      if (profileON()) _solveStat->start();
      _pdrSvr->simplify();
      const bool result = _pdrSvr->assump_solve();
      if (profileON()) _solveStat->end();
      _pdrSvr->assertProperty(_pdrSvr->getNegFormula(_pdrSvrData));  // Invalidate ~cube in future solving
      --_pdrActCount; return result;
   }
   else {
      if (profileON()) _solveStat->start();
      _pdrSvr->simplify();
      const bool result = _pdrSvr->assump_solve();
      if (profileON()) _solveStat->end();
      return result;
   }
}

const bool
V3VrfMPDR::isBlocked(const V3MPDRTimedCube& timedCube) {
   // Check if cube has already been blocked by R (at specified frame)
   // Perform Subsumption Check : cube implies some C in R (at specified frame)
   for (uint32_t i = timedCube.first; i < _pdrFrame.size(); ++i) 
      if (_pdrFrame[i]->subsumes(timedCube.second)) return true;
   ///*
   // Check by SAT
   _pdrSvr->assumeRelease(); assumeReachability(timedCube.first);
   const V3NetVec& state = timedCube.second->getState(); addCubeToSolver(state, 0);
   for (uint32_t i = 0; i < state.size(); ++i)
      _pdrSvr->assumeProperty(_pdrSvr->getFormula(_vrfNtk->getLatch(state[i].id), 0), state[i].cp);
   if (profileON()) _solveStat->start();
   const bool result = _pdrSvr->assump_solve();
   if (profileON()) _solveStat->end();
   return !result;
   //*/
   //return false;
}

const bool
V3VrfMPDR::existInitial(const V3NetVec& state) {
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _pdrInitConst.size());
      assert (state[i].id < _pdrInitValue.size());
      if (_pdrInitConst[state[i].id] && (_pdrInitValue[state[i].id] ^ state[i].cp)) return false;
   }
   return true;
}

void
V3VrfMPDR::assumeReachability(const unsigned& k) {
   uint32_t i = _pdrFrame.size();
   while (i-- > k) _pdrSvr->assumeProperty(_pdrFrame[i]->getActivator());
}

void
V3VrfMPDR::resolveInitial(V3MPDRCube* const cube, const V3SvrDataSet& coreProofVars) {
   // Get Proof Related State Variables
   assert (cube); //if (coreProofVars.size() == cube->getState().size()) return;
   // Store Values of State Variable
   const V3NetVec& state = cube->getState();
   // Remove Variables to Form New State
   assert (!existInitial(state));
   bool conflictInitial = false;
   V3NetId conflictId = V3NetUD;
   uint32_t pos = 0;
   V3NetVec newState; newState.reserve(state.size());
   for (uint32_t i = 0; i < state.size(); ++i) {
      if (coreProofVars.end() != coreProofVars.find(_pdrSvr->getFormula(_vrfNtk->getLatch(state[i].id), 1))) {
         newState.push_back(state[i]);
         if (!conflictInitial && (_pdrInitConst[state[i].id] && (_pdrInitValue[state[i].id] ^ state[i].cp))) {
            assert (!existInitial(newState)); conflictInitial = true;
         }
      }
      else if (!conflictInitial && V3NetUD == conflictId) {
         if (_pdrInitConst[state[i].id] && (_pdrInitValue[state[i].id] ^ state[i].cp)) {
            conflictId = state[i]; assert (V3NetUD != conflictId); pos = newState.size();
         }
      }
   }
   // Resolve Intersection with Initial State
   if (!conflictInitial) { assert (V3NetUD != conflictId); newState.insert(newState.begin() + pos, conflictId); }
   if (newState.size() < state.size()) cube->setState(newState);
}

V3MPDRCube* const
V3VrfMPDR::extractModel(const V3MPDRCube* const nextCube, const uint32_t& depth) {
   // This function can ONLY be called after SAT of (R ^ T ^ nextCube') and generalize curCube from R
   V3MPDRCube* const cube = new V3MPDRCube(nextCube);  // Create Cube
   generalizeSimulation(cube, nextCube, depth);  // Apply Simulation for the witness
   // Record Input to Proof Obligation for Trace Logging
   if (_pdrSize) recordCubeInputForTraceLog(cube); return cube;
}

V3MPDRCube* const
V3VrfMPDR::forwardModel(const V3MPDRCube* const curCube) {
   assert (curCube); if (!curCube->getNextCube()) return 0;
   // Set Pattern Values for Simulator
   if (_pdrSize) {
      const V3BitVecX& inputData = curCube->getInputData();
      assert (inputData.size() == _pdrSize); uint32_t j = 0;
      for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i, ++j)
         _pdrSim->setSource(_vrfNtk->getInput(i), inputData.bv_slice(j, j));
      for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i, ++j)
         _pdrSim->setSource(_vrfNtk->getInout(i), inputData.bv_slice(j, j));
      assert (j == inputData.size());
   }
   // Set State Variable Values for Simulator
   const V3NetVec& state = curCube->getState(); _pdrSim->setSourceFree(V3_FF, false);
   V3BitVecX value0(1), value1(1); value0.set0(0); value1.set1(0);
   for (uint32_t i = 0; i < state.size(); ++i)
      _pdrSim->setSource(_vrfNtk->getLatch(state[i].id), (state[i].cp ? value0 : value1));
   // Simulate for One Cycle
   _pdrSim->simulate(); _pdrSim->updateNextStateValue();
   // Return the Cube that it Reached
   V3NetVec nextState; nextState.clear(); nextState.reserve(_vrfNtk->getLatchSize());
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
      switch (_pdrSim->getSimValue(_vrfNtk->getLatch(i))[0]) {
         case '0' : nextState.push_back(V3NetId::makeNetId(i, 1)); break;
         case '1' : nextState.push_back(V3NetId::makeNetId(i, 0)); break;
      }
   }
   if (existInitial(nextState)) return 0;
   V3MPDRCube* const cube = new V3MPDRCube(0); assert (cube);
   cube->setState(nextState); return cube;
}

void
V3VrfMPDR::generalization(V3MPDRTimedCube& generalizedCube) {
   removeFromProof(generalizedCube, false);
   generalizeProof(generalizedCube);
   forwardProof(generalizedCube);
}

void
V3VrfMPDR::addBlockedCube(const V3MPDRTimedCube& cube) {
   assert (cube.first <= getPDRFrame()); assert (cube.second->getState().size());
   // Push cube into corresponding frame that it should be blocked
   if (!_pdrFrame[cube.first]->pushCube(cube.second)) return;
   // Renders this cube to be blocked as frame activator is asserted
   const V3NetVec& state = cube.second->getState(); addCubeToSolver(state, 0);
   V3SvrDataVec formula; formula.clear(); formula.reserve(1 + state.size()); size_t fId;
   formula.push_back(_pdrSvr->getNegFormula(_pdrFrame[cube.first]->getActivator()));
   for (uint32_t i = 0; i < state.size(); ++i) {
      fId = _pdrSvr->getFormula(_vrfNtk->getLatch(state[i].id), 0);
      formula.push_back(state[i].cp ? fId : _pdrSvr->getNegFormula(fId));
      ++_pdrPriority[state[i].id];  // Increase Signal Priority
   }
   _pdrSvr->assertProperty(_pdrSvr->setImplyUnion(formula));
   // Share Invariants
   if (_sharedInv && (cube.first == getPDRFrame())) _sharedInv->updateInv(state);
}

void
V3VrfMPDR::recordCubeInputForTraceLog(V3MPDRCube* const& cube) {
   assert (cube); assert (_pdrSim);
   V3BitVecX value(_pdrSize), tempValue;
   uint32_t j = 0;
   for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i, ++j) {
      tempValue = _pdrSim->getSimValue(_vrfNtk->getInput(i));
      if ('1' == tempValue[0]) value.set1(j);
      else if ('0' == tempValue[0]) value.set0(j);
   }
   for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i, ++j) {
      tempValue = _pdrSim->getSimValue(_vrfNtk->getInout(i));
      if ('1' == tempValue[0]) value.set1(j);
      else if ('0' == tempValue[0]) value.set0(j);
   }
   assert (j == value.size()); cube->setInputData(value);
}

// PDR Generalization Functions
void
V3VrfMPDR::generalizeSimulation(V3MPDRCube* const cube, const V3MPDRCube* const nextCube, const uint32_t& depth) {
   assert (cube); assert (nextCube); assert (nextCube == cube->getNextCube()); assert (_pdrSim);
   // Set Values for Simulator
   for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i) {
      if (!_pdrSvr->existVerifyData(_vrfNtk->getInput(i), 0)) _pdrSim->clearSource(_vrfNtk->getInput(i), true);
      else _pdrSim->setSource(_vrfNtk->getInput(i), _pdrSvr->getDataValue(_vrfNtk->getInput(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i) {
      if (!_pdrSvr->existVerifyData(_vrfNtk->getInout(i), 0)) _pdrSim->clearSource(_vrfNtk->getInout(i), true);
      else _pdrSim->setSource(_vrfNtk->getInout(i), _pdrSvr->getDataValue(_vrfNtk->getInout(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
      if (!_pdrSvr->existVerifyData(_vrfNtk->getLatch(i), 0)) _pdrSim->clearSource(_vrfNtk->getLatch(i), true);
      else _pdrSim->setSource(_vrfNtk->getLatch(i), _pdrSvr->getDataValue(_vrfNtk->getLatch(i), 0));
   }
   _pdrSim->simulate();

   // Perform SAT Generalization
   if (_pdrBad != nextCube) _pdrGen->setTargetNets(V3NetVec(), nextCube->getState());
   else {
      V3NetVec constrCube(1, nextCube->getState()[0]);
      assert (1 == nextCube->getState().size()); _pdrGen->setTargetNets(constrCube);
   }
   // Set Priority
   V3UI32Vec prioNets; prioNets.clear(); prioNets.reserve(_pdrPriority.size());
   for (uint32_t i = 0; i < _pdrPriority.size(); ++i) if (!_pdrPriority[i]) prioNets.push_back(i);
   for (uint32_t i = 0; i < _pdrPriority.size(); ++i) if ( _pdrPriority[i]) prioNets.push_back(i);
   _pdrGen->performSetXForNotCOIVars(); _pdrGen->performXPropForExtensibleVars(prioNets);
   cube->setState(_pdrGen->getGeneralizationResult());
}

const bool
V3VrfMPDR::removeFromProof(V3MPDRTimedCube& timedCube, const bool& pushForward) {
   // This function can ONLY be called after UNSAT of (R ^ T ^ cube')
   // Generate UNSAT Source from Solver if Possible
   V3SvrDataVec coreProofVars; coreProofVars.clear(); _pdrSvr->getDataConflict(coreProofVars);
   if (!coreProofVars.size()) return false;  // Solver does not Support Analyze Conflict
   V3SvrDataSet coreProofVarSet; coreProofVarSet.clear();
   for (uint32_t i = 0; i < coreProofVars.size(); ++i) coreProofVarSet.insert(coreProofVars[i]);
   const bool isSvrDataInvolved = coreProofVarSet.end() != coreProofVarSet.find(_pdrSvrData);
   // Get Generalized State from Solver Proof
   assert (!existInitial(timedCube.second->getState()));
   resolveInitial(timedCube.second, coreProofVarSet);
   assert (!existInitial(timedCube.second->getState()));
   if (!pushForward) return isSvrDataInvolved;
   // Return the Smallest Activity Index Used
   uint32_t i = timedCube.first;
   for (; i < getPDRFrame(); ++i) 
      if (coreProofVarSet.find(_pdrFrame[i - 1]->getActivator()) != coreProofVarSet.end()) { timedCube.first = i; break; }
   if (i == getPDRFrame()) {
      if (coreProofVarSet.find(_pdrFrame[getPDRDepth()]->getActivator()) == coreProofVarSet.end())
         timedCube.first = getPDRFrame();
      else timedCube.first = getPDRDepth();
   }
   assert (!checkReachability(timedCube.first, timedCube.second->getState())); return isSvrDataInvolved;
}

void
V3VrfMPDR::generalizeProof(V3MPDRTimedCube& timedCube) {
   // Apply SAT Solving to further generalize cube
   // Remove Variables from cube after Proof Success
   V3MPDRCube* const& cube = timedCube.second; assert (!existInitial(cube->getState()));
   V3NetVec state(cube->getState()); V3NetId id;
   // Sort Priority of Signals
   V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map priorityMap; priorityMap.clear();
   V3Map<uint32_t, uint32_t>::Map::iterator it, is;
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _pdrPriority.size());
      priorityMap.insert(make_pair(_pdrPriority[state[i].id], i));
   }
   for (it = priorityMap.begin(); it != priorityMap.end(); ++it) {
      // Try Removing A State Variable on this Cube
      id = state[it->second]; assert (state.size() >= (1 + it->second));
      if (state.size() != (1 + it->second)) state[it->second] = state.back(); state.pop_back();
      if (existInitial(state) || checkReachability(timedCube.first, state)) {
         if (state.size() == it->second) state.push_back(id);
         else { state.push_back(state[it->second]); state[it->second] = id; }
         assert (state.size() >= (1 + it->second));
      }
      else {
         if (state.size() > (1 + it->second)) {
            id = state[it->second];
            for (uint32_t i = it->second; i < (state.size() - 1); ++i) state[i] = state[1 + i];
            state.back() = id;
         }
         for (is = it, ++is; is != priorityMap.end(); ++is) {
            assert (is->second != it->second); if (is->second > it->second) --(is->second);
         }
      }
      assert (!existInitial(state)); assert (!checkReachability(timedCube.first, state));
   }
   if (state.size() < cube->getState().size()) cube->setState(state);
   assert (!existInitial(timedCube.second->getState()));
   assert (!checkReachability(timedCube.first, timedCube.second->getState()));
}

void
V3VrfMPDR::forwardProof(V3MPDRTimedCube& timedCube) {
   // Try Pushing the cube to higher frames if possible
   assert (getPDRDepth());  // R0 can never call this function
   bool unsatGen = false;
   while (timedCube.first < getPDRDepth()) {
      if (!checkReachability(++timedCube.first, timedCube.second->getState())) unsatGen = removeFromProof(timedCube);
      else {
         if (isForwardUNSATGen() && unsatGen && generalizeCex(timedCube)) { unsatGen = false; continue; }
         --timedCube.first; break;
      }
      assert (!existInitial(timedCube.second->getState()));
      assert (!(checkReachability(timedCube.first, timedCube.second->getState())));
   }
}

const bool
V3VrfMPDR::generalizeCex(V3MPDRTimedCube& timedCube) {
   // Try Turning the Reachability Check from SAT to UNSAT
   V3MPDRCube* cube = new V3MPDRCube(*(timedCube.second)); assert (cube);
   assert (!existInitial(cube->getState()));
   while (true) {
      // Get the SAT Model
      if (!checkReachability(timedCube.first, cube->getState())) break;
      V3MPDRCube* const prevCube = new V3MPDRCube(cube);  // Create Cube
      generalizeSimulation(prevCube, cube, timedCube.first - 1);  // Apply Simulation for the witness
      // Remove Variables of cube with Opposite Polarity in prevCube
      const V3NetVec cubeState = prevCube->getState(); delete prevCube;
      V3NetVec state; state.clear(); state.reserve(cube->getState().size());
      uint32_t j = 0, k = 0, cubeSize = cube->getState().size();
      while (j < cubeState.size() && k < cube->getState().size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (cube->getState()[k].id > cube->getState()[k - 1].id));
         if (cubeState[j].id > cube->getState()[k].id) { state.push_back(cube->getState()[k]); ++k; }
         else if (cubeState[j].id < cube->getState()[k].id) ++j;
         else { if (cubeState[j].cp == cube->getState()[k].cp) state.push_back(cube->getState()[k]); ++j; ++k; }
      }
      for (; k < cube->getState().size(); ++k) state.push_back(cube->getState()[k]);
      if (cubeSize == state.size() || existInitial(state)) { delete cube; return false; }
      // Update Cube for the Next Iteration
      assert (cubeSize > state.size()); cube->setState(state);
   }
   assert (cube); timedCube.second->setState(cube->getState());
   delete cube; return true;
}

// PDR Helper Functions
const bool
V3VrfMPDR::reportUnsupportedInitialState() {
   _pdrInitConst.clear(); _pdrInitConst.reserve(_vrfNtk->getLatchSize());
   _pdrInitValue.clear(); _pdrInitValue.reserve(_vrfNtk->getLatchSize());
   // Currently Not Support Non-Constant Initial State
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(_vrfNtk);
   V3NetId id; V3BitVecX value; bool ok = true;
   if (bvNtk) {
      for (uint32_t i = 0; i < bvNtk->getLatchSize(); ++i) {
         id = bvNtk->getInputNetId(bvNtk->getLatch(i), 1);
         if (BV_CONST == bvNtk->getGateType(id)) {
            value = bvNtk->getInputConstValue(id);
            _pdrInitConst.push_back('X' != value[0]);
            _pdrInitValue.push_back('0' == value[0]);
         }
         else if (bvNtk->getLatch(i) == id) {
            _pdrInitConst.push_back(0);
            _pdrInitValue.push_back(0);
         }
         else {
            Msg(MSG_WAR) << "DFF " << i << " : " << _handler->getNetName(bvNtk->getLatch(i))
                         << " has Non-Constant Initial Value !!" << endl; ok = false; }
      }
   }
   else {
      const V3AigNtk* const aigNtk = _vrfNtk; assert (aigNtk);
      for (uint32_t i = 0; i < aigNtk->getLatchSize(); ++i) {
         id = aigNtk->getInputNetId(aigNtk->getLatch(i), 1);
         if (AIG_FALSE == aigNtk->getGateType(id)) {
            _pdrInitConst.push_back(1);
            _pdrInitValue.push_back(!id.cp);
         }
         else if (aigNtk->getLatch(i) == id) {
            _pdrInitConst.push_back(0);
            _pdrInitValue.push_back(0);
         }
         else {
            Msg(MSG_WAR) << "DFF " << i << " : " << _handler->getNetName(aigNtk->getLatch(i))
                         << " has Non-Constant Initial Value !!" << endl; ok = false; }
      }
   }
   assert (_pdrInitConst.size() == _pdrInitValue.size());
   return ok;
}

// PDR Debug Functions
void
V3VrfMPDR::printState(const V3NetVec& state) const {
   for (uint32_t i = 0; i < state.size(); ++i)
      Msg(MSG_IFO) << (state[i].cp ? "~" : "") << state[i].id << " ";
}

#endif

