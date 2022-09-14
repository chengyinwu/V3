/****************************************************************************
  FileName     [ v3MCAiger.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Model Checker AIGER Input Handling. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_AIGER_C
#define V3_MC_AIGER_C

#include "v3MCMain.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkWriter.h"

// AIGER Parsing Helper Functions
unsigned char hwmcc_aiger_getnoneofch(FILE* file) {
   int ch = getc(file);
   if (ch != EOF) return ch;
   Msg(MSG_ERR) << "Unexpected EOF of AIGER Input Found !!!" << endl;
   return EOF;
}

unsigned hwmcc_aiger_decode(FILE* file) {
   unsigned x = 0, i = 0;
   unsigned char ch;
   while ((ch = hwmcc_aiger_getnoneofch(file)) & 0x80)
      x |= (ch & 0x7f) << (7 * i++);
   return x | (ch << (7 * i));
}

bool hwmcc_aiger_symbol(FILE* file, unsigned char& c, uint32_t& index, string& name) {
   int ch = getc(file); if (EOF == ch) return false;
   c = (unsigned char)ch; if ('c' == c) return true; name = "";
   while (true) {
      ch = getc(file); if (EOF == ch) return false;
      if (' ' == ch && name.size()) break;
      else name += (unsigned char)ch;
   }
   int temp; if (!v3Str2Int(name, temp)) return false;
   if (temp < 0) return false; index = (uint32_t)temp; name = "";
   while (true) {
      ch = getc(file); if (EOF == ch) return false;
      if ('\n' == ch) break;
      else name += (unsigned char)ch;
   }
   return true;
}

void hwmcc_encode_aig(ofstream& output, int x) {
   unsigned char enc;
   while (x & ~0x7f) { enc = (x & 0x7f) | 0x80; output.put(enc); x >>= 7; }
   enc = x; output.put(enc);
}

// V3 AIGER Specification Handling Functions
V3NtkInput* readAiger(const string& fileName, V3NetVec& badList, V3NetVec& invList, V3NetTable& fairList, const bool& hwmcc) {
   assert (fileName.size());
   // Open Input AIGER File
   FILE* input = fopen(fileName.c_str(), "r");
   if (!input) { Msg(MSG_ERR) << "AIG Input File \"" << fileName << "\" Not Found !!" << endl; return 0; }
   // Start Parsing AIG Header
   uint32_t m = 0, in = 0, l = 0, o = 0, a = 0, b = 0, c = 0, jt = 0, f = 0;
   char header[8];
   // Parse Header
   if (fscanf(input, "%s %u %u %u %u %u", header, &m, &in, &l, &o, &a) != 6) {
      Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0;
   }
   else if ((strcmp(header, "aig")) || (m != (in + l + a))) {
      Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0;
   }
   // Extension for AIGER 1.9: Header with B C J F
   unsigned char ch = getc(input);
   while ('\n' != ch) {
      if (fscanf(input, "%u", &b) != 1) { Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0; }
      ch = getc(input); if ('\n' == ch) break;
      if (fscanf(input, "%u", &c) != 1) { Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0; }
      ch = getc(input); if ('\n' == ch) break;
      if (fscanf(input, "%u", &jt) != 1) { Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0; }
      ch = getc(input); if ('\n' == ch) break;
      if (fscanf(input, "%u", &f) != 1) { Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0; }
      ch = getc(input); if ('\n' != ch) { Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0; }
   }
   // Check Validation for HWMCC
   if (hwmcc && jt > 1) {
      Msg(MSG_ERR) << "HWMCC Does NOT Support AIGER Inputs with Multiple Justice Properties !!" << endl;
      return 0;
   }
   if (hwmcc && b && jt) {
      Msg(MSG_ERR) << "HWMCC Does NOT Support AIGER Inputs with Both Bad and Justice Properties !!" << endl;
      return 0;
   }
   if (hwmcc && b && f) {
      Msg(MSG_WAR) << "HWMCC Omits Fairness Constraints for AIGER Inputs with Bad State Properties !!" << endl;
   }
   // Create Network Handler
   V3NtkInput* aigHandler = new V3NtkInput(true, fileName);
   if (!aigHandler) { Msg(MSG_ERR) << "Create AIG Parser Failed !!" << endl; return 0; }
   if (!aigHandler->getNtk()) { Msg(MSG_ERR) << "Create AIG Network Failed !!" << endl; return 0; }
   V3AigNtk* const ntk = aigHandler->getNtk(); assert (ntk);
   // Start Parsing AIG Content
   V3NetVec vars(m + 1);  // For even number nodes (including AIG_FALSE)
   uint32_t i, j;
   // Create Constant (AIG_FALSE)
   V3NetId aigConst = V3NetId::makeNetId(0); vars[0] = aigConst;
   // Create m Nets
   for (i = 1; i <= m; ++i) {
      vars[i] = aigHandler->createNet();
      if (V3NetUD == vars[i]) { delete aigHandler; return 0; }
      assert (i == vars[i].id);  // Obey with V3NetId Encoding
   }
   // Create Input Gates
   for (i = 1; i <= in; ++i)
      if (!createInput(ntk, vars[i])) { delete aigHandler; return 0; }
   // Create FF Gates
   V3NetType in1, in2;
   for (i = in + 1, j = in + l; i <= j; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Latch Input id !!!" << endl; delete aigHandler; return 0;
      }
      // Extension for AIGER 1.9: Latch Initial Values
      ch = getc(input);
      if ('\n' == ch) {  // FF are initialized to AIG_FALSE by default
         if (!createV3FFGate(ntk, vars[i], in1.id, aigConst)) { delete aigHandler; return 0; }
      }
      else {  // FF are initialized to specified values
         if (fscanf(input, "%u", &in2.value) != 1) {
            Msg(MSG_ERR) << "AIGER Header has Incorrect Format !!!" << endl; return 0;
         }
         if (0 == in2.value) {
            if (!createV3FFGate(ntk, vars[i], in1.id, aigConst)) { delete aigHandler; return 0; }
         }
         else if (1 == in2.value) {
            if (!createV3FFGate(ntk, vars[i], in1.id, ~aigConst)) { delete aigHandler; return 0; }
         }
         else {
            if (in2.value == (i << 1)) {
               if (!createV3FFGate(ntk, vars[i], in1.id, vars[i])) { delete aigHandler; return 0; }
            }
            else {
               Msg(MSG_ERR) << "Unexpected Latch Initial Value !!!" << endl; delete aigHandler; return 0;
            }
         }
      }
   }
   // Put Property Related Signals into Corresponding Lists
   badList.clear(); badList.reserve(o + b);
   for (i = 0; i < o; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Output Input id !!!" << endl; delete aigHandler; return 0;
      }
      badList.push_back(in1.id);
   }
   // Extensions for AIGER 1.9: Create Bad State Properties
   for (i = 0; i < b; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Bad State id !!!" << endl; delete aigHandler; return 0;
      }
      badList.push_back(in1.id);
   }
   // Extensions for AIGER 1.9: Create Invariant Constraints
   invList.clear(); invList.reserve(c);
   for (i = 0; i < c; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Invariant Constraint id !!!" << endl; delete aigHandler; return 0;
      }
      invList.push_back(in1.id);
   }
   // Extensions for AIGER 1.9: Create Justice Properties  (a set of constraints for a property)
   fairList.clear(); fairList.reserve(jt);
   for (i = 0; i < jt; ++i) {
      fairList.push_back(V3NetVec()); fairList.back().clear(); uint32_t jtSize = 0;
      if (fscanf(input, "%u", &jtSize) != 1) {
         Msg(MSG_ERR) << "Unexpected Number of Fairness Constraints for Justice Property " << i << " !!!" << endl;
         delete aigHandler; return 0;
      }
      fairList.back().reserve(jtSize + f);
      for (j = 0; j < jtSize; ++j) {
         if (fscanf(input, "%u", &in1.value) != 1) {
            Msg(MSG_ERR) << "Unexpected Fairness Constraint " << j << " id for Justice Property " << i << " !!!" << endl;
            delete aigHandler; return 0;
         }
         fairList.back().push_back(in1.id);
      }
   }
   // Extensions for AIGER 1.9: Create Fairness Constraints
   for (i = 0; i < f; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Fairness Constraint id !!!" << endl; delete aigHandler; return 0;
      }
      for (j = 0; j < jt; ++j) fairList[j].push_back(in1.id);
   }
   // Create AND Gates
   getc(input);  // End of Line Character
   for (i = in + l + 1, j = m; i <= j; ++i) {
      in1.value = hwmcc_aiger_decode(input); in1.value = (i << 1) - in1.value; //assert (in1 < (i << 1));
      in2.value = hwmcc_aiger_decode(input); in2.value = in1.value - in2.value; //assert (in2 <= in1);
      if (!createAigAndGate(ntk, vars[i], in2.id, in1.id)) { delete aigHandler; return 0; }
   }
   Msg(MSG_IFO) << "Totally " << (o + b) << " Safety, " << jt << " Liveness Properties are Added." << endl;
   fclose(input); assert (aigHandler); return aigHandler;
};

void writeAiger(V3NtkHandler* const handler, const string& fileName, const bool& l2s, const bool& invc2Prop) {
   // This Function Write Property Specifications into AIGER Format
   assert (handler); assert (handler->getNtk()); assert (fileName.size());
   if (dynamic_cast<V3BvNtk*>(handler->getNtk())) {
      Msg(MSG_ERR) << "Input Network is NOT Boolean-level !!" << endl; return; }
   // Check if Primary Inout Exists
   if (handler->getNtk()->getInoutSize()) {
      Msg(MSG_ERR) << "AIGER Incompatible Primary Inout Found !!" << endl; return; }
   // Check if Module Instance Exists
   if (handler->getNtk()->getModuleSize()) {
      Msg(MSG_ERR) << "AIGER Incompatible Module Instance Found !!" << endl; return; }
   // Elaborate Properties
   V3StrVec name; V3UI32Vec prop; V3UI32Table invc, fair;
   V3NtkElaborate* const pNtk = elaborateProperties(handler, name, prop, invc, fair, l2s, invc2Prop, 0, 0); assert (pNtk);
   assert (handler->getPropertyList().size() == prop.size()); assert (prop.size() == name.size());
   assert (prop.size() == invc.size()); assert (prop.size() == fair.size());
   // Check if the Set of Properties Conforms to AIGER Format
   for (uint32_t i = 1; i < invc.size(); ++i) {
      if (invc[0] == invc[i]) continue;
      Msg(MSG_ERR) << "AIGER Does NOT Support Properties with Different Invariant Constraints !!" << endl; return; }
   for (uint32_t i = 0; i < prop.size(); ++i) {
      if (!(1ul & prop[i]) || fair[i].size()) continue;
      Msg(MSG_ERR) << "AIGER Does NOT Support Justice Properties without Fairness Constraints !!" << endl; return; }
   // Open AIGER Output File
   ofstream output; output.open(fileName.c_str());
   if (!output.is_open()) {
      Msg(MSG_ERR) << "AIGER Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Write the Network and Properties into AIGER 1.9 Format
   V3Ntk* const ntk = pNtk->getNtk(); assert (ntk); V3NtkHandler::setAuxRenaming();
   // Mapping from Current Ntk to AIGER Output Id
   V3NetVec orderMap; dfsNtkForGeneralOrder(ntk, orderMap); assert (orderMap.size());
   assert (!orderMap[0].id); assert (orderMap.size() <= ntk->getNetSize());
   V3Vec<V3NetId>::Vec c2bMap(ntk->getNetSize(), V3NetUD); c2bMap[0] = V3NetId::makeNetId(0);
   for (uint32_t i = 1; i < orderMap.size(); ++i) {
      assert (V3NetUD == c2bMap[orderMap[i].id]);
      c2bMap[orderMap[i].id] = V3NetId::makeNetId(i);
   }
   // Output AIGER 1.9 Header : M I L O A B C J F
   uint32_t liveSize = 0; for (uint32_t i = 0; i < prop.size(); ++i) if (1ul & prop[i]) ++liveSize;
   output << "aig " << orderMap.size() - 1 << " " << ntk->getInputSize() << " " << ntk->getLatchSize() << " 0 "
          << (orderMap.size() - ntk->getInputSize() - ntk->getLatchSize() - 1) << " "
          << (prop.size() - liveSize) << " " << invc[0].size() << " " << liveSize << " 0" << endl;
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
   // Output AIGER 1.9 Bad Signals
   for (uint32_t i = 0; i < prop.size(); ++i) {
      if (1ul & prop[i]) continue;
      id1 = ntk->getOutput(prop[i] >> 1); assert (V3NetUD != c2bMap[id1.id]);
      output << ((c2bMap[id1.id].id << 1) + id1.cp) << endl;
   }
   // Output AIGER 1.9 Invariant Constraints
   for (uint32_t i = 0; i < invc[0].size(); ++i) {
      id1 = ntk->getOutput(invc[0][i]); assert (V3NetUD != c2bMap[id1.id]);
      output << ((c2bMap[id1.id].id << 1) + id1.cp) << endl;
   }
   // Output AIGER 1.9 Justice Properties
   for (uint32_t i = 0; i < prop.size(); ++i) {
      if (!(1ul & prop[i])) continue; assert (fair[i].size());
      output << fair[i].size() << endl;
      for (uint32_t j = 0; j < fair[i].size(); ++j) {
         id1 = ntk->getOutput(fair[i][j]); assert (V3NetUD != c2bMap[id1.id]);
         output << ((c2bMap[id1.id].id << 1) + id1.cp) << endl;
      }
   }
   // Output AIGER AIG_NODE
   uint32_t in1, in2;
   for (uint32_t i = 1 + ntk->getInputSize() + ntk->getLatchSize(), j = orderMap.size(); i < j; ++i) {
      if (AIG_NODE == ntk->getGateType(orderMap[i])) {
         id1 = ntk->getInputNetId(orderMap[i], 0); assert (V3NetUD != c2bMap[id1.id]);
         id2 = ntk->getInputNetId(orderMap[i], 1); assert (V3NetUD != c2bMap[id2.id]);
         assert (i > c2bMap[id1.id].id); in1 = (c2bMap[id1.id].id << 1) + id1.cp;
         assert (i > c2bMap[id2.id].id); in2 = (c2bMap[id2.id].id << 1) + id2.cp;
         if (in1 >= in2) { hwmcc_encode_aig(output, (i << 1) - in1); hwmcc_encode_aig(output, in1 - in2); }
         else { hwmcc_encode_aig(output, (i << 1) - in2); hwmcc_encode_aig(output, in2 - in1); }
      }
      else {
         Msg(MSG_WAR) << "Extra Constant in AIGER with Literal = " << (i << 1) << endl;
         assert (AIG_FALSE == ntk->getGateType(orderMap[i]));
         hwmcc_encode_aig(output, (i << 1)); hwmcc_encode_aig(output, 0);
      }
   }
   // Output AIGER Symbolic Table
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
      output << "i" << i << " " << V3RTLNameBase(pNtk, pNtk->getInputName(i)) << endl;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
      output << "l" << i << " " << V3RTLNameOrId(pNtk, ntk->getLatch(i)) << endl;
   // Footer (Instead of Header) in AIGER Output
   output << "c " << endl; writeV3GeneralHeader("AIGER with Symbol Table", output, "c");
   output.close(); V3NtkHandler::resetAuxRenaming(); delete pNtk;
}

#endif

