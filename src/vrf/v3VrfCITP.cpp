/****************************************************************************
  FileName     [ v3VrfCITP.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ A Counterexample-Guided Interpolant Generation Algorithm
                 for SAT-based Model Checking (NewITP) on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_CITP_C
#define V3_VRF_CITP_C

#include "v3Msg.h"
#include "v3VrfCITP.h"

#include <iomanip>

// MACROS for GENERALIZATION TESTS
//#define V3_CITP_UNSAT_TRY_STRONGER_GEN

// MACROS for STATES TO REFINEMENT
//#define V3_CITP_ADD_REPEATED_SREF_ONLY

// MACROS for RECYCLE
#define V3_CITP_IMPROVED_RECYCLE

/* -------------------------------------------------- *\
 * Class V3CITPFrame Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3CITPFrame::V3CITPFrame() {
   _cubeList.clear();
}

V3CITPFrame::~V3CITPFrame() {
   for (V3CITPCubeList::iterator it = _cubeList.begin(); it != _cubeList.end(); ++it) delete *it;
   _cubeList.clear();
}

// Retrieval Functions
const bool
V3CITPFrame::pushCube(V3CITPCube* const c) {
   const uint32_t cubeSize = _cubeList.size(); _cubeList.insert(c);
   return cubeSize != _cubeList.size();
}

// Cube Containment Functions
void
V3CITPFrame::removeSelfSubsumed() {
   // This function checks whether some cubes in this frame can be subsumed by the other cube
   // Remove all cubes that are subsumed by the cube in this frame from _cubeList
   V3CITPCubeList::const_iterator ix;
   uint32_t candidates = 1;
   while (candidates <= _cubeList.size()) {
      ix = _cubeList.begin(); for (uint32_t i = candidates; i < _cubeList.size(); ++i) ++ix;
      removeSubsumed(*ix, ix); ++candidates;
   }
}

// Private Cube Containment Functions
void
V3CITPFrame::removeSubsumed(const V3CITPCube* const cube, const V3CITPCubeList::const_iterator& ix) {
   // This function checks whether there's any existing cube in this frame subsumed by cube.
   // If such cube is found, remove it from _cubeList
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3CITPCubeList::iterator it = _cubeList.begin();
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

/* -------------------------------------------------- *\
 * Class V3VrfCITP Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfCITP::V3VrfCITP(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   // Private Data Members
   _itpFrame.clear(); _itpReuse.clear(); _itpBad = 0; _itpAttr = 0; _itpBMCDepth = 0; _itpBadCount = 1;
   // Private Engines
   _itpBMCSvr = 0; _itpITPSvr = 0; _itpGSt = 0; _itpSim = 0; _itpGen = 0;
   // Extended Data Members
   _itpPriority.clear(); _itpForceUAct = 0; _itpBlockCubes.clear(); _itpBadCubeStr.clear();
   // Statistics
   if (profileON()) {
      _totalStat     = new V3Stat("TOTAL");
      _initSvrStat   = new V3Stat("SVR INIT",    _totalStat);
      _solveStat     = new V3Stat("SVR SOLVE",   _totalStat);
      _generalStat   = new V3Stat("GENERALIZE",  _totalStat);
      _ternaryStat   = new V3Stat("TERNARY SIM", _totalStat);
   }
}

V3VrfCITP::~V3VrfCITP() {
   // Private Data Members
   for (uint32_t i = 0; i < _itpFrame.size(); ++i) { assert (_itpFrame[i]); delete _itpFrame[i]; } _itpFrame.clear();
   for (uint32_t i = 0; i < _itpReuse.size(); ++i) { assert (_itpReuse[i]); delete _itpReuse[i]; } _itpReuse.clear();
   // Private Engines
   if (_itpBMCSvr) delete _itpBMCSvr; _itpBMCSvr = 0; if (_itpGSt) delete _itpGSt; _itpGSt = 0;
   if (_itpITPSvr) delete _itpITPSvr; _itpITPSvr = 0; if (_itpGen) delete _itpGen; _itpGen = 0;
   // Extended Data Members
   _itpPriority.clear(); _itpForceUAct = 0; _itpBlockCubes.clear(); _itpBadCubeStr.clear();
   // Statistics
   if (profileON()) {
      if (_totalStat)   delete _totalStat;
      if (_initSvrStat) delete _initSvrStat;
      if (_solveStat)   delete _solveStat; 
      if (_generalStat) delete _generalStat;
      if (_ternaryStat) delete _ternaryStat;
   }
}

// Verification Main Functions
/* ---------------------------------------------------------------------------------------------------- *\
isIncKeepLastReachability(): Keep _itpFrame in _itpReuse.
\* ---------------------------------------------------------------------------------------------------- */
void
V3VrfCITP::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p); if (profileON()) _totalStat->start();
   
   // Consistency Check
   consistencyCheck(); assert (!_constr.size());

   // Initialize Backup Frames
   for (uint32_t i = 0; i < _itpReuse.size(); ++i) delete _itpReuse[i]; _itpReuse.clear();
   if (isIncKeepLastReachability() && _itpFrame.size()) {
      _itpReuse.reserve(_itpFrame.size() - 1); delete _itpFrame[0];
      for (uint32_t i = 1; i < _itpFrame.size(); ++i) _itpReuse.push_back(_itpFrame[i]);
   }
   else {
      for (uint32_t i = 0; i < _itpFrame.size(); ++i) delete _itpFrame[i];
      _itpBlockCubes.clear(); _itpBadCubeStr.clear(); _itpPriority.clear();
   }
   _itpFrame.clear();

   // Initialize Other Members
   if (_itpGSt) delete _itpGSt; _itpGSt = 0; if (_itpGen) delete _itpGen; _itpGen = 0;
   if (dynamic_cast<V3BvNtk*>(_vrfNtk)) {
      _itpGen = new V3AlgBvGeneralize(_handler); assert (_itpGen);
      _itpSim = dynamic_cast<V3AlgBvSimulate*>(_itpGen); assert (_itpSim);
   }
   else {
      _itpGen = new V3AlgAigGeneralize(_handler); assert (_itpGen);
      _itpSim = dynamic_cast<V3AlgAigSimulate*>(_itpGen); assert (_itpSim);
   }
   V3NetVec simTargets(1, _vrfNtk->getOutput(p)); _itpSim->reset(simTargets); _itpForceUAct = 0;

   // Initialize Parameters
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _vrfNtk->getOutput(p); assert (V3NetUD != pId);
   const string flushSpace = string(100, ' ');
   uint32_t proved = V3NtkUD, fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);
   uint32_t lastDepth = getIncLastDepthToKeepGoing(); if (10000000 < lastDepth) lastDepth = _itpBMCDepth;
   
   // Initialize Generalization Struct
   _itpGSt = new V3GenStruct(_vrfNtk->getLatchSize()); assert (_itpGSt);

   // Initialize Signal Priority List
   if (_itpPriority.size() != _vrfNtk->getLatchSize()) _itpPriority.resize(_vrfNtk->getLatchSize(), 0);

   // Initialize Bad Cube
   _itpBad = new V3CITPCube(0); assert (_itpBad); _itpBad->setState(V3NetVec(1, pId));

   // Initialize Solvers
   if (_itpBMCSvr) {
      if (isIncContinueOnLastSolver()) _itpBMCSvr->update();
      else { delete _itpBMCSvr; _itpBMCSvr = 0; }
   }
   if (!_itpBMCSvr) {
      _itpBMCSvr = allocSolver(getSolver(), _vrfNtk, isForceUnreachable());
      assert (_itpBMCSvr->totalSolves() == 0);
   }
   if (_itpITPSvr && !isIncContinueOnLastSolver()) { delete _itpITPSvr; _itpITPSvr = 0; }
   initializeBMCSolver(0, 1 + lastDepth); initializeITPSolver();

   // Initialize Frame 0
   assert (!_itpFrame.size()); _itpFrame.push_back(new V3CITPFrame());
   assert (_itpFrame.back()); _itpFrame.back()->pushCube(_itpBad);

   // Start CITP Based Verification
   uint32_t inc = 1; int result = 0;
   while (1) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
      // Increase BMC Depth from Porfolio Data
      if (_sharedBound) {
         const uint32_t k = _sharedBound->getBound(p); if (V3NtkUD == k) break;
         if (_itpBMCDepth <= k && !isNoIncBySharedBound()) 
            initializeBMCSolver(_itpBMCDepth, (1 + k - _itpBMCDepth));
      }
      // Simplify Solvers
      _itpBMCSvr->simplify(); _itpITPSvr->simplify();
      // Check Real Counterexample by BMC Solver
      if (!_sharedBound || _itpBMCDepth > _sharedBound->getBound(p)) {
         // SAT(I0 ^ T(_itpBMCDepth) ^ !p)
         if (checkCounterExample()) { fired = _itpBMCDepth; break; }
         else if (_sharedBound) _sharedBound->updateBound(p, _itpBMCDepth);
      }
      // Check Shared Networks
      if (_sharedNtk) {
         V3NtkHandler* const sharedNtk = _sharedNtk->getNtk(_handler);
         if (sharedNtk) {
            setIncKeepLastReachability(true); setIncContinueOnLastSolver(false); setIncLastDepthToKeepGoing(_itpBMCDepth);
            _handler = sharedNtk; _vrfNtk = sharedNtk->getNtk(); goto vrfRestart;
         }
      }
      while (true) {
         // Check Time Bounds
         gettimeofday(&curtime, NULL);
         if (_maxTime < getTimeUsed(inittime, curtime)) break;
         // Check Memory Bound
         if (_sharedMem && !_sharedMem->isMemValid()) break;
         // Check Shared Results
         if (_sharedBound && (V3NtkUD == _sharedBound->getBound(p))) break;
         // Initialize New Frame
         _itpFrame.push_back(new V3CITPFrame());
         // Block New Cubes and Store in the New Frame
         result = blockNewCube();  // SAT (T ^ Ri')
         if (-1 == result) {
            assert (!_itpFrame.back()->getCubeList().size());
            proved = _itpFrame.size() - 1; break;
         }
         // Print Interactive Messages
         if (!isIncKeepSilent() && intactON()) {
            if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
            Msg(MSG_IFO) << setw(3) << left << _itpBMCDepth << " :";
            const uint32_t j = (_itpFrame.size() > 25) ? _itpFrame.size() - 25 : 0; if (j) Msg(MSG_IFO) << " ...";
            for (uint32_t i = j; i < _itpFrame.size(); ++i) 
               Msg(MSG_IFO) << " " << _itpFrame[i]->getCubeList().size();
            if (svrInfoON()) { Msg(MSG_IFO) << "  ("; _itpITPSvr->printInfo(); Msg(MSG_IFO) << ")"; }
            if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
         }
         // Check Spurious Counterexample by BMC Solver
         if (1 == result) {  // Spurious Cex Found
            if (_maxDepth <= _itpBMCDepth) { inc = V3NtkUD; break; }
            else inc = isIncrementDepth() ? (_itpFrame.size() - 1) : 1;
            // Update Reachability
            if (isRecycleInterpolants()) {
               for (uint32_t i = 0; i < _itpReuse.size(); ++i) delete _itpReuse[i];
               _itpReuse.clear(); _itpReuse.reserve(_itpFrame.size() - 1);
               for (uint32_t i = 1; i < _itpFrame.size(); ++i) _itpReuse.push_back(_itpFrame[i]);
               while (1 < _itpFrame.size()) _itpFrame.pop_back();
            }
            else { while (1 < _itpFrame.size()) { delete _itpFrame.back(); _itpFrame.pop_back(); } }
            initializeITPSolver(); break;
         }
      }
      if (V3NtkUD != proved || V3NtkUD == inc) break;
      else initializeBMCSolver(_itpBMCDepth, inc);
      if (!isIncKeepSilent() && intactON() && !endLineON()) Msg(MSG_IFO) << endl;
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != proved) Msg(MSG_IFO) << "Inductive Invariant found at depth = " << proved;
      else if (V3NtkUD != fired) Msg(MSG_IFO) << "Counter-example found at depth = " << fired;
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
         Msg(MSG_IFO) << *_ternaryStat << endl;
         Msg(MSG_IFO) << *_totalStat << endl;
      }
   }

   // Record CounterExample Trace or Invariant
   if (V3NtkUD != fired) {  // Record Counter-Example
      V3CexTrace* const cex = new V3CexTrace(fired); assert (cex);
      // Set Pattern Value
      uint32_t patternSize = _vrfNtk->getInputSize() + _vrfNtk->getInoutSize();
      V3BitVecX dataValue, patternValue(patternSize ? patternSize : 1);
      for (uint32_t i = 0; i < fired; ++i) {
         patternSize = 0; patternValue.clear();
         for (uint32_t j = 0; j < _vrfNtk->getInputSize(); ++j, ++patternSize) {
            if (!_itpBMCSvr->existVerifyData(_vrfNtk->getInput(j), i)) continue;
            dataValue = _itpBMCSvr->getDataValue(_vrfNtk->getInput(j), i);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         for (uint32_t j = 0; j < _vrfNtk->getInoutSize(); ++j, ++patternSize) {
            if (!_itpBMCSvr->existVerifyData(_vrfNtk->getInout(j), i)) continue;
            dataValue = _itpBMCSvr->getDataValue(_vrfNtk->getInout(j), i);
            if ('0' == dataValue[0]) patternValue.set0(patternSize);
            else if ('1' == dataValue[0]) patternValue.set1(patternSize);
         }
         assert (_itpBMCSvr->existVerifyData(pId, i));
         assert (!patternSize || patternSize == patternValue.size()); cex->pushData(patternValue);
      }
      // Set Initial State Value
      if (_vrfNtk->getLatchSize()) {
         patternValue.resize(_vrfNtk->getLatchSize());
         patternValue.clear(); V3NetId id;
         for (uint32_t j = 0; j < _vrfNtk->getLatchSize(); ++j) {
            if (_itpBMCSvr->existVerifyData(_vrfNtk->getLatch(j), 0)) {
               dataValue = _itpBMCSvr->getDataValue(_vrfNtk->getLatch(j), 0);
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
      // Put the Inductive Invariant to the Last Frame
      for (uint32_t i = 1, j = _itpFrame.size() - 1; i < j; ++i) {
         const V3CITPCubeList& cubeList = _itpFrame[i]->getCubeList(); V3CITPCubeList::const_iterator it;
         for (it = cubeList.begin(); it != cubeList.end(); ++it) _itpFrame.back()->pushCube(*it);
         _itpFrame[i]->clearCubeList();
      }
      _itpFrame.back()->removeSelfSubsumed();
      // Check Common Results
      if (isIncVerifyUsingCurResult()) {
         const V3CITPCubeList& invCubeList = _itpFrame.back()->getCubeList();
         V3NetTable invList; invList.clear(); invList.reserve(invCubeList.size());
         for (V3CITPCubeList::const_iterator it = invCubeList.begin(); it != invCubeList.end(); ++it)
            invList.push_back((*it)->getState()); checkCommonProof(p, invList, true);
      }
   }
}

// CITP Initialization Functions
void
V3VrfCITP::initializeBMCSolver(const uint32_t& start, const uint32_t& frames) {
   if (profileON()) _initSvrStat->start();
   assert (_itpBMCSvr); assert (_itpBad);

   uint32_t d = start, e = start + frames; assert (d < e);
   if (!d) {
      // Set Initial State to Solver
      for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _itpBMCSvr->addBoundedVerifyData(_vrfNtk->getLatch(i), d);
      if (!isForceUnreachable()) _itpBMCSvr->assertInit();
   }
   while (d < e) {
      _itpBMCSvr->addBoundedVerifyData(_itpBad->getState()[0], d); ++d;
      for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _itpBMCSvr->addBoundedVerifyData(_vrfNtk->getLatch(i), d);
      // Check Shared Invariants
      if (_sharedInv) _sharedInv->pushInv(_itpBMCSvr, _vrfNtk, d - 1);
      // Assert Property Logic
      if (d != e) _itpBMCSvr->assertProperty(_itpBad->getState()[0], true, d - 1);
   }
   assert (e == d); _itpBMCDepth = e;

   // Force Unreachability Generalization
   if (isForceUnreachable()) {
      V3SvrDataVec initAct; initAct.clear(); initAct.reserve(e);
      //if (start) _itpBMCSvr->assertProperty(_itpBMCSvr->getNegFormula(_itpForceUAct));
      _itpForceUAct = _itpBMCSvr->reserveFormula(); assert (_itpForceUAct);
      // Reserve Activation Variables Over all Frames
      V3SvrDataVec initVars(2), nextVars(3);
      for (d = 0; d <= e; ++d) initAct.push_back(_itpBMCSvr->reserveFormula());
      // Compute Initial State Over all Frames
      for (d = 0; d < e; ++d) {
         assert (initAct[d]); initVars[0] = _itpBMCSvr->getNegFormula(initAct[d]); assert (initVars[0]);
         for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
            const V3NetId initId = _vrfNtk->getInputNetId(_vrfNtk->getLatch(i), 1);
            if (initId.id) { assert (_vrfNtk->getLatch(i) == initId); continue; }
            initVars[1] = _itpBMCSvr->getFormula(_vrfNtk->getLatch(i), d);
            if (!initId.cp) initVars[1] = _itpBMCSvr->getNegFormula(initVars[1]);
            _itpBMCSvr->assertImplyUnion(initVars);
         }
      }
      // Compute Next Frame Relation Over all Frames
      for (d = 1; d <= e; ++d) {
         assert (initAct[d]); nextVars[0] = initAct[d]; assert (nextVars[0]);
         for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
            initVars[0] = _itpBMCSvr->getFormula(_vrfNtk->getLatch(i), d);
            initVars[1] = _itpBMCSvr->getFormula(_vrfNtk->getInputNetId(_vrfNtk->getLatch(i), 0), d - 1);
            nextVars[1] = initVars[0]; nextVars[2] = _itpBMCSvr->getNegFormula(initVars[1]);
            _itpBMCSvr->assertImplyUnion(nextVars);
            nextVars[1] = _itpBMCSvr->getNegFormula(initVars[0]); nextVars[2] = initVars[1];
            _itpBMCSvr->assertImplyUnion(nextVars);
         }
         initVars[0] = _itpBMCSvr->getNegFormula(nextVars[0]);
         for (uint32_t i = 0; i < d; ++i) {
            initVars[1] = _itpBMCSvr->getNegFormula(initAct[i]);
            _itpBMCSvr->assertImplyUnion(initVars);
         }
      }
      initAct.back() = _itpBMCSvr->getNegFormula(_itpForceUAct); _itpBMCSvr->assertImplyUnion(initAct);
   }
   
   _itpBMCSvr->simplify();
   if (profileON()) _initSvrStat->end();
}

void
V3VrfCITP::initializeITPSolver() {
   // NOTE: We Currently Add Invariant Constraint on the BMC Solver ONLY
   if (profileON()) _initSvrStat->start();
   if (_itpITPSvr) _itpITPSvr->reset();
   else { _itpITPSvr = allocSolver(getSolver(), _vrfNtk); assert (_itpITPSvr->totalSolves() == 0); }
   // Set Initial State to Solver
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _itpITPSvr->addBoundedVerifyData(_vrfNtk->getLatch(i), 0);
   assert (1 == _itpBad->getState().size()); _itpITPSvr->addBoundedVerifyData(_itpBad->getState()[0], 0);
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) _itpITPSvr->addBoundedVerifyData(_vrfNtk->getLatch(i), 1);
   // Check Shared Invariants
   if (_sharedInv) _sharedInv->pushInv(_itpITPSvr, _vrfNtk, 0);
   if (_sharedInv) _sharedInv->pushInv(_itpITPSvr, _vrfNtk, 1);
   _itpITPSvr->simplify();
   if (profileON()) _initSvrStat->end();
}

