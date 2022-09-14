/****************************************************************************
  FileName     [ v3BtorWriter.CPP ]
  PackageName  [ v3/src/io ]
  Synopsis     [ BTOR Writer for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BTOR_WRITER_H
#define V3_BTOR_WRITER_H

#include "v3Msg.h"
#include "v3StrUtil.h"
#include "v3NtkWriter.h"

// BTOR Primitive Types
const string V3TypeBTOR[] = {
   "var", "", "var", "", "", "",                         // PI, PIO, FF, MODULE, AIG GATES
   "redand", "redor", "redxor", "cond", "and", "xor",    // BV_(RED, LOGIC)
   "add", "sub", "mul", "udiv", "urem", "sll", "srl",    // BV_(ARITH)
   "concat", "eq", "ugte", "slice", "const"              // BV_(MODEL), BV_(COMP)
};

// BTOR Writer Main Function
void V3BtorWriter(const V3NtkHandler* const handler, const char* fileName, const bool& symbol) {
   // Check if Ntk is BV
   assert (handler); assert (handler->getNtk());
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(handler->getNtk()); assert (ntk);
   // Check if Primary Inout Exists
   if (ntk->getInoutSize()) {
      Msg(MSG_ERR) << "BTOR Incompatible Primary Inout Found (" << ntk->getInoutSize() << ") !!" << endl; return;
   }
   // Check if Module Instance Exists
   if (ntk->getModuleSize()) {
      Msg(MSG_ERR) << "BTOR Incompatible Module Instance Found (" << ntk->getModuleSize() << ") !!" << endl; return;
   }
   // Open BTOR Output File
   assert (fileName); ofstream output; output.open(fileName);
   if (!output.is_open()) { Msg(MSG_ERR) << "BTOR Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   const_cast<V3NtkHandler*>(handler)->setAuxRenaming();
   // Mapping from Current Ntk to BTOR Output Id
   V3NetVec orderMap; dfsNtkForGeneralOrder(ntk, orderMap); assert (orderMap.size());
   assert (!orderMap[0].id); assert (orderMap.size() <= ntk->getNetSize());
   V3UI32Vec idMap(ntk->getNetSize(), V3NtkUD); uint32_t index = 0;
   for (uint32_t i = 0; i < orderMap.size(); ++i) {
      assert (idMap.size() > orderMap[i].id);
      assert (V3NtkUD == idMap[orderMap[i].id]);
      idMap[orderMap[i].id] = ++index;
   }
   // Output Ntk to BTOR Format
   V3NetId id1, id2, id3; V3GateType type;
   // Output in Original BTOR Format
   for (uint32_t i = 0; i < orderMap.size(); ++i) {
      type = ntk->getGateType(orderMap[i]); assert (type != AIG_NODE && type != AIG_FALSE);
      assert (type < V3_XD); assert (orderMap.size() >= idMap[orderMap[i].id]);
      output << idMap[orderMap[i].id] << " " << V3TypeBTOR[type] << " " << ntk->getNetWidth(orderMap[i]);
      if (V3_FF >= type) {
         if (symbol) output << " " << V3RTLNameOrId(handler, orderMap[i]); output << endl;
      }
      else if (isV3PairType(type)) {
         id1 = ntk->getInputNetId(orderMap[i], 0);
         id2 = ntk->getInputNetId(orderMap[i], 1);
         output << " " << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id] 
                << " " << (isV3NetInverted(id2) ? "-" : "") << idMap[id2.id] << endl;
      }
      else if (isV3ReducedType(type)) {
         id1 = ntk->getInputNetId(orderMap[i], 0);
         output << " " << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id] << endl;
      }
      else if (BV_MUX == type) {
         id1 = ntk->getInputNetId(orderMap[i], 0);
         id2 = ntk->getInputNetId(orderMap[i], 1);
         id3 = ntk->getInputNetId(orderMap[i], 2);
         output << " " << (isV3NetInverted(id3) ? "-" : "") << idMap[id3.id]
                << " " << (isV3NetInverted(id2) ? "-" : "") << idMap[id2.id]
                << " " << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id] << endl;
      }
      else if (BV_SLICE == type) {
         id1 = ntk->getInputNetId(orderMap[i], 0);
         output << " " << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id]
                << " " << ntk->getInputSliceBit(orderMap[i], true) 
                << " " << ntk->getInputSliceBit(orderMap[i], false) << endl;
      }
      else if (BV_CONST == type) {
         output << " " << ntk->getInputConstValue(V3NetId::makeNetId(orderMap[i].id)).regEx() << endl;
      }
   }
   const uint32_t latchStart = 1 + ntk->getInputSize() + ntk->getInoutSize();
   for (uint32_t i = latchStart, j = 0; j < ntk->getLatchSize(); ++i, ++j) {
      assert (V3_FF == ntk->getGateType(orderMap[i]));
      id1 = ntk->getInputNetId(orderMap[i], 0);
      output << v3Int2Str(++index) << " next " << ntk->getNetWidth(orderMap[i]) << " " << idMap[orderMap[i].id]
             << " " << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id] << endl;
      // Output Warning for FF with Non-Zero Initial State
      id2 = ntk->getInputNetId(orderMap[i], 1);
      if (BV_CONST != ntk->getGateType(id2) || !ntk->getInputConstValue(id2).all0())
         Msg(MSG_WAR) << "DFF " << idMap[orderMap[i].id] << " has Non-Zero Initial value = "
                      << ntk->getInputConstValue(id2) << endl;
   }
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      id1 = ntk->getOutput(i);
      output << v3Int2Str(++index) << " root " << ntk->getNetWidth(id1) << " " 
             << (isV3NetInverted(id1) ? "-" : "") << idMap[id1.id];
      if (symbol) output << " " << (handler->getOutputName(i).size() ? V3RTLNameBase(handler, handler->getOutputName(i)) 
                                                                     : V3RTLNameById(handler, ntk->getOutput(i)));
      output << endl;
   }
   // Footer (Instead of Header) in BTOR Output
   writeV3GeneralHeader(symbol ? "BTOR with Symbolic Annotation" : "BTOR", output, ";");
   output.close(); const_cast<V3NtkHandler*>(handler)->resetAuxRenaming();
}

#endif

