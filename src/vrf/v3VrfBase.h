/****************************************************************************
  FileName     [ v3VrfBase.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Base Class for Verification on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_BASE_H
#define V3_VRF_BASE_H

#include "v3Usage.h"
#include "v3Property.h"
#include "v3VrfShared.h"
#include "v3NtkHandler.h"
#include "v3SvrHandler.h"

// class V3VrfBase : Base Verification Handler
class V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfBase(const V3NtkHandler* const);
      virtual ~V3VrfBase();
      // Constraints Setting Functions
      void setConstraint(const V3NetVec&, const uint32_t&);
      // Verification Main Functions
      void verifyInOrder();
      void verifyProperty(const uint32_t&);
      // Verification Property Retrieval Functions
      inline const V3NtkHandler* const getNtkHandler() const { return _handler; }
      // Verification Configuration Functions
      inline void setMaxTime(const double& t) { assert (t); _maxTime = t; }
      inline void setMaxDepth(const uint32_t& d) { assert (d); _maxDepth = d; }
      inline void setSolver(const V3SolverType& s) { assert (V3_SVR_TOTAL > s); _solverType = s; }
      // Verification Shared Information Functions
      inline void setSharedBound(V3VrfSharedBound* const s) { _sharedBound = s; }
      inline void setSharedInv(V3VrfSharedInv* const i) { _sharedInv = i; }
      inline void setSharedNtk(V3VrfSharedNtk* const n) { _sharedNtk = n; }
      inline void setSharedMem(V3VrfSharedMem* const m) { _sharedMem = m; }
      // Incremental Verification Functions
      inline void setIncKeepSilent          (const bool& s) { _incKeepSilent           = s; }
      inline void setIncKeepLastReachability(const bool& k) { _incKeepLastReachability = k; }
      inline void setIncContinueOnLastSolver(const bool& c) { _incContinueOnLastSolver = c; }
      inline void setIncVerifyUsingCurResult(const bool& v) { _incVerifyUsingCurResult = v; }
      inline void setIncLastDepthToKeepGoing(const uint32_t& d) { _incLastDepthToKeepGoing = d; }
      inline void clearResult(const uint32_t& i) { assert (i < _result.size()); _result[i].clear(); }
      inline const V3VrfResult& getResult(const uint32_t& i) const { assert (i < _result.size()); return _result[i]; }
      // Interactive Verbosity Setting Functions
      static void printReportSettings();
      static inline void resetReportSettings() { setReport(1); setUsage(1); }
      static inline void setReport (const bool& t) { if (t) _extVerbosity |= 1ul;  else _extVerbosity &= ~1ul;  }
      static inline void setRstOnly(const bool& t) { if (t) _extVerbosity |= 2ul;  else _extVerbosity &= ~2ul;  }
      static inline void setEndline(const bool& t) { if (t) _extVerbosity |= 4ul;  else _extVerbosity &= ~4ul;  }
      static inline void setSolver (const bool& t) { if (t) _extVerbosity |= 8ul;  else _extVerbosity &= ~8ul;  }
      static inline void setUsage  (const bool& t) { if (t) _extVerbosity |= 16ul; else _extVerbosity &= ~16ul; }
      static inline void setProfile(const bool& t) { if (t) _extVerbosity |= 32ul; else _extVerbosity &= ~32ul; }
      // Verbosity Helper Functions
      static inline const bool reportON()  { return (1ul & _extVerbosity); }
      static inline const bool intactON()  { return reportON() && !(2ul  & _extVerbosity); }
      static inline const bool endLineON() { return intactON() &&  (4ul  & _extVerbosity); }
      static inline const bool svrInfoON() { return reportON() &&  (8ul  & _extVerbosity); }
      static inline const bool usageON()   { return reportON() &&  (16ul & _extVerbosity); }
      static inline const bool profileON() { return reportON() &&  (32ul & _extVerbosity); }
      // Solver List Setting Functions
      static void printSolverSettings();
      static inline void resetSolverSettings() { _extSolverType = V3_SVR_MINISAT; }
      static inline void setDefaultSolver(const V3SolverType& type) { _extSolverType = type; }
      // Solver List Helper Functions
      static inline const V3SolverType getDefaultSolver() { return _extSolverType; }
   protected : 
      // Private Verification Main Functions
      void consistencyCheck() const;
      virtual void startVerify(const uint32_t&);
      // Functions for Checking Common Results
      void checkCommonCounterexample(const uint32_t&, const V3CexTrace&);
      void checkCommonProof(const uint32_t&, const V3NetTable&, const bool&);
      // Functions for Incremental Verification
      inline const bool isIncKeepSilent          () const { return _incKeepSilent;           }
      inline const bool isIncKeepLastReachability() const { return _incKeepLastReachability; }
      inline const bool isIncContinueOnLastSolver() const { return _incContinueOnLastSolver; }
      inline const bool isIncVerifyUsingCurResult() const { return _incVerifyUsingCurResult; }
      inline const uint32_t getIncLastDepthToKeepGoing() const { return _incLastDepthToKeepGoing; }
      // Functions for Internal Verbosity Handlings
      inline const V3SolverType getSolver() const { return (V3_SVR_TOTAL == _solverType) ? _extSolverType : _solverType; }
      // Private Data Members
      V3NtkHandler*              _handler;
      V3Ntk*                     _vrfNtk;
      V3NetTable                 _constr;
      double                     _maxTime;
      uint32_t                   _maxDepth;
      V3SolverType               _solverType;
      // Private Members for Verification Results
      V3VrfResultVec             _result;
      // Private Members for Shared Information
      V3VrfSharedBound*          _sharedBound;
      V3VrfSharedInv*            _sharedInv;
      V3VrfSharedNtk*            _sharedNtk;
      V3VrfSharedMem*            _sharedMem;
      // Private Members for Incremental Verification
      uint32_t                   _incKeepSilent           :  1;
      uint32_t                   _incKeepLastReachability :  1;
      uint32_t                   _incContinueOnLastSolver :  1;
      uint32_t                   _incVerifyUsingCurResult :  1;
      uint32_t                   _incLastDepthToKeepGoing : 28;
   private : 
      // Static Members for Global Control
      static unsigned char       _extVerbosity;
      static V3SolverType        _extSolverType;
};

#endif

