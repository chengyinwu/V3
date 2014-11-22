/****************************************************************************
  FileName     [ v3VrfUMC.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Unbounded Model Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_UMC_H
#define V3_VRF_UMC_H

#include "v3VrfBase.h"

// class V3VrfUMC : Verification Handler for Unbounded Model Checking
class V3VrfUMC : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfUMC(const V3NtkHandler* const);
      ~V3VrfUMC();
      // UMC Specific Functions
      inline void setPreDepth(const uint32_t& d) { _preDepth = d; }
      inline void setIncDepth(const uint32_t& d) { _incDepth = d; }
      inline void setFireOnly  (const bool& f) { if (f) _umcAttr |= 1ul; else _umcAttr &= ~1ul; }
      inline void setProveOnly (const bool& p) { if (p) _umcAttr |= 2ul; else _umcAttr &= ~2ul; }
      inline void setUniqueness(const bool& u) { if (u) _umcAttr |= 4ul; else _umcAttr &= ~4ul; }
      inline void setDynamicInc(const bool& d) { if (d) _umcAttr |= 8ul; else _umcAttr &= ~8ul; }
   private : 
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      void addUniqueness(const uint32_t&, const uint32_t&, const bool& = false);
      void computeLatchBoundsForUniqueness(const uint32_t&);
      const bool generateAndAddUniquenessBound(const uint32_t&);
      // UMC Specific Helper Functions
      inline const bool isFireOnly  () { return _umcAttr & 1ul; }
      inline const bool isProveOnly () { return _umcAttr & 2ul; }
      inline const bool isUniqueness() { return _umcAttr & 4ul; }
      inline const bool isDynamicInc() { return _umcAttr & 8ul; }
      // Private Data Members
      uint32_t       _preDepth;     // Pre-Run Depth (default = 0)
      uint32_t       _incDepth;     // Increment Depth (default = 1)
      unsigned char  _umcAttr;      // Special Settings for UMC
      V3SvrBase*     _solver;       // Single Instance SAT Solver
      // Uniqueness Constraints
      V3UI32Vec      _uniList;      // List of Uniqueness Bounds  (Indexed by uniIndex)
      V3UI32Vec      _uniLatch;     // Upper Bound of Depth for the Existence of Latch in the Solver
      V3SvrDataTable _uniAssump;    // Assumptions to Enable Uniqueness for Every Pair [depth][uniIndex]
};

#endif

