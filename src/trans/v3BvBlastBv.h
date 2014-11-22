/****************************************************************************
  FileName     [ v3BvBlastBv.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Bit-Blasting (Pseudo) Primary Ports for BV Networks. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BV_BLAST_BV_H
#define V3_BV_BLAST_BV_H

#include "v3NtkHandler.h"

// Defines for Hash Tables
typedef V3Map<uint32_t, uint32_t>::Map    V3BvBlastBvMap;

// class V3BvBlastBv : Bit-Blast Ports for BV Networks
class V3BvBlastBv : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3BvBlastBv(V3NtkHandler* const);
      ~V3BvBlastBv();
      // I/O Ancestry Functions
      const string getInputName(const uint32_t&) const;
      const string getOutputName(const uint32_t&) const;
      const string getInoutName(const uint32_t&) const;
      // Net Ancestry Functions
      void getNetName(V3NetId&, string&) const;
      const V3NetId getNetFromName(const string&) const;
      const V3NetId getParentNetId(const V3NetId&) const;
      const V3NetId getCurrentNetId(const V3NetId&, const uint32_t& = 0) const;
   private : 
      // Ancestry Helper Functions
      const uint32_t getParentIndex(const V3NetId&) const;
      // Transformation Functions
      void performNtkTransformation();
      // Transformation Helper Functions
      void mergeBitBlastedNets(V3NetId, const uint32_t&, const V3NetId&);
      const V3NetId getBitBlastedNet(const V3NetId&, const uint32_t&);
      // Private Members
      V3BvBlastBvMap    _p2cMap;       // Net Index Mapping From Parent to Current Ntk
      V3UI32Vec         _c2pMap;       // Net Index Mapping From Current to Parent Ntk
};

#endif

