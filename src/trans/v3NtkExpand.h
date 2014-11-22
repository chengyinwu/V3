/****************************************************************************
  FileName     [ v3NtkExpand.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Time-Frame Expansion to V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_EXPAND_H
#define V3_NTK_EXPAND_H

#include "v3NtkHandler.h"

// class V3NtkExpand : Time-Frame Expansion
class V3NtkExpand : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkExpand(V3NtkHandler* const, const uint32_t&, const bool& = false);
      ~V3NtkExpand();
      // Inline Member Functions
      inline const uint32_t& getCycle() const { return _cycle; }
      // I/O Ancestry Functions
      const string getInputName(const uint32_t&) const;
      const string getOutputName(const uint32_t&) const;
      const string getInoutName(const uint32_t&) const;
      // Net Ancestry Functions
      void getNetName(V3NetId&, string&) const;
      const V3NetId getNetFromName(const string&) const;
      const V3NetId getParentNetId(const V3NetId&) const;
      const V3NetId getCurrentNetId(const V3NetId&, const uint32_t&) const;
   private : 
      // Transformation Functions
      void performNtkTransformation(const bool& = false);
      // Private Members
      const uint32_t _cycle;     // Number of Cycles for Expansion
      V3NetVec       _c2pMap;    // V3NetId Mapping From Current to Parent Ntk
      V3NetTable     _p2cMap;    // V3NetId Mapping From Parent to Current Ntk
};

#endif

