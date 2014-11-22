/****************************************************************************
  FileName     [ v3Property.h ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Property Handler for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_PROPERTY_H
#define V3_PROPERTY_H

#include "v3AlgType.h"
#include "v3LTLFormula.h"

// Forward Declarations
class V3FSM;
class V3Constraint;
class V3LTLFormula;

// Defines
typedef V3Vec<V3Constraint*>::Vec   V3ConstraintVec;

// V3CexTrace : Class for Counterexample Trace
// NOTE : For Cex of Ntk without PI, _data is empty while _size is not 0
class V3CexTrace
{
   public : 
      // Constructor and Destructor
      V3CexTrace(const uint32_t& s) : _size(s) { _init = 0; _data.clear(); _data.reserve(s); }
      ~V3CexTrace() { if (_init) delete _init; _data.clear(); }
      // Inline Member Functions
      inline const uint32_t getTraceSize() const { return _size; }
      inline const uint32_t getTraceDataSize() const { return _data.size(); }
      inline V3BitVecX* const getInit() const { return _init; }
      inline const V3BitVecX& getData(const uint32_t& i) const { assert (i < _data.size()); return _data[i]; }
      inline void setInit(const V3BitVecX& i) { _init = new V3BitVecX(i); assert (_init); }
      inline void pushData(const V3BitVecX& v) { _data.push_back(v); }
   private : 
      // Private Data Members
      const uint32_t    _size;
      V3BitVecX*        _init;
      V3SimTrace        _data;
};

// Class V3VrfResult : Class for Verification Result Storage
class V3VrfResult
{
   public : 
      // Constructor and Destructor
      V3VrfResult() { _cexOrInv = 0; }
      ~V3VrfResult() { /* Memory Will NOT be Released !! Call clear() Explicitly !! */ _cexOrInv = 0; }
      // Inline Member Functions
      inline void clear() { if (isCex()) delete getCexTrace(); /*else if (isInv()) delete getIndInv();*/ _cexOrInv = 0; }
      inline const bool isCex() const { return ((_cexOrInv & ~1ul) && !(_cexOrInv & 1ul)); }
      inline const bool isInv() const { return ((_cexOrInv & ~1ul) &&  (_cexOrInv & 1ul)); }
      inline void setCexTrace(V3CexTrace* const cex) {
         assert (!isInv()); if (isCex()) delete getCexTrace(); _cexOrInv = (size_t)(cex); assert (isCex()); }
      inline void setIndInv(V3Ntk* const indInv) {
         assert (!isCex()); /*if (isInv()) delete getIndInv();*/ _cexOrInv = (1ul | (size_t)indInv); assert (isInv()); }
      inline V3CexTrace* const getCexTrace() const { assert (isCex()); return (V3CexTrace*)(_cexOrInv); }
      inline V3Ntk* const getIndInv() const { assert (isInv()); return (V3Ntk*)(_cexOrInv & ~1ul); }
   private : 
      // Private Member : Pointer Type with Encoding
      size_t         _cexOrInv;
};

// V3Constraint : Handler for Constraints
class V3Constraint
{
   public : 
      // Constructor and Destructor
      V3Constraint(V3NtkHandler* const, const uint32_t&, const uint32_t&);
      V3Constraint(V3NtkHandler* const, V3FSM* const);
      ~V3Constraint();
      // Inline Retrieval Functions
      inline const bool isPOConstr()  const { return 0   == (3ul & (size_t)(_handler)); }
      inline const bool isFSMConstr() const { return 1ul == (3ul & (size_t)(_handler)); }
      inline V3NtkHandler* const getHandler() const { return (V3NtkHandler*)(~3ul & (size_t)(_handler)); }
      inline V3FSM* const getFSM() const { assert (isFSMConstr()); return (V3FSM* const)(_constrInfo); }
      inline const uint32_t getStart() const { assert (isPOConstr()); return V3PairType(_constrInfo).bus[0]; }
      inline const uint32_t getEnd()   const { assert (isPOConstr()); return V3PairType(_constrInfo).bus[1]; }
   private : 
      // Private Data Members
      V3NtkHandler* const  _handler;
      const uint64_t       _constrInfo;
};

// V3Property : Handler for Property
class V3Property
{
   public : 
      // Constructor and Destructor
      V3Property(V3LTLFormula* const);
      ~V3Property();
      // Inline Property Interface Functions
      inline V3LTLFormula* const getLTLFormula() const { return _formula; }
      inline const uint32_t getInvariantSize() const { return _invariants.size(); }
      inline const uint32_t getInvConstrSize() const { return _invConstrs.size(); }
      inline const uint32_t getFairConstrSize() const { return _fairConstrs.size(); }
      void setInvariant(V3NtkHandler* const, const uint32_t&, const uint32_t&);
      void setInvariant(V3NtkHandler* const, V3FSM* const);
      void setInvConstr(V3NtkHandler* const, const uint32_t&, const uint32_t&);
      void setInvConstr(V3NtkHandler* const, V3FSM* const);
      void setFairConstr(V3NtkHandler* const, const uint32_t&, const uint32_t&);
      void setFairConstr(V3NtkHandler* const, V3FSM* const);
      V3Constraint* const getInvariant(const uint32_t&) const;
      V3Constraint* const getInvConstr(const uint32_t&) const;
      V3Constraint* const getFairConstr(const uint32_t&) const;
      inline const bool isProven() const { return _result.isInv(); }
      inline const bool isFired() const { return _result.isCex(); }
      // Record Verification Result Functions
      void setResult(const V3VrfResult&);
      // Retrieve Verification Result Functions
      inline const V3CexTrace* const getCexTrace() const { assert (isFired()); return _result.getCexTrace(); }
      inline const V3Ntk* const getIndInv() const { assert (isProven()); return _result.getIndInv(); }
   private : 
      // Private Data Members
      V3LTLFormula* const  _formula;
      V3VrfResult          _result;
      V3ConstraintVec      _invariants;
      V3ConstraintVec      _invConstrs;
      V3ConstraintVec      _fairConstrs;
};

#endif