// CITP Main Functions
const int
V3VrfCITP::blockNewCube() {
   // Block out New Cubes Reachable to _itpFrame[_itpFrame.size() - 2] and Record into _itpFrame.back()
   // NOTE: Return 1 if Spurious Counterexample Found, -1 for Inductive Invariant, and 0 Otherwise
   assert (_itpFrame.size() > 1); assert (!_itpFrame.back()->getCubeList().size());
   const V3CITPCubeList& cubeList = _itpFrame[_itpFrame.size() - 2]->getCubeList(); assert (cubeList.size());
   // Reuse Previously Computed Interpolants
   if (_itpReuse.size() >= (_itpFrame.size() - 1)) {
      V3CITPCubeList& reuseList = _itpReuse[_itpFrame.size() - 2]->getCubeList();
      V3CITPCubeList::reverse_iterator it = reuseList.rbegin();
      while (it != reuseList.rend()) {
         if ((*it)->isEmpty()) { ++it; continue; } assert ((*it)->getState().size());
#ifdef V3_CITP_IMPROVED_RECYCLE
         if ((*it)->getNextCube() && (*it)->getNextCube()->isEmpty()) { (*it)->clearState(); ++it; continue; }
#endif
         if (checkBMCReachability((*it)->getState())) { (*it)->clearState(); ++it; }
         else {
#ifdef V3_CITP_IMPROVED_RECYCLE
            if (_itpFrame.back()->pushCube(*it)) addBlockedCube(*it);
            reuseList.erase(--it.base());
#else
            V3CITPCube* const newCube = new V3CITPCube(*(*it)); assert (newCube);
            if (_itpFrame.back()->pushCube(newCube)) addBlockedCube(newCube); ++it;
#endif
         }
      }
   }
   // Compute New Interpolants
   for (V3CITPCubeList::const_reverse_iterator it = cubeList.rbegin(); it != cubeList.rend(); ++it) {
      while (checkReachability(*it)) {
         V3CITPCube* const newCube = extractNewCube(*it); assert (newCube);
         // Further Generalize the Cube for Overapproximation
         if (checkBMCReachability(newCube->getState())) {
            // Find the first bad cube from BMC solver
            V3NetVec badCube; badCube.clear();
            for (uint32_t i = 0, j = _vrfNtk->getLatchSize(); i < j; ++i)
               badCube.push_back(V3NetId::makeNetId(i,
                                 '0' == _itpBMCSvr->getDataValue(_vrfNtk->getLatch(i), _itpBMCDepth)[0]));
            assert (checkBMCReachability(badCube));
            if (_itpBadCount) {
#ifdef V3_CITP_ADD_REPEATED_SREF_ONLY
               const string stateStr = getStateStr(badCube);
               V3StrSet::iterator ix = _itpBadCubeStr.find(stateStr);
               if (_itpBadCubeStr.end() == ix) _itpBadCubeStr.insert(stateStr);
               else {
                  _itpBadCubeStr.erase(ix);
#endif
                  _itpBlockCubes.push_back(badCube);
                  blockBadCubes(_itpBlockCubes.size() - 1);
#ifdef V3_CITP_ADD_REPEATED_SREF_ONLY
               }
#endif
            }
            return 1;
         }
         if (!removeFromProof(newCube)) return 1;
#ifdef V3_CITP_UNSAT_TRY_STRONGER_GEN
         if (!generalizeProof(newCube)) return 1;
#endif
         if (_itpFrame.back()->pushCube(newCube)) addBlockedCube(newCube);
      }
   }
   // Check Containment
   if (!_itpFrame.back()->getCubeList().size()) return -1;
   // Perform Self Subsumption
#ifdef V3_CITP_IMPROVED_RECYCLE
   if (isRecycleInterpolants()) return 0;  // Disable Remove Self Subsumption
#endif
   _itpFrame.back()->removeSelfSubsumed(); assert (_itpFrame.back()->getCubeList().size());
   return 0;
}

