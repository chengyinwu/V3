/****************************************************************************
  FileName     [ v3VrfShared.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Shared Information Among Verification Engines. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SHARED_C
#define V3_VRF_SHARED_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3VrfShared.h"

/* -------------------------------------------------- *\
 * Class V3VrfShared Implementations
\* -------------------------------------------------- */
// Static Member Initialization
pthread_mutex_t V3VrfShared::_printMutex  = PTHREAD_MUTEX_INITIALIZER;

/* -------------------------------------------------- *\
 * Class V3VrfSharedBound Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfSharedBound::V3VrfSharedBound(const uint32_t& size, const bool& print) : _print(print) {
   if (size) _bound = V3UI32Vec(size, 0); else _bound.clear(); _unsvd = size;
}

V3VrfSharedBound::~V3VrfSharedBound() {
   lock(); _bound.clear(); unlock();
}

// Public Member Functions
const bool
V3VrfSharedBound::updateBound(const uint32_t& p, const uint32_t& bound) {
   // Please Note that Checkers Never set Bound to V3NtkUD, ONLY the one who calls it does
   // This guarantees that the verification result is correct and avoids repeated print messages
   assert (p < _bound.size()); lock();
   const bool update = (V3NtkUD != _bound[p]);
   if (_print && update && (V3NtkUD != bound)) {  // Print in HWMCC deep bound track format
      printLock(); for (uint32_t i = _bound[p]; i < bound; ++i) Msg(MSG_IFO) << "u" << i << endl << flush; printUnlock();
   }
   if (update && (V3NtkUD == bound)) { assert (_unsvd); --_unsvd; }
   if (_bound[p] < bound) _bound[p] = bound; unlock(); return update;
}

const uint32_t
V3VrfSharedBound::getBound(const uint32_t& p) {
   assert (p < _bound.size()); lock();
   const uint32_t bound = _bound[p]; unlock();
   return bound;
}

/* -------------------------------------------------- *\
 * Class V3VrfSharedInv Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfSharedInv::V3VrfSharedInv() {
   _inv.clear();
}

V3VrfSharedInv::~V3VrfSharedInv() {
   _inv.clear();
}

// Public Member Functions
void
V3VrfSharedInv::pushInv(V3SvrBase* const solver, V3Ntk* const ntk, const uint32_t& depth) {
   assert (solver); assert (ntk); lock();
   V3SvrDataVec formula; formula.clear(); V3NetId id;
   for (uint32_t i = 0, j; i < _inv.size(); ++i) {
      for (j = 0; j < _inv[i].size(); ++j) {
         assert (ntk->getLatchSize() > _inv[i][j].id); id = ntk->getLatch(_inv[i][j].id);
         if (!solver->existVerifyData(id, depth)) break;
         formula.push_back(_inv[i][j].cp ? solver->getFormula(id, depth) : solver->getFormula(~id, depth));
      }
      if (j == _inv[i].size()) solver->assertImplyUnion(formula); formula.clear();
   }
   unlock();
}

/* -------------------------------------------------- *\
 * Class V3VrfSharedNtk Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3VrfSharedNtk::V3VrfSharedNtk() {
   _data.clear();
}

V3VrfSharedNtk::~V3VrfSharedNtk() {
   lock(); for (uint32_t i = 0; i < _data.size(); ++i) if (_data[i]._handler) delete _data[i]._handler;
   _data.clear(); unlock();
}

// Public Member Functions
const uint32_t
V3VrfSharedNtk::getNetSize() {
   lock();
   const uint32_t size = _data.size() ? _data.back()._handler->getNtk()->getNetSize() : 0;
   unlock(); return size;
}

void
V3VrfSharedNtk::updateNtk(V3Ntk* const ntk, const V3NetTable& constr) {
   assert (ntk); lock();
   if (_data.size() && ntk->getNetSize() >= _data.back()._handler->getNtk()->getNetSize()) { unlock(); return; }
   V3Ntk* const simpNtk = copyV3Ntk(ntk); assert (simpNtk);
   _data.push_back(V3VrfSharedNtkData()); _data.back()._refCount = 0;
   _data.back()._handler = new V3NtkHandler(0, simpNtk); _data.back()._constr = constr; unlock();
}

void
V3VrfSharedNtk::releaseNtk(V3NtkHandler* const handler) {
   assert (handler); lock();
   uint32_t i = 0; for (; i < _data.size(); ++i) if (handler == _data[i]._handler) break;
   if (i < _data.size()) {
      assert (_data[i]._refCount);
      if (!(--(_data[i]._refCount))) {
         delete _data[i]._handler;
         for (uint32_t j = 1 + i; j < _data.size(); ++j) _data[j - 1] = _data[j]; _data.pop_back();
      }
   }
   unlock(); return;
}

V3NtkHandler* const
V3VrfSharedNtk::getNtk(V3NtkHandler* const handler) {
   V3NetTable constr; return getNtk(handler, constr);
}

V3NtkHandler* const
V3VrfSharedNtk::getNtk(V3NtkHandler* const handler, V3NetTable& constr) {
   assert (handler); lock();
   uint32_t i = 0; for (; i < _data.size(); ++i) if (handler == _data[i]._handler) break;
   V3NtkHandler* const simpHandler = (((1 + i) == _data.size()) || !_data.size()) ? 0 : _data.back()._handler;
   if ((1 + i) < _data.size()) {
      assert (_data[i]._refCount);
      if (!(--(_data[i]._refCount))) {
         delete _data[i]._handler;
         for (uint32_t j = 1 + i; j < _data.size(); ++j) _data[j - 1] = _data[j]; _data.pop_back();
      }
   }
   if (simpHandler) {
      ++(_data.back()._refCount); constr = _data.back()._constr;
   }
   unlock(); return simpHandler;
}

#endif

