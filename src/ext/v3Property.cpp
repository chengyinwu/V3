/****************************************************************************
  FileName     [ v3Property.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Property Handler for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_PROPERTY_C
#define V3_PROPERTY_C

#include "v3StgFSM.h"
#include "v3Property.h"

/* -------------------------------------------------- *\
 * Class V3Constraint Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3Constraint::V3Constraint(V3NtkHandler* const handler, const uint32_t& start, const uint32_t& end) 
   : _handler(handler), _constrInfo(V3PairType(start, end).pair) {
   assert (_handler); assert (isPOConstr());
   assert (getStart() < _handler->getNtk()->getOutputSize());
   assert (getEnd() < _handler->getNtk()->getOutputSize());
}

V3Constraint::V3Constraint(V3NtkHandler* const handler, V3FSM* const fsm) 
   : _handler((V3NtkHandler* const)(1ul | (size_t)(handler))), _constrInfo((size_t)(fsm)) {
   assert (getHandler()); assert (getFSM());
   assert (!getFSM()->getNtkHandler() || getHandler() == getFSM()->getNtkHandler());
}

V3Constraint::~V3Constraint() {
}

/* -------------------------------------------------- *\
 * Class V3Property Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3Property::V3Property(V3LTLFormula* const formula) : _formula(formula) {
   assert (formula); _invariants.clear(); _invConstrs.clear(); _fairConstrs.clear();
}

V3Property::~V3Property() {
   if (_formula) delete _formula;
   _result.clear(); _invariants.clear(); _invConstrs.size(); _fairConstrs.size();
}

// Record Verification Result Functions
void
V3Property::setResult(const V3VrfResult& r) {
   assert (r.isCex() || r.isInv());
   if (r.isInv()) { assert (!isFired()); _result.setIndInv(r.getIndInv()); assert (isProven()); }
   else { assert (!isProven()); _result.setCexTrace(r.getCexTrace()); assert (isFired()); }
}

void
V3Property::setInvariant(V3NtkHandler* const constrHandler, const uint32_t& start, const uint32_t& end) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _invariants.push_back(new V3Constraint(constrHandler, start, end)); assert (_invariants.back());
}

void
V3Property::setInvariant(V3NtkHandler* const constrHandler, V3FSM* const fsm) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _invariants.push_back(new V3Constraint(constrHandler, fsm)); assert (_invariants.back());
}

void
V3Property::setInvConstr(V3NtkHandler* const constrHandler, const uint32_t& start, const uint32_t& end) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _invConstrs.push_back(new V3Constraint(constrHandler, start, end)); assert (_invConstrs.back());
}

void
V3Property::setInvConstr(V3NtkHandler* const constrHandler, V3FSM* const fsm) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _invConstrs.push_back(new V3Constraint(constrHandler, fsm)); assert (_invConstrs.back());
}

void
V3Property::setFairConstr(V3NtkHandler* const constrHandler, const uint32_t& start, const uint32_t& end) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _fairConstrs.push_back(new V3Constraint(constrHandler, start, end)); assert (_fairConstrs.back());
}

void
V3Property::setFairConstr(V3NtkHandler* const constrHandler, V3FSM* const fsm) {
   assert (constrHandler); assert (constrHandler->getNtk());
   _fairConstrs.push_back(new V3Constraint(constrHandler, fsm)); assert (_fairConstrs.back());
}

V3Constraint* const
V3Property::getInvariant(const uint32_t& i) const {
   assert (i < getInvariantSize()); return _invariants[i];
}

V3Constraint* const
V3Property::getInvConstr(const uint32_t& i) const {
   assert (i < getInvConstrSize()); return _invConstrs[i];
}

V3Constraint* const
V3Property::getFairConstr(const uint32_t& i) const {
   assert (i < getFairConstrSize()); return _fairConstrs[i];
}

#endif

