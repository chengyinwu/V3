/****************************************************************************
  FileName     [ v3VrfIPDR.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Incremental Property Directed Reachability on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_IPDR_C
#define V3_VRF_IPDR_C

#include "v3Msg.h"
#include "v3Bucket.h"
#include "v3VrfIPDR.h"

#include <iomanip>

//#define V3_IPDR_USE_PROPAGATE_BACKWARD
#define V3_IPDR_USE_PROPAGATE_LOW_COST

/* -------------------------------------------------- *\
 * Class V3IPDRFrame Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3IPDRFrame::V3IPDRFrame() {
   _cubeList.clear();
}

V3IPDRFrame::~V3IPDRFrame() {
   for (V3IPDRCubeList::iterator it = _cubeList.begin(); it != _cubeList.end(); ++it) delete *it;
   _cubeList.clear();
}

// Retrieval Functions
const bool
V3IPDRFrame::pushCube(V3IPDRCube* const c) {
   const uint32_t cubeSize = _cubeList.size(); _cubeList.insert(c);
   return cubeSize != _cubeList.size();
}

// Cube Containment Functions
const bool
V3IPDRFrame::subsumes(const V3IPDRCube* const cube) const {
   // This function checks whether cube is subsumed by any cube in this frame.
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = ~(cube->getSignature());
   V3IPDRCubeList::const_reverse_iterator it = _cubeList.rbegin();
   for (; it != _cubeList.rend(); ++it) {
      const V3NetVec& state = (*it)->getState(); assert (state.size());
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
V3IPDRFrame::removeSubsumed(const V3IPDRCube* const cube) {
   // This function checks whether there's any existing cube in this frame subsumed by cube.
   // If such cube is found, remove it from _cubeList
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3IPDRCubeList::iterator it = _cubeList.begin();
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
V3IPDRFrame::removeSubsumed(const V3IPDRCube* const cube, const V3IPDRCubeList::const_iterator& ix) {
   // This function checks whether there's any existing cube in this frame subsumed by cube.
   // If such cube is found, remove it from _cubeList
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3IPDRCubeList::iterator it = _cubeList.begin();
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
V3IPDRFrame::removeSelfSubsumed() {
   // This function checks whether some cubes in this frame can be subsumed by the other cube
   // Remove all cubes that are subsumed by the cube in this frame from _cubeList
   V3IPDRCubeList::const_iterator ix;
   uint32_t candidates = 1;
   while (candidates <= _cubeList.size()) {
      ix = _cubeList.begin(); for (uint32_t i = candidates; i < _cubeList.size(); ++i) ++ix;
      removeSubsumed(*ix, ix); ++candidates;
   }
}

/* -------------------------------------------------- *\
 * Class V3VrfIPDR Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfIPDR::V3VrfIPDR(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   // Private Data Members
   _pdrFrame.clear(); _pdrBad = 0; _pdrSize = 0; _pdrAttr = 0;
   // Recycle Members
   _pdrBackup.clear(); _pdrActBackup.clear(); _pdrSvrBackup.clear(); _pdrRecycle = V3NtkUD; _pdrActCount.clear();
   // Private Engines
   _pdrSvr.clear(); _pdrSim = 0; _pdrGen = 0;
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

V3VrfIPDR::~V3VrfIPDR() {
   // Private Data Members
   for (uint32_t i = 0; i < _pdrFrame.size(); ++i) delete _pdrFrame[i]; _pdrFrame.clear();
   if (_pdrBad) delete _pdrBad; _pdrBad = 0;
   // Recycle Members
   for (uint32_t i = 0; i < _pdrBackup.size(); ++i) delete _pdrBackup[i]; _pdrBackup.clear();
   for (uint32_t i = 0; i < _pdrSvrBackup.size(); ++i) delete _pdrSvrBackup[i];
   _pdrSvrBackup.clear(); _pdrActBackup.clear(); _pdrActCount.clear();
   // Private Engines
   for (uint32_t i = 0; i < _pdrSvr.size(); ++i) delete _pdrSvr[i];
   _pdrSvr.clear(); if (_pdrGen) delete _pdrGen; _pdrGen = 0;
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
isIncContinueOnLastSolver(): Valid only if isIncKeepLastReachability() is true.
\* ---------------------------------------------------------------------------------------------------- */
void
V3VrfIPDR::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p); if (profileON()) _totalStat->start();
   
   // Consistency Check
   consistencyCheck(); assert (!_constr.size());
   if (!reportUnsupportedInitialState()) return;
   
   // Initialize Backup Data
   for (uint32_t i = 0; i < _pdrBackup.size(); ++i) delete _pdrBackup[i];
   for (uint32_t i = 0; i < _pdrSvrBackup.size(); ++i) delete _pdrSvrBackup[i];
   _pdrBackup.clear(); _pdrSvrBackup.clear(); _pdrActBackup.clear();
   if (_pdrFrame.size()) {
      if (isIncKeepLastReachability()) {
         // Backup frames in the order: ..., 2, 1
         _pdrBackup.reserve(_pdrFrame.size() - 1); delete _pdrFrame[0];
         for (uint32_t i = _pdrFrame.size() - 1; i > 0; --i) _pdrBackup.push_back(_pdrFrame[i]);
      }
      else { for (uint32_t i = 0; i < _pdrFrame.size(); ++i) delete _pdrFrame[i]; }
      // Backup solvers
      if (isIncKeepLastReachability() && isIncContinueOnLastSolver()) {
         uint32_t i = _pdrSvr.size(); assert (_pdrSvr.size() == _pdrActCount.size());
         _pdrSvrBackup.reserve(i); _pdrActBackup.reserve(i);
         while (i--) { _pdrSvrBackup.push_back(_pdrSvr[i]); _pdrActBackup.push_back(_pdrActCount[i]); }
      }
      else { for (uint32_t i = 0; i < _pdrSvr.size(); ++i) delete _pdrSvr[i]; }
      _pdrFrame.clear(); _pdrActCount.clear(); _pdrSvr.clear();
   }

   // Initialize Other Members
   if (!isIncKeepLastReachability()) _pdrPriority.clear();
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
   if (_pdrPriority.size() != _vrfNtk->getLatchSize()) _pdrPriority.resize(_vrfNtk->getLatchSize());
   
   // Initialize Bad Cube
   _pdrBad = new V3IPDRCube(0); assert (_pdrBad); _pdrBad->setState(V3NetVec(1, pId));

   // Initialize Frame 0
   _pdrFrame.push_back(new V3IPDRFrame()); assert (_pdrFrame.size() == 1);
   
   // Initialize Solver 0
   if (_pdrSvrBackup.size()) {
      _pdrSvr.push_back(_pdrSvrBackup.back()); _pdrSvrBackup.pop_back();
      _pdrActCount.push_back(_pdrActBackup.back()); _pdrActBackup.pop_back();
   }
   initializeSolver(0, isIncContinueOnLastSolver());
   assert (_pdrSvr.size() == 1); assert (_pdrSvr.back());
   if (_vrfNtk->getLatchSize()) _pdrSvr.back()->assertInit();  // R0 = I0

   // Continue on the Last Depth
   while (_pdrBackup.size() && (getIncLastDepthToKeepGoing() > getPDRFrame())) {
      _pdrFrame.push_back(_pdrBackup.back()); _pdrBackup.pop_back();
      if (_pdrSvrBackup.size()) {
         _pdrSvr.push_back(_pdrSvrBackup.back()); _pdrSvrBackup.pop_back();
         _pdrActCount.push_back(_pdrActBackup.back()); _pdrActBackup.pop_back();
         initializeSolver(getPDRDepth(), true);
      }
      else { initializeSolver(getPDRDepth()); addLastFrameInfoToSolvers(); }
      // Set p to the Last Frame
      _pdrSvr[_pdrSvr.size() - 2]->assertProperty(pId, true, 0);
   }

   // Start PDR Based Verification
   V3IPDRCube* badCube = 0;
   while (true) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
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
            Msg(MSG_IFO) << endl;  // Always Endline At the End of Each Frame
         }
         if (_sharedBound) _sharedBound->updateBound(p, _pdrFrame.size());
         // Set p to the Last Frame
         _pdrSvr.back()->assertProperty(pId, true, 0);
         // Push New Frame
         if (!_pdrBackup.size()) _pdrFrame.push_back(new V3IPDRFrame());
         else { _pdrFrame.push_back(_pdrBackup.back()); _pdrBackup.pop_back(); }
         if (_pdrSvrBackup.size()) {
            _pdrSvr.push_back(_pdrSvrBackup.back()); _pdrSvrBackup.pop_back();
            _pdrActCount.push_back(_pdrActBackup.back()); _pdrActBackup.pop_back();
            initializeSolver(getPDRDepth(), true);
         }
         else { initializeSolver(getPDRDepth()); addLastFrameInfoToSolvers(); }
         assert (_pdrSvr.back()); assert (_pdrSvr.size() == _pdrFrame.size());
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
      const V3IPDRCube* traceCube = badCube; assert (traceCube); assert (existInitial(traceCube->getState()));
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
      const V3IPDRCube* lastCube; traceCube = badCube;
      while (_pdrBad != traceCube) { lastCube = traceCube->getNextCube(); delete traceCube; traceCube = lastCube; }
      // Check Common Results
      if (isIncVerifyUsingCurResult()) checkCommonCounterexample(p, *cex);
   }
   else if (V3NtkUD != proved) {  // Record Inductive Invariant
      _result[p].setIndInv(_vrfNtk); assert (_result[p].isInv());
      // Put the Inductive Invariant to the Last Frame
      uint32_t f = 1; for (; f < getPDRDepth(); ++f) if (!_pdrFrame[f]->getCubeList().size()) break;
      assert (f < getPDRDepth());
      for (uint32_t i = 1 + f; i < getPDRDepth(); ++i) {
         const V3IPDRCubeList& cubeList = _pdrFrame[i]->getCubeList(); V3IPDRCubeList::const_iterator it;
         for (it = cubeList.begin(); it != cubeList.end(); ++it) _pdrFrame.back()->pushCube(*it);
         _pdrFrame[i]->clearCubeList(); delete _pdrFrame[i]; delete _pdrSvr[i];
      }
      // Remove Empty Frames
      _pdrFrame[f] = _pdrFrame.back(); while ((1 + f) != _pdrFrame.size()) _pdrFrame.pop_back();
      _pdrActCount[f] = _pdrActCount.back(); while ((1 + f) != _pdrActCount.size()) _pdrActCount.pop_back();
      _pdrFrame.back()->removeSelfSubsumed(); delete _pdrSvr.back(); while ((1 + f) != _pdrSvr.size()) _pdrSvr.pop_back();
      // Check Common Results
      if (isIncVerifyUsingCurResult()) {
         const V3IPDRCubeList& invCubeList = _pdrFrame.back()->getCubeList();
         V3NetTable invList; invList.clear(); invList.reserve(invCubeList.size());
         for (V3IPDRCubeList::const_iterator it = invCubeList.begin(); it != invCubeList.end(); ++it)
            invList.push_back((*it)->getState()); checkCommonProof(p, invList, false);
      }
   }
}

