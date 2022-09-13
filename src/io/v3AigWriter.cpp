/****************************************************************************
  FileName     [ v3AigWriter.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ AIGER Writer for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_AIG_WRITER_H
#define V3_AIG_WRITER_H

#include "v3Msg.h"
#include "v3StrUtil.h"
#include "v3NtkParser.h"
#include "v3NtkWriter.h"

// AIGER Writer Helper Function
void encode_aig(ofstream& output, int x) {
   unsigned char enc;
   while (x & ~0x7f) {
      enc = (x & 0x7f) | 0x80;
      output.put(enc);
      x >>= 7;
   }
   enc = x;
   output.put(enc);
}

// AIGER Writer Main Function
void V3AigWriter(const V3NtkHandler* const handler, const char* fileName, const bool& symbol) {
   // Check if Ntk is AIG
   assert (handler); assert (handler->getNtk());
   V3AigNtk* const ntk = handler->getNtk(); assert (!dynamic_cast<V3BvNtk*>(ntk));
   // Check if Primary Inout Exists
   if (ntk->getInoutSize()) {
      Msg(MSG_ERR) << "AIGER Incompatible Primary Inout Found (" << ntk->getInoutSize() << ") !!" << endl; return;
   }
   // Check if Module Instance Exists
   if (ntk->getModuleSize()) {
      Msg(MSG_ERR) << "AIGER Incompatible Module Instance Found (" << ntk->getModuleSize() << ") !!" << endl; return;
   }
   // Open AIGER Output File
   assert (fileName); ofstream output; output.open(fileName);
   if (!output.is_open()) { Msg(MSG_ERR) << "AIGER Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   const_cast<V3NtkHandler*>(handler)->setAuxRenaming();
   // Mapping from Current Ntk to AIGER Output Id
   V3NetVec orderMap; dfsNtkForGeneralOrder(ntk, orderMap); assert (orderMap.size());
   assert (!orderMap[0].id); assert (orderMap.size() <= ntk->getNetSize());
   V3Vec<V3NetId>::Vec c2bMap(ntk->getNetSize(), V3NetUD); c2bMap[0] = V3NetId::makeNetId(0);
   for (uint32_t i = 1; i < orderMap.size(); ++i) {
      assert (V3NetUD == c2bMap[orderMap[i].id]);
      c2bMap[orderMap[i].id] = V3NetId::makeNetId(i);
   }
   // Output AIGER Header : M I L O A
   output << "aig " << orderMap.size() - 1 << " " << ntk->getInputSize() << " " << ntk->getLatchSize() << " " 
          << ntk->getOutputSize() << " " << (orderMap.size() - ntk->getInputSize() - ntk->getLatchSize() - 1) << endl;
   // Output AIGER DFF : #PI + 1 ~ #PI + #DFF
   V3NetId id1, id2;
   for (uint32_t i = 1 + ntk->getInputSize(), j = i + ntk->getLatchSize(); i < j; ++i) {
      assert (V3_FF == ntk->getGateType(orderMap[i]));
      id1 = ntk->getInputNetId(orderMap[i], 0); assert (V3NetUD != c2bMap[id1.id]);
      output << ((c2bMap[id1.id].id << 1) + id1.cp);
      // Output Initial State Value if Necessary
      id2 = ntk->getInputNetId(orderMap[i], 1); assert (V3NetUD != c2bMap[id2.id]);
      if (AIG_FALSE == ntk->getGateType(c2bMap[id2.id])) { if (isV3NetInverted(id2)) output << " 1"; output << endl; }
      else if (orderMap[i] == id2) output << " " << ((c2bMap[id2.id].id << 1) + id2.cp) << endl;
      else  // Output Warning for FF with Non-Zero Initial State
         Msg(MSG_WAR) << "DFF " << c2bMap[orderMap[i].id].id << " has Non-Constant Initial value with Literal = " 
                      << ((c2bMap[id2.id].id << 1) + id2.cp) << endl;
   }
   // Output AIGER PO
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      id1 = ntk->getOutput(i); assert (V3NetUD != c2bMap[id1.id]);
      output << ((c2bMap[id1.id].id << 1) + id1.cp) << endl;
   }
   // Output AIGER AIG_NODE
   uint32_t in1, in2;
   for (uint32_t i = 1 + ntk->getInputSize() + ntk->getLatchSize(), j = orderMap.size(); i < j; ++i) {
      if (AIG_NODE == ntk->getGateType(orderMap[i])) {
         id1 = ntk->getInputNetId(orderMap[i], 0); assert (V3NetUD != c2bMap[id1.id]);
         id2 = ntk->getInputNetId(orderMap[i], 1); assert (V3NetUD != c2bMap[id2.id]);
         assert (i > c2bMap[id1.id].id); in1 = (c2bMap[id1.id].id << 1) + id1.cp;
         assert (i > c2bMap[id2.id].id); in2 = (c2bMap[id2.id].id << 1) + id2.cp;
         if (in1 >= in2) { encode_aig(output, (i << 1) - in1); encode_aig(output, in1 - in2); }
         else { encode_aig(output, (i << 1) - in2); encode_aig(output, in2 - in1); }
      }
      else {
         Msg(MSG_WAR) << "Extra Constant in AIGER with Literal = " << (i << 1) << endl;
         assert (AIG_FALSE == ntk->getGateType(orderMap[i]));
         encode_aig(output, (i << 1)); encode_aig(output, 0);
      }
   }
   // Output AIGER Symbolic Table
   if (symbol) {
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
         output << "i" << i << " " << V3RTLNameBase(handler, handler->getInputName(i)) << endl;
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
         output << "l" << i << " " << V3RTLNameOrId(handler, ntk->getLatch(i)) << endl;
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i)
         output << "o" << i << " " << V3RTLNameBase(handler, handler->getOutputName(i)) << endl;
   }
   // Footer (Instead of Header) in AIGER Output
   output << "c " << endl; writeV3GeneralHeader(symbol ? "AIGER with Symbol Table" : "AIGER", output, "c");
   output.close(); const_cast<V3NtkHandler*>(handler)->resetAuxRenaming();
}

#endif