const bool
V3VrfCITP::checkCounterExample() {
   assert (_itpBMCSvr); assert (_itpBad); assert (1 == _itpBad->getState().size());
   assert (_itpBMCSvr->existVerifyData(_itpBad->getState()[0], _itpBMCDepth - 1));
   // Assume !p at the Last Frame
   _itpBMCSvr->assumeRelease();
   if (isForceUnreachable()) _itpBMCSvr->assumeProperty(_itpForceUAct);
   _itpBMCSvr->assumeProperty(_itpBad->getState()[0], false, _itpBMCDepth - 1);
   // Check if there is a Real Counterexample
   if (profileON()) _solveStat->start();
   const bool result = _itpBMCSvr->assump_solve();
   if (profileON()) _solveStat->end();
   return result;
}

// CITP Auxiliary Functions
const bool
V3VrfCITP::checkReachability(V3CITPCube* const nextCube) {
   assert (nextCube); assert (nextCube->getState().size());
   const V3NetVec& state = nextCube->getState();
   // Assume cube'
   _itpITPSvr->assumeRelease();
   if (_itpBad == nextCube) _itpITPSvr->assumeProperty(_itpITPSvr->getFormula(state[0], 0));
   else {
      for (uint32_t i = 0; i < state.size(); ++i)
         _itpITPSvr->assumeProperty(_itpITPSvr->getFormula(_vrfNtk->getLatch(state[i].id), 1), state[i].cp);
   }
   // Check Reachability
   if (profileON()) _solveStat->start();
   const bool result = _itpITPSvr->assump_solve();
   if (profileON()) _solveStat->end();
   return result;
}

