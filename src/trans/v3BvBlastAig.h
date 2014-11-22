/****************************************************************************
  FileName     [ v3BvBlastAig.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Bit Blasting of BV Network to AIG Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BV_BLAST_AIG_H
#define V3_BV_BLAST_AIG_H

#include "v3NtkHandler.h"

// class V3BvBlastAig : Bit Blast BV to AIG
class V3BvBlastAig : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3BvBlastAig(V3NtkHandler* const);
      ~V3BvBlastAig();
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
      // Ancestry Helper Functions
      const uint32_t getParentIndex(const V3NetId&) const;
      // Transformation Functions
      void performNtkTransformation();
      // Transformation Helper Functions
      void bitBlast_FF(const V3BvNtk* const, const V3NetId&);
      void bitBlast_RED_AND(const V3BvNtk* const, const V3NetId&);
      void bitBlast_RED_OR(const V3BvNtk* const, const V3NetId&);
      void bitBlast_RED_XOR(const V3BvNtk* const, const V3NetId&);
      void bitBlast_AND(const V3BvNtk* const, const V3NetId&);
      void bitBlast_XOR(const V3BvNtk* const, const V3NetId&);
      void bitBlast_ADD(const V3BvNtk* const, const V3NetId&);
      void bitBlast_SUB(const V3BvNtk* const, const V3NetId&);
      void bitBlast_MULT(const V3BvNtk* const, const V3NetId&);
      void bitBlast_DIV(const V3BvNtk* const, const V3NetId&);
      void bitBlast_MODULO(const V3BvNtk* const, const V3NetId&);
      void bitBlast_SHL(const V3BvNtk* const, const V3NetId&);
      void bitBlast_SHR(const V3BvNtk* const, const V3NetId&);
      void bitBlast_MERGE(const V3BvNtk* const, const V3NetId&);
      void bitBlast_EQUALITY(const V3BvNtk* const, const V3NetId&);
      void bitBlast_GEQ(const V3BvNtk* const, const V3NetId&);
      void bitBlast_MUX(const V3BvNtk* const, const V3NetId&);
      void bitBlast_SLICE(const V3BvNtk* const, const V3NetId&);
      void bitBlast_CONST(const V3BvNtk* const, const V3NetId&);
      // Divider / Modular Helper Functions
      void bitBlast_DIV_MODULO(const V3BvNtk* const, const V3NetId&, const bool&);
      // Private Members
      V3NetVec       _c2pMap;    // V3NetId Mapping From Current to Parent Ntk
      V3NetTable     _p2cMap;    // V3NetId Mapping From Parent to Current Ntk
};

#endif

