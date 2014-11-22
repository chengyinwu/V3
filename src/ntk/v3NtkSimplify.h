/****************************************************************************
  FileName     [ v3NtkSimplify.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Simplifications for V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_SIMPLIFY_H
#define V3_NTK_SIMPLIFY_H

#include "v3NtkHandler.h"

// class V3NtkSimplify : Perform Network Simplifications
class V3NtkSimplify : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkSimplify(V3NtkHandler* const);
      ~V3NtkSimplify();
      // Net Ancestry Functions
      const V3NetId getParentNetId(const V3NetId&) const;
      const V3NetId getCurrentNetId(const V3NetId&, const uint32_t& = 0) const;
   private : 
      // Private Members
      V3NetVec    _c2pMap;    // V3NetId Mapping From Current to Parent Ntk
      V3NetVec    _p2cMap;    // V3NetId Mapping From Parent to Current Ntk
};

#endif