// PDR Initialization Functions
void
V3VrfIPDR::initializeSolver(const uint32_t& d, const bool& isReuse) {
   if (profileON()) _initSvrStat->start();
   const bool isNewSolver = (d >= _pdrSvr.size());
   if (d < _pdrSvr.size()) {  // Recycle Solver
      assert (d < _pdrActCount.size());
      if (isReuse) _pdrSvr[d]->update();
      else { _pdrSvr[d]->reset(); _pdrActCount[d] = _pdrRecycle; }
   }
   else {  // New Solver
      assert (d == _pdrActCount.size()); assert (d == _pdrSvr.size());
      _pdrSvr.push_back(d ? referenceSolver(_pdrSvr[0]) : allocSolver(getSolver(), _vrfNtk));
      assert (_pdrSvr[d]->totalSolves() == 0); _pdrActCount.push_back(_pdrRecycle);
   }
   // Set Initial State to Solver
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _pdrSvr[d]->addBoundedVerifyData(_vrfNtk->getLatch(i), 0);
   // Set p to this Frame if it is NOT the Last Frame
   _pdrSvr[d]->addBoundedVerifyData(_pdrBad->getState()[0], 0);
   if (d != getPDRDepth()) _pdrSvr[d]->assertProperty(_pdrBad->getState()[0], true, 0);
   // Check Shared Invariants
   if (_sharedInv && ((1 + d) == _pdrSvr.size())) {
      V3NetTable sharedInv; _sharedInv->getInv(sharedInv);
      for (uint32_t i = 0; i < sharedInv.size(); ++i) {
         V3IPDRCube* const inv = new V3IPDRCube(0); assert (inv);
         inv->setState(sharedInv[i]); addBlockedCube(make_pair(getPDRDepth(), inv));
      }
   }
   // Consistency Check
   assert (_pdrFrame.size() == _pdrSvr.size());
   assert (_pdrFrame.size() == _pdrActCount.size());
   if (isNewSolver) _pdrSvr[d]->simplify();
   if (profileON()) _initSvrStat->end();
}