const bool
V3VrfCITP::checkBMCReachability(const V3NetVec& state) {
   // Assume cube'
   _itpBMCSvr->assumeRelease();
   for (uint32_t i = 0; i < state.size(); ++i)
      _itpBMCSvr->assumeProperty(_itpBMCSvr->getFormula(_vrfNtk->getLatch(state[i].id), _itpBMCDepth), state[i].cp);
   if (isForceUnreachable()) _itpBMCSvr->assumeProperty(_itpForceUAct);
   // Check Reachability
   if (profileON()) _solveStat->start();
   const bool result = _itpBMCSvr->assump_solve();
   if (profileON()) _solveStat->end();
   return result;
}

V3CITPCube* const
V3VrfCITP::extractNewCube(V3CITPCube* const nextCube) {
   // This function can ONLY be called after SAT of (R ^ T ^ nextCube') and generalize curCube from R
   V3CITPCube* const cube = new V3CITPCube(nextCube); assert (cube);  // Create Cube
   generalizeSimulation(cube, nextCube);  // Apply Simulation for the witness
   assert (cube); return cube;
}

const bool
V3VrfCITP::resolveInitial(V3CITPCube* const cube, const V3SvrDataSet& coreProofVars) {
   // Get Proof Related State Variables
   assert (cube); //if (coreProofVars.size() == cube->getState().size()) return;
   // Store Values of State Variable
   const V3NetVec& state = cube->getState();
   // Remove Variables to Form New State
   V3NetVec newState; newState.reserve(state.size());
   for (uint32_t i = 0; i < state.size(); ++i)
      if (coreProofVars.end() != coreProofVars.find(_itpBMCSvr->getFormula(_vrfNtk->getLatch(state[i].id), _itpBMCDepth))) 
         newState.push_back(state[i]);
   // Collect Blocked Cubes Intersect with the newState
   V3UI32Vec intersectCubes; intersectCubes.clear();
   for (uint32_t i = 0; i < _itpBlockCubes.size(); ++i) {
      uint32_t j = 0, k = 0; bool conflict = false;
      while (j < newState.size() && k < _itpBlockCubes[i].size()) {
         if (newState[j].id == _itpBlockCubes[i][k].id) {
            if (newState[j].cp ^ _itpBlockCubes[i][k].cp) { conflict = true; break; }
            ++j; ++k;
         }
         else if (newState[j].id < _itpBlockCubes[i][k].id) ++j;
         else ++k;
      }
      if (!conflict) intersectCubes.push_back(i);
   }
   // Eliminate All Conflict Cubes
   if (intersectCubes.size()) {
      // Compute Conflict Cube, i.e. state \ newState
      V3NetVec conflictCube; conflictCube.clear();
      uint32_t i = 0, j = 0;
      while (i < state.size() && j < newState.size()) {
         if (state[i].id < newState[j].id) { conflictCube.push_back(state[i]); ++i; }
         else if (state[i].id > newState[j].id) ++j;
         else { assert (state[i].cp == newState[j].cp); ++i; ++j; }
      }
      while (i < state.size()) { conflictCube.push_back(state[i]); ++i; }
      if (!conflictCube.size()) return false;  // Can never eliminate
      // Resolve the State with Less 0/1 Recovery
      if (!setBlockingStruct(conflictCube, intersectCubes)) return false; _itpGSt->startVarOrder(conflictCube);
      V3Map<uint32_t, bool>::Map sortedState; sortedState.clear();
      for (i = 0; i < state.size(); ++i) sortedState.insert(make_pair(state[i].id, state[i].cp));
      assert (state.size() == sortedState.size()); V3NetId id; uint32_t cost = 0;
      while (true) {
         // Try Removing A State Variable on this Cube
         id = _itpGSt->getNextVar(cost); if (cost > 1 || V3NetUD == id) break;
         // Remove Generalized Variable from sortedState
         assert (sortedState.end() != sortedState.find(id.id));
         sortedState.erase(sortedState.find(id.id)); _itpGSt->updateVarOrder(true);
      }
      // Compute Final State
      assert (newState.size() < sortedState.size());
      newState.clear(); newState.reserve(sortedState.size());
      for (V3Map<uint32_t, bool>::Map::const_iterator it = sortedState.begin(); it != sortedState.end(); ++it)
         newState.push_back(V3NetId::makeNetId(it->first, it->second));
   }
   // Resolve Intersection with Initial State
   if (newState.size() < state.size()) cube->setState(newState); return true;
}

