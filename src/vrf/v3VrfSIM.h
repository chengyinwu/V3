/****************************************************************************
  FileName     [ v3VrfSIM.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Constrained Random Simulation on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SIM_H
#define V3_VRF_SIM_H

#include "v3VrfBase.h"

// class V3VrfSIM : Verification Handler for Constrained Random Simulation
class V3VrfSIM : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfSIM(const V3NtkHandler* const);
      ~V3VrfSIM();
   private : 
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
};

#endif