void
V3VrfIPDR::addCubeToSolver(const uint32_t& frame, const V3NetVec& state, const uint32_t& d) {
   assert (frame < _pdrSvr.size()); assert (_pdrSvr[frame]);
   assert (state.size()); assert (d < 2);
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _vrfNtk->getLatchSize());
      _pdrSvr[frame]->addBoundedVerifyData(_vrfNtk->getLatch(state[i].id), d);
   }
}
void
V3VrfIPDR::addLastFrameInfoToSolvers() {
   assert (_pdrFrame.size() > 1);
   const V3IPDRCubeList& cubeList = _pdrFrame.back()->getCubeList(); if (!cubeList.size()) return;
   V3SvrDataVec formula; formula.clear(); size_t fId;
   for (V3IPDRCubeList::const_reverse_iterator it = cubeList.rbegin(); it != cubeList.rend(); ++it) {
      const V3NetVec& state = (*it)->getState(); assert (state.size());
      for (uint32_t d = 1; d < _pdrFrame.size(); ++d) {
         formula.reserve(state.size()); addCubeToSolver(d, state, 0);
         for (uint32_t i = 0; i < state.size(); ++i) {
            fId = _pdrSvr[d]->getFormula(_vrfNtk->getLatch(state[i].id), 0);
            formula.push_back(state[i].cp ? fId : _pdrSvr[d]->getNegFormula(fId));
         }
         _pdrSvr[d]->assertImplyUnion(formula); formula.clear();
      }
   }
}

