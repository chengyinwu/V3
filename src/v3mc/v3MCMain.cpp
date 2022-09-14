/****************************************************************************
  FileName     [ v3MCMain.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Model Checker. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_MAIN_C
#define V3_MC_MAIN_C

#include "v3Msg.h"
#include "v3MCMain.h"
#include "v3NumUtil.h"
#include "v3VrfResult.h"

/* -------------------------------------------------- *\
 * Class V3MCResource Implementations
\* -------------------------------------------------- */
// Constructor and Destructors
V3MCResource::V3MCResource(const V3UI32Vec& c, const V3UI32Vec& p) : V3VrfShared() {
   assert (V3MC_TYPE_TOTAL == c.size()); assert (V3MC_TYPE_TOTAL == p.size());
   _numOfCores = c; _remaining = p; _usedCores = V3UI32Vec(c.size(), 0);
   _emptyCores = 0; _cpRatio.clear(); _cpRatio.reserve(c.size());
   for (uint32_t i = 0; i < c.size(); ++i)
      _cpRatio.push_back(p[i] ? ((0.5 + (double)c[i]) / (double)p[i]) : 0);
}

V3MCResource::~V3MCResource() {
}

// Member Functions
void
V3MCResource::reallocNumOfCores() {
   uint32_t totalCores = 0, totalUsed = 0; double totalRatio = 0;
   V3DblVec ratio(_remaining.size(), 0); lock();
   for (uint32_t i = 0; i < _remaining.size(); ++i) {
      ratio[i] = _cpRatio[i] * (double)(_remaining[i]); totalRatio += ratio[i];
      totalCores += _numOfCores[i]; totalUsed += _usedCores[i];
   }
   assert (totalCores >= totalUsed); totalCores -= totalUsed;
   // Realloc Cores to Main Threads while NO Less than The Number Used Now
   for (uint32_t i = 1; i < _remaining.size(); ++i) {
      uint32_t extra = ceil((double)(totalCores) * ((double)(ratio[i]) / (double)(totalRatio)));
      if (extra > totalCores) extra = totalCores; _numOfCores[i] = _usedCores[i] + extra;
      totalRatio -= ratio[i]; totalCores -= extra;
   }
   _numOfCores[0] = _usedCores[0] + totalCores;
   if (v3MCInteractive) {
      V3VrfShared::printLock();
      Msg(MSG_DBG) << "REALLOC: ";
      for (uint32_t i = 0; i < V3MC_TYPE_TOTAL; ++i)
         Msg(MSG_DBG) << V3MCMainTypeStr[i] << " = " << _usedCores[i] << "/" << _numOfCores[i] << "  ";
      Msg(MSG_DBG) << endl;
      V3VrfShared::printUnlock();
   }
   unlock();
}

const uint32_t
V3MCResource::getActiveMainSize() {
   lock(); uint32_t active = 0;
   for (uint32_t i = 0; i < _remaining.size(); ++i) if (_remaining[i]) ++active;
   unlock(); return active;
}

const uint32_t
V3MCResource::getTotalOccupiedCores() {
   lock(); uint32_t cores = 0;
   for (uint32_t i = 0; i < _usedCores.size(); ++i) cores += _usedCores[i];
   unlock(); return cores;
}

const bool
V3MCResource::updateRemaining(const V3MCMainType& t, const uint32_t& rem) {
   lock(); assert ((uint32_t)t < _remaining.size());
   uint32_t j = 0; for (; j < _remaining.size(); ++j) if (_remaining[j]) break;
   assert (rem <= _remaining[t]); _remaining[t] = rem;
   uint32_t k = 0; for (; k < _remaining.size(); ++k) if (_remaining[k]) break;
   const bool complete = (_remaining.size() != j) && (_remaining.size() == k);
   unlock(); return complete;
}

const uint32_t
V3MCResource::getAvailableCores(const V3MCMainType& t) {
   lock(); assert ((uint32_t)t < _numOfCores.size());
   const uint32_t cores = _numOfCores[t]; unlock(); return cores;
}

const uint32_t
V3MCResource::getOccupiedCores(const V3MCMainType& t) {
   lock(); assert ((uint32_t)t < _usedCores.size());
   const uint32_t cores = _usedCores[t]; unlock(); return cores;
}

const bool
V3MCResource::incOccupiedCores(const V3MCMainType& t) {
   lock(); assert ((uint32_t)t < _usedCores.size());
   const bool ok = _usedCores[t] < _numOfCores[t];
   if (ok) ++(_usedCores[t]); unlock(); return ok;
}

