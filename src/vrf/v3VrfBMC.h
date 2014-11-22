/****************************************************************************
  FileName     [ v3VrfBMC.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Bounded Model Checking on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_BMC_H
#define V3_VRF_BMC_H

#include "v3VrfBase.h"

// class V3VrfBMC : Verification Handler for Bounded Model Checking
class V3VrfBMC : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfBMC(const V3NtkHandler* const);
      ~V3VrfBMC();
      // BMC Specific Functions
      inline void setPreDepth(const uint32_t& d) { _preDepth = d; }
      inline void setIncDepth(const uint32_t& d) { _incDepth = d; }
   private : 
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      // Private Data Members
      uint32_t       _preDepth;     // Pre-Run Depth (default = 0)
      uint32_t       _incDepth;     // Increment Depth (default = 1)
};

#endif