void
V3VrfIPDR::recycleSolver(const uint32_t& d) {
   assert (d < _pdrActCount.size()); assert (!_pdrActCount[d]);
   // Initialize Solver
   initializeSolver(d);
   // Push Cubes into the Solver
   if (!d) { if (_vrfNtk->getLatchSize()) _pdrSvr[d]->assertInit(); }
   else {
      V3SvrDataVec formula; formula.clear(); size_t fId;
      for (uint32_t i = d; i < _pdrFrame.size(); ++i) {
         const V3IPDRCubeList& cubeList = _pdrFrame[i]->getCubeList();
         for (V3IPDRCubeList::const_reverse_iterator it = cubeList.rbegin(); it != cubeList.rend(); ++it) {
            formula.reserve((*it)->getState().size()); addCubeToSolver(d, (*it)->getState(), 0);
            for (uint32_t j = 0; j < (*it)->getState().size(); ++j) {
               fId = _pdrSvr[d]->getFormula(_vrfNtk->getLatch((*it)->getState()[j].id), 0);
               formula.push_back((*it)->getState()[j].cp ? fId : _pdrSvr[d]->getNegFormula(fId));
            }
            _pdrSvr[d]->assertImplyUnion(formula); formula.clear();
         }
      }
   }
   // Add Shared Invariants
   _pdrSvr[d]->simplify();
}

// PDR Main Functions
V3IPDRCube* const
V3VrfIPDR::getInitialObligation() {  // If SAT(R ^ T ^ !p)
   const uint32_t d = getPDRDepth(); _pdrSvr[d]->assumeRelease(); assert (_pdrBad);
   const V3NetVec& state = _pdrBad->getState(); assert (1 == state.size());
   _pdrSvr[d]->assumeProperty(state[0], false, 0);
   if (profileON()) _solveStat->start();
   _pdrSvr[d]->simplify();
   const bool result = _pdrSvr[d]->assump_solve();
   if (profileON()) _solveStat->end();
   if (!result) return 0;
   V3IPDRCube* const cube = extractModel(d, _pdrBad);
   assert (cube); return cube;
}

