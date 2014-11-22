/****************************************************************************
  FileName     [ v3NtkElaborate.h ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Ntk Elaboration. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_ELABORATE_H
#define V3_NTK_ELABORATE_H

#include "v3NtkHash.h"
#include "v3Property.h"
#include "v3NtkHandler.h"

// class V3NtkElaborate : Ntk Elaboration
class V3NtkElaborate : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkElaborate(V3NtkHandler* const, const V3NetVec& = V3NetVec());
      ~V3NtkElaborate();
      // Net Ancestry Functions
      const V3NetId getParentNetId(const V3NetId&) const;
      const V3NetId getCurrentNetId(const V3NetId&, const uint32_t& = 0) const;
      // Elaboration Functions
      const uint32_t elaborateProperty(V3Property* const, V3NetVec&, const bool& = true);
      const uint32_t elaborateLTLFormula(V3LTLFormula* const, const bool& = true);
      const V3NetId elaborateInvariants(V3Constraint* const);
      void elaborateConstraints(V3Constraint* const, V3NetVec&);
      void elaborateFairness(V3Constraint* const, V3NetVec&);
      void elaborateFairnessL2S(V3Constraint* const, V3NetVec&);
      void createConstrOutputs(const V3NetVec&);
      void combineInvariantToOutputs(const uint32_t&, const V3NetVec&);
      void combineConstraintsToOutputs(const uint32_t&, const V3NetVec&);
      void combineFSMInvariantsToOutputs(const uint32_t&, V3FSM* const);
      const V3NetId combineConstraints(const V3NetVec&);
      // Formula Maintenance Functions
      inline const uint32_t getLTLFormulSize() const { return _pOutput.size(); }
      V3LTLFormula* const getLTLFormula(const uint32_t&) const;
   private : 
      typedef V3Vec<V3LTLFormula*>::Vec   V3LTLFormulaVec;
      // Elaboration Helper Functions
      void elaborate(V3LTLFormula* const);
      void elaboratePOConstraints(const uint32_t&, const uint32_t&, V3NetVec&);
      void elaborateFSMConstraints(V3FSM* const, V3NetVec&);
      void elaborateFSMInvariants(V3FSM* const, V3NetVec&);
      const V3NetId elaborateFSMState(const V3NetVec&, const bool&);
      const V3NetId elaborateLTLFormula(V3LTLFormula* const, const uint32_t&);
      const V3NetId elaborateL2S(const V3NetId&);
      // Private Members
      V3NetVec          _c2pMap;    // V3NetId Mapping From Current to Parent Ntk
      V3NetVec          _p2cMap;    // V3NetId Mapping From Parent to Current Ntk
      // Formula Elaboration Members for Property
      V3PortableType    _netHash;   // Hash Table for Ntk Elaboration
      V3LTLFormulaVec   _pOutput;   // List of V3LTLFormula* for Corresponding Elaborated Output
      // Formula Elaboration Members for Delay
      V3NetId           _nextOpId;  // Delay NetId for the Delay Operator
      V3UI32Hash        _mirror;    // Mirror V3NetId for the Delay Operator
      // Formula Elaboration Members for L2S
      V3NetVec          _shadow;    // V3NetId of Shadow Latches
      V3NetId           _saved;     // V3NetId of State Recorded Latch
      V3NetId           _1stSave;   // V3NetId of First Time Oracle ON for L2S
      V3NetId           _inLoop;    // V3NetId for the Formula "_1stSave || _saved"
      V3NetId           _looped;    // V3NetId for Loop Found in L2S
};

#endif

