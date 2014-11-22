/****************************************************************************
  FileName     [ v3NtkRewrite.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Implementation of Rule-based Rewriting. ]
  Author       [ Mao-Kai (Nanny) Huang, Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_REWRITE_C
#define V3_NTK_REWRITE_C

#include "v3Msg.h"
#include "v3StrUtil.h"
#include "v3NtkStrash.h"
#include "v3NtkRewrite.h"

#define V3_REWRITE_STRASH_DUAL_REPR
#define V3_REWRITE_CONST_PROP_ENABLED
//#define V3_REWRITE_CONST_REDUCTION_ENABLED

// NOTICE: Some Possible Implementations in the Future are Labelled as REWRITE_FUTURE

// Rewrite Utility Functions
inline void rewriteSwapNets(V3NetType& in1, V3NetType& in2) { const V3NetType in = in1; in1 = in2; in2 = in; }
inline const string rewriteGetExpr(const string& width, const string& value) { return width + "'b" + value; }

// Rewrite Helper Functions
const V3NetId rewriteReturnNetId(V3BvNtk* const ntk, const V3GateType& type, V3PortableType& netHash, const bool& cpId, const V3NetType& in1, const V3NetType& in2 = V3NetUD, const V3NetType& in3 = V3NetUD) {
   // This Function Returns a NetId under Specified GateType and Inputs
   assert (ntk); assert (AIG_FALSE < type); assert (V3_XD > type);
   V3InputVec inputs; inputs.clear(); inputs.push_back(in1);
   if (!isV3ReducedType(type) && BV_CONST != type) inputs.push_back(in2);
   if (BV_MUX == type) inputs.push_back(in3);
   const V3NetId id = elaborateBvGate(ntk, type, inputs, netHash);
   assert (id.id < ntk->getNetSize()); return cpId ? ~id : id;
}

const V3NetId rewriteReturnAigNode(V3AigNtk* const ntk, V3PortableType& netHash, const bool& cpId, const V3NetType& in1, const V3NetType& in2) {
   // This Function Returns a NetId Representing AND of Inputs
   assert (ntk); V3InputVec inputs(2); inputs[0] = in1; inputs[1] = in2;
   const V3NetId id = elaborateAigGate(ntk, AIG_NODE, inputs, netHash);
   assert (id.id < ntk->getNetSize()); return cpId ? ~id : id;
}

const bool rewriteReturnTerminal(V3Ntk* const ntk, V3GateType& type, V3InputVec& inputs, const V3NetId& id, const bool& cpId) {
   // This Function Returns Existing NetId (Terminal Case)
   assert (ntk); assert (id.id < ntk->getNetSize());
   type = V3_PI;  // To indicate the terminal case
   assert (inputs.size()); inputs[0] = id; return cpId;
}

const bool rewriteReturnConst(V3BvNtk* const ntk, V3GateType& type, V3PortableType& netHash, const bool& cpId, V3InputVec& inputs, const string& expr) {
   // This Function Returns a CONST with Expression expr
   assert (ntk); assert (expr.size());
   type = BV_CONST; inputs.clear(); inputs.push_back(ntk->hashV3ConstBitVec(expr));
   return cpId ^ rewrite_CONST(ntk, type, inputs, netHash);
}

// Rewrite Sub-Functions
const bool rewrite_AND(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_AND == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Perform Normalization on Inputs
   if (type == ntk->getGateType(inputs[0].id)) rewriteSwapNets(inputs[0], inputs[1]);
   if (BV_CONST == ntk->getGateType(inputs[1].id)) rewriteSwapNets(inputs[0], inputs[1]);
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (value1.all0()) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      if (value1.all1()) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
      if (BV_CONST == ntk->getGateType(id2)) {
         V3BitVecX value2 = ntk->getInputConstValue(id2); value2 &= value1;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value2.regEx()));
      }
      // Check if there Exists a Constant Grandson who Has a Non-Inverting AND Parent
      if (BV_AND == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         if (BV_CONST == ntk->getGateType(id21)) {
            V3BitVecX value2 = ntk->getInputConstValue(id21); value2 &= value1;
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value2.regEx())));
            inputs[1] = id22; return rewrite_AND(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_AND == ntk->getGateType(id1) && !isV3NetInverted(id1) && 
       BV_AND == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id11) && BV_CONST == ntk->getGateType(id21)) {
         const V3BitVecX value2 = ntk->getInputConstValue(id11);
         V3BitVecX value = ntk->getInputConstValue(id21); value &= value2;
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
         inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, id12, id22);
         return rewrite_AND(ntk, type, inputs, netHash);
      }
   }
#endif
   // Perform Identity Check
   if (id1 ==  id2) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   if (id1 == ~id2) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
   if (BV_AND == ntk->getGateType(id1)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      if (!isV3NetInverted(id1)) {
         if (id2 == ~id11 || id2 == ~id12) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                                                  rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
         if (id2 ==  id11 || id2 ==  id12) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      }
      else {
         if (id2 == ~id11 || id2 == ~id12) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
         if (id2 == id11) { inputs[0] = ~id12; return rewrite_AND(ntk, type, inputs, netHash); }
         if (id2 == id12) { inputs[0] = ~id11; return rewrite_AND(ntk, type, inputs, netHash); }
      }
   }
   if (BV_AND == ntk->getGateType(id2)) {
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (!isV3NetInverted(id2)) {
         if (id1 == ~id21 || id1 == ~id22) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                                                  rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
         if (id1 ==  id21 || id1 ==  id22) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
      }
      else {
         if (id1 == ~id21 || id1 == ~id22) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
         if (id1 == id21) { inputs[1] = ~id22; return rewrite_AND(ntk, type, inputs, netHash); }
         if (id1 == id22) { inputs[1] = ~id21; return rewrite_AND(ntk, type, inputs, netHash); }
      }
   }
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_AND: 
               if (!isV3NetInverted(id1) && !isV3NetInverted(id2)) {
                  if (id11 == ~id21 || id11 == ~id22 || id12 == ~id21 || id12 == ~id22) 
                     return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
                  if (id11 == id21) {
                     inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, id12, id22);
                     inputs[0] = id11; return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id11 == id22) {
                     inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, id12, id21);
                     inputs[0] = id11; return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id12 == id22) {
                     inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, id11, id21);
                     inputs[0] = id12; return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id12 == id21) {
                     inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, id11, id22);
                     inputs[0] = id12; return rewrite_AND(ntk, type, inputs, netHash);
                  }
               }
               else if (isV3NetInverted(id1) && isV3NetInverted(id2)) {
                  if (((id11 == id21) && (id12 == ~id22)) || ((id11 == id22) && (id12 == ~id21))) 
                     return rewriteReturnTerminal(ntk, type, inputs, id11, true);
                  if (((id12 == id21) && (id11 == ~id22)) || ((id12 == id22) && (id11 == ~id21))) 
                     return rewriteReturnTerminal(ntk, type, inputs, id12, true);
                  if (((id11 == ~id21) && (id12 == ~id22)) || ((id11 == ~id22) && (id12 == ~id21))) {
                     type = BV_XOR; inputs[0] = id11; inputs[1] = id12;
                     return rewrite_XOR(ntk, type, inputs, netHash);
                  }
               }
               else {
                  if ((id11 == ~id21) || (id11 == ~id22) || (id12 == ~id21) || (id12 == ~id22)) 
                     return rewriteReturnTerminal(ntk, type, inputs, (id1.cp ? id2 : id1), false);
                  if (id11 == id21) {
                     inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id12 : ~id22;
                     return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id12 == id21) {
                     inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id11 : ~id22;
                     return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id11 == id22) {
                     inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id12 : ~id21;
                     return rewrite_AND(ntk, type, inputs, netHash);
                  }
                  if (id12 == id22) {
                     inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id11 : ~id21;
                     return rewrite_AND(ntk, type, inputs, netHash);
                  }
               }
               break;
            case BV_GEQ: 
               if (id11 == id22 && id12 == id21) {
                  if (isV3NetInverted(id1)) {
                     if (!isV3NetInverted(id2)) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
                     else return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
                  }
                  else {
                     if (isV3NetInverted(id2)) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
                     else {
                        type = BV_EQUALITY; inputs[0] = id11; inputs[1] = id12;
                        return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                  }
               }
               break;
            case BV_MERGE: 
               if (ntk->getNetWidth(id11) == ntk->getNetWidth(id21)) {
                  assert (ntk->getNetWidth(id12) == ntk->getNetWidth(id22));
                  inputs[0] = rewriteReturnNetId(ntk, BV_AND, netHash, false, 
                        (id1.cp ? ~id11 : id11), (id2.cp ? ~id21 : id21));
                  inputs[1] = rewriteReturnNetId(ntk, BV_AND, netHash, false, 
                        (id1.cp ? ~id12 : id12), (id2.cp ? ~id22 : id22));
                  type = BV_MERGE; return rewrite_MERGE(ntk, type, inputs, netHash);
               }
               break;
            default: break;
         }
      }
   }
   // Perform Constant Reduction
#ifdef V3_REWRITE_CONST_REDUCTION_ENABLED
   V3UI32Queue idQueue; idQueue.push(V3NetType(id1).value); idQueue.push(V3NetType(id2).value);
   V3UI32Set idSet; idSet.clear(); V3BitVecX* idValue = 0; V3NetId id;
   while (!idQueue.empty()) {
      id = ((V3NetType)(idQueue.front())).id; assert (id.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id)) {
         if (!idValue) { idValue = new V3BitVecX(ntk->getInputConstValue(id)); assert (idValue); }
         else { (*idValue) &= ntk->getInputConstValue(id); if (idValue->all0()) break; }
      }
      else {
         if (idSet.end() != idSet.find(V3NetType(~id).value)) break;
         if (BV_AND == ntk->getGateType(id) && !isV3NetInverted(id)) {
            idQueue.push(V3NetType(ntk->getInputNetId(id, 0)).value);
            idQueue.push(V3NetType(ntk->getInputNetId(id, 1)).value);
         }
         idSet.insert(idQueue.front());
      }
      idQueue.pop();
   }
   if (!idQueue.empty()) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
#endif
   // Consistency Check (Corresponds to Normalization)
   assert (BV_AND == ntk->getGateType(id2) || BV_AND != ntk->getGateType(id1));
   assert (BV_CONST == ntk->getGateType(id1) || BV_CONST != ntk->getGateType(id2));
   return false;
}

const bool rewrite_XOR(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_XOR == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : a' ^ b' == a ^ b
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
      id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
      // Dual Representation : a' ^ b == (a ^ b)'
      inputs[0].id.cp ^= 1;
      id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, true);
      inputs[0].id.cp ^= 1;
      // Dual Representation : a ^ b' == (a ^ b)'
      inputs[1].id.cp ^= 1;
      id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, true);
      inputs[1].id.cp ^= 1;
#endif
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Perform Normalization on Inputs
   if (type == ntk->getGateType(inputs[0].id)) rewriteSwapNets(inputs[0], inputs[1]);
   if (BV_CONST == ntk->getGateType(inputs[1].id)) rewriteSwapNets(inputs[0], inputs[1]);
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (BV_CONST == ntk->getGateType(id2)) {
         V3BitVecX value2 = ntk->getInputConstValue(id2); value2 ^= value1;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value2.regEx()));
      }
      // Check if there Exists a Constant Grandson who Has a Non-Inverting XOR Parent
      if (BV_XOR == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         if (BV_CONST == ntk->getGateType(id21)) {
            V3BitVecX value = ntk->getInputConstValue(id21); value ^= value1;
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
            inputs[1] = id22; return rewrite_XOR(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_XOR == ntk->getGateType(id1) && !isV3NetInverted(id1) && 
       BV_XOR == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id11) && BV_CONST == ntk->getGateType(id21)) {
         const V3BitVecX value2 = ntk->getInputConstValue(id11);
         V3BitVecX value = ntk->getInputConstValue(id21); value ^= value2;
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
         inputs[1] = rewriteReturnNetId(ntk, BV_XOR, netHash, false, id12, id22);
         return rewrite_XOR(ntk, type, inputs, netHash);
      }
   }
#endif
   // Perform Identity Check
   if (id1 ==  id2) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
   if (id1 == ~id2) return rewriteReturnConst(ntk, type, netHash, true, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
   if (BV_XOR == ntk->getGateType(id1)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      if (id2.id == id11.id) return rewriteReturnTerminal(ntk, type, inputs, id12, (id1.cp ^ (id2.cp != id11.cp)));
      if (id2.id == id12.id) return rewriteReturnTerminal(ntk, type, inputs, id11, (id1.cp ^ (id2.cp != id12.cp)));
   }
   if (BV_XOR == ntk->getGateType(id2)) {
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (id1.id == id21.id) return rewriteReturnTerminal(ntk, type, inputs, id22, (id2.cp ^ (id1.cp != id21.cp)));
      if (id1.id == id22.id) return rewriteReturnTerminal(ntk, type, inputs, id21, (id2.cp ^ (id1.cp != id22.cp)));
   }
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_XOR: 
               if (id11.id == id21.id) {
                  inputs[0] = id12; inputs[1] = id22;
                  return (id1.cp == id2.cp) ^ (id11.cp == id21.cp) ^ rewrite_XOR(ntk, type, inputs, netHash);
               }
               if (id11.id == id22.id) {
                  inputs[0] = id12; inputs[1] = id21;
                  return (id1.cp == id2.cp) ^ (id11.cp == id22.cp) ^ rewrite_XOR(ntk, type, inputs, netHash);
               }
               if (id12.id == id21.id) {
                  inputs[0] = id11; inputs[1] = id22;
                  return (id1.cp == id2.cp) ^ (id12.cp == id21.cp) ^ rewrite_XOR(ntk, type, inputs, netHash);
               }
               if (id12.id == id22.id) {
                  inputs[0] = id11; inputs[1] = id21;
                  return (id1.cp == id2.cp) ^ (id12.cp == id22.cp) ^ rewrite_XOR(ntk, type, inputs, netHash);
               }
               break;
            case BV_MERGE: 
               if (ntk->getNetWidth(id11) == ntk->getNetWidth(id21)) {
                  assert (ntk->getNetWidth(id12) == ntk->getNetWidth(id22));
                  inputs[0] = rewriteReturnNetId(ntk, BV_XOR, netHash, false, 
                        (id1.cp ? ~id11 : id11), (id2.cp ? ~id21 : id21));
                  inputs[1] = rewriteReturnNetId(ntk, BV_XOR, netHash, false, 
                        (id1.cp ? ~id12 : id12), (id2.cp ? ~id22 : id22));
                  type = BV_MERGE; return rewrite_MERGE(ntk, type, inputs, netHash);
               }
               break;
            default: break;
         }
      }
   }
   // Consistency Check (Corresponds to Normalization)
   assert (BV_XOR == ntk->getGateType(id2) || BV_XOR != ntk->getGateType(id1));
   assert (BV_CONST == ntk->getGateType(id1) || BV_CONST != ntk->getGateType(id2));
   return false;
}

const bool rewrite_ADD(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_ADD == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   if (1 == ntk->getNetWidth(inputs[0].id)) { type = BV_XOR; return rewrite_XOR(ntk, type, inputs, netHash); }
   // Perform Normalization on Inputs
   if (type == ntk->getGateType(inputs[0].id)) rewriteSwapNets(inputs[0], inputs[1]);
   if (BV_CONST == ntk->getGateType(inputs[1].id)) rewriteSwapNets(inputs[0], inputs[1]);
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (value1.all0()) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
      if (BV_CONST == ntk->getGateType(id2)) {
         V3BitVecX value = ntk->getInputConstValue(id2); value += value1;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
      // Check if there Exists a Constant Grandson who Has a Non-Inverting ADD Parent
      if (BV_ADD == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         if (BV_CONST == ntk->getGateType(id21)) {
            V3BitVecX value = ntk->getInputConstValue(id21); value += value1;
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
            inputs[1] = id22; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_ADD == ntk->getGateType(id1) && !isV3NetInverted(id1) && 
       BV_ADD == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id11) && BV_CONST == ntk->getGateType(id21)) {
         const V3BitVecX value2 = ntk->getInputConstValue(id11);
         V3BitVecX value = ntk->getInputConstValue(id21); value += value2;
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
         inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id12, id22);
         return rewrite_ADD(ntk, type, inputs, netHash);
      }
   }
#endif
   // Perform Identity Check
   if (id1 ==  id2) {
      inputs[1] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                  ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "1")));
      inputs[0] = id1; type = BV_SHL; return rewrite_SHL(ntk, type, inputs, netHash);
   }
   if (id1 == ~id2) return rewriteReturnConst(ntk, type, netHash, true, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
   // NOTE: Rule for Inverting ADDER: (a + b)' --> 1 + a' + b'
   if (BV_ADD == ntk->getGateType(id1)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      if (id2 == id11) {
         if (isV3NetInverted(id1)) return rewriteReturnTerminal(ntk, type, inputs, id12, true);
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id2, id2);
            inputs[1] = id12; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id2 == id12) {
         if (isV3NetInverted(id1)) return rewriteReturnTerminal(ntk, type, inputs, id11, true);
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id2, id2);
            inputs[1] = id11; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id2 == ~id11) {
         if (!isV3NetInverted(id1)) {
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, true, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
            inputs[1] = id12; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id2 == ~id12) {
         if (!isV3NetInverted(id1)) {
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, true, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
            inputs[1] = id11; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_ADD == ntk->getGateType(id2)) {
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (id1 == id21) {
         if (isV3NetInverted(id2)) return rewriteReturnTerminal(ntk, type, inputs, id22, true);
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id1, id1);
            inputs[1] = id22; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id1 == id22) {
         if (isV3NetInverted(id2)) return rewriteReturnTerminal(ntk, type, inputs, id21, true);
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id1, id1);
            inputs[1] = id21; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id1 == ~id21) {
         if (!isV3NetInverted(id2)) {
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, true, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id2)), "0")));
            inputs[1] = id22; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
      if (id1 == ~id22) {
         if (!isV3NetInverted(id2)) {
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, true, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id2)), "0")));
            inputs[1] = id21; return rewrite_ADD(ntk, type, inputs, netHash);
         }
      }
   }
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_ADD: 
               if (!isV3NetInverted(id1) && !isV3NetInverted(id2)) {
                  if (id11.id == id21.id) {
                     inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id11, id21);
                     inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id12, id22);
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id11.id == id22.id) {
                     inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id11, id22);
                     inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id12, id21);
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id12.id == id21.id) {
                     inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id12, id21);
                     inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id11, id22);
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id12.id == id22.id) {
                     inputs[0] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id12, id22);
                     inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, id11, id21);
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
               }
               else if (isV3NetInverted(id1) ^ isV3NetInverted(id2)) {
                  if (id11 == id21) {
                     inputs[0] = id1.cp ? ~id12 : id12; inputs[1] = id2.cp ? ~id22 : id22;
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id11 == id22) {
                     inputs[0] = id1.cp ? ~id12 : id12; inputs[1] = id2.cp ? ~id21 : id21;
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id12 == id21) {
                     inputs[0] = id1.cp ? ~id11 : id11; inputs[1] = id2.cp ? ~id22 : id22;
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
                  if (id12 == id22) {
                     inputs[0] = id1.cp ? ~id11 : id11; inputs[1] = id2.cp ? ~id21 : id21;
                     return rewrite_ADD(ntk, type, inputs, netHash);
                  }
               }
               break;
            case BV_MULT: 
               if (!isV3NetInverted(id1) && !isV3NetInverted(id2)) {
                  V3NetId in1, in2;
                  if (id11 == id21) { type = BV_MULT; inputs[0] = id11; in1 = id12; in2 = id22; }
                  else if (id11 == id22) { type = BV_MULT; inputs[0] = id11; in1 = id12; in2 = id21; }
                  else if (id12 == id22) { type = BV_MULT; inputs[0] = id12; in1 = id11; in2 = id21; }
                  else if (id12 == id21) { type = BV_MULT; inputs[0] = id12; in1 = id11; in2 = id22; }
                  if (BV_MULT == type) {
                     inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, in1, in2);
                     return rewrite_MULT(ntk, type, inputs, netHash);
                  }
               }
               break;
            default: break;
         }
      }
   }
   // Consistency Check (Corresponds to Normalization)
   assert (BV_ADD == ntk->getGateType(id2) || BV_ADD != ntk->getGateType(id1));
   assert (BV_CONST == ntk->getGateType(id1) || BV_CONST != ntk->getGateType(id2));
   return false;
}

const bool rewrite_SUB(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_SUB == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_XOR; return rewrite_XOR(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (value2.all0()) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value -= value2;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
   }
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (value1.all0()) {
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "1")));
         inputs[1] = ~id2; type = BV_ADD; return rewrite_ADD(ntk, type, inputs, netHash);
      }
   }
#endif
   // Perform Identity Check
   if (id1 ==  id2) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                           rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
   if (id1 == ~id2) { type = BV_ADD; inputs[0] = inputs[1] = id2; return rewrite_ADD(ntk, type, inputs, netHash); }
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_MULT: 
               if (!isV3NetInverted(id1) && !isV3NetInverted(id2)) {
                  if (id11 == id21) {
                     type = BV_MULT; inputs[0] = id11;
                     inputs[1] = rewriteReturnNetId(ntk, BV_SUB, netHash, false, id12, id22);
                  }
                  else if (id12 == id22) {
                     type = BV_MULT; inputs[0] = id12;
                     inputs[1] = rewriteReturnNetId(ntk, BV_SUB, netHash, false, id11, id21);
                  }
                  if (BV_MULT == type) return rewrite_MULT(ntk, type, inputs, netHash);
               }
               break;
            default: break;
         }
      }
   }
   // Rewrite into ADDER: (a - b) --> 1 + (a + b') --> (a' + b)'
   type = BV_ADD; inputs[0].id.cp ^= 1;
   return !rewrite_ADD(ntk, type, inputs, netHash);
}

const bool rewrite_MULT(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_MULT == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   if (1 == ntk->getNetWidth(inputs[0].id)) { type = BV_AND; return rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Normalization on Inputs
   if (type == ntk->getGateType(inputs[0].id)) rewriteSwapNets(inputs[0], inputs[1]);
   if (BV_CONST == ntk->getGateType(inputs[1].id)) rewriteSwapNets(inputs[0], inputs[1]);
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (value1.all0()) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      if (value1.regEx() == (string('0', ntk->getNetWidth(id1) - 1) + "1")) 
         return rewriteReturnTerminal(ntk, type, inputs, id2, false);
      if (BV_CONST == ntk->getGateType(id2)) {
         V3BitVecX value = ntk->getInputConstValue(id2); value *= value1;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
      // Check if there Exists a Constant Grandson who Has a Non-Inverting MULT Parent
      if (BV_MULT == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         if (BV_CONST == ntk->getGateType(id21)) {
            V3BitVecX value = ntk->getInputConstValue(id21); value *= value1;
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id2)), value.regEx())));
            inputs[1] = id22; return rewrite_MULT(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_MULT == ntk->getGateType(id1) && !isV3NetInverted(id1) && 
       BV_MULT == ntk->getGateType(id2) && !isV3NetInverted(id2)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id11) && BV_CONST == ntk->getGateType(id21)) {
         const V3BitVecX value2 = ntk->getInputConstValue(id11);
         V3BitVecX value = ntk->getInputConstValue(id21); value *= value2;
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id21)), value.regEx())));
         inputs[1] = rewriteReturnNetId(ntk, BV_MULT, netHash, false, id12, id22);
         return rewrite_MULT(ntk, type, inputs, netHash);
      }
   }
#endif
   // Perform Constant Reduction
#ifdef V3_REWRITE_CONST_REDUCTION_ENABLED
   V3UI32Queue idQueue; idQueue.push(V3NetType(id1).value); idQueue.push(V3NetType(id2).value);
   V3UI32Set idSet; idSet.clear(); V3BitVecX* idValue = 0; V3NetId id;
   while (!idQueue.empty()) {
      id = ((V3NetType)(idQueue.front())).id; assert (id.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id)) {
         if (!idValue) { idValue = new V3BitVecX(ntk->getInputConstValue(id)); assert (idValue); }
         else { (*idValue) *= ntk->getInputConstValue(id); if (idValue->all0()) break; }
      }
      else {
         if (BV_MULT == ntk->getGateType(id) && !isV3NetInverted(id)) {
            idQueue.push(V3NetType(ntk->getInputNetId(id, 0)).value);
            idQueue.push(V3NetType(ntk->getInputNetId(id, 1)).value);
         }
         idSet.insert(idQueue.front());
      }
      idQueue.pop();
   }
   if (!idQueue.empty()) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
#endif
   // Consistency Check (Corresponds to Normalization)
   assert (BV_MULT == ntk->getGateType(id2) || BV_MULT != ntk->getGateType(id1));
   assert (BV_CONST == ntk->getGateType(id1) || BV_CONST != ntk->getGateType(id2));

   return false;
}

const bool rewrite_DIV(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_DIV == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_AND; inputs[0] = ~id1; return !rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (value2.regEx() == (string('0', ntk->getNetWidth(id1) - 1) + "1")) 
         return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value /= value2;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
   }
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value = ntk->getInputConstValue(id1);
      if (value.all0()) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   }
#endif
   // Perform Identity Check
   if (id1 == id2) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                          rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "1"));
   
   return false;
}

const bool rewrite_MODULO(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_MODULO == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_AND; inputs[1] = ~id2; return rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (value2.regEx() == (string('0', ntk->getNetWidth(id1) - 1) + "1")) 
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value %= value2;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
   }
   if (ntk->getGateType(id1) == BV_CONST) {
      const V3BitVecX value = ntk->getInputConstValue(id1);
      if (value.all0()) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   }
#endif
   // Perform Identity Check
   if (id1 == id2) return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                          rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));

   return false;
}

const bool rewrite_SHL(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_SHL == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_AND; inputs[1] = ~id2; return rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value <<= value2;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
      else if (value2.size() < (sizeof(uint32_t) << 3)) {
         const uint32_t shiftValue = value2.value();
         if (!shiftValue)
            return rewriteReturnTerminal(ntk, type, inputs, id1, false);
         else if (shiftValue >= ntk->getNetWidth(id1)) 
            return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                   rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id1, 
                        ntk->hashV3BusId(ntk->getNetWidth(id1) - shiftValue - 1, 0));
            inputs[1] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(shiftValue), "0")));
            type = BV_MERGE; return rewrite_MERGE(ntk, type, inputs, netHash);
         }
      }
   }
#endif
   // Rewrite on Shift Chain
   if ((BV_SHL == ntk->getGateType(id1)) && !isV3NetInverted(id1)) {
      inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, ntk->getInputNetId(id1, 1), id2);
      inputs[0] = ntk->getInputNetId(id1, 0); return rewrite_SHL(ntk, type, inputs, netHash);
   }
   return false;
}

const bool rewrite_SHR(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_SHR == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_AND; inputs[1] = ~id2; return rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value >>= value2;
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), value.regEx()));
      }
      else if (value2.size() < (sizeof(uint32_t) << 3)) {
         const uint32_t shiftValue = value2.value();
         if (!shiftValue)
            return rewriteReturnTerminal(ntk, type, inputs, id1, false);
         else if (shiftValue >= ntk->getNetWidth(id1)) 
            return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                   rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0"));
         else {
            inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(shiftValue), "0")));
            inputs[1] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id1, 
                        ntk->hashV3BusId(ntk->getNetWidth(id1) - 1, shiftValue));
            type = BV_MERGE; return rewrite_MERGE(ntk, type, inputs, netHash);
         }
      }
   }
#endif
   // Rewrite on Shift Chain
   if ((BV_SHR == ntk->getGateType(id1)) && !isV3NetInverted(id1)) {
      inputs[1] = rewriteReturnNetId(ntk, BV_ADD, netHash, false, ntk->getInputNetId(id1, 1), id2);
      inputs[0] = ntk->getInputNetId(id1, 0); return rewrite_SHR(ntk, type, inputs, netHash);
   }

   return false;
}

const bool rewrite_MERGE(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_MERGE == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : {a', b'} == {a, b}'
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
      id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, true);
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
#endif
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (BV_CONST == ntk->getGateType(id1)) {
         V3BitVecX value = ntk->getInputConstValue(id1); value = value.bv_concat(value2);
         return rewriteReturnConst(ntk, type, netHash, false, inputs, 
                rewriteGetExpr(v3Int2Str(value.size()), value.regEx()));
      }
      if (BV_MERGE == ntk->getGateType(id1)) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         if (BV_CONST == ntk->getGateType(id12)) {
            V3BitVecX value = ntk->getInputConstValue(id12);
            if (id1.cp) value = ~value; value = value.bv_concat(value2);
            inputs[0] = id1.cp ? ~id11 : id11;
            inputs[1] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                        ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(value.size()), value.regEx())));
            return rewrite_MERGE(ntk, type, inputs, netHash);
         }
      }
   }
   if (BV_CONST == ntk->getGateType(id1) && BV_MERGE == ntk->getGateType(id2)) {
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (BV_CONST == ntk->getGateType(id21)) {
         const V3BitVecX value1 = ntk->getInputConstValue(id1);
         V3BitVecX value = ntk->getInputConstValue(id21);
         if (id2.cp) value = ~value; value = value1.bv_concat(value);
         inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                     ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(value.size()), value.regEx())));
         inputs[1] = id2.cp ? ~id22 : id22;
         return rewrite_MERGE(ntk, type, inputs, netHash);
      }
   }
#endif

   return false;
}

const bool rewrite_EQUALITY(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_EQUALITY == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : (a' == b') == (a == b)
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
      id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
      inputs[0].id.cp ^= 1; inputs[1].id.cp ^= 1;
#endif
   }
   if (!V3NtkHandler::rewriteON()) return false;
   
   // Reduce to Boolean Cases
   if (1 == ntk->getNetWidth(inputs[0].id)) { type = BV_XOR; return !rewrite_XOR(ntk, type, inputs, netHash); }
   // Perform Normalization on Inputs
   if (type == ntk->getGateType(inputs[0].id)) rewriteSwapNets(inputs[0], inputs[1]);
   if (BV_CONST == ntk->getGateType(inputs[1].id)) rewriteSwapNets(inputs[0], inputs[1]);
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1) && BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1), value2 = ntk->getInputConstValue(id2);
      return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), (value1 == value2));
   }
#endif
   // Perform Identity Check
   if (id1 ==  id2) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
   if (id1 == ~id2) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_AND: 
               if (isV3NetInverted(id1) == isV3NetInverted(id2)) {
                  if (id11 == ~id21) {
                     if ((id12 == ~id22)) {
                        inputs[0] = id12; inputs[1] = id21;
                        return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                     if (id12 == id22) {
                        inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                                    ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
                        inputs[1] = id12; return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                  }
                  if (id11 == ~id22) {
                     if (id12 == ~id21) {
                        inputs[0] = id12; inputs[1] = id22;
                        return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                     if (id12 == id21) {
                        inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                                    ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
                        inputs[1] = id12; return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                  }
                  if (id12 == ~id21) {
                     if ((id11 == ~id22)) {
                        inputs[0] = id11; inputs[1] = id21;
                        return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                     if (id11 == id22) {
                        inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                                    ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
                        inputs[1] = id11; return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                  }
                  if (id12 == ~id22) {
                     if (id11 == ~id21) {
                        inputs[0] = id11; inputs[1] = id22;
                        return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                     if (id11 == id21) {
                        inputs[0] = rewriteReturnNetId(ntk, BV_CONST, netHash, false, 
                                    ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(ntk->getNetWidth(id1)), "0")));
                        inputs[1] = id11; return rewrite_EQUALITY(ntk, type, inputs, netHash);
                     }
                  }
               }
               break;
            case BV_XOR: 
            case BV_ADD: 
               if (isV3NetInverted(id1) == isV3NetInverted(id2)) {
                  if (id11 == id21) {
                     inputs[0] = id12; inputs[1] = id22;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  else if (id11 == id22) {
                     inputs[0] = id12; inputs[1] = id21;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  else if (id12 == id22) {
                     inputs[0] = id11; inputs[1] = id21;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  else if (id12 == id21) {
                     inputs[0] = id11; inputs[1] = id22;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
               }
               break;
            case BV_SUB: 
               if (isV3NetInverted(id1) == isV3NetInverted(id2)) {
                  if (id11 == id21) { 
                     inputs[0] = id12; inputs[1] = id22;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  if (id12 == id22) { 
                     inputs[0] = id11; inputs[1] = id21;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
               }
            case BV_MERGE: 
               // REWRITE_FUTURE: 32'd0 == {30'd0, x} --> 2'd0 == x
               //                 32'd0 == {x, 30'd0} --> 2'd0 == x
               if (isV3NetInverted(id1) == isV3NetInverted(id2)) {
                  if (id11 == id21) { 
                     inputs[0] = id12; inputs[1] = id22;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  if (id12 == id22) { 
                     inputs[0] = id11; inputs[1] = id21;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
               }
               else {
                  if (id11 == ~id21) { 
                     inputs[0] = id12; inputs[1] = ~id22;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
                  if (id12 == ~id22) { 
                     inputs[0] = id11; inputs[1] = ~id21;
                     return rewrite_EQUALITY(ntk, type, inputs, netHash);
                  }
               }
               break;
            default: break;
         }
      }
   }

   // Consistency Check (Corresponds to Normalization)
   assert (BV_EQUALITY == ntk->getGateType(id2) || BV_EQUALITY != ntk->getGateType(id1));
   assert (BV_CONST == ntk->getGateType(id1) || BV_CONST != ntk->getGateType(id2));

   return false;
}

const bool rewrite_GEQ(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_GEQ == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvPairTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) { type = BV_AND; inputs[0] = ~id1; return !rewrite_AND(ntk, type, inputs, netHash); }
   // Perform Constant Propagation
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id2)) {
      const V3BitVecX value2 = ntk->getInputConstValue(id2);
      if (value2.all0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
      if (value2.all1()) { type = BV_EQUALITY; return rewrite_EQUALITY(ntk, type, inputs, netHash); }
      if (BV_CONST == ntk->getGateType(id1)) {
         const V3BitVecX value1 = ntk->getInputConstValue(id1);
         return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), ('1' == (value1.bv_geq(value2))[0]));
      }
   }
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      if (value1.all0()) { type = BV_EQUALITY; return rewrite_EQUALITY(ntk, type, inputs, netHash); }
      if (value1.all1()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
   }
#endif
   // Perform Identity Check
   if (id1 == id2) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
   // REWRITE_FUTURE: id1 == ~id2 case  (not sure)

   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(id1) == ntk->getGateType(id2)) {
      if (isV3PairType(ntk->getGateType(id1))) {
         const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
         const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
         const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
         const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
         switch (ntk->getGateType(id1)) {
            case BV_MERGE: 
               if (ntk->getNetWidth(id11) == ntk->getNetWidth(id21)) {
                  assert (ntk->getNetWidth(id12) == ntk->getNetWidth(id22));
                  if ((id1.cp ? ~id11 : id11) == (id2.cp ? ~id21 : id21)) {
                     inputs[0] = id1.cp ? ~id12 : id12; inputs[1] = id2.cp ? ~id22 : id22;
                     return rewrite_GEQ(ntk, type, inputs, netHash);
                  }
                  if ((id1.cp ? ~id12 : id12) == (id2.cp ? ~id22 : id22)) {
                     inputs[0] = id1.cp ? ~id11 : id11; inputs[1] = id2.cp ? ~id21 : id21;
                     return rewrite_GEQ(ntk, type, inputs, netHash);
                  }
               }
               break;
            default: break;
         }
      }
   }

   return false;
}

const bool rewrite_RED_AND(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_RED_AND == type); assert (inputs.size() == 1);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvReducedTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value = ntk->getInputConstValue(id1);
      return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), '1' == (value.bv_red_and())[0]);
   }
#endif
   // Perform Rewrite Rules for Child with Specific Type
   if (isV3PairType(ntk->getGateType(id1))) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      switch (ntk->getGateType(id1)) {
         case BV_AND: 
            if (!id1.cp && BV_CONST == ntk->getGateType(id11)) {
               const V3BitVecX value = ntk->getInputConstValue(id11);
               if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
            }
            break;
         case BV_MERGE: 
            if (BV_CONST == ntk->getGateType(id11)) {
               const V3BitVecX value = ntk->getInputConstValue(id11);
               if (!id1.cp) {
                  if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
                  if (value.all1()) { inputs[0] = id12; return rewrite_RED_AND(ntk, type, inputs, netHash); }
               }
               else {
                  if (value.exist1()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
                  if (value.all0()) { inputs[0] = ~id12; return rewrite_RED_AND(ntk, type, inputs, netHash); }
               }
            }
            if (BV_CONST == ntk->getGateType(id12)) {
               const V3BitVecX value = ntk->getInputConstValue(id12);
               if (!id1.cp) {
                  if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
                  if (value.all1()) { inputs[0] = id11; return rewrite_RED_AND(ntk, type, inputs, netHash); }
               }
               else {
                  if (value.exist1()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
                  if (value.all0()) { inputs[0] = ~id11; return rewrite_RED_AND(ntk, type, inputs, netHash); }
               }
            }
            break;
         default: break;
      }
   }

   return false;
}

const bool rewrite_RED_OR(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_RED_OR == type); assert (inputs.size() == 1);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvReducedTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value = ntk->getInputConstValue(id1);
      return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), '1' == (value.bv_red_or())[0]);
   }
#endif
   // Perform Rewrite Rules for Child with Specific Type
   if (isV3PairType(ntk->getGateType(id1))) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      switch (ntk->getGateType(id1)) {
         case BV_AND: 
            if (id1.cp && BV_CONST == ntk->getGateType(id11)) {
               const V3BitVecX value = ntk->getInputConstValue(id11);
               if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
            }
            break;
         case BV_MERGE: 
            if (BV_CONST == ntk->getGateType(id11)) {
               const V3BitVecX value = ntk->getInputConstValue(id11);
               if (!id1.cp) {
                  if (value.exist1()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
                  if (value.all0()) { inputs[0] = id12; return rewrite_RED_OR(ntk, type, inputs, netHash); }
               }
               else {
                  if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
                  if (value.all1()) { inputs[0] = ~id12; return rewrite_RED_OR(ntk, type, inputs, netHash); }
               }
            }
            if (BV_CONST == ntk->getGateType(id12)) {
               const V3BitVecX value = ntk->getInputConstValue(id12);
               if (!id1.cp) {
                  if (value.exist1()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
                  if (value.all0()) { inputs[0] = id11; return rewrite_RED_OR(ntk, type, inputs, netHash); }
               }
               else {
                  if (value.exist0()) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), true);
                  if (value.all1()) { inputs[0] = ~id11; return rewrite_RED_OR(ntk, type, inputs, netHash); }
               }
            }
            break;
         default: break;
      }
   }

   return false;
}

const bool rewrite_RED_XOR(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_RED_XOR == type); assert (inputs.size() == 1);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashBvReducedTypeGate(ntk, type, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   if (1 == ntk->getNetWidth(id1)) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value = ntk->getInputConstValue(id1);
      return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), '1' == (value.bv_red_xor())[0]);
   }
#endif
   // Perform Rewrite Rules for Child with Specific Type
   if (isV3PairType(ntk->getGateType(id1))) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      switch (ntk->getGateType(id1)) {
         case BV_MERGE: 
            if (BV_CONST == ntk->getGateType(id11)) {
               const V3BitVecX value = ntk->getInputConstValue(id11);
               if (!id1.cp) { 
                  inputs[0] = id12;
                  return ('1' == (value.bv_red_xor())[0]) ^ rewrite_RED_XOR(ntk, type, inputs, netHash);
               }
               // REWRITE_FUTURE: Currently not sure for id1.cp cases
            }
            if (BV_CONST == ntk->getGateType(id12)) {
               const V3BitVecX value = ntk->getInputConstValue(id12);
               if (!id1.cp) {
                  inputs[0] = id11;
                  return ('1' == (value.bv_red_xor())[0]) ^ rewrite_RED_XOR(ntk, type, inputs, netHash);
               }
               // REWRITE_FUTURE: Currently not sure for id1.cp cases
            }
            break;
         default: break;
      }
   }

   return false;
}

const bool rewrite_MUX(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_MUX == type); assert (inputs.size() == 3);
   // Perform Normalization on Inputs  (Must Perform before Structural Hashing)
   if (ntk->getGateType(inputs[0].id) == BV_MUX) { rewriteSwapNets(inputs[0], inputs[1]); (inputs[2].id).cp ^= 1; }
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvMuxGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : (s' ? t : f) == (s ? f : t)
      rewriteSwapNets(inputs[0], inputs[1]); inputs[2].id.cp ^= 1;
      id = strashBvMuxGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
      rewriteSwapNets(inputs[0], inputs[1]); inputs[2].id.cp ^= 1;
#endif
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Perform Constant Propagation
   const V3NetId fId = inputs[0].id; assert (fId.id < ntk->getNetSize());
   const V3NetId tId = inputs[1].id; assert (tId.id < ntk->getNetSize());
   const V3NetId sId = inputs[2].id; assert (sId.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(sId)) {
      const V3BitVecX value = ntk->getInputConstValue(sId);
      return rewriteReturnTerminal(ntk, type, inputs, (('1' == value[0]) ? tId : fId), false);
   }
   if (BV_CONST == ntk->getGateType(fId) && BV_CONST == ntk->getGateType(tId)) {
      const V3BitVecX fValue = ntk->getInputConstValue(fId), tValue = ntk->getInputConstValue(tId);
      if (fValue == tValue) return rewriteReturnTerminal(ntk, type, inputs, fId, false);
      if (1 == ntk->getNetWidth(fId)) return rewriteReturnTerminal(ntk, type, inputs, sId, '0' == tValue[0]);
   }
#endif
   // Perform Identity Check
   if (tId == fId) return rewriteReturnTerminal(ntk, type, inputs, tId, false);
   // Perform Rewrite Rules for Children with Same Type
   if (ntk->getGateType(fId) == ntk->getGateType(tId)) {
      if (isV3PairType(ntk->getGateType(fId))) {
         const V3NetId fId1 = ntk->getInputNetId(fId, 0); assert (fId1.id < ntk->getNetSize());
         const V3NetId fId2 = ntk->getInputNetId(fId, 1); assert (fId2.id < ntk->getNetSize());
         const V3NetId tId1 = ntk->getInputNetId(tId, 0); assert (tId1.id < ntk->getNetSize());
         const V3NetId tId2 = ntk->getInputNetId(tId, 1); assert (tId2.id < ntk->getNetSize());
         if (isV3NetInverted(fId) == isV3NetInverted(tId)) {
            if (fId1 == tId1) {
               type = ntk->getGateType(fId); inputs.pop_back(); assert (2 == inputs.size());
               inputs[1] = rewriteReturnNetId(ntk, BV_MUX, netHash, false, fId2, tId2, sId);
               inputs[0] = fId1; return isV3NetInverted(fId) ^ rewriteBvGate(ntk, type, inputs, netHash);
            }
            if (fId2 == tId2) {
               type = ntk->getGateType(fId); inputs.pop_back(); assert (2 == inputs.size());
               inputs[0] = rewriteReturnNetId(ntk, BV_MUX, netHash, false, fId1, tId1, sId);
               inputs[1] = fId2; return isV3NetInverted(fId) ^ rewriteBvGate(ntk, type, inputs, netHash);
            }
         }
         switch (ntk->getGateType(fId)) {
            case BV_AND: 
            case BV_XOR: 
            case BV_ADD: 
            case BV_MULT: 
               if (isV3NetInverted(fId) == isV3NetInverted(tId)) {
                  if (fId1 == tId2) {
                     type = ntk->getGateType(fId); inputs.pop_back(); assert (2 == inputs.size());
                     inputs[1] = rewriteReturnNetId(ntk, BV_MUX, netHash, false, fId2, tId1, sId);
                     inputs[0] = fId1; return isV3NetInverted(fId) ^ rewriteBvGate(ntk, type, inputs, netHash);
                  }
                  if (fId2 == tId1) {
                     type = ntk->getGateType(fId); inputs.pop_back(); assert (2 == inputs.size());
                     inputs[0] = rewriteReturnNetId(ntk, BV_MUX, netHash, false, fId1, tId2, sId);
                     inputs[1] = fId2; return isV3NetInverted(fId) ^ rewriteBvGate(ntk, type, inputs, netHash);
                  }
               }
               break;
            default: break;
         }
      }
   }
   // Perform MUX Chain Rewriting
   if (BV_MUX == ntk->getGateType(tId) && !isV3NetInverted(tId)) {
      const V3NetId tIdf = ntk->getInputNetId(tId, 0); assert (tIdf.id < ntk->getNetSize());
      const V3NetId tIdt = ntk->getInputNetId(tId, 1); assert (tIdt.id < ntk->getNetSize());
      const V3NetId tIds = ntk->getInputNetId(tId, 2); assert (tIds.id < ntk->getNetSize());
      if (sId.id == tIds.id) {
         inputs[1] = (sId.cp == tIds.cp) ? tIdt : tIdf;
         return rewrite_MUX(ntk, type, inputs, netHash);
      }
      if (fId == tIdt) { 
         inputs[2] = rewriteReturnNetId(ntk, BV_AND, netHash, false, sId, ~tIds);
         inputs[1] = tIdf; return rewrite_MUX(ntk, type, inputs, netHash);
      }
      if (fId == tIdf) {
         inputs[2] = rewriteReturnNetId(ntk, BV_AND, netHash, false, sId, tIds);
         inputs[1] = tIdt; return rewrite_MUX(ntk, type, inputs, netHash);
      }
   }

   // Consistency Check (Corresponds to Normalization)
   assert (BV_MUX == ntk->getGateType(tId) || BV_MUX != ntk->getGateType(fId));
   return false;
}

const bool rewrite_SLICE(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_SLICE == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvSliceGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : s'[slice] == s[slice]'
      inputs[0].id.cp ^= 1; id = strashBvSliceGate(ntk, inputs, netHash); inputs[0].id.cp ^= 1;
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, true);
#endif
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Reduce to Boolean Cases
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3BusId busId = inputs[1].value; assert (ntk->getSliceWidth(busId) <= ntk->getNetWidth(id1));
   if ((ntk->getNetWidth(id1) == ntk->getSliceWidth(busId)) && !ntk->getSliceBit(busId, 0)) 
      return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   // Perform Constant Propagation
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (BV_CONST == ntk->getGateType(id1)) {
      const V3BitVecX value1 = ntk->getInputConstValue(id1);
      const V3BitVecX value = value1.bv_slice(ntk->getSliceBit(busId, 1), ntk->getSliceBit(busId, 0));
      return rewriteReturnConst(ntk, type, netHash, false, inputs, 
             rewriteGetExpr(v3Int2Str(value.size()), value.regEx()));
   }
#endif
   // Perform Rewrite Rules for Children with Same Type
   const uint32_t msb = ntk->getSliceBit(busId, 1); assert (msb <= ntk->getNetWidth(id1));
   const uint32_t lsb = ntk->getSliceBit(busId, 0); assert (lsb <= ntk->getNetWidth(id1));
   if (isV3PairType(ntk->getGateType(id1))) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const uint32_t width2 = ntk->getNetWidth(id12);
      switch (ntk->getGateType(id1)) {
         //case BV_AND: 
         //case BV_XOR: 
         //   inputs[0] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id11, busId);
         //   inputs[1] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id12, busId);
         //   type = ntk->getGateType(id1); return id1.cp ^ rewriteBvGate(ntk, type, inputs, netHash);
         case BV_MERGE: 
            if (lsb >= width2 && msb >= width2) {
               inputs[0] = id11; inputs[1] = ntk->hashV3BusId(msb - width2, lsb - width2);
               return id1.cp ^ rewrite_SLICE(ntk, type, inputs, netHash);
            }
            if (msb < width2 && lsb < width2) {
               inputs[0] = id12;
               return id1.cp ^ rewrite_SLICE(ntk, type, inputs, netHash);
            }
            break;
         default: break;
      }
   }
   //if (BV_MUX == ntk->getGateType(id1)) {
   //   const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
   //   const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
   //   inputs[0] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id11, busId);
   //   inputs[1] = rewriteReturnNetId(ntk, BV_SLICE, netHash, false, id12, busId);
   //   inputs.push_back(ntk->getInputNetId(id1, 2)); assert (inputs.back().id.id < ntk->getNetSize());
   //   type = BV_MUX; return id1.cp ^ rewrite_MUX(ntk, type, inputs, netHash);
   //}
   if (BV_SLICE == ntk->getGateType(id1)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const uint32_t lsb1 = ntk->getInputSliceBit(id1, 0); assert (lsb1 <= ntk->getNetWidth(id11));
      inputs[0] = id11; inputs[1] = ntk->hashV3BusId(msb + lsb1, lsb + lsb1);
      return id1.cp ^ rewrite_SLICE(ntk, type, inputs, netHash);
   }
   return false;
}

const bool rewrite_CONST(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (BV_CONST == type); assert (inputs.size() == 1);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      V3NetId id = strashBvConstGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
#ifdef V3_REWRITE_STRASH_DUAL_REPR
      // Dual Representation : (a == expr') == (a' == expr)
      const V3NetType input = inputs[0]; const V3BitVecX value = ~(ntk->getConstValue(input.value));
      inputs[0] = ntk->hashV3ConstBitVec(rewriteGetExpr(v3Int2Str(value.size()), value.regEx()));
      id = strashBvConstGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, true); inputs[0] = input;
#endif
   }
   return false;
}

const bool rewrite_AIG_NODE(V3AigNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (AIG_NODE == type); assert (inputs.size() == 2);
   // Perform Structural Hashing
   if (V3NtkHandler::strashON()) {
      const V3NetId id = strashAigNodeGate(ntk, inputs, netHash);
      if (V3NetUD != id) return rewriteReturnTerminal(ntk, type, inputs, id, false);
   }
   if (!V3NtkHandler::rewriteON()) return false;
   // Perform Constant Propagation
   const V3NetId id1 = inputs[0].id; assert (id1.id < ntk->getNetSize());
   const V3NetId id2 = inputs[1].id; assert (id2.id < ntk->getNetSize());
#ifdef V3_REWRITE_CONST_PROP_ENABLED
   if (AIG_FALSE == ntk->getGateType(id1)) 
      return rewriteReturnTerminal(ntk, type, inputs, id1.cp ? id2 : id1, false);
   if (AIG_FALSE == ntk->getGateType(id2)) 
      return rewriteReturnTerminal(ntk, type, inputs, id2.cp ? id1 : id2, false);
#endif
   // Perform Identity Check
   if (id1 ==  id2) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
   if (id1 == ~id2) return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
   if (AIG_NODE == ntk->getGateType(id1)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      if (!isV3NetInverted(id1)) {
         if (id2 == ~id11 || id2 == ~id12) 
            return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
         if (id2 ==  id11 || id2 ==  id12) 
            return rewriteReturnTerminal(ntk, type, inputs, id1, false);
      }
      else {
         if (id2 == ~id11 || id2 == ~id12) return rewriteReturnTerminal(ntk, type, inputs, id2, false);
         if (id2 == id11) { inputs[0] = ~id12; return rewrite_AIG_NODE(ntk, type, inputs, netHash); }
         if (id2 == id12) { inputs[0] = ~id11; return rewrite_AIG_NODE(ntk, type, inputs, netHash); }
      }
   }
   if (AIG_NODE == ntk->getGateType(id2)) {
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (!isV3NetInverted(id2)) {
         if (id1 == ~id21 || id1 == ~id22) 
            return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
         if (id1 ==  id21 || id1 ==  id22) 
            return rewriteReturnTerminal(ntk, type, inputs, id2, false);
      }
      else {
         if (id1 == ~id21 || id1 == ~id22) return rewriteReturnTerminal(ntk, type, inputs, id1, false);
         if (id1 == id21) { inputs[1] = ~id22; return rewrite_AIG_NODE(ntk, type, inputs, netHash); }
         if (id1 == id22) { inputs[1] = ~id21; return rewrite_AIG_NODE(ntk, type, inputs, netHash); }
      }
   }
   // Perform Rewrite Rules for Children with Same Type
   if (AIG_NODE == ntk->getGateType(id1) && AIG_NODE == ntk->getGateType(id2)) {
      const V3NetId id11 = ntk->getInputNetId(id1, 0); assert (id11.id < ntk->getNetSize());
      const V3NetId id12 = ntk->getInputNetId(id1, 1); assert (id12.id < ntk->getNetSize());
      const V3NetId id21 = ntk->getInputNetId(id2, 0); assert (id21.id < ntk->getNetSize());
      const V3NetId id22 = ntk->getInputNetId(id2, 1); assert (id22.id < ntk->getNetSize());
      if (!isV3NetInverted(id1) && !isV3NetInverted(id2)) {
         if (id11 == ~id21 || id11 == ~id22 || id12 == ~id21 || id12 == ~id22) 
            return rewriteReturnTerminal(ntk, type, inputs, V3NetId::makeNetId(0), false);
         if (id11 == id21) {
            inputs[1] = rewriteReturnAigNode(ntk, netHash, false, id12, id22);
            inputs[0] = id11; return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id11 == id22) {
            inputs[1] = rewriteReturnAigNode(ntk, netHash, false, id12, id21);
            inputs[0] = id11; return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id12 == id22) {
            inputs[1] = rewriteReturnAigNode(ntk, netHash, false, id11, id21);
            inputs[0] = id12; return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id12 == id21) {
            inputs[1] = rewriteReturnAigNode(ntk, netHash, false, id11, id22);
            inputs[0] = id12; return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
      }
      else if (isV3NetInverted(id1) && isV3NetInverted(id2)) {
         if (((id11 == id21) && (id12 == ~id22)) || ((id11 == id22) && (id12 == ~id21))) 
            return rewriteReturnTerminal(ntk, type, inputs, id11, true);
         if (((id12 == id21) && (id11 == ~id22)) || ((id12 == id22) && (id11 == ~id21))) 
            return rewriteReturnTerminal(ntk, type, inputs, id12, true);
      }
      else {
         if ((id11 == ~id21) || (id11 == ~id22) || (id12 == ~id21) || (id12 == ~id22)) 
            return rewriteReturnTerminal(ntk, type, inputs, (id1.cp ? id2 : id1), false);
         if (id11 == id21) {
            inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id12 : ~id22;
            return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id12 == id21) {
            inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id11 : ~id22;
            return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id11 == id22) {
            inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id12 : ~id21;
            return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
         if (id12 == id22) {
            inputs[0] = id1.cp ? id2 : id1; inputs[1] = id1.cp ? ~id11 : ~id21;
            return rewrite_AIG_NODE(ntk, type, inputs, netHash);
         }
      }
   }
   return false;
}

// Rewrite Main Functions
const bool rewriteBvGate(V3BvNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (V3_XD > type); assert (AIG_FALSE < type);
   if (isV3PairType(type)) {
      assert (inputs.size() == 2);
      switch (type) {
         case BV_AND       : return rewrite_AND(ntk, type, inputs, netHash);
         case BV_XOR       : return rewrite_XOR(ntk, type, inputs, netHash);
         case BV_ADD       : return rewrite_ADD(ntk, type, inputs, netHash);
         case BV_SUB       : return rewrite_SUB(ntk, type, inputs, netHash);
         case BV_SHL       : return rewrite_SHL(ntk, type, inputs, netHash);
         case BV_SHR       : return rewrite_SHR(ntk, type, inputs, netHash);
         case BV_MERGE     : return rewrite_MERGE(ntk, type, inputs, netHash);
         case BV_EQUALITY  : return rewrite_EQUALITY(ntk, type, inputs, netHash);
         case BV_GEQ       : return rewrite_GEQ(ntk, type, inputs, netHash);
         case BV_MULT      : return rewrite_MULT(ntk, type, inputs, netHash);
         case BV_DIV       : return rewrite_DIV(ntk, type, inputs, netHash);
         default           : return rewrite_MODULO(ntk, type, inputs, netHash);
      }
   }
   else if (isV3ReducedType(type)) {
      assert (inputs.size() == 1);
      switch (type) {
         case BV_RED_AND   : return rewrite_RED_AND(ntk, type, inputs, netHash);
         case BV_RED_OR    : return rewrite_RED_OR(ntk, type, inputs, netHash);
         default           : return rewrite_RED_XOR(ntk, type, inputs, netHash);
      }
   }
   else if (BV_MUX == type) {
      assert (inputs.size() == 3);
      return rewrite_MUX(ntk, type, inputs, netHash);
   }
   else if (BV_SLICE == type) {
      assert (inputs.size() == 2);
      return rewrite_SLICE(ntk, type, inputs, netHash);
   }
   else return rewrite_CONST(ntk, type, inputs, netHash);
}

const bool rewriteAigGate(V3AigNtk* const ntk, V3GateType& type, V3InputVec& inputs, V3PortableType& netHash) {
   assert (ntk); assert (AIG_NODE == type || AIG_FALSE == type);
   if (AIG_NODE == type) return rewrite_AIG_NODE(ntk, type, inputs, netHash);
   else { assert (!inputs.size()); return false; }
}

#endif

