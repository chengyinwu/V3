/****************************************************************************
  FileName     [ v3ExtElaborate.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Generic Functions for V3 Formula Elaboration. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_EXT_ELABORATE_C
#define V3_EXT_ELABORATE_C

#include "v3Msg.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"

// General Elaboration Functions for V3 Ntk
const V3NetId elaborateAigFormula(V3Formula* const formula, const uint32_t& rootIndex, V3PortableType& netHash) {
   assert (formula); assert (formula->isValid(rootIndex));
   if (formula->isLeaf(rootIndex)) return formula->getNetId(rootIndex);
   V3Ntk* const ntk = formula->getHandler()->getNtk(); assert (ntk);
   assert (!dynamic_cast<V3BvNtk*>(formula->getHandler()->getNtk()));
   V3GateType type = formula->getGateType(rootIndex); assert (V3_XD != type);
   assert (AIG_FALSE < type); assert (V3_GATE_TOTAL > type);
   V3InputVec inputs; inputs.clear(); inputs.reserve(4);
   // Elaborate Formula According to Gate Types
   switch (type) {
      // Simply Return
      case BV_RED_AND  : 
      case BV_RED_OR   : 
      case BV_RED_XOR  : 
         assert (1 == formula->getBranchSize(rootIndex));
         return elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash);
      case BV_INV      : 
         assert (1 == formula->getBranchSize(rootIndex));
         return ~(elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
      // General AIG_NODE
      case BV_AND      : 
      case BV_MULT     : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return elaborateAigGate(ntk, type, inputs, netHash);
      // AIG_NODE with I/O Inverted
      case BV_OR       : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return ~elaborateAigGate(ntk, type, inputs, netHash);
      case BV_NAND     :
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return ~elaborateAigGate(ntk, type, inputs, netHash);
      case BV_NOR      :
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return elaborateAigGate(ntk, type, inputs, netHash);
      case BV_GEQ      : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return ~elaborateAigGate(ntk, type, inputs, netHash);
      case BV_LEQ      : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return ~elaborateAigGate(ntk, type, inputs, netHash);
      case BV_LESS     : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return elaborateAigGate(ntk, type, inputs, netHash);
      case BV_GREATER  : 
      case BV_SHL      : 
      case BV_SHR      : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         type = AIG_NODE; return elaborateAigGate(ntk, type, inputs, netHash);
      // Two Level AIG_NODE
      case BV_XOR      : 
      case BV_NEQ      : 
      case BV_ADD      : 
      case BV_SUB      : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         return elaborateAigAndOrAndGate(ntk, inputs, netHash);
      case BV_XNOR     : 
      case BV_EQUALITY : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         return elaborateAigAndOrAndGate(ntk, inputs, netHash);
      case BV_MUX      : 
         assert (2 == formula->getBranchSize(rootIndex));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 2), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
         inputs.push_back(~elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 2), netHash));
         inputs.push_back( elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
         return elaborateAigAndOrAndGate(ntk, inputs, netHash);
      // Special Handlings
      case BV_SLICE    : 
         assert (2 == formula->getBranchSize(rootIndex));
         return elaborateAigFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash);
      case BV_CONST    : 
         assert (1 == formula->getBranchSize(rootIndex));
         return V3NetId::makeNetId(0, formula->getBranchIndex(rootIndex, 0));
      // NOT Supported : BV_DIV, BV_MODULO  (BV_BUF, BV_MERGE)
      default :
         Msg(MSG_ERR) << "Unpexected Operator Type = \"" << V3GateTypeStr[type] << " in Formula Elaboration !!" << endl;
   }
   return V3NetUD;
}

const V3NetId elaborateBvFormula(V3Formula* const formula, const uint32_t& rootIndex, V3PortableType& netHash) {
   assert (formula); assert (formula->isValid(rootIndex));
   if (formula->isLeaf(rootIndex)) return formula->getNetId(rootIndex);
   assert (formula->getHandler()->getNtk());
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(formula->getHandler()->getNtk()); assert (ntk);
   V3GateType type = formula->getGateType(rootIndex); assert (V3_XD != type);
   assert (AIG_FALSE < type); assert (V3_GATE_TOTAL > type);
   V3InputVec inputs; inputs.clear(); inputs.reserve(3);
   if (BV_SLICE == type) {
      assert (2 == formula->getBranchSize(rootIndex));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
      inputs.push_back(formula->getBranchIndex(rootIndex, 1));
      return elaborateBvGate(ntk, type, inputs, netHash);
   }
   else if (BV_CONST == type) {
      assert (1 == formula->getBranchSize(rootIndex));
      inputs.push_back(formula->getBranchIndex(rootIndex, 0));
      return elaborateBvGate(ntk, type, inputs, netHash);
   }
   else if (isV3PairType(type)) {
      assert (2 == formula->getBranchSize(rootIndex));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
      return elaborateBvGate(ntk, type, inputs, netHash);
   }
   else if (isV3ReducedType(type)) {
      assert (1 == formula->getBranchSize(rootIndex));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
      return elaborateBvGate(ntk, type, inputs, netHash);
   }
   else if (BV_MUX == type) {
      assert (3 == formula->getBranchSize(rootIndex));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash));
      inputs.push_back(elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 2), netHash));
      return elaborateBvGate(ntk, type, inputs, netHash);
   }
   else if (BV_INV == type) {
      assert (1 == formula->getBranchSize(rootIndex));
      return ~elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash);
   }
   else {
      assert (isV3ExtendType(type)); assert (2 == formula->getBranchSize(rootIndex));
      V3NetId id1 = elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 0), netHash);
      V3NetId id2 = elaborateBvFormula(formula, formula->getBranchIndex(rootIndex, 1), netHash);
      if (isV3ExtendSwapIn(type)) { V3NetId id = id1; id1 = id2; id2 = id; }
      if (isV3ExtendInvIn(type)) { id1 = ~id1; id2 = ~id2; }
      const bool cp = isV3ExtendInvOut(type); type = getV3ExtendNormal(type);
      assert (V3_XD > type); inputs.push_back(id1); inputs.push_back(id2);
      V3NetId id = elaborateBvGate(ntk, type, inputs, netHash); return (cp ? ~id : id);
   }
}

#endif

