/****************************************************************************
  FileName     [ v3AigParser.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ AIG to V3 Network Parser. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_AIG_PARSER_C
#define V3_AIG_PARSER_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkParser.h"

#include <cstdio>

// Function aiger_getnoneofch : Referenced from AIGER Document
unsigned char aiger_getnoneofch(FILE* file) {
   int ch = getc(file);
   if (ch != EOF) return ch;
   Msg(MSG_ERR) << "Unexpected EOF of AIGER Input Found !!!" << endl;
   return EOF;
}

// Function decode : Referenced from AIGER Document
unsigned aiger_decode(FILE* file) {
   unsigned x = 0, i = 0;
   unsigned char ch;
   while ((ch = aiger_getnoneofch(file)) & 0x80)
      x |= (ch & 0x7f) << (7 * i++);
   return x | (ch << (7 * i));
}

bool aiger_symbol(FILE* file, unsigned char& c, uint32_t& index, string& name) {
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

// Main AIG Parsing Function
V3NtkInput* const V3AigParser(const char* fileName, const bool& symbol) {
   // Open Input AIGER File
   assert (fileName); FILE* input; input = fopen(fileName, "r");
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
      vars[i] = aigHandler->createNet(symbol ? "" : v3Int2Str(i));
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
   // Create Outputs
   for (i = 0; i < o; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Output Input id !!!" << endl; delete aigHandler; return 0;
      }
      if (!createOutput(ntk, in1.id)) { delete aigHandler; return 0; }
   }
   // Extensions for AIGER 1.9: Create Bad State Properties
   for (i = 0; i < b; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Bad State id !!!" << endl; delete aigHandler; return 0;
      }
      if (!createOutput(ntk, in1.id)) { delete aigHandler; return 0; }
   }
   // Extensions for AIGER 1.9: Create Invariant Constraints
   for (i = 0; i < c; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Invariant Constraint id !!!" << endl; delete aigHandler; return 0;
      }
      if (!createOutput(ntk, in1.id)) { delete aigHandler; return 0; }
   }
   // Extensions for AIGER 1.9: Create Justice Properties  (a set of constraints for a property)
   V3UI32Vec jtSize(jt, 0);
   for (i = 0; i < jt; ++i) {
      if (fscanf(input, "%u", &jtSize[i]) != 1) {
         Msg(MSG_ERR) << "Unexpected Number of Fairness Constraints for Justice Property " << i << " !!!" << endl;
         delete aigHandler; return 0;
      }
      for (j = 0; j < jtSize[i]; ++j) {
         if (fscanf(input, "%u", &in1.value) != 1) {
            Msg(MSG_ERR) << "Unexpected Fairness Constraint " << j << " id for Justice Property " << i << " !!!" << endl;
            delete aigHandler; return 0;
         }
         if (!createOutput(ntk, in1.id)) { delete aigHandler; return 0; }
      }
   }
   // Extensions for AIGER 1.9: Create Fairness Constraints
   for (i = 0; i < f; ++i) {
      if (fscanf(input, "%u", &in1.value) != 1) {
         Msg(MSG_ERR) << "Unexpected Fairness Constraint id !!!" << endl; delete aigHandler; return 0;
      }
      if (!createOutput(ntk, in1.id)) { delete aigHandler; return 0; }
   }
   // Create AND Gates
   getc(input);  // End of Line Character
   for (i = in + l + 1, j = m; i <= j; ++i) {
      in1.value = aiger_decode(input); in1.value = (i << 1) - in1.value; //assert (in1 < (i << 1));
      in2.value = aiger_decode(input); in2.value = in1.value - in2.value; //assert (in2 <= in1);
      if (!createAigAndGate(ntk, vars[i], in2.id, in1.id)) { delete aigHandler; return 0; }
   }
   V3StrVec outputName(ntk->getOutputSize(), "");
   // Parse Symbol Table for Signal Names
   if (symbol) {
      unsigned char c; uint32_t ioIndex; string name;
      while (!feof(input)) {
         if (aiger_symbol(input, c, ioIndex, name)) {
            if ('i' == c) {  // Set PI Names
               if (ioIndex >= ntk->getInputSize()) {
                  Msg(MSG_ERR) << "Unexpected Symbol Item for Primary Input with Exceeded Index = \"" 
                               << ioIndex << "\" !!" << endl; delete aigHandler; return 0;
               }
               else if (!aigHandler->resetNetName(ntk->getInput(ioIndex).id, name)) {
                  delete aigHandler; return 0;
               }
            }
            else if ('l' == c) {  // Set Latch Names
               if (ioIndex >= ntk->getLatchSize()) {
                  Msg(MSG_ERR) << "Unexpected Symbol Item for Latch with Exceeded Index = \"" 
                               << ioIndex << "\" !!" << endl; delete aigHandler; return 0;
               }
               else if (!aigHandler->resetNetName(ntk->getLatch(ioIndex).id, name)) {
                  delete aigHandler; return 0;
               }
            }
            else if ('o' == c) {  // Set Output Names
               if (ioIndex >= o) {
                  Msg(MSG_ERR) << "Unexpected Symbol Item for Primary Output with Exceeded Index = \"" 
                               << ioIndex << "\" !!" << endl; delete aigHandler; return 0;
               }
               else if (outputName[ioIndex].size()) {
                  Msg(MSG_ERR) << "Name of Output " << ioIndex << " has Already Set !!" << endl;
                  delete aigHandler; return 0;
               }
               else outputName[ioIndex] = name;
            }
            else if ('b' == c) {  // Extension for AIGER 1.9: Set Names for Bad State Properties
               if (ioIndex >= b) {
                  Msg(MSG_ERR) << "Unexpected Symbol Item for Bad State Properties with Exceeded Index = \"" 
                               << ioIndex << "\" !!" << endl; delete aigHandler; return 0;
               }
               else if (outputName[o + ioIndex].size()) {
                  Msg(MSG_ERR) << "Name of Bad State Property " << ioIndex << " has Already Set !!" << endl;
                  delete aigHandler; return 0;
               }
               else outputName[o + ioIndex] = name;
            }
            else if ('j' == c) {  // Extension for AIGER 1.9: Set Names for Justice Properties
               if (ioIndex >= jt) {
                  Msg(MSG_ERR) << "Unexpected Symbol Item for Justice Properties with Exceeded Index = \"" 
                               << ioIndex << "\" !!" << endl; delete aigHandler; return 0;
               }
               else {
                  for (i = 0, j = o + b + c; i < ioIndex; ++i) j += jtSize[i];
                  for (i = 0; i < jtSize[ioIndex]; ++i) {
                     if (outputName[j + i].size()) {
                        Msg(MSG_ERR) << "Name of Justice Property " << ioIndex << " has Already Set !!" << endl;
                        delete aigHandler; return 0;
                     }
                     else outputName[j + i] = name + "_" + v3Int2Str(i);
                  }
               }
            }
            else if ('c' == c) break;
            else {
               Msg(MSG_ERR) << "Unexpected Symbolic Prefix = \'" << c << "\' !!" << endl;
               delete aigHandler; return 0;
            }
         }
         else if (!feof(input)) {
            Msg(MSG_ERR) << "Unexpected Format in the Symbolic Table !!" << endl;
            delete aigHandler; return 0;
         }
      }
   }
   // Set Output Names if Not Exists
   for (i = 0; i < o; ++i) 
      if (outputName[i].size()) aigHandler->resetOutName(i, outputName[i]);
      else aigHandler->resetOutName(i, "aiger_output_" + v3Int2Str(i));
   // Extension for AIGER 1.9: Set Names for Bad State Properties if Not Exists
   for (i = 0, j = o; i < b; ++i, ++j) 
      if (outputName[j].size()) aigHandler->resetOutName(j, outputName[j]);
      else aigHandler->resetOutName(j, "aiger_bad_property_" + v3Int2Str(i));
   // Extension for AIGER 1.9: Set Names for Invariant Constraints if Not Exists
   for (i = 0; i < c; ++i, ++j) 
      if (outputName[j].size()) aigHandler->resetOutName(j, outputName[j]);
      else aigHandler->resetOutName(j, "aiger_inv_constraint_" + v3Int2Str(i));
   // Extension for AIGER 1.9: Set Names for Justice Properties if Not Exists
   for (i = 0; i < jt; ++i)
      for (uint32_t k = 0; k < jtSize[i]; ++k, ++j)
         if (outputName[j].size()) aigHandler->resetOutName(j, outputName[j]);
         else aigHandler->resetOutName(j, "aiger_justice_property_" + v3Int2Str(i) + "_" + v3Int2Str(k));
   // Extension for AIGER 1.9: Set Names for Fairness Constraints if Not Exists
   for (i = 0; i < f; ++i, ++j) 
      if (outputName[j].size()) aigHandler->resetOutName(j, outputName[j]);
      else aigHandler->resetOutName(j, "aiger_fair_constraint_" + v3Int2Str(i));
   fclose(input); assert (aigHandler); return aigHandler;
}

#endif

