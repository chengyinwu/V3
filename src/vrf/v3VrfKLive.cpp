/****************************************************************************
  FileName     [ v3VrfKLive.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ K-Liveness Algorithm. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_KLIVE_C
#define V3_VRF_KLIVE_C

#include "v3NtkUtil.h"
#include "v3VrfKLive.h"

//#define V3_KLIVE_USE_SHIFT_REGISTERS

/* -------------------------------------------------- *\
 * Class V3VrfKLive Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfKLive::V3VrfKLive(const V3NtkHandler* const handler) : V3VrfBase(handler) {
   assert (handler); assert (handler->getNtk());
   // Private Data Members
   _klNtk = copyV3Ntk(handler->getNtk()); assert (_klNtk); _klNtk->createOutput(V3NetId::makeNetId(0));
   _klHandler = new V3NtkHandler(0, _klNtk); assert (_klHandler); _klChecker = 0; _incDepth = 1; _klNetList.clear();
   // Statistics
   if (profileON()) {
      _totalStat  = new V3Stat("TOTAL");
      _checkStat  = new V3Stat("FORMAL CHECK", _totalStat);
      _ntkGenStat = new V3Stat("NTK GENERATE", _totalStat);
   }
}

V3VrfKLive::~V3VrfKLive() {
   // Private Data Members
   if (_klHandler) delete _klHandler; _klHandler = 0; _klNtk = 0;
   if (_klChecker) delete _klChecker; _klChecker = 0; _klNetList.clear();
   // Statistics
   if (profileON()) {
      if (_totalStat ) delete _totalStat;
      if (_checkStat ) delete _checkStat;
      if (_ntkGenStat) delete _ntkGenStat;
   }
}

// Private Verification Main Functions
void
V3VrfKLive::startVerify(const uint32_t& p) {
vrfRestart: 
   // Check Shared Results
   if (_sharedBound && V3NtkUD == _sharedBound->getBound(p)) return;
   
   // Clear Verification Results
   clearResult(p); if (profileON()) _totalStat->start();
   
   // Consistency Check
   consistencyCheck(); assert (p < _constr.size() && _constr[p].size());
   if (!_klChecker || !isValidChecker()) return;

   // Initialize Checker
   if (_klChecker) {
      _klChecker->setMaxTime(_maxTime);
      _klChecker->setMaxDepth(V3NtkUD);
      _klChecker->setSolver(getSolver());
      _klChecker->setIncKeepSilent(true);
      if (_sharedMem) _klChecker->setSharedMem(_sharedMem);
   }

   // Initialize Parameters
   assert (p < _result.size()); assert (p < _vrfNtk->getOutputSize());
   const V3NetId& pId = _klNtk->getOutput(p); assert (V3NetUD != pId);
   const uint32_t logMaxWidth = (uint32_t)(ceil(log10(_maxDepth)));
   const string flushSpace = string(100, ' ');
   uint32_t proved = V3NtkUD;
   struct timeval inittime, curtime; gettimeofday(&inittime, NULL);

   // Initialize KLiveness Nets
   _klNetList.clear();

   // Start K-Liveness Algorithm
   uint32_t count = 0, lastCount = getIncLastDepthToKeepGoing();
   bool keepLastReachability = false, keepLastSolver = true;
   while (count < _maxDepth) {
      // Check Time Bounds
      gettimeofday(&curtime, NULL);
      if (_maxTime < getTimeUsed(inittime, curtime)) break;
      // Check Memory Bound
      if (_sharedMem && !_sharedMem->isMemValid()) break;
      // Check Shared Results
      if (_sharedBound) {
         const uint32_t k = _sharedBound->getBound(p);
         if (V3NtkUD == k) break; if (k >= lastCount) lastCount = k;
      }
      // Check Shared Networks
      if (_sharedNtk) {
         V3NtkHandler* const sharedNtk = _sharedNtk->getNtk(_handler, _constr);
         if (sharedNtk) {
            setIncKeepLastReachability(true); setIncContinueOnLastSolver(false); setIncLastDepthToKeepGoing(count);
            _handler = sharedNtk; _vrfNtk = sharedNtk->getNtk(); if (_klHandler) delete _klHandler;
            _klNtk = copyV3Ntk(_handler->getNtk()); assert (_klNtk); _klNtk->createOutput(V3NetId::makeNetId(0));
            _klHandler = new V3NtkHandler(0, _klNtk); assert (_klHandler); goto vrfRestart;
         }
      }
      // Elaborate Network
      if (profileON()) _ntkGenStat->start();
      for (uint32_t i = 0; i < _incDepth; ++i, ++count) if (!count) initializeNetwork(p); else attachNetwork(p);
      // Update Output for Safety Check
      updateOutput(p); if (profileON()) _ntkGenStat->end();
      if (count < lastCount) continue;
      
      // Perform Safety Property Checking on the Last Output for Checking if K-Liveness Bound is Reached
      if (profileON()) _checkStat->start();
      _klChecker->setIncKeepLastReachability(keepLastReachability);
      _klChecker->setIncLastDepthToKeepGoing(V3NtkUD);
      _klChecker->setIncContinueOnLastSolver(keepLastSolver);
      _klChecker->verifyProperty(_klNtk->getOutputSize() - 1);
      if (profileON()) _checkStat->end(); keepLastReachability = keepLastSolver = true;
      if (_klChecker->getResult(_klNtk->getOutputSize() - 1).isInv()) { proved = count; break; }
      if (!_klChecker->getResult(_klNtk->getOutputSize() - 1).isCex()) break;
      if (_sharedBound) _sharedBound->updateBound(p, count);

      // Simplify the K-Liveness Network if it Grows too Large
      //simplifyNetwork(p); keepLastSolver = false;
      
      // Report Verification Progress
      if (!isIncKeepSilent() && intactON()) {
         if (!endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << "Verification completed under depth = " << setw(logMaxWidth) << count;
         if (endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
   }

   // Report Verification Result
   if (!isIncKeepSilent() && reportON()) {
      if (intactON()) {
         if (endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      if (V3NtkUD != proved) Msg(MSG_IFO) << "Inductive Invariant found at depth = " << proved;
      else Msg(MSG_IFO) << "UNDECIDED at depth = " << _maxDepth;
      if (usageON()) {
         gettimeofday(&curtime, NULL);
         Msg(MSG_IFO) << "  (time = " << setprecision(5) << getTimeUsed(inittime, curtime) << "  sec)" << endl;
      }
      if (profileON()) {
         _totalStat->end();
         Msg(MSG_IFO) << *_checkStat  << endl;
         Msg(MSG_IFO) << *_ntkGenStat << endl;
         Msg(MSG_IFO) << *_totalStat  << endl;
      }
   }
   // Record CounterExample Trace or Invariant
   if (V3NtkUD != proved) {  // Record Inductive Invariant
      _result[p].setIndInv(_vrfNtk); assert (_result[p].isInv());
   }
}

// Private KLive Network Transformation Functions
void
V3VrfKLive::simplifyNetwork(const uint32_t& p) {
   assert (_klNtk); assert (_klNetList.size());
   if (profileON()) _ntkGenStat->start();
   // Put Constraints and K-Liveness Signals to POs
   assert (_klNtk->getOutputSize() == (1 + _result.size()));
   for (uint32_t i = 0; i < _constr[p].size(); ++i) _klNtk->createOutput(_constr[p][i]);
   for (uint32_t i = 0; i < _klNetList.size(); ++i) _klNtk->createOutput(_klNetList[i]);
   // Simplify K-Liveness Network
   V3NetVec p2cMap, c2pMap; p2cMap.clear(); c2pMap.clear(); V3NetId id;
   V3Ntk* const simpNtk = duplicateNtk(_klHandler, p2cMap, c2pMap); assert (simpNtk);
   for (uint32_t i = 0; i <= _result.size(); ++i) {
      id = _klNtk->getOutput(i); assert (V3NetUD != p2cMap[id.id]);
      simpNtk->createOutput(V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp));
   }
   // Update Fairness Constraints and K-Liveness Signals
   for (uint32_t i = 0; i < _constr[p].size(); ++i) {
      id = _constr[p][i]; assert (V3NetUD != p2cMap[id.id]);
      _constr[p][i] = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
   }
   for (uint32_t i = 0; i < _klNetList.size(); ++i) {
      id = _klNetList[i]; assert (V3NetUD != p2cMap[id.id]);
      _klNetList[i] = V3NetId::makeNetId(p2cMap[id.id].id, p2cMap[id.id].cp ^ id.cp);
   }
   replaceV3Ntk(_klNtk, simpNtk); delete simpNtk; if (profileON()) _ntkGenStat->end();
}

void
V3VrfKLive::attachNetwork(const uint32_t& p) {
   assert (_klNtk); assert (_klNetList.size());
#ifdef V3_KLIVE_USE_SHIFT_REGISTERS
   assert (_klNetList.size() == _constr[p].size());
   const V3NetId const1 = V3NetId::makeNetId(0, 1);
   // Construct Logic for Extra Bound
   if (dynamic_cast<V3BvNtk*>(_klNtk)) {
      V3NetId id1, id2; V3InputVec inputs; inputs.clear(); inputs.reserve(3);
      for (uint32_t i = 0; i < _klNetList.size(); ++i) {
         // Create Net for K-Liveness Latch
         id1 = _klNtk->createNet(); assert (V3NetUD != id1);
         // Create Input Logic for K-Liveness Latch
         id2 = _klNtk->createNet(); assert (V3NetUD != id2);
         inputs.push_back(id1); inputs.push_back(_klNetList[i]); inputs.push_back(_constr[p][i]);
         _klNtk->setInput(id2, inputs); _klNtk->createGate(BV_MUX, id2); inputs.clear();
         // Create K-Liveness Latch
         _klNetList[i] = id1; inputs.push_back(id2); inputs.push_back(const1);
         _klNtk->setInput(id1, inputs); _klNtk->createLatch(id1); inputs.clear();
      }
   }
   else {
      V3NetId id1, id2, id3, id4; V3InputVec inputs; inputs.clear(); inputs.reserve(2);
      for (uint32_t i = 0; i < _klNetList.size(); ++i) {
         // Create Net for K-Liveness Latch
         id1 = _klNtk->createNet(); assert (V3NetUD != id1);
         // Create Input Logic for K-Liveness Latch
         id3 = _klNtk->createNet(); assert (V3NetUD != id3);
         inputs.push_back(_constr[p][i]); inputs.push_back(_klNetList[i]);
         _klNtk->setInput(id3, inputs); _klNtk->createGate(AIG_NODE, id3); inputs.clear();
         id4 = _klNtk->createNet(); assert (V3NetUD != id4);
         inputs.push_back(~(_constr[p][i])); inputs.push_back(id1);
         _klNtk->setInput(id4, inputs); _klNtk->createGate(AIG_NODE, id4); inputs.clear();
         id2 = _klNtk->createNet(); assert (V3NetUD != id2);
         inputs.push_back(~id3); inputs.push_back(~id4);
         _klNtk->setInput(id2, inputs); _klNtk->createGate(AIG_NODE, id2); inputs.clear();
         // Create K-Liveness Latch
         _klNetList[i] = id1; inputs.push_back(~id2); inputs.push_back(const1);
         _klNtk->setInput(id1, inputs); _klNtk->createLatch(id1); inputs.clear();
      }
   }
#else
   const V3GateType type = dynamic_cast<V3BvNtk*>(_klNtk) ? BV_AND : AIG_NODE;
   V3NetId id1, id2; V3InputVec inputs(2, V3NetUD); assert (1 == _klNetList.size());
   // Create Net for K-Liveness Latch
   id1 = _klNtk->createNet(); assert (V3NetUD != id1);
   // Create Input Logic for K-Liveness Latch
   id2 = _klNtk->createNet(); assert (V3NetUD != id2);
   inputs[0] = _klNetList[0]; inputs[1] = ~id1;
   _klNtk->setInput(id2, inputs); _klNtk->createGate(type, id2);
   // Create K-Liveness Latch
   inputs[0] = ~id2; inputs[1] = V3NetId::makeNetId(0);
   _klNtk->setInput(id1, inputs); _klNtk->createLatch(id1);
   // Create New K-Liveness Signal
   id2 = _klNtk->createNet(); assert (V3NetUD != id2);
   inputs[0] = ~_klNetList[0]; inputs[1] = id1;
   _klNtk->setInput(id2, inputs); _klNtk->createGate(type, id2); _klNetList[0] = ~id2;
#endif
}

void
V3VrfKLive::initializeNetwork(const uint32_t& p) {
   assert (_klNtk); assert (!_klNetList.size());
   // Construct Logic for the Base Bound
   const V3GateType type = dynamic_cast<V3BvNtk*>(_klNtk) ? BV_AND : AIG_NODE;
   V3NetId id1; V3InputVec inputs(2, V3NetUD); _klNetList.clear();
#ifdef V3_KLIVE_USE_SHIFT_REGISTERS
   const V3NetId const1 = V3NetId::makeNetId(0, 1); _klNetList.reserve(_constr[p].size());
   for (uint32_t i = 0; i < _constr[p].size(); ++i) {
      // Create Net for K-Liveness Latch
      _klNetList.push_back(_klNtk->createNet()); assert (V3NetUD != _klNetList.back());
      // Create Input Logic for K-Liveness Latch
      id1 = _klNtk->createNet(); assert (V3NetUD != id1);
      inputs[0] = ~(_constr[p][i]); inputs[1] = _klNetList.back();
      _klNtk->setInput(id1, inputs); _klNtk->createGate(type, id1);
      // Create K-Liveness Latch
      inputs[0] = id1; inputs[1] = const1;
      _klNtk->setInput(_klNetList.back(), inputs); _klNtk->createLatch(_klNetList.back());
   }
#else
   // Merge Fairness Constraints
   if (_constr[p].size() == 1) { _klNetList.push_back(~(_constr[p][0])); return; }
   V3NetVec fairLatch(_constr[p].size(), V3NetUD), fairFound(_constr[p].size(), V3NetUD);
   const V3NetId const0 = V3NetId::makeNetId(0);
   for (uint32_t i = 0; i < _constr[p].size(); ++i) {
      // Create Representative Signals for Each Fairness Constraints f
      fairLatch[i] = _klNtk->createNet(); assert (V3NetUD != fairLatch[i]);
      fairFound[i] = _klNtk->createNet(); assert (V3NetUD != fairFound[i]);
      // Create Logic for "f happens now or after RESET"
      inputs[0] = ~(_constr[p][i]); inputs[1] = ~(fairLatch[i]);
      _klNtk->setInput(fairFound[i], inputs); _klNtk->createGate(type, fairFound[i]);
   }
   // Create the "RESET" Logic for the Resulting Fairness Constraint
   inputs[0] = ~fairFound[0];
   for (uint32_t i = 1; i < fairFound.size(); ++i) {
      id1 = _klNtk->createNet(); assert (V3NetUD != id1); inputs[1] = ~fairFound[i];
      _klNtk->setInput(id1, inputs); _klNtk->createGate(type, id1); inputs[0] = id1;
   }
   _klNetList.reserve(1); _klNetList.push_back(~(inputs[0].id));
   for (uint32_t i = 0; i < _constr[p].size(); ++i) {
      // Create Input Logic for Latch Representing "f happens after RESET"
      id1 = _klNtk->createNet(); assert (V3NetUD != id1);
      inputs[0] = _klNetList[0]; inputs[1] = ~(fairFound[i]);
      _klNtk->setInput(id1, inputs); _klNtk->createGate(type, id1);
      inputs[0] = id1; inputs[1] = const0;
      _klNtk->setInput(fairLatch[i], inputs); _klNtk->createLatch(fairLatch[i]);
   }
#endif
}

void
V3VrfKLive::updateOutput(const uint32_t& p) {
   assert (_klNtk); assert (_klNetList.size()); V3NetId id; V3InputVec inputs(2, V3NetUD);
   const V3GateType type = dynamic_cast<V3BvNtk*>(_klNtk) ? BV_AND : AIG_NODE;
   // Replace Output Signal for Safety Check
#ifdef V3_KLIVE_USE_SHIFT_REGISTERS
   assert (_klNetList.size() == _constr[p].size());
   for (uint32_t i = 0; i < _klNetList.size(); ++i) {
      if (!i) inputs[0] = ~(_klNtk->getInputNetId(_klNetList[i], 0));
      else {
         inputs[1] = ~(_klNtk->getInputNetId(_klNetList[i], 0));
         id = _klNtk->createNet(); assert (V3NetUD != id);
         _klNtk->setInput(id, inputs); _klNtk->createGate(type, id); inputs[0] = id;
      }
   }
#else
   assert (1 == _klNetList.size()); inputs[0] = ~_klNetList[0];
#endif
   id = _klNtk->createNet(); assert (V3NetUD != id); inputs[1] = _klNtk->getOutput(p);
   _klNtk->setInput(id, inputs); _klNtk->createGate(type, id);
   _klNtk->replaceOutput(_klNtk->getOutputSize() - 1, id);
}

// Private KLive Helper Functions
const bool
V3VrfKLive::isValidChecker() const {
   assert (_klChecker);
   if (_klHandler == _klChecker->getNtkHandler()) return true;
   Msg(MSG_ERR) << "Network handler that model checker works on must be getSECHandler() !!" << endl;
   return false;
}

#endif

