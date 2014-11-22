/****************************************************************************
  FileName     [ v3VrfSEC.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Sequential Equivalence Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SEC_C
#define V3_VRF_SEC_C

#include "v3VrfSEC.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3VrfMPDR.h"

/* -------------------------------------------------- *\
 * Class V3VrfSEC Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfSEC::V3VrfSEC(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   assert (handler); assert (handler->getNtk());
   // Private Data Members
   _secNtk = copyV3Ntk(handler->getNtk()); assert (_secNtk); _secNtk->createOutput(V3NetId::makeNetId(0));
   _secHandler = new V3NtkHandler(0, _secNtk); assert (_secHandler); _secSim = 0; _secChecker = 0;
   // Private Mapping Tables
   _p2cMap.clear(); _c2pMap.clear();
   // Private EC Pair Storage Data
   _secCandidate.clear(); _secCheckFail.clear(); _secEQSize = 0;
   _secSynthesis = 0; _secIsCECorSEC = 0; _secCECAssumed = 0; _secSECAssumed = 0; _secLastDepth = 0;
   // Statistics
   if (profileON()) {
      _totalStat = new V3Stat("TOTAL");
      _simStat   = new V3Stat("SIMULATION",   _totalStat);
      _simpStat  = new V3Stat("NTK SIMPLIFY", _totalStat);
      _checkStat = new V3Stat("FORMAL CHECK", _totalStat);
      _specuStat = new V3Stat("SPECULATIVE",  _totalStat);
   }
}

V3VrfSEC::~V3VrfSEC() {
   // Private Data Members
   if (_secHandler) delete _secHandler; _secHandler = 0; if (_secSim) delete _secSim; _secSim = 0;
   if (_secChecker) delete _secChecker; _secChecker = 0; _secNtk = 0;
   // Private Mapping Tables
   _p2cMap.clear(); _c2pMap.clear();
   // Private EC Pair Storage Data
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) _secCandidate[i].clear();
   _secCandidate.clear(); _secCheckFail.clear();
   // Statistics
   if (profileON()) {
      if (_totalStat) delete _totalStat;
      if (_simStat  ) delete _simStat;
      if (_simpStat ) delete _simpStat;
      if (_checkStat) delete _checkStat;
      if (_specuStat) delete _specuStat;
   }
}

// Verification Main Functions
void
V3VrfSEC::startVerify(const uint32_t& p) {
   // Check Shared Results
   if (!isSynthesisMode() && (_sharedBound && V3NtkUD == _sharedBound->getBound(p))) return;
   
   // Clear Verification Results
   clearResult(p); if (profileON()) _totalStat->start();
   
   // Consistency Check
   consistencyCheck();
   if (_secChecker && !isValidChecker()) return;

   // Initialize Simulator
   if (!_secSim) {
      if (dynamic_cast<V3BvNtk*>(_secNtk)) _secSim = new V3AlgBvSimulate(_secHandler);
      else _secSim = new V3AlgAigSimulate(_secHandler); assert (_secSim);
   }

   // Initialize Checker
   if (_secChecker) {
      _secChecker->setSolver(getSolver());
      _secChecker->setIncKeepSilent(true);
      if (_sharedInv) _secChecker->setSharedInv(_sharedInv);
   }

   // Initialize EC Pair Storage Data
   _secCandidate.clear(); _secCheckFail.clear(); _secEQSize = 0; _secLastDepth = 1;

   // Initialize Parameters
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _secNtk->getOutput(p); assert (V3NetUD != pId);
   const uint32_t secIndex = _secNtk->getOutputSize() - 1;
   const string flushSpace = string(100, ' ');
   uint32_t proved = V3NtkUD, fired = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);

   // Initialize CEC or SEC Mode
   if (isAssumeCECMiter()) setInternalCEC();
   else if (isAssumeSECMiter()) setInternalSEC();
   
   // Initialize Unsolved List
   V3UI32Vec unsolved; unsolved.clear(); unsolved.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i) 
      unsolved.push_back((_result[i].isCex() || _result[i].isInv()) ? V3NtkUD : 0);

   // Start SEC Based Verification
   bool continueOnSolver = false, keepReachability = true, initValid = true;
   V3RepIdHash repIdHash; repIdHash.clear(); uint32_t noProof = 0;
   while (_maxDepth >= _secLastDepth) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
      // Check Shared Results
      if (!isSynthesisMode() && _sharedBound && (V3NtkUD == _sharedBound->getBound(p))) break;
      // Random Simulation for Update EC Candidates
      if (profileON()) _simStat->start(); randomSimulate(p, _secLastDepth << 1, initValid); initValid = false;
      if (profileON()) _simStat->end(); if (_result[p].isCex()) { fired = _secLastDepth; break; }
      // Formal Verification on SEC Candidates
      while (_secChecker) {
         const uint32_t x = getTargetCandidate(); if (V3NtkUD == x) break;
         assert (_secCandidate.size() > x); assert (1 < _secCandidate[x].size());
         keepReachability = (++noProof <= (uint32_t)(sqrt(_secCandidate.size())));
         // Replace the SEC Output by Equating SEC Candidates
         replaceSECOutput(x);
         // Run Formal Checker
         if (profileON()) _checkStat->start();
         _secChecker->setMaxTime(_maxTime);
         _secChecker->setMaxDepth(_secLastDepth);
         _secChecker->setIncKeepLastReachability( keepReachability);
         _secChecker->setIncLastDepthToKeepGoing(_secLastDepth >> 1);
         _secChecker->setIncContinueOnLastSolver( continueOnSolver);
         _secChecker->verifyProperty(secIndex); continueOnSolver = true;
         if (profileON()) _checkStat->end();
         // Update SEC Candidates According to the Result
         if (_secChecker->getResult(secIndex).isCex()) {
            V3CexTrace* const cex = _secChecker->getResult(secIndex).getCexTrace();
            assert (cex); if (profileON()) _simStat->start();
            addSimulationTrace(*cex); if (profileON()) _simStat->end();
            if (_result[p].isCex()) { fired = _secLastDepth; break; }
         }
         else if (_secChecker->getResult(secIndex).isInv()) {
            // Update Candidate List
            updateEQList(x, repIdHash); noProof = 0;
            // Check if Some Properties are EQ with CONST 0
            if (_result[p].isInv()) { proved = _secLastDepth; break; }
            // Update SEC Network
            replaceSECNetwork(repIdHash); continueOnSolver = false;
         }
         else _secCheckFail[x] = true;

         // Report Verification Progress
         if (!isIncKeepSilent() && intactON()) {
            if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
            Msg(MSG_IFO) << (isInternalCEC() ? "CEC-based " : isInternalSEC() ? "SEC-based " : "")
                         << "Verification on Depth = " << _secLastDepth << ": " 
                         << "Candidate Sets EQ / Remaining = " << _secEQSize << " / " << _secCandidate.size();
            if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
         }
      }
      if (_result[p].isCex() || _result[p].isInv()) break;
      if (!isSynthesisMode() && _sharedBound && (V3NtkUD == _sharedBound->getBound(p))) break;

      // Check Correctness
      //assert (isValidCandidates(_secLastDepth));

      // Perform Speculative Reduction
      if (profileON()) _specuStat->start(); speculativeReduction(_secLastDepth);
      if (profileON()) _specuStat->end(); continueOnSolver = false;
      
      // Check if Some Properties are EQ with CONST 0
      if (_sharedBound) _sharedBound->getBound(unsolved);
      for (uint32_t i = 0; i < unsolved.size(); ++i) {
         if (isSynthesisMode() || V3NtkUD == unsolved[i] || _result[i].isCex() || _result[i].isInv()) continue;
         if (!_secNtk->getOutput(i).id) {
            assert (!_secNtk->getOutput(i).cp); _result[i].setIndInv(_vrfNtk);
         }
      }
      if (_result[p].isInv()) { proved = _secLastDepth; break; }

      // Update Simplified Network to Shared Ntk
      if (_sharedNtk) {
         const uint32_t sharedNtkSize = _sharedNtk->getNetSize();
         const bool update = sharedNtkSize ? (sharedNtkSize > _secNtk->getNetSize()) 
                                           : (_vrfNtk->getNetSize() > _secNtk->getNetSize());
         if (update) _sharedNtk->updateNtk(_secNtk, _constr);
      }

      // Report Verification Progress
      if (!isIncKeepSilent() && intactON()) {
         if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << (isInternalCEC() ? "CEC-based " : isInternalSEC() ? "SEC-based " : "")
                      << "Verification on Depth = " << _secLastDepth << ": " 
                      << "Candidate Sets EQ / Remaining = " << _secEQSize << " / " << _secCandidate.size();
         if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
      if (!_secCandidate.size()) {
         if (isInternalCEC() && isAssumeSECMiter()) { setInternalSEC(); _secLastDepth = 1; initValid = true; }
         else break;
      }
      else _secLastDepth *= 2;
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != proved) Msg(MSG_IFO) << "Inductive Invariant found at depth = " << proved;
      else if (V3NtkUD != fired) Msg(MSG_IFO) << "Counter-example found at depth = " << fired;
      else Msg(MSG_IFO) << "UNDECIDED at depth = " << _secLastDepth;
      if (usageON()) {
         gettimeofday(&curtime, NULL);
         Msg(MSG_IFO) << "  (time = " << setprecision(5) << getTimeUsed(inittime, curtime) << "  sec)" << endl;
      }
      if (profileON()) {
         _totalStat->end();
         Msg(MSG_IFO) << *_simStat << endl;
         Msg(MSG_IFO) << *_checkStat << endl;
         Msg(MSG_IFO) << *_simpStat << endl;
         Msg(MSG_IFO) << *_totalStat << endl;
      }
   }
}

// Private EC Pair Simulation Functions
void
V3VrfSEC::randomSimulate(const uint32_t& p, const uint32_t& cycle, const bool& initValid) {
   // Initialize Simulation Trace Data
   V3VrfSimTraceVec traceData; traceData.clear(); assert (_secSim);
   V3SimTrace initData; initData.clear(); V3CexTrace* cex = 0;
   // Initialize Unsolved List
   V3UI32Vec unsolved; unsolved.clear(); unsolved.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i) 
      unsolved.push_back((_result[i].isCex() || _result[i].isInv()) ? V3NtkUD : 0);
   // Initialize the List of Uninitialized Latches
   V3UI32Vec uninitLatch; uninitLatch.clear();
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i)
      if (_secNtk->getLatch(i).id == _secNtk->getInputNetId(_secNtk->getLatch(i), 1).id) uninitLatch.push_back(i);
   // Start Random Simulation
   uint32_t noImprove = 0, size = _secCandidate.size();
   V3BitVecX tempValue; bool init = initValid, update = false;
   while (noImprove < 3) {
      // Reset Simulator to an Initial State
      for (uint32_t i = 0; i < traceData.size(); ++i) traceData[i].clear(); traceData.clear(); traceData.reserve(cycle);
      initData.clear(); initData.reserve(_secNtk->getLatchSize());
      V3NetVec simTargets; simTargets.clear(); simTargets.reserve(unsolved.size());
      for (uint32_t i = 0; i < unsolved.size(); ++i)
         if (V3NtkUD != unsolved[i]) simTargets.push_back(_secNtk->getOutput(i));
      _secSim->reset(simTargets); _secSim->updateNextStateValue();
      for (uint32_t i = 0; i < uninitLatch.size(); ++i) _secSim->clearSource(_secNtk->getLatch(uninitLatch[i]), true);
      // Record Initial State
      for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) initData.push_back(_secSim->getSimValue(_secNtk->getLatch(i)));
      // Try Random Simulate a Number of Cycles
      for (uint32_t i = 0, k = 1 + (cycle >> 1); i < cycle; ++i) {
         // Assign Random Input Values
         _secSim->setSourceFree(V3_PI, true);
         _secSim->setSourceFree(V3_PIO, true);
         // Simulate for One Cycle
         _secSim->simulate(); updateECPair(init, update); init = update = false;
         if (size == _secCandidate.size() && !(--k)) { ++noImprove; break; } size = _secCandidate.size();
         // Record Trace Data
         traceData.push_back(V3SimTrace()); traceData.back().reserve(_secNtk->getInputSize() + _secNtk->getInoutSize());
         for (uint32_t j = 0; j < _secNtk->getInputSize(); ++j) 
            traceData.back().push_back(_secSim->getSimValue(_secNtk->getInput(j)));
         for (uint32_t j = 0; j < _secNtk->getInoutSize(); ++j) 
            traceData.back().push_back(_secSim->getSimValue(_secNtk->getInout(j)));
         // Check if Property Asserted
         for (uint32_t j = 0, idx; j < unsolved.size(); ++j) {
            if (isSynthesisMode() || V3NtkUD == unsolved[j]) continue;
            tempValue = _secSim->getSimValue(_secNtk->getOutput(j));
            idx = tempValue.first1(); if (tempValue.size() == idx) continue; assert (tempValue.exist1());
            // Record Counterexample for Property j
            cex = computeTrace(initData, traceData, idx); assert (cex);
            // Record the CounterExample Trace
            _result[j].setCexTrace(cex); unsolved[j] = 0; update = true;
         }
         if (_result[p].isCex()) { noImprove = V3NtkUD; break; }
         // Update FF Next State Values
         _secSim->updateNextStateValue();
      }
   }
}

void
V3VrfSEC::addSimulationTrace(const V3CexTrace& cex, const bool& checkValid) {
   V3SimTrace initData; V3VrfSimTraceVec traceData;
   computeTrace(cex, initData, traceData);
   addSimulationTrace(initData, traceData, checkValid);
}

void
V3VrfSEC::addSimulationTrace(const V3SimTrace& initData, const V3VrfSimTraceVec& traceData, const bool& checkValid) {
   assert (traceData.size()); assert (_secSim);
   // Initialize Simulator
   V3NetVec simTargets; simTargets.clear(); simTargets.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i) simTargets.push_back(_secNtk->getOutput(i));
   _secSim->reset(simTargets); _secSim->updateNextStateValue();
   // Simulate the Trace
   for (uint32_t i = 0, j; i < traceData.size(); ++i) {
      if (!i) {
         // Set Initial State Values
         assert (_secNtk->getLatchSize() == initData.size());
         for (j = 0; j < _secNtk->getLatchSize(); ++j) _secSim->setSource(_secNtk->getLatch(j), initData[j]);
      }
      // Set Trace Values
      assert ((_secNtk->getInputSize() + _secNtk->getInoutSize()) == traceData[i].size());
      for (j = 0; j < _secNtk->getInputSize(); ++j) _secSim->setSource(_secNtk->getInput(j), traceData[i][j]);
      for (; j < _secNtk->getInoutSize(); ++j) _secSim->setSource(_secNtk->getInout(j), traceData[i][j]);
      // Simulate for One Cycle
      _secSim->simulate(); bool update = false;
      if (checkValid) {
         // Check if Property Asserted
         for (uint32_t j = 0; j < _result.size(); ++j) {
            if (isSynthesisMode() || _result[j].isCex() || _result[j].isInv()) continue;
            if ('1' != _secSim->getSimValue(_secNtk->getOutput(j))[0]) continue;
            // Record Counterexample for Property j
            V3VrfSimTraceVec subTraceData; subTraceData.clear(); subTraceData.reserve(1 + i);
            for (uint32_t x = 0; x <= i; ++x) subTraceData.push_back(traceData[x]);
            V3CexTrace* const cex = computeTrace(initData, subTraceData, 0); assert (cex);
            // Record the CounterExample Trace
            _result[j].setCexTrace(cex); assert (_result[j].isCex()); update = true;
         }
      }
      updateECPair(false, update);
      // Update FF Next State Values
      _secSim->updateNextStateValue();
   }
}

V3CexTrace* const
V3VrfSEC::getSolverCexTrace(V3SvrBase* const solver, const uint32_t& cycle) {
   V3CexTrace* const cex = new V3CexTrace(cycle); assert (cex);
   uint32_t patternSize = _secNtk->getInputSize() + _secNtk->getInoutSize();
   V3BitVecX dataValue, patternValue(patternSize ? patternSize : 1);
   // Set Pattern Value
   for (uint32_t i = 0; i < cycle; ++i) {
      patternSize = 0; patternValue.clear();
      for (uint32_t j = 0; j < _secNtk->getInputSize(); ++j, ++patternSize) {
         if (!solver->existVerifyData(_secNtk->getInput(j), i)) continue;
         dataValue = solver->getDataValue(_secNtk->getInput(j), i);
         if ('0' == dataValue[0]) patternValue.set0(patternSize);
         else if ('1' == dataValue[0]) patternValue.set1(patternSize);
      }
      for (uint32_t j = 0; j < _secNtk->getInoutSize(); ++j, ++patternSize) {
         if (!solver->existVerifyData(_secNtk->getInout(j), i)) continue;
         dataValue = solver->getDataValue(_secNtk->getInout(j), i);
         if ('0' == dataValue[0]) patternValue.set0(patternSize);
         else if ('1' == dataValue[0]) patternValue.set1(patternSize);
      }
      assert (!patternSize || patternSize == patternValue.size()); cex->pushData(patternValue);
   }
   // Set Initial State Value
   if (_secNtk->getLatchSize()) {
      patternValue.resize(_secNtk->getLatchSize()); patternValue.clear();
      for (uint32_t j = 0; j < _secNtk->getLatchSize(); ++j) {
         if (!solver->existVerifyData(_secNtk->getLatch(j), 0)) continue;
         dataValue = solver->getDataValue(_secNtk->getLatch(j), 0);
         if ('0' == dataValue[0]) patternValue.set0(j);
         else if ('1' == dataValue[0]) patternValue.set1(j);
      }
      cex->setInit(patternValue);
   }
   return cex;
}

void
V3VrfSEC::computeTrace(const V3CexTrace& cex, V3SimTrace& initData, V3VrfSimTraceVec& traceData) {
   initData.clear(); initData.reserve(_secNtk->getLatchSize());
   traceData.clear(); traceData.reserve(cex.getTraceSize());
   for (uint32_t i = 0; i < cex.getTraceSize(); ++i) traceData.push_back(V3SimTrace());
   // Set Initial State Value
   V3BitVecX pattern; uint32_t k = 0;
   if (cex.getInit()) {
      V3BitVecX* const initValue = cex.getInit(); assert (initValue); k = 0;
      for (uint32_t j = 0; j < _secNtk->getLatchSize(); ++j) {
         initData.push_back(initValue->bv_slice(k, k)); ++k;
         if ('X' == initData.back()[0]) initData.back().random();
         assert ('X' != initData.back()[0]); assert (k <= initValue->size());
      }
   }
   // Set Trace Value
   if (cex.getTraceDataSize()) {
      for (uint32_t i = 0; i < cex.getTraceSize(); ++i) {
         traceData[i].clear(); traceData[i].reserve(_secNtk->getInputSize() + _secNtk->getOutputSize());
         pattern = cex.getData(i); k = 0;
         for (uint32_t j = 0; j < _secNtk->getInputSize(); ++j) {
            traceData[i].push_back(pattern.bv_slice(k, k)); ++k;
            if ('X' == traceData[i].back()[0]) traceData[i].back().random();
            assert ('X' != traceData[i].back()[0]); assert (k <= pattern.size());
         }
         for (uint32_t j = 0; j < _secNtk->getInoutSize(); ++j) {
            traceData[i].push_back(pattern.bv_slice(k, k)); ++k;
            if ('X' == traceData[i].back()[0]) traceData[i].back().random();
            assert ('X' != traceData[i].back()[0]); assert (k <= pattern.size());
         }
      }
   }
}

V3CexTrace* const
V3VrfSEC::computeTrace(const V3SimTrace& initData, const V3VrfSimTraceVec& traceData, const uint32_t& idx) {
   V3CexTrace* const cex = new V3CexTrace(traceData.size()); assert (cex);
   uint32_t patternSize = _secNtk->getInputSize() + _secNtk->getInoutSize();
   uint32_t initSize = _secNtk->getLatchSize(); V3BitVecX tempValue(initSize);
   if (initSize) {
      tempValue.resize(initSize, true);
      for (uint32_t k = 0; k < initSize; ++k) {
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
         for (uint32_t k = 0; k < patternSize; ++k) {
            switch (traceData[v][k][idx]) {
               case '1': tempValue.set1(k); break;
               case '0': tempValue.set0(k); break;
            }
         }
         cex->pushData(tempValue);
      }
   }
   return cex;
}

// Private EC Pair Construction Functions
void
V3VrfSEC::initializeECPair() {
   assert (!_secCandidate.size());
   // Compute Net Levels
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(_result.size());
   for (uint32_t i = 0; i < _result.size(); ++i) targetNets.push_back(_secNtk->getOutput(i));
   V3UI32Vec levelData; levelData.clear(); computeLevel(_secNtk, levelData, targetNets);
   V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map levelNets;
   V3BitVecX value; V3NetId id; levelNets.clear(); levelNets.insert(make_pair(0, 0));
   for (id = V3NetId::makeNetId(1); id.id < levelData.size(); ++id.id) {
      if (V3NtkUD == levelData[id.id]) continue;
      if (isInternalCEC() && (BV_CONST == _secNtk->getGateType(id) || levelData[id.id])) continue;
      if (V3_FF <= _secNtk->getGateType(id)) levelNets.insert(make_pair(1 + levelData[id.id], id.id));
   }
   // Initialize SEC Candidate Sets
   // NOTE: Nets in a Candidate Set is Ordered by Their Levels
   V3Map<string, uint32_t>::Map secCandidate; V3Map<string, uint32_t>::Map::iterator is;
   for (V3Map<uint32_t, uint32_t>::Map::const_iterator it = levelNets.begin(); it != levelNets.end(); ++it) {
      assert (V3NtkUD != it->first); assert (_secNtk->getNetSize() > it->second);
      id = V3NetId::makeNetId(it->second); assert (V3_XD > _secNtk->getGateType(id));
      value = _secSim->getSimValue(id);
      is = secCandidate.find(value.toExp());
      if (secCandidate.end() != is) {
         assert (_secCandidate.size() > is->second);
         assert (levelData[id.id] >= levelData[_secCandidate[is->second].back().id]);
         _secCandidate[is->second].push_back(id); continue;
      }
      is = secCandidate.find((~value).toExp());
      if (secCandidate.end() != is) {
         assert (_secCandidate.size() > is->second);
         assert (levelData[id.id] >= levelData[_secCandidate[is->second].back().id]);
         _secCandidate[is->second].push_back(~id); continue;
      }
      secCandidate.insert(make_pair(value.toExp(), _secCandidate.size()));
      _secCandidate.push_back(V3NetVec(1, id)); _secCheckFail.push_back(false);
   }
}

void
V3VrfSEC::removeECPairOutOfCOI() {
   V3BoolVec m(_secNtk->getNetSize(), false);
   for (uint32_t i = 0; i < _secNtk->getInputSize(); ++i) m[_secNtk->getInput(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) m[_secNtk->getInout(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) m[_secNtk->getLatch(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getConstSize(); ++i) m[_secNtk->getConst(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 1), m);
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 0), m);
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getInout(i), 0), m);
   for (uint32_t i = 0; i < _result.size(); ++i) 
      if (_result[i].isCex() || _result[i].isInv()) continue;
      else dfsMarkFaninCone(_secNtk, _secNtk->getOutput(i), m);
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      for (uint32_t j = 0; j < _secCandidate[i].size(); ++j) {
         if (m[_secCandidate[i][j].id]) continue;
         _secCandidate[i].erase(_secCandidate[i].begin() + j); --j;
      }
   }
}

void
V3VrfSEC::updateECPair(const bool& initValid, const bool& update) {
   V3BitVecX value;
   if (_secCandidate.size()) {
      V3Map<string, uint32_t>::Map secCandidate; V3Map<string, uint32_t>::Map::iterator it;
      for (uint32_t i = 0, k = _secCandidate.size(); i < k; ++i) {
         value = _secSim->getSimValue(_secCandidate[i][0]);
         secCandidate.clear(); secCandidate.insert(make_pair(value.toExp(), i));
         for (uint32_t j = 1; j < _secCandidate[i].size(); ++j) {
            value = _secSim->getSimValue(_secCandidate[i][j]);
            it = secCandidate.find(value.toExp());
            if (secCandidate.end() != it) {
               assert (_secCandidate.size() > it->second); if (i == it->second) continue;
               _secCandidate[it->second].push_back(_secCandidate[i][j]);
            }
            else {
               secCandidate.insert(make_pair(value.toExp(), _secCandidate.size()));
               _secCandidate.push_back(V3NetVec(1, _secCandidate[i][j])); _secCheckFail.push_back(false);
            }
            // Remove from this Bucket
            _secCandidate[i].erase(_secCandidate[i].begin() + j); --j;
         }
      }
   }
   else if (initValid) initializeECPair();
   
   // Update EC Pairs Since Some Properties are Fired
   if (update) removeECPairOutOfCOI();
   // Remove EC Pairs with Less Than One Element
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      if (_secCandidate[i].size() > 1) continue;
      _secCandidate[i] = _secCandidate.back(); _secCandidate.pop_back();
      _secCheckFail[i] = _secCheckFail.back(); _secCheckFail.pop_back(); --i;
   }
   // Remove EC Pairs without Leading V3_FF, BV_CONST, or AIG_FALSE, if isInternalSEC() is true
   if (!isInternalSEC()) return; V3GateType type;
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      assert (_secCandidate[i].size()); type = _secNtk->getGateType(_secCandidate[i][0]);
      if (V3_FF == type || BV_CONST == type || AIG_FALSE == type) continue;
      _secCandidate[i] = _secCandidate.back(); _secCandidate.pop_back();
      _secCheckFail[i] = _secCheckFail.back(); _secCheckFail.pop_back(); --i;
   }
}

void
V3VrfSEC::updateEQList(const uint32_t& x, V3RepIdHash& repIdHash) {
   assert (x < _secCandidate.size()); ++_secEQSize;
   // Compute Hash Table for EC Nets
   const V3NetId id = _secCandidate[x][0];  // Representative
   for (uint32_t i = 1; i < _secCandidate[x].size(); ++i)
      repIdHash.insert(make_pair(_secCandidate[x][i].id, (_secCandidate[x][i].cp ? ~id : id)));
   // Check if Some Properties are EQ with CONST 0
   if (!isSynthesisMode() && (_secCandidate[x][0] == V3NetId::makeNetId(0))) {
      // Get the List of Unsolved Properties
      V3UI32Vec unsolved(_result.size(), 0);
      if (_sharedBound) _sharedBound->getBound(unsolved);
      for (uint32_t i = 0; i < unsolved.size(); ++i) {
         if (V3NtkUD == unsolved[i]) continue;
         for (uint32_t j = 0; j < _secCandidate[x].size(); ++j)
            if (_secNtk->getOutput(i) == _secCandidate[x][j]) {
               _result[i].setIndInv(_vrfNtk);
            }
      }
   }
   // Remove this Candidate
   _secCandidate[x] = _secCandidate.back(); _secCandidate.pop_back();
   _secCheckFail[x] = _secCheckFail.back(); _secCheckFail.pop_back();
}

const uint32_t
V3VrfSEC::getTargetCandidate() {
   uint32_t index = V3NtkUD;
   // Choose an Untested Candidate Set that Involves Fewer Nets
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      assert (_secCandidate[i].size() > 1); if (_secCheckFail[i]) continue;
      if (V3NtkUD == index || _secCandidate[index].size() > _secCandidate[i].size()) {
         index = i; if (2 == _secCandidate[index].size()) break; }
   }
   return index;
}

// Private SEC Network Transformation Functions
void
V3VrfSEC::replaceSECOutput(const uint32_t& x) {
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(_secNtk); V3InputVec inputs(2, V3NetUD);
   V3NetVec eqPairs; eqPairs.clear(); eqPairs.reserve(_secCandidate[x].size() - 1);
   if (isBvNtk) {
      V3NetId id;
      for (uint32_t i = 1; i < _secCandidate[x].size(); ++i) {
         id = _secNtk->createNet(); assert (V3NetUD != id);
         inputs[0] = _secCandidate[x][0]; inputs[1] = _secCandidate[x][i];
         _secNtk->setInput(id, inputs); _secNtk->createGate(BV_EQUALITY, id); eqPairs.push_back(id);
      }
      inputs[0] = eqPairs[0];
      for (uint32_t i = 1; i < eqPairs.size(); ++i) {
         id = _secNtk->createNet(); assert (V3NetUD != id); inputs[1] = eqPairs[i];
         _secNtk->setInput(id, inputs); _secNtk->createGate(BV_AND, id); inputs[0] = id;
      }
   }
   else {
      V3NetId id, id1, id2;
      for (uint32_t i = 1; i < _secCandidate[x].size(); ++i) {
         id1 = _secNtk->createNet(); assert (V3NetUD != id1);
         inputs[0] = _secCandidate[x][0]; inputs[1] = ~_secCandidate[x][i];
         _secNtk->setInput(id1, inputs); _secNtk->createGate(AIG_NODE, id1);
         id2 = _secNtk->createNet(); assert (V3NetUD != id2);
         inputs[0] = ~_secCandidate[x][0]; inputs[1] = _secCandidate[x][i];
         _secNtk->setInput(id2, inputs); _secNtk->createGate(AIG_NODE, id2);
         id = _secNtk->createNet(); assert (V3NetUD != id); inputs[0] = ~id1; inputs[1] = ~id2;
         _secNtk->setInput(id, inputs); _secNtk->createGate(AIG_NODE, id); eqPairs.push_back(id);
      }
      inputs[0] = eqPairs[0];
      for (uint32_t i = 1; i < eqPairs.size(); ++i) {
         id = _secNtk->createNet(); assert (V3NetUD != id); inputs[1] = eqPairs[i];
         _secNtk->setInput(id, inputs); _secNtk->createGate(AIG_NODE, id); inputs[0] = id;
      }
   }
   // Replace Property Output
   _secNtk->replaceOutput(_secNtk->getOutputSize() - 1, ~(inputs[0].id));
}

void
V3VrfSEC::replaceSECNetwork(V3RepIdHash& repIdHash) {
   if (profileON()) _simpStat->start();
   // Update (Unsolved) Property Outputs and Constraints
   V3RepIdHash::const_iterator it; if (profileON()) _simpStat->start();
   for (uint32_t i = 0; i < _result.size(); ++i) {
      it = repIdHash.find(_secNtk->getOutput(i).id); if (repIdHash.end() == it) continue;
      _secNtk->replaceOutput(i, (_secNtk->getOutput(i).cp ? ~(it->second) : it->second));
   }
   for (uint32_t j = 0; j < _constr.size(); ++j) {
      for (uint32_t i = 0; i < _constr[j].size(); ++i) {
         it = repIdHash.find(_constr[j][i].id); if (repIdHash.end() == it) continue;
         _constr[j][i] = _constr[j][i].cp ? ~(it->second) : it->second;
      }
   }

   // Initialize Unsolved List
   V3UI32Vec unsolved(_result.size(), 0); if (_sharedBound) _sharedBound->getBound(unsolved);
   
   // Remove Floating Nets and Update Network
   V3BoolVec m(_secNtk->getNetSize(), false); _secNtk->replaceFanin(repIdHash); repIdHash.clear();
   for (uint32_t i = 0; i < _secNtk->getInputSize(); ++i) m[_secNtk->getInput(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) m[_secNtk->getInout(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) m[_secNtk->getLatch(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getConstSize(); ++i) m[_secNtk->getConst(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 1), m);
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 0), m);
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getInout(i), 0), m);
   for (uint32_t i = 0; i < unsolved.size(); ++i) 
      if (V3NtkUD == unsolved[i] || _result[i].isCex() || _result[i].isInv()) continue;
      else dfsMarkFaninCone(_secNtk, _secNtk->getOutput(i), m);
   // Remove Floating EC Nets from SEC Candidates
   V3NetVec reachableNets;
   for (uint32_t i = 0, j; i < _secCandidate.size(); ++i) {
      assert (_secCandidate[i].size() > 1); reachableNets.clear();
      reachableNets.reserve(_secCandidate[i].size());
      for (j = 0; j < _secCandidate[i].size(); ++j)
         if (m[_secCandidate[i][j].id]) reachableNets.push_back(_secCandidate[i][j]);
      if (reachableNets.size() > 1) _secCandidate[i] = reachableNets;
      else {
         _secCandidate[i] = _secCandidate.back(); _secCandidate.pop_back();
         _secCheckFail[i] = _secCheckFail.back(); _secCheckFail.pop_back(); --i;
      }
   }
   if (profileON()) _simpStat->end();
}

void
V3VrfSEC::simplifySECNetwork(V3NetVec& p2cMap, V3NetVec& c2pMap, const V3NetVec& eqNets) {
   // This Function Assumes EC Candidates are Real, and then
   // Simplify the SEC Network Considering POs and eqNets
   assert (_secNtk); if (profileON()) _simpStat->start();
   // Put Constraints and eqNets to POs
   assert (_secNtk->getOutputSize() == (1 + _result.size())); _secNtk->removeLastOutput();
   for (uint32_t i = 0; i < eqNets.size(); ++i) _secNtk->createOutput(eqNets[i]);
   if (!eqNets.size()) {
      for (uint32_t j = 0; j < _constr.size(); ++j)
         for (uint32_t i = 0; i < _constr[j].size(); ++i) _secNtk->createOutput(_constr[j][i]);
   }
   // Simplify SEC Network
   V3Ntk* const simpNtk = duplicateNtk(_secHandler, p2cMap, c2pMap); assert (simpNtk);
   for (uint32_t i = 0; i < _result.size(); ++i) {
      const V3NetId id = _secNtk->getOutput(i); assert (V3NetUD != p2cMap[id.id]);
      simpNtk->createOutput(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
   }
   replaceV3Ntk(_secNtk, simpNtk); delete simpNtk;
   _secNtk->createOutput(V3NetId::makeNetId(0));
   if (profileON()) _simpStat->end();
}

void
V3VrfSEC::speculativeReduction(const uint32_t& cycle) {
   // Given that All EC Candidates are EQ within cycle Frames from Initial States, 
   // This Function Leaves a Subset of EC Candidates that are Inductive.
   V3Ntk* const secNtk = copyV3Ntk(_secNtk); assert (secNtk);
   V3NetVec eqNets, p2cMap, c2pMap; V3SvrDataVec varList(2);
   const V3NetTable secCandidate = _secCandidate;
   V3Vec<V3CexTrace*>::Vec cexList; V3NetId id;
   V3RepIdHash repIdHash; repIdHash.clear();
   while (_secCandidate.size()) {
      assert (_secNtk->getOutputSize() == (1 + _result.size()));
      // Replace Fanins According to EC Candidates
      for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
         assert (1 < _secCandidate[i].size()); id = _secCandidate[i][0];  // Representative
         for (uint32_t j = 1; j < _secCandidate[i].size(); ++j)
            repIdHash.insert(make_pair(_secCandidate[i][j].id, (_secCandidate[i][j].cp ? ~id : id)));
      }
      _secNtk->replaceFanin(repIdHash); repIdHash.clear();
      // Collect Nets for Equating Each EC Candidate Class
      eqNets.clear(); eqNets.reserve(_secCandidate.size());
      for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
         replaceSECOutput(i); eqNets.push_back(_secNtk->getOutput(_result.size()));
      }
      // Construct Network for Speculative Reduction
      p2cMap.clear(); c2pMap.clear(); simplifySECNetwork(p2cMap, c2pMap, eqNets);
      // Update EQ Nets and Invert Them
      for (uint32_t i = 0; i < eqNets.size(); ++i) {
         id = eqNets[i]; assert (V3NetUD != p2cMap[id.id]);
         eqNets[i] = V3NetId::makeNetId(p2cMap[id.id].id, !(p2cMap[id.id].cp ^ id.cp));
      }
      // Initialize Solver
      V3SvrBase* const solver = allocSolver(getSolver(), _secNtk); assert (solver);
      // Add Logic and Constraints into Solver
      for (uint32_t i = 0; i < cycle; ++i)
         solver->assertBoundedVerifyData(eqNets, i);  // Assume Candidates are Real
      // Check if Candidates are Real in the Next Cycle
      cexList.clear();
      for (uint32_t i = 0; i < eqNets.size(); ++i) {
         solver->addBoundedVerifyData(eqNets[i], cycle); solver->simplify();
         solver->assumeRelease(); solver->assumeProperty(eqNets[i], true, cycle);
         if (solver->assump_solve()) cexList.push_back(getSolverCexTrace(solver, 1 + cycle));
      }
      // Recover Network
      replaceV3Ntk(_secNtk, secNtk); delete solver; if (!cexList.size()) break;
      // Extract Counterexample for Updating EC Candidates
      for (uint32_t i = 0; i < cexList.size(); ++i) {
         if (_secCandidate.size()) addSimulationTrace(*(cexList[i]), false);
         delete cexList[i];
      }
   }

   // Replace Fanins According to EC Candidates
   if (_secCandidate.size()) {
      for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
         assert (1 < _secCandidate[i].size()); id = _secCandidate[i][0];  // Representative
         for (uint32_t j = 1; j < _secCandidate[i].size(); ++j)
            repIdHash.insert(make_pair(_secCandidate[i][j].id, (_secCandidate[i][j].cp ? ~id : id)));
      }
      _secNtk->replaceFanin(repIdHash); repIdHash.clear(); _secEQSize += _secCandidate.size();
   }
   // Update SEC Network Constraints
   p2cMap.clear(); c2pMap.clear(); simplifySECNetwork(p2cMap, c2pMap);
   for (uint32_t j = 0; j < _constr.size(); ++j) {
      for (uint32_t i = 0; i < _constr[j].size(); ++i) {
         id = _constr[j][i]; assert (V3NetUD != p2cMap[id.id]);
         _constr[j][i] = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
      }
   }
   // Update SEC Candidates
   _secCheckFail.clear(); _secCandidate.clear(); _secCandidate.reserve(secCandidate.size());
   for (uint32_t i = 0; i < secCandidate.size(); ++i) {
      assert (secCandidate[i].size() > 1); _secCandidate.push_back(V3NetVec());
      for (uint32_t j = 0; j < secCandidate[i].size(); ++j) {
         id = secCandidate[i][j]; assert (p2cMap.size() > id.id); if (V3NetUD == p2cMap[id.id]) continue;
         assert (c2pMap.size() > p2cMap[id.id].id); if (id.id != c2pMap[p2cMap[id.id].id].id) continue;
         _secCandidate.back().push_back(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
      }
      _secCheckFail.push_back(false); assert (_secCandidate.size() == _secCheckFail.size());
   }
   // Update EC Pairs Since Some Properties are Fired
   removeECPairOutOfCOI();
   // Remove EC Pairs with Less Than One Element
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      if (_secCandidate[i].size() > 1) continue;
      _secCandidate[i] = _secCandidate.back(); _secCandidate.pop_back();
      _secCheckFail[i] = _secCheckFail.back(); _secCheckFail.pop_back(); --i;
   }
   // Update Mapping Tables
   if (!_p2cMap.size()) {
      assert (!_c2pMap.size()); _p2cMap.reserve(_vrfNtk->getNetSize()); _c2pMap.reserve(_vrfNtk->getNetSize());
      for (V3NetId id = V3NetId::makeNetId(0); id.id < _vrfNtk->getNetSize(); ++id.id) {
         _p2cMap.push_back(id); _c2pMap.push_back(id); }
   }
   for (uint32_t i = 0; i < _p2cMap.size(); ++i) {
      if (V3NetUD == _p2cMap[i]) continue; assert (_p2cMap[i].id < p2cMap.size());
      if (V3NetUD == p2cMap[_p2cMap[i].id]) { _p2cMap[i] = V3NetUD; continue; }
      _p2cMap[i] = V3NetId::makeNetId(p2cMap[_p2cMap[i].id].id, _p2cMap[i].cp ^ p2cMap[_p2cMap[i].id].cp);
   }
   for (uint32_t i = 0; i < c2pMap.size(); ++i) {
      if (V3NetUD == c2pMap[i]) continue;
      if (c2pMap[i].id >= _c2pMap.size() || V3NetUD == _c2pMap[c2pMap[i].id]) { c2pMap[i] = V3NetUD; continue; }
      c2pMap[i] = V3NetId::makeNetId(_c2pMap[c2pMap[i].id].id, c2pMap[i].cp ^ _c2pMap[c2pMap[i].id].cp);
   }
   delete secNtk; _c2pMap = c2pMap; p2cMap.clear(); c2pMap.clear();
}

// Private SEC Helper Functions
const bool
V3VrfSEC::isValidChecker() const {
   assert (_secChecker);
   if (_secHandler == _secChecker->getNtkHandler()) return true;
   Msg(MSG_ERR) << "Network handler that model checker works on must be getSECHandler() !!" << endl;
   return false;
}

const bool
V3VrfSEC::isValidNet(const V3NetId& id) const {
   V3BoolVec m(_secNtk->getNetSize(), false);
   for (uint32_t i = 0; i < _secNtk->getInputSize(); ++i) m[_secNtk->getInput(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) m[_secNtk->getInout(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) m[_secNtk->getLatch(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getConstSize(); ++i) m[_secNtk->getConst(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 1), m);
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 0), m);
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getInout(i), 0), m);
   for (uint32_t i = 0; i < _result.size(); ++i) 
      if (_result[i].isCex() || _result[i].isInv()) continue;
      else dfsMarkFaninCone(_secNtk, _secNtk->getOutput(i), m);
   return m[id.id];
}

const bool
V3VrfSEC::isValidInvariant(const uint32_t& x) {
   // Check Correctness
   V3Ntk* const ntk = copyV3Ntk(_secNtk); assert (ntk); replaceV3Ntk(_secNtk, _vrfNtk);
   const V3NetVec candi = _secCandidate[x];
   for (uint32_t i = 0; i < _secCandidate[x].size(); ++i) {
      const V3NetId id = _secCandidate[x][i];
      if (_c2pMap.size()) {
         assert (id.id < _c2pMap.size()); assert (V3NetUD != _c2pMap[id.id]);
         _secCandidate[x][i] = V3NetId::makeNetId(_c2pMap[id.id].id, _c2pMap[id.id].cp ^ id.cp);
      }
   }
   replaceSECOutput(x); bool ok = true;

   V3VrfMPDR* const checker = new V3VrfMPDR(_secHandler); assert (checker);
   checker->setSolver(getSolver()); checker->setIncKeepSilent(true);
   checker->setMaxTime(_maxTime); checker->setMaxDepth(V3NtkUD); checker->verifyProperty(0);
   if (checker->getResult(0).isCex()) {
      Msg(MSG_IFO) << "FOUND CEX to Proven Candidate " << x << " !!" << endl; ok = false;
   }
   delete checker; replaceV3Ntk(_secNtk, ntk); _secCandidate[x] = candi; delete ntk;
   return ok;
}

const bool
V3VrfSEC::isValidCandidates(const uint32_t& cycle) const {
   // Check if there exists two elements of a candidate that could be NEQ
   V3NetId id1, id2; bool ok = true;
   for (uint32_t j = 0; j < _secCandidate.size(); ++j) {
      for (uint32_t k = 1; k < _secCandidate[j].size(); ++k) {
         id1 = _secCandidate[j][0]; id2 = _secCandidate[j][k];
         if (_c2pMap.size()) {
            assert (id1.id < _c2pMap.size()); assert (V3NetUD != _c2pMap[id1.id]);
            id1 = V3NetId::makeNetId(_c2pMap[id1.id].id, _c2pMap[id1.id].cp ^ id1.cp);
            assert (id2.id < _c2pMap.size()); assert (V3NetUD != _c2pMap[id2.id]);
            id2 = V3NetId::makeNetId(_c2pMap[id2.id].id, _c2pMap[id2.id].cp ^ id2.cp);
         }
         V3SvrBase* const solver = allocSolver(getSolver(), _vrfNtk); assert (solver);
         for (uint32_t x = 0; x < cycle; ++x) {
            solver->addBoundedVerifyData(id1, x); solver->addBoundedVerifyData(id2, x);
            solver->assumeRelease(); solver->assumeInit();
            solver->assumeProperty(id1, false, x); solver->assumeProperty(id2, true, x);
            if (solver->assump_solve()) {
               Msg(MSG_IFO) << "Found CEX @ " << x << " in Candidate " << j << " : "
                            << (_secCandidate[j][0].cp ? "~" : "") << _secCandidate[j][0].id << " != "
                            << (_secCandidate[j][k].cp ? "~" : "") << _secCandidate[j][k].id << " != "
                            << endl; ok = false;
            }
            solver->assumeRelease(); solver->assumeInit();
            solver->assumeProperty(id1, true, x); solver->assumeProperty(id2, false, x);
            if (solver->assump_solve()) {
               Msg(MSG_IFO) << "Found CEX @ " << x << " in Candidate " << j << " : "
                            << (_secCandidate[j][0].cp ? "~" : "") << _secCandidate[j][0].id << " != "
                            << (_secCandidate[j][k].cp ? "~" : "") << _secCandidate[j][k].id << " != "
                            << endl; ok = false;
            }
         }
         delete solver;
      }
   }
   return ok;
}

void
V3VrfSEC::printCandidate(const uint32_t& x) const {
   assert (x < _secCandidate.size());
   Msg(MSG_IFO) << "Candidate[" << x << "] = ";
   for (uint32_t i = 0; i < _secCandidate[x].size(); ++i)
      Msg(MSG_IFO) << (i ? " " : "") << (_secCandidate[x][i].cp ? "~" : "") << _secCandidate[x][i].id;
   Msg(MSG_IFO) << endl;
}

const string
V3VrfSEC::getCOISize() {
   V3BoolVec m(_secNtk->getNetSize(), false);
   for (uint32_t i = 0; i < _secNtk->getInputSize(); ++i) m[_secNtk->getInput(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) m[_secNtk->getInout(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) m[_secNtk->getLatch(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getConstSize(); ++i) m[_secNtk->getConst(i).id] = true;
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 1), m);
   for (uint32_t i = 0; i < _secNtk->getLatchSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getLatch(i), 0), m);
   for (uint32_t i = 0; i < _secNtk->getInoutSize(); ++i) 
      dfsMarkFaninCone(_secNtk, _secNtk->getInputNetId(_secNtk->getInout(i), 0), m);
   for (uint32_t i = 0; i < _result.size(); ++i) 
      if (_result[i].isCex() || _result[i].isInv()) continue;
      else dfsMarkFaninCone(_secNtk, _secNtk->getOutput(i), m);
   uint32_t COISize = 0, CandiSize = 0, totalCandi = 0;
   for (uint32_t i = 0; i < m.size(); ++i) if (m[i]) ++COISize;
   for (uint32_t i = 0; i < _secCandidate.size(); ++i) {
      for (uint32_t j = 0; j < _secCandidate[i].size(); ++j)
         if (m[_secCandidate[i][j].id]) ++CandiSize;
      totalCandi += _secCandidate[i].size();
   }
   return "COI: " + v3Int2Str(COISize) + ", In/Total: " + v3Int2Str(CandiSize) + "/" + v3Int2Str(totalCandi);
}

#endif