void
V3MCResource::decOccupiedCores(const V3MCMainType& t) {
   lock(); assert ((uint32_t)t < _usedCores.size());
   assert (_usedCores[t]); --(_usedCores[t]); unlock();
}

/* -------------------------------------------------- *\
 * Class V3MCResult Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3MCResult::V3MCResult(const uint32_t& size) {
   assert (size); _result = V3VrfResultVec(size, V3VrfResult());
}

V3MCResult::~V3MCResult() {
   lock(); for (uint32_t i = 0; i < _result.size(); ++i) _result[i].clear(); _result.clear(); unlock();
}

// Public Member Functions
const bool
V3MCResult::updateResult(const uint32_t& p, const V3VrfResult& result) {
   assert (_result.size() > p); lock();
   const bool update = !(_result[p].isCex() || _result[p].isInv());
   if (update) _result[p] = result; unlock(); return update;
}

// V3 Verification Checker Functions
void* startChecker(void* param) {
   V3MCMainParam* const p = ((V3MCParam*)param)->_mainParam; assert (p); assert (p->_res);
   V3NtkHandler* const pNtk = p->_handler; assert (pNtk); assert (pNtk->getNtk());
   V3MCEngineType type = ((V3MCParam*)param)->_engineType; assert (V3MC_TOTAL > type);
   const bool isPropertyIndep = (V3MC_SEC_BMC == type) || (V3MC_SEC_UMC == type) ||
                                (V3MC_SEC_IPDR == type) || (V3MC_SEC_MPDR == type) ||
                                (V3MC_SEC_SYN_BMC == type) || (V3MC_SEC_SYN_UMC == type) ||
                                (V3MC_SEC_SYN_IPDR == type) || (V3MC_SEC_SYN_MPDR == type) ||
                                (V3MC_SIM == type);
   // Get Latest Optimized Network if Possible
   V3NetTable constr = p->_constr;
   V3NtkHandler* handler = (p->_sharedNtk) ? p->_sharedNtk->getNtk(pNtk, constr) : 0;
   if (!handler) handler = pNtk;
   // Get Number of Unsolved Properties
   uint32_t unsolvedSize = p->_sharedBound->getUnsolvedSize();
   // Start Checker
   if (unsolvedSize) {
      if (!V3SafeMult(unsolvedSize, unsolvedSize)) unsolvedSize = V3NtkUD;
      if (!V3SafeMult(unsolvedSize, handler->getNtk()->getNetSize())) unsolvedSize = V3NtkUD;
   }
   V3VrfBase* checker = unsolvedSize ? getChecker(type, handler, unsolvedSize) : 0;
   if (checker) {
      if (v3MCInteractive) {
         V3VrfShared::printLock();
         Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Creates " << V3MCEngineTypeStr[type] << "." << endl;
         V3VrfShared::printUnlock();
      }
      assert (p->_res->getOccupiedCores(p->_type));
      const double ratio = 1.00 - (1.00 / (double)(((V3MCParam*)param)->_threadIdx));
      const uint32_t startPropIndex = floor((double)(pNtk->getNtk()->getOutputSize()) * ratio);
      V3BoolVec solved(pNtk->getNtk()->getOutputSize(), false);
      struct timeval curTime; double remainTime, maxTime;
      for (uint32_t round = 1; round <= 5; round += 1) {
         // Check Memory Bound
         if (p->_sharedMem && !p->_sharedMem->isMemValid()) break;
         // Start Verification on Each Property
         uint32_t property = startPropIndex, latestBound, iter = 0;
         V3BoolVec tried(pNtk->getNtk()->getOutputSize(), false);
         while (true) {
            // Try Finding the First Unsolved Property
            while (true) {
               if (++property >= pNtk->getNtk()->getOutputSize()) property = 0;
               if (!solved[property] && !tried[property]) {
                  latestBound = p->_sharedBound->getBound(property);
                  if (!latestBound || (iter && V3NtkUD != latestBound)) break;
               }
               if ((startPropIndex == property) && (++iter > 1)) break;
            }
            if (iter > 1) { assert (startPropIndex == property); break; }
            // Get Number of Unsolved Properties
            unsolvedSize = p->_sharedBound->getUnsolvedSize(); if (!unsolvedSize) break;
            // Set Bound to Checker
            gettimeofday(&curTime, NULL); remainTime = p->_maxTime - getTimeUsed(p->_initTime, curTime);
            if (remainTime < 1.0) break;
            if (isPropertyIndep) maxTime = 0.5 * remainTime;
            else {
               maxTime = 2 + ceil(remainTime / (double)unsolvedSize);
               if (!V3SafeMult(maxTime, pow(round, 2)) || maxTime > remainTime) maxTime = remainTime;
            }
            checker->setMaxTime(maxTime);
            // Set Check Remaining Properties By the Latest Result
            checker->setIncVerifyUsingCurResult(unsolvedSize > 1);
            if (v3MCInteractive) {
               V3VrfShared::printLock();
               Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << "/" << V3MCEngineTypeStr[type]
                            << " Start Verifying PO[" << property << "] At Round = " << round
                            << " with MaxTime = " << maxTime << ", Unsolved = " << unsolvedSize << "." << endl;
               V3VrfShared::printUnlock();
            }
            // Start Verification
            if (constr.size() && ((V3MC_LIVE == p->_type) || (V3MC_SIM == type)))
               for (uint32_t i = 0; i < constr.size(); ++i) checker->setConstraint(constr[i], i);
            checker->setIncKeepLastReachability((V3MC_LIVE == p->_type) ? false : true);
            checker->setIncContinueOnLastSolver(false);
            checker->setIncLastDepthToKeepGoing(0);
            checker->setSharedBound(p->_sharedBound);
            checker->setSharedInv(p->_sharedInv);
            checker->setSharedNtk(p->_sharedNtk);
            checker->setSharedMem(p->_sharedMem);
            checker->verifyProperty(property);
            // Set Result for Reporting
            bool decided = false;
            for (uint32_t i = 0; i < solved.size(); ++i) {
               if (!(checker->getResult(i).isCex() || checker->getResult(i).isInv())) continue;
               decided = true; if (solved[i]) continue; solved[i] = true;
               p->_result->updateResult(i, checker->getResult(i));
               if (v3MCInteractive) {
                  V3VrfShared::printLock();
                  uint32_t netSize = (p->_sharedNtk ? p->_sharedNtk->getNetSize() : 0);
                  if (!netSize) netSize = pNtk->getNtk()->getNetSize();
                  struct timeval endTime; gettimeofday(&endTime, NULL);
                  Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << "/" << V3MCEngineTypeStr[type]
                               << (checker->getResult(i).isCex() ? " Disproves " : " Proves ") << "Property ("
                               << i << ") " << pNtk->getOutputName(i) << " at time = " << getTimeUsed(p->_initTime, endTime)
                               << " sec, ntk = " << netSize << endl;
                  V3VrfShared::printUnlock();
               }
            }
            if (isPropertyIndep && !decided) break;
            if (p->_sharedMem && !p->_sharedMem->isMemValid()) break;
            tried[property] = true;
         }
         if (isPropertyIndep) break;
      }
      delete checker; checker = 0;
   }
   // Release Core Before Return
   p->_res->decOccupiedCores(p->_type);
   if (v3MCInteractive) {
      V3VrfShared::printLock();
      Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << "/" << V3MCEngineTypeStr[type] << " Terminates." << endl;
      V3VrfShared::printUnlock();
   }

   return 0;
}

// V3 Verification Main Functions
void* startVerificationMain(void* param) {
   V3MCMainParam* p = (V3MCMainParam*)param; assert (p); assert (V3MC_TYPE_TOTAL > p->_type);
   V3NtkHandler* const handler = p->_handler; assert (handler); assert (handler->getNtk());
   // Initialize Data Members
   V3Vec<pthread_t>::Vec threads; threads.clear(); threads.reserve(32);
   V3Vec<V3MCParam>::Vec params; params.clear(); params.reserve(32);
   V3MCEngineList checkers; getDefaultCheckerList(p->_type, handler->getNtk()->getOutputSize(), checkers);
   // Start Property Checking
   if (v3MCInteractive) {
      V3VrfShared::printLock();
      Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Created : " << "NetSize = " << handler->getNtk()->getNetSize() << ", "
           << "NumOfProperty = " << handler->getNtk()->getOutputSize() << endl;
      V3VrfShared::printUnlock();
   }
   // Reorder Properties
   if (V3MC_SAFE == p->_type) reorderPropertyByNetLevel(handler, p->_constr, false);
   if (v3MCInteractive) {
      V3VrfShared::printLock();
      Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Properties: " << endl;
      for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i) Msg(MSG_DBG) << handler->getOutputName(i) << " ";
      Msg(MSG_DBG) << endl;
      V3VrfShared::printUnlock();
   }
   // Start Surveillance Mode
   double usedTimePercentage, usedMemoryPercentage; uint32_t index = 0;
   V3UI32Vec bound, mirror; V3VrfResultVec result; bool firstRound = 1;
   uint32_t unsolved = handler->getNtk()->getOutputSize();
   while (true) {
      struct timeval endTime; gettimeofday(&endTime, NULL);
      usedTimePercentage = getTimeUsed(p->_initTime, endTime) / p->_maxTime;
      usedMemoryPercentage = p->_sharedMem->getMemory() / p->_sharedMem->getMaxMemory();
      if (usedTimePercentage >= 1.0 || p->_sharedMem->getMemory() >= p->_maxMemory) {
         if (v3MCInteractive) {
            V3VrfShared::printLock();
            Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Detected Out of Resources. Waiting for children termination ..." << endl;
            V3VrfShared::printUnlock();
         }
         for (uint32_t i = 0; i < threads.size(); ++i) pthread_join(threads[i], 0);
         return 0;
      }
      // Resource Control and Update
      const uint32_t occupied = p->_res->getOccupiedCores(p->_type);
      const uint32_t available = p->_res->getAvailableCores(p->_type);
      if (v3MCInteractive) {
         const uint32_t optSize = (p->_sharedNtk ? p->_sharedNtk->getNetSize() : 0);
         const double optRatio = (double)(optSize) / (double)(handler->getNtk()->getNetSize());
         V3VrfShared::printLock();
         Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " REPORT : "
                      << "Time Used = " << (100.00 * usedTimePercentage) << " \%, "
                      << "Memory Used = " << (100.00 * usedMemoryPercentage) << " \%, "
                      << "Max Memory = " << p->_sharedMem->getMaxMemory() << " MB, "
                      << "Ntk Size = " << (100.00 * (optSize ? optRatio : 1.00)) << " \%, "
                      << "Used Cores = " << occupied << ", " << "Available = " << available << " "
                      << "Remaining = " << unsolved << " / " << handler->getNtk()->getOutputSize() << endl;
         V3VrfShared::printUnlock();
      }
      if (occupied < available) {
         uint32_t netSize = (p->_sharedNtk ? p->_sharedNtk->getNetSize() : 0);
         if (!netSize) netSize = handler->getNtk()->getNetSize();
         if (unsolved > 1) usedTimePercentage = pow(usedTimePercentage, 1.00 / sqrt(2.00 + log10(unsolved)));
         const double ratio = (netSize > 1000000) ? 0.3 : (netSize > 100000) ? 0.5 : 0.7;
         const uint32_t occupyBound = ceil((ratio + (0.5 * usedTimePercentage)) * available);
         const uint32_t memoryBound = ((double)occupied) / usedMemoryPercentage;
         if (v3MCInteractive) {
            V3VrfShared::printLock();
            Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " CREATE : "
                         << "Bounded by Time = " << occupyBound << ", Bounded by Mem = " << memoryBound << endl;
            V3VrfShared::printUnlock();
         }
         for (uint32_t i = occupied; i < occupyBound; ++i) {
            if (i >= available) break;
            if (!firstRound && (i >= memoryBound)) break;
            if (!p->_res->incOccupiedCores(p->_type)) break;
            // Create Thread for New Verification Engines
            threads.push_back(pthread_t()); params.push_back(V3MCParam());
            params.back()._mainParam = p;
            params.back()._engineType = checkers[index % checkers.size()];
            params.back()._threadIdx = params.size(); ++index;
            pthread_create(&threads.back(), NULL, &startChecker, (void*)&params.back());
         }
      }
      p->_sharedMem->updateMaxMemory(p->_maxMemory * (0.5 + (0.4 * sqrt(usedTimePercentage))));
      const uint32_t runEngines = p->_res->getOccupiedCores(p->_type); firstRound = !occupied; sleep(1);
      // Update and Print Verification Results
      const double remainingTimePercentage = (usedTimePercentage > 1.00) ? 0 : (1.00 - usedTimePercentage);
      uint32_t iter = 1 + floor(remainingTimePercentage * (100.00 / (double)(p->_res->getActiveMainSize())));
      while (iter--) {
         // Report New Verification Results
         p->_result->getResult(result); p->_sharedBound->getBound(bound); mirror.clear();
         if (p->_mirrorBound) p->_mirrorBound->getBound(mirror); unsolved = 0; bool ud = false;
         for (uint32_t i = 0; i < bound.size(); ++i) {
            if (V3NtkUD == bound[i]) { if (i < mirror.size()) p->_mirrorBound->updateBound(i, V3NtkUD); continue; }
            if (i < mirror.size() && V3NtkUD == mirror[i]) { p->_sharedBound->updateBound(i, V3NtkUD); continue; }
            if (result[i].isCex()) {
               // Check if it is a Real Counterexample
               V3CexTrace* const cex = result[i].getCexTrace(); assert (cex);
               const int checkResult = (!v3MCResultCheck) ? 1 : (V3MC_LIVE == p->_type) ? 
                                       simulationCheckFiredResult(*cex, V3NetVec(), p->_constr[i], false, handler, i) : 
                                       simulationCheckFiredResult(*cex, V3NetVec(), V3NetVec(), true, handler, i);
               if (checkResult > 0) {
                  if (i < mirror.size() && !p->_mirrorBound->updateBound(i, V3NtkUD)) continue;
                  if (!p->_sharedBound->updateBound(i, V3NtkUD)) continue;
                  if (p->_baseNtk) {
                     assert (p->_baseNtk->existProperty(handler->getOutputName(i)));
                     p->_baseNtk->getProperty(handler->getOutputName(i))->setResult(result[i]);
                  }
                  //reportAigerCex(p->_inputSize, p->_latchSize, *cex, handler->getOutputName(i));
                  if (v3MCInteractive) {
                     V3VrfShared::printLock();
                     Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Reports Result for Property (" << i << ") "
                                  << handler->getOutputName(i) << " At Iter = " << iter << "." << endl;
                     V3VrfShared::printUnlock();
                  }
                  else {
                     V3VrfShared::printLock(); gettimeofday(&endTime, NULL);
                     Msg(MSG_IFO) << "Counter-example for property " << handler->getOutputName(i) << " found at time = "
                                  << (getTimeUsed(p->_initTime, endTime)) << " sec." << endl;
                     V3VrfShared::printUnlock();
                  }
                  p->_result->updateResult(i, V3VrfResult()); ud = true;
               }
               else {
                  if (v3MCInteractive) {
                     V3VrfShared::printLock();
                     Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Confirms SPURIOUS Result for Property (" << i << ") "
                                  << handler->getOutputName(i) << " At Iter = " << iter << "!!" << endl;
                     V3VrfShared::printUnlock();
                  }
                  ++unsolved;
               }
            }
            else if (result[i].isInv()) {
               if (i < mirror.size() && !p->_mirrorBound->updateBound(i, V3NtkUD)) continue;
               if (!p->_sharedBound->updateBound(i, V3NtkUD)) continue;
               if (p->_baseNtk) {
                  assert (p->_baseNtk->existProperty(handler->getOutputName(i)));
                  p->_baseNtk->getProperty(handler->getOutputName(i))->setResult(result[i]);
               }
               //reportAigerInv(p->_inputSize, p->_latchSize, handler->getOutputName(i));
               if (v3MCInteractive) {
                  V3VrfShared::printLock();
                  Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Reports Result for Property (" << i << ") "
                               << handler->getOutputName(i) << " At Iter = " << iter << "." << endl;
                  V3VrfShared::printUnlock();
               }
               else {
                  V3VrfShared::printLock(); gettimeofday(&endTime, NULL);
                  Msg(MSG_IFO) << "Inductive Invariant for property " << handler->getOutputName(i) << " found at time = "
                               << (getTimeUsed(p->_initTime, endTime)) << " sec." << endl;
                  V3VrfShared::printUnlock();
               }
               p->_result->updateResult(i, V3VrfResult()); ud = true;
            }
            else ++unsolved;
         }
         // Update Verification Results
         const bool complete = p->_res->updateRemaining(p->_type, unsolved);
         if (ud) p->_res->reallocNumOfCores(); assert (!unsolved || !complete);
         if (!unsolved) {
            if (v3MCInteractive) {
               V3VrfShared::printLock();
               Msg(MSG_DBG) << V3MCMainTypeStr[p->_type] << " Completed. Waiting for children termination ..." << endl;
               V3VrfShared::printUnlock();
            }
            for (uint32_t i = 0; i < threads.size(); ++i) pthread_join(threads[i], 0); return 0;
         }
         gettimeofday(&endTime, NULL); if (p->_maxTime <= getTimeUsed(p->_initTime, endTime)) break;
         //if (!unsolved) { if (complete) { sleep(1); exit(0); } sleep(1000000); }
         // Check if More Cores is Available
         if (runEngines > p->_res->getOccupiedCores(p->_type)) break;
         if (available < p->_res->getAvailableCores(p->_type)) break;
         sleep((int)(p->_type) + p->_res->getActiveMainSize()); if (firstRound) break;
      }
   }
}

#endif