V3IPDRCube* const
V3VrfIPDR::recursiveBlockCube(V3IPDRCube* const badCube) {
   // Create a Queue for Blocking Cubes
   V3BucketList<V3IPDRCube*> badQueue(getPDRFrame());
   assert (badCube); badQueue.add(getPDRDepth(), badCube);
   // Block Cubes from the Queue
   V3IPDRTimedCube baseCube, generalizedCube;
   while (badQueue.pop(baseCube.first, baseCube.second)) {
      assert (baseCube.first < getPDRFrame());
      if (!baseCube.first) {
         // Clear All Cubes in badQueue before Return
         V3Set<const V3IPDRCube*>::Set traceCubes; traceCubes.clear();
         const V3IPDRCube* traceCube = baseCube.second;
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
            generalizedCube.second = extractModel(baseCube.first - 1, baseCube.second);
            if (profileON()) _ternaryStat->end();
            badQueue.add(baseCube.first - 1, generalizedCube.second);  // This Cube should be blocked in previous frame
            badQueue.add(baseCube.first, baseCube.second);  // This Cube has not yet been blocked (postpone to future)
         }
         else {  // UNSAT, Blocked
            bool satGen = true;
            while (true) {
               if (profileON()) _generalStat->start();
               generalizedCube.first = baseCube.first;
               generalizedCube.second = new V3IPDRCube(*(baseCube.second));
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

#ifdef V3_IPDR_USE_PROPAGATE_LOW_COST
const bool
V3VrfIPDR::propagateCubes() {
   if (profileON()) _propagateStat->start();
   // Check Each Frame if some Cubes can be Further Propagated
   for (uint32_t i = 1; i < getPDRDepth(); ++i) {
      const V3IPDRCubeList& cubeList = _pdrFrame[i]->getCubeList();
#ifdef V3_IPDR_USE_PROPAGATE_BACKWARD
      // Backward Version  (Check from rbegin() to rend())
      uint32_t candidates = 1; V3IPDRCubeList::const_iterator it;
      while (candidates <= cubeList.size()) {
         it = cubeList.begin(); for (uint32_t j = candidates; j < cubeList.size(); ++j) ++it;
#else
      // Forward Version  (Check from begin() to end())
      V3IPDRCubeList::const_iterator it = cubeList.begin();
      while (it != cubeList.end()) {
#endif
         // Check if this cube can be pushed forward (closer to All Frame)
         if (!checkReachability(i + 1, (*it)->getState(), false)) {
            V3IPDRTimedCube cube = make_pair(i + 1, new V3IPDRCube(*(*it)));
            // Remove cubes in this frame that can be subsumed by the cube
            _pdrFrame[i]->removeSubsumed(cube.second, ++it);
            // Remove Cubes in the Next Frame that can be Subsumed by the cube
            removeFromProof(cube); _pdrFrame[i + 1]->removeSubsumed(cube.second);
            // Block this cube again at higher frames
            addBlockedCube(cube);
         }
         else {
            // Remove cubes in this frame that can be subsumed by the cube
            _pdrFrame[i]->removeSubsumed(*it, it);
#ifdef V3_IPDR_USE_PROPAGATE_BACKWARD
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
      if (!_pdrFrame[i]->getCubeList().size()) {
         if (profileON()) _propagateStat->end();
         return true;
      }
   }
   // Check if Any Remaining Cubes in the Latest Frame can be Subsumed
   _pdrFrame[getPDRDepth()]->removeSelfSubsumed();
   if (profileON()) _propagateStat->end();
   return false;
}
#else
const bool
V3VrfIPDR::propagateCubes() {
   if (profileON()) _propagateStat->start();
   // Check Each Frame if some Cubes can be Further Propagated
   for (uint32_t i = 1; i < getPDRDepth(); ++i) {
      const V3IPDRCubeList& cubeList = _pdrFrame[i]->getCubeList();
#ifdef V3_IPDR_USE_PROPAGATE_BACKWARD
      // Backward Version  (Check from rbegin() to rend())
      uint32_t candidates = 1; V3IPDRCubeList::const_iterator it;
      while (candidates <= cubeList.size()) {
         it = cubeList.begin(); for (uint32_t j = candidates; j < cubeList.size(); ++j) ++it;
#else
      // Forward Version  (Check from begin() to end())
      V3IPDRCubeList::const_iterator it = cubeList.begin();
      while (it != cubeList.end()) {
#endif
         // Check if this cube can be pushed forward (closer to the last frame)
         if (!checkReachability(i + 1, (*it)->getState(), false)) {
            V3IPDRTimedCube cube = make_pair(i + 1, new V3IPDRCube(*(*it)));
            // Block this cube again at higher frames
            removeFromProof(cube); addBlockedCube(cube);
            // Remove blocked cubes in lower frames that can be subsumed by the cube
            for (uint32_t j = 1; j < i; ++j) _pdrFrame[j]->removeSubsumed(cube.second);
            // Remove Cubes in this Frame that can be Subsumed by the cube
            _pdrFrame[i]->removeSubsumed(cube.second, ++it);
            // NOTE: Need not to recover iterator after subsumption (set.erase), 
            //       the iterator it will point to the next candidate
         }
         else {
            // Remove blocked cubes in lower frames that can be subsumed by the cube
            for (uint32_t j = 1; j < i; ++j) _pdrFrame[j]->removeSubsumed(*it);
#ifdef V3_IPDR_USE_PROPAGATE_BACKWARD
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
      for (uint32_t j = 1; j <= i; ++j) 
         if (!_pdrFrame[j]->getCubeList().size()) {
            if (profileON()) _propagateStat->end();
            return true;
         }
   }
   // Check if Any Remaining Cubes in the Last Frame can be Subsumed
   _pdrFrame[getPDRDepth()]->removeSelfSubsumed();
   if (profileON()) _propagateStat->end();
   return false;
}
#endif

// PDR Auxiliary Functions
const bool
V3VrfIPDR::checkReachability(const uint32_t& frame, const V3NetVec& cubeState, const bool& extend) {
   assert (frame > 0); assert (frame < getPDRFrame());
   // Check if Recycle is Triggered
   const uint32_t& d = frame - 1; assert (d < _pdrActCount.size());
   if (!_pdrActCount[d]) recycleSolver(d); _pdrSvr[d]->assumeRelease();
   // Assume cube'
   addCubeToSolver(d, cubeState, 1);
   for (uint32_t i = 0; i < cubeState.size(); ++i)
      _pdrSvr[d]->assumeProperty(_pdrSvr[d]->getFormula(_vrfNtk->getLatch(cubeState[i].id), 1), cubeState[i].cp);
   if (extend) {
      // Assume ~cube
      addCubeToSolver(d, cubeState, 0);
      V3SvrDataVec blockCube; blockCube.clear(); blockCube.reserve(cubeState.size()); size_t fId;
      for (uint32_t i = 0; i < cubeState.size(); ++i) {
         fId = _pdrSvr[d]->getFormula(_vrfNtk->getLatch(cubeState[i].id), 0);
         blockCube.push_back(cubeState[i].cp ? fId : _pdrSvr[d]->getNegFormula(fId));
      }
      _pdrSvrData = _pdrSvr[d]->setImplyUnion(blockCube);
      assert (_pdrSvrData); _pdrSvr[d]->assumeProperty(_pdrSvrData);
      // Check Reachability by SAT Calling
      if (profileON()) _solveStat->start();
      _pdrSvr[d]->simplify();
      const bool result = _pdrSvr[d]->assump_solve();
      if (profileON()) _solveStat->end();
      _pdrSvr[d]->assertProperty(_pdrSvr[d]->getNegFormula(_pdrSvrData));  // Invalidate ~cube in future solving
      --_pdrActCount[d]; return result;
   }
   else {
      if (profileON()) _solveStat->start();
      _pdrSvr[d]->simplify();
      const bool result = _pdrSvr[d]->assump_solve();
      if (profileON()) _solveStat->end();
      return result;
   }
}

const bool
V3VrfIPDR::isBlocked(const V3IPDRTimedCube& timedCube) {
   // Check if cube has already been blocked by R (at specified frame)
   // Perform Subsumption Check : cube implies some C in R (at specified frame)
   for (uint32_t i = timedCube.first; i < _pdrFrame.size(); ++i) 
      if (_pdrFrame[i]->subsumes(timedCube.second)) return true;
   /*
   // Check by SAT
   const uint32_t d = timedCube.first; _pdrSvr[d]->assumeRelease();
   const V3NetVec& state = timedCube.second->getState(); addCubeToSolver(d, state, 0);
   for (uint32_t i = 0; i < state.size(); ++i)
      _pdrSvr[d]->assumeProperty(_pdrSvr[d]->getFormula(_vrfNtk->getLatch(state[i].id), 0), state[i].cp);
   if (profileON()) _solveStat->start();
   const bool result = _pdrSvr[timedCube.first]->assump_solve();
   if (profileON()) _solveStat->end();
   return !result;
   */
   return false;
}

const bool
V3VrfIPDR::existInitial(const V3NetVec& state) {
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _pdrInitConst.size());
      assert (state[i].id < _pdrInitValue.size());
      if (_pdrInitConst[state[i].id] && (_pdrInitValue[state[i].id] ^ state[i].cp)) return false;
   }
   return true;
}

V3IPDRCube* const
V3VrfIPDR::extractModel(const uint32_t& d, const V3IPDRCube* const nextCube) {
   // This function can ONLY be called after SAT of (R ^ T ^ nextCube') and generalize curCube from R
   V3IPDRCube* const cube = new V3IPDRCube(nextCube);  // Create Cube
   generalizeSimulation(d, cube, nextCube);  // Apply Simulation for the witness
   // Record Input to Proof Obligation for Trace Logging
   if (_pdrSize) recordCubeInputForTraceLog(cube); return cube;
}

V3IPDRCube* const
V3VrfIPDR::forwardModel(const V3IPDRCube* const curCube) {
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
   V3IPDRCube* const cube = new V3IPDRCube(0); assert (cube);
   cube->setState(nextState); return cube;
}

void
V3VrfIPDR::resolveInitial(const uint32_t& d, V3IPDRCube* const cube, const V3SvrDataSet& coreProofVars) {
   // Get Proof Related State Variables
   assert (d < _pdrSvr.size()); assert (cube);
   // Store Values of State Variable
   const V3NetVec& state = cube->getState();
   // Remove Variables to Form New State
   assert (!existInitial(state));
   bool conflictInitial = false;
   V3NetId conflictId = V3NetUD;
   uint32_t pos = 0;
   V3NetVec newState; newState.reserve(state.size());
   for (uint32_t i = 0; i < state.size(); ++i) {
      if (coreProofVars.end() != coreProofVars.find(_pdrSvr[d]->getFormula(_vrfNtk->getLatch(state[i].id), 1))) {
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

void
V3VrfIPDR::generalization(V3IPDRTimedCube& generalizedCube) {
   removeFromProof(generalizedCube);
   generalizeProof(generalizedCube);
   forwardProof(generalizedCube);
}

void
V3VrfIPDR::addBlockedCube(const V3IPDRTimedCube& cube) {
   assert (cube.first < _pdrSvr.size()); assert (cube.second->getState().size());
   // Push cube into corresponding frame that it should be blocked
   if (!_pdrFrame[cube.first]->pushCube(cube.second)) return;
   // Renders this cube to be blocked for frames lower than cube.first
   const V3NetVec& state = cube.second->getState();
   V3SvrDataVec formula; formula.clear(); size_t fId;
   for (uint32_t d = 1; d <= cube.first; ++d) {
      formula.reserve(state.size()); addCubeToSolver(d, state, 0);
      for (uint32_t i = 0; i < state.size(); ++i) {
         fId = _pdrSvr[d]->getFormula(_vrfNtk->getLatch(state[i].id), 0);
         formula.push_back(state[i].cp ? fId : _pdrSvr[d]->getNegFormula(fId));
         ++_pdrPriority[state[i].id];  // Increase Signal Priority
      }
      _pdrSvr[d]->assertImplyUnion(formula); formula.clear();
   }
}

void
V3VrfIPDR::recordCubeInputForTraceLog(V3IPDRCube* const& cube) {
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
V3VrfIPDR::generalizeSimulation(const uint32_t& d, V3IPDRCube* const cube, const V3IPDRCube* const nextCube) {
   assert (d < _pdrSvr.size()); assert (cube); assert (nextCube);
   assert (nextCube == cube->getNextCube()); assert (_pdrSim);
   // Set Values for Simulator
   for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i) {
      if (!_pdrSvr[d]->existVerifyData(_vrfNtk->getInput(i), 0)) _pdrSim->clearSource(_vrfNtk->getInput(i), true);
      else _pdrSim->setSource(_vrfNtk->getInput(i), _pdrSvr[d]->getDataValue(_vrfNtk->getInput(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i) {
      if (!_pdrSvr[d]->existVerifyData(_vrfNtk->getInout(i), 0)) _pdrSim->clearSource(_vrfNtk->getInout(i), true);
      else _pdrSim->setSource(_vrfNtk->getInout(i), _pdrSvr[d]->getDataValue(_vrfNtk->getInout(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
      if (!_pdrSvr[d]->existVerifyData(_vrfNtk->getLatch(i), 0)) _pdrSim->clearSource(_vrfNtk->getLatch(i), true);
      else _pdrSim->setSource(_vrfNtk->getLatch(i), _pdrSvr[d]->getDataValue(_vrfNtk->getLatch(i), 0));
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
V3VrfIPDR::removeFromProof(V3IPDRTimedCube& timedCube) {
   // This function can ONLY be called after UNSAT of (R ^ T ^ cube')
   // Generate UNSAT Source from Solver if Possible
   V3SvrDataVec coreProofVars; coreProofVars.clear(); assert (timedCube.first < _pdrSvr.size());
   assert (timedCube.first); _pdrSvr[timedCube.first - 1]->getDataConflict(coreProofVars);
   if (!coreProofVars.size()) return false;  // Solver does not Support Analyze Conflict
   V3SvrDataSet coreProofVarSet; coreProofVarSet.clear();
   for (uint32_t i = 0; i < coreProofVars.size(); ++i) coreProofVarSet.insert(coreProofVars[i]);
   const bool isSvrDataInvolved = coreProofVarSet.end() != coreProofVarSet.find(_pdrSvrData);
   // Get Generalized State from Solver Proof
   assert (!existInitial(timedCube.second->getState()));
   resolveInitial(timedCube.first - 1, timedCube.second, coreProofVarSet);
   assert (!existInitial(timedCube.second->getState()));
   assert (!checkReachability(timedCube.first, timedCube.second->getState())); return isSvrDataInvolved;
}

void
V3VrfIPDR::generalizeProof(V3IPDRTimedCube& timedCube) {
   // Apply SAT Solving to further generalize cube
   // Remove Variables from cube after Proof Success
   V3IPDRCube* const& cube = timedCube.second; assert (!existInitial(cube->getState()));
   V3NetVec state(cube->getState()); V3NetId id;
   // Sort Priority of Signals
   V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map priorityMap; priorityMap.clear();
   V3Map<uint32_t, uint32_t>::Map::iterator it, is;
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (state[i].id < _pdrPriority.size()); priorityMap.insert(make_pair(_pdrPriority[state[i].id], i));
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
V3VrfIPDR::forwardProof(V3IPDRTimedCube& timedCube) {
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
V3VrfIPDR::generalizeCex(V3IPDRTimedCube& timedCube) {
   // Try Turning the Reachability Check from SAT to UNSAT
   V3IPDRCube* cube = new V3IPDRCube(*(timedCube.second)); assert (cube);
   assert (!existInitial(cube->getState()));
   while (true) {
      // Get the SAT Model
      if (!checkReachability(timedCube.first, cube->getState())) break;
      V3IPDRCube* const prevCube = new V3IPDRCube(cube);  // Create Cube
      generalizeSimulation(timedCube.first - 1, prevCube, cube);  // Apply Simulation for the witness
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
V3VrfIPDR::reportUnsupportedInitialState() {
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
V3VrfIPDR::printState(const V3NetVec& state) const {
   for (uint32_t i = 0; i < state.size(); ++i)
      Msg(MSG_IFO) << (state[i].cp ? "~" : "") << state[i].id << " ";
}

#endif

