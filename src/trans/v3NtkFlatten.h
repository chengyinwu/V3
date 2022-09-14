/****************************************************************************
  FileName     [ v3NtkFlatten.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Flatten Hierarchical Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_FLATTEN_H
#define V3_NTK_FLATTEN_H

#include "v3NtkHandler.h"

// class V3NtkFlatten : Flatten Network
class V3NtkFlatten : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkFlatten(V3NtkHandler* const, const uint32_t&);
      ~V3NtkFlatten();
      // Net Ancestry Functions
      void getNetName(V3NetId&, string&) const;
      const V3NetId getNetFromName(const string&) const;
      const V3NetId getParentNetId(const V3NetId&) const;
      const V3NetId getCurrentNetId(const V3NetId&, const uint32_t&) const;
   private : 
      // Ancestry Helper Functions
      const uint32_t getParentIndex(const V3NetId&) const;
      // Private Members
      V3NetVec       _c2pMap;    // V3NetId Mapping From Current to Parent Ntk
      V3NtkHierInfo  _hierInfo;  // Hierarchical Information for Flattened Ntk
};

#endif

