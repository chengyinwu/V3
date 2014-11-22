/****************************************************************************
  FileName     [ v3VrfSEC.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Sequential Equivalence Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SEC_H
#define V3_VRF_SEC_H

#include "v3VrfBase.h"
#include "v3AlgSimulate.h"

// Type Defines
typedef V3Vec<V3SimTrace>::Vec   V3VrfSimTraceVec;

// class V3VrfSEC : Verification Handler for Sequential Equivalence Checking
class V3VrfSEC : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfSEC(const V3NtkHandler* const);
      ~V3VrfSEC();
      // SEC Model Checker Setting Functions
      inline void setChecker(V3VrfBase* const c) { _secChecker = c; }
      inline void setAssumeCECMiter() { _secCECAssumed = 1; }
      inline void setAssumeSECMiter() { _secSECAssumed = 1; }
      inline void setSynthesisMode(const bool& s) { _secSynthesis = s; }
      // SEC Network Retrieval Functions
      // NOTE: An extra primary output is augmented for SEC checks
      inline V3NtkHandler* const getSECHandler() const { return _secHandler; }
   private : 
      // Private SEC Model Checker Setting Functions
      inline const bool isAssumeCECMiter() const { return _secCECAssumed; }
      inline const bool isAssumeSECMiter() const { return _secSECAssumed; }
      inline const bool isSynthesisMode() const { return _secSynthesis; }
      inline const bool isInternalCEC() const { return _secCECAssumed && !_secIsCECorSEC; }
      inline const bool isInternalSEC() const { return _secSECAssumed &&  _secIsCECorSEC; }
      inline void setInternalCEC() { assert (isAssumeCECMiter()); _secIsCECorSEC = 0; }
      inline void setInternalSEC() { assert (isAssumeSECMiter()); _secIsCECorSEC = 1; }
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      // Private EC Pair Simulation Functions
      void randomSimulate(const uint32_t&, const uint32_t&, const bool&);
      void addSimulationTrace(const V3CexTrace&, const bool& = true);
      void addSimulationTrace(const V3SimTrace&, const V3VrfSimTraceVec&, const bool&);
      void computeTrace(const V3CexTrace&, V3SimTrace&, V3VrfSimTraceVec&);
      V3CexTrace* const getSolverCexTrace(V3SvrBase* const, const uint32_t&);
      V3CexTrace* const computeTrace(const V3SimTrace&, const V3VrfSimTraceVec&, const uint32_t&);
      // Private EC Pair Construction Functions
      void initializeECPair();
      void removeECPairOutOfCOI();
      void updateECPair(const bool&, const bool&);
      void updateEQList(const uint32_t&, V3RepIdHash&);
      const uint32_t getTargetCandidate();
      // Private SEC Network Transformation Functions
      void replaceSECOutput(const uint32_t&);
      void replaceSECNetwork(V3RepIdHash&);
      void simplifySECNetwork(V3NetVec&, V3NetVec&, const V3NetVec& = V3NetVec());
      void speculativeReduction(const uint32_t&);
      // Private SEC Helper Functions
      const bool isValidChecker() const;
      const bool isValidNet(const V3NetId&) const;
      const bool isValidInvariant(const uint32_t&);
      const bool isValidCandidates(const uint32_t&) const;
      void printCandidate(const uint32_t&) const;
      const string getCOISize();
      // Private Data Members
      V3NtkHandler*     _secHandler;         // SEC Network Handler
      V3Ntk*            _secNtk;             // SEC Network
      V3AlgSimulate*    _secSim;             // Simulator for SEC
      V3VrfBase*        _secChecker;         // Formal Engine for SEC
      // Private Mapping Tables
      V3NetVec          _p2cMap;             // Mapping from old to new
      V3NetVec          _c2pMap;             // Mapping from new to old
      // Private EC Pair Storage Data
      V3NetTable        _secCandidate;       // List of SEC Candidates
      V3BoolVec         _secCheckFail;       // List of Undecided Candidates
      uint32_t          _secEQSize;          // Number of Proven EQ Sets
      uint32_t          _secSynthesis  :  1; // Toggle Synthesis Mode
      uint32_t          _secIsCECorSEC :  1; // Toggle Internal CEC or SEC
      uint32_t          _secCECAssumed :  1; // Toggle Assuming CEC Miter
      uint32_t          _secSECAssumed :  1; // Toggle Assuming SEC Miter
      uint32_t          _secLastDepth  : 28; // Last Checked Deep Bound
      // Statistics
      V3Stat*           _totalStat;          // Total Statistic
      V3Stat*           _simStat;            // Simulation
      V3Stat*           _simpStat;           // Simplification
      V3Stat*           _checkStat;          // Formal Checking
      V3Stat*           _specuStat;          // Speculative Reduction
};

#endif