void
V3VrfCITP::addBlockedCube(V3CITPCube* const cube) {
   assert (cube); const V3NetVec& state = cube->getState(); assert (state.size());
   V3SvrDataVec formula; formula.clear(); formula.reserve(state.size()); size_t fId;
   for (uint32_t i = 0; i < state.size(); ++i) {
      fId = _itpITPSvr->getFormula(_vrfNtk->getLatch(state[i].id), 0);
      formula.push_back(state[i].cp ? fId : _itpITPSvr->getNegFormula(fId));
      ++_itpPriority[state[i].id];  // Increase Signal Priority
   }
   _itpITPSvr->assertImplyUnion(formula);
}

void
V3VrfCITP::blockBadCubes(const uint32_t& index) {
   // This function blocks existing bad cubes that are proven reachable from BMC solver
   // i.e. These cubes are not welcomed to be contained in the future interpolants
   assert (_itpBadCount); assert (index < _itpBlockCubes.size());
   uint32_t depth = _itpFrame.size() - 1; assert (depth);
   uint32_t start = index, end = index + 1, k, succStart = _itpBlockCubes.size();
   while (depth--) {
      const V3CITPCubeList& prevCubes = _itpFrame[depth]->getCubeList(); assert (prevCubes.size());
      k = succStart; assert (_itpBlockCubes.size() == k);
      // For each bad cube, find its sucessor states if possible
      for (; start != end; ++start) {
         V3CITPCubeList::const_iterator it = prevCubes.begin();
         if (isBlockBadCountIndep()) k = _itpBlockCubes.size();
         for (; it != prevCubes.end(); ++it) {
            _itpITPSvr->assumeRelease(); assert (*it);
            const V3NetVec& state = (*it)->getState(); assert (state.size());
            // Assert cubes in _itpFrame[depth] into ITP solver
            if (_itpBad == *it) _itpITPSvr->assumeProperty(state[0], false, 0);
            else {
               for (uint32_t i = 0; i < state.size(); ++i)
                  _itpITPSvr->assumeProperty(_itpITPSvr->getFormula(_vrfNtk->getLatch(state[i].id), 1), state[i].cp);
            }
            // Assert _itpBlockCubes[start]
            for (uint32_t i = 0; i < _itpBlockCubes[start].size(); ++i)
               _itpITPSvr->assumeProperty(_itpITPSvr->getFormula(_vrfNtk->getLatch(_itpBlockCubes[start][i].id), 0),
                                          _itpBlockCubes[start][i].cp);
            // If SAT, find state in _itpFrame[depth]
            if (_itpITPSvr->assump_solve()) {
               // Push badCube into _itpBlockCubes
               _itpBlockCubes.push_back(V3NetVec(_vrfNtk->getLatchSize()));
               for (uint32_t i = 0, j = _vrfNtk->getLatchSize(); i < j; ++i)
                  _itpBlockCubes.back()[i] = V3NetId::makeNetId(i, 
                                             '0' == _itpITPSvr->getDataValue(_vrfNtk->getLatch(i), 1)[0]);
#ifdef V3_CITP_ADD_REPEATED_SREF_ONLY
               const string stateStr = getStateStr(_itpBlockCubes.back());
               V3StrSet::iterator ix = _itpBadCubeStr.find(stateStr);
               if (_itpBadCubeStr.end() != ix) _itpBadCubeStr.erase(ix);
               else { _itpBadCubeStr.insert(stateStr); _itpBlockCubes.pop_back(); }
#endif
               if (isRecycleInterpolants()) (*it)->clearState();
               if (_itpBadCount == (_itpBlockCubes.size() - k)) break;
            }
         }
      }
      if (_itpBlockCubes.size() == succStart) break;
      start = succStart; end = succStart = _itpBlockCubes.size();
   }
}

