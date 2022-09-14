/****************************************************************************
  FileName     [ v3VrfKLive.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ K-Liveness Algorithm. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_KLIVE_H
#define V3_VRF_KLIVE_H

#include "v3VrfBase.h"

// class V3VrfKLive : Verification Handler for K-Liveness
class V3VrfKLive : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfKLive(const V3NtkHandler* const);
      ~V3VrfKLive();
      // KLive Model Checker Setting Functions
      inline void setIncDepth(const uint32_t& i) { _incDepth = i; }
      inline void setChecker(V3VrfBase* const c) { _klChecker = c; }
      // KLive Network Retrieval Functions
      // NOTE: An extra primary output is augmented for K-Liveness checks
      inline V3NtkHandler* const getKLHandler() const { return _klHandler; }
   private : 
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      // Private KLive Network Transformation Functions
      void attachNetwork(const uint32_t&);
      void simplifyNetwork(const uint32_t&);
      void initializeNetwork(const uint32_t&);
      void updateOutput(const uint32_t&);
      // Private KLive Helper Functions
      const bool isValidChecker() const;
      // Private Data Members
      V3NtkHandler*     _klHandler;       // KLiveness Network Handler
      V3Ntk*            _klNtk;           // KLiveness Network
      V3VrfBase*        _klChecker;       // Formal Engine for KLiveness
      uint32_t          _incDepth;        // Increment Depth
      V3NetVec          _klNetList;       // List of V3NetId for K-Liveness Signals
      // Statistics
      V3Stat*           _totalStat;       // Total Statistic
      V3Stat*           _checkStat;       // Formal Checking
      V3Stat*           _ntkGenStat;      // Network Generation
};

#endif