// CITP Generalization Functions
const bool
V3VrfCITP::setBlockingStruct(const V3NetVec& baseCube) {
   assert (_itpGSt); _itpGSt->clear(baseCube);
   for (uint32_t i = 0; i < _itpBlockCubes.size(); ++i) {
      if (!_itpGSt->pushCube(_itpBlockCubes[i])) {
         blockBadCubes(i); return false;
      }
   }
   return true;
}

const bool
V3VrfCITP::setBlockingStruct(const V3NetVec& baseCube, const V3UI32Vec& blockIndex) {
   assert (_itpGSt); _itpGSt->clear(baseCube);
   for (uint32_t i = 0; i < blockIndex.size(); ++i) {
      assert (_itpBlockCubes.size() > blockIndex[i]);
      if (!_itpGSt->pushCube(_itpBlockCubes[blockIndex[i]])) {
         blockBadCubes(blockIndex[i]); return false;
      }
   }
   return true;
}

void
V3VrfCITP::generalizeSimulation(V3CITPCube* const cube, const V3CITPCube* const nextCube) {
   assert (cube); assert (nextCube); assert (_itpSim);
   // Set Values for Simulator
   for (uint32_t i = 0; i < _vrfNtk->getInputSize(); ++i) {
      if (!_itpITPSvr->existVerifyData(_vrfNtk->getInput(i), 0)) _itpSim->clearSource(_vrfNtk->getInput(i), true);
      else _itpSim->setSource(_vrfNtk->getInput(i), _itpITPSvr->getDataValue(_vrfNtk->getInput(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getInoutSize(); ++i) {
      if (!_itpITPSvr->existVerifyData(_vrfNtk->getInout(i), 0)) _itpSim->clearSource(_vrfNtk->getInout(i), true);
      else _itpSim->setSource(_vrfNtk->getInout(i), _itpITPSvr->getDataValue(_vrfNtk->getInout(i), 0));
   }
   for (uint32_t i = 0; i < _vrfNtk->getLatchSize(); ++i) {
      if (!_itpITPSvr->existVerifyData(_vrfNtk->getLatch(i), 0)) _itpSim->clearSource(_vrfNtk->getLatch(i), true);
      else _itpSim->setSource(_vrfNtk->getLatch(i), _itpITPSvr->getDataValue(_vrfNtk->getLatch(i), 0));
   }
   _itpSim->simulate();
   
   // Perform SAT Generalization
   if (_itpFrame.size() > 2) _itpGen->setTargetNets(V3NetVec(), nextCube->getState());
   else {
      V3NetVec constrCube(1, nextCube->getState()[0]);
      assert (1 == nextCube->getState().size()); _itpGen->setTargetNets(constrCube);
   }
   // Set Priority
   V3UI32Vec prioNets; prioNets.clear(); prioNets.reserve(_itpPriority.size());
   for (uint32_t i = 0; i < _itpPriority.size(); ++i) if (!_itpPriority[i]) prioNets.push_back(i);
   for (uint32_t i = 0; i < _itpPriority.size(); ++i) if ( _itpPriority[i]) prioNets.push_back(i);
   _itpGen->performSetXForNotCOIVars(); _itpGen->performXPropForExtensibleVars(prioNets);
   cube->setState(_itpGen->getGeneralizationResult());
}

const bool
V3VrfCITP::removeFromProof(V3CITPCube* const cube) {
   // This function can ONLY be called after UNSAT of (R ^ T ^ cube')
   // Generate UNSAT Source from Solver if Possible
   V3SvrDataVec coreProofVars; coreProofVars.clear(); _itpBMCSvr->getDataConflict(coreProofVars);
   if (!coreProofVars.size()) return true;  // Solver does not Support Analyze Conflict
   V3SvrDataSet coreProofVarSet; coreProofVarSet.clear();
   for (uint32_t i = 0; i < coreProofVars.size(); ++i) coreProofVarSet.insert(coreProofVars[i]);
   // Get Generalized State from Solver Proof
   const bool isValid = resolveInitial(cube, coreProofVarSet);
   assert (!checkBMCReachability(cube->getState())); return isValid;
}

const bool
V3VrfCITP::generalizeProof(V3CITPCube* const cube) {
   // Apply SAT Solving to further generalize cube
   // Remove Variables from cube after Proof Success
   V3NetVec state(cube->getState()); V3NetId id;
   V3UI32Vec stateIndex(_vrfNtk->getLatchSize(), V3NtkUD); uint32_t cost = 0;
   for (uint32_t i = 0; i < state.size(); ++i) stateIndex[state[i].id] = i;
   if (!setBlockingStruct(state)) return false; _itpGSt->startVarOrder(state);
   while (true) {
      // Try Removing A State Variable on this Cube
      id = _itpGSt->getNextVar(cost); if (cost > 1 || V3NetUD == id) break;
      assert (state.size() > stateIndex[id.id]);
      // Remove this Variable from state
      if (state.size() != (1 + stateIndex[id.id])) state[stateIndex[id.id]] = state.back(); state.pop_back();
      if (checkBMCReachability(state)) {
         state.push_back(state[stateIndex[id.id]]); assert (state.size() >= (1 + stateIndex[id.id]));
         if (state.size() != (1 + stateIndex[id.id])) state[stateIndex[id.id]] = id;
         _itpGSt->updateVarOrder(false);
      }
      else {
         if (state.size() > (1 + stateIndex[id.id])) {
            uint32_t i = stateIndex[id.id]; id = state[stateIndex[id.id]];
            for (; i < (state.size() - 1); ++i) {
               assert ((i + 1) == stateIndex[state[i + 1].id]);
               state[i] = state[1 + i]; stateIndex[state[i].id] = i;
            }
            state.back() = id;
         }
         if (state.size()) stateIndex[state.back().id] = state.size() - 1;
         _itpGSt->updateVarOrder(true);
      }
   }
   if (state.size() < cube->getState().size()) cube->setState(state);
   assert (!checkBMCReachability(cube->getState())); return true;
}

// CITP Debug Functions
void
V3VrfCITP::printState(const V3NetVec& state) const {
   for (uint32_t i = 0; i < state.size(); ++i)
      Msg(MSG_IFO) << (state[i].cp ? "~" : "") << state[i].id << " ";
}

const string
V3VrfCITP::getStateStr(const V3NetVec& state) const {
   string str(_vrfNtk->getLatchSize(), 'X');
   for (uint32_t i = 0; i < state.size(); ++i) {
      assert (str.size() > state[i].id);
      str[state[i].id] = (state[i].cp ? '0' : '1');
   }
   return str;
}

#endif

