/****************************************************************************
  FileName     [ v3RTLParser.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ Primitive RTL to V3 Network Parser. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_RTL_PARSER_C
#define V3_RTL_PARSER_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkParser.h"

#include <fstream>
#include <string>

// Defines
typedef V3Vec<char>::Vec      V3RTLSplitToken;

// Parser Helper Functions
bool isCharMatchToken(const char& c, const V3RTLSplitToken& split) {
   // NOTE: We do not check the validation of split characters !!
   for (uint32_t i = 0; i < split.size(); ++i)
      if (c == split[i]) return true;
   return false;
}

bool lexLineRTL(ifstream& input, V3StrVec& tokens, const V3RTLSplitToken& split) {
   assert (input.is_open()); tokens.clear();
   string buffer = ""; size_t i, j;
   bool lineComment = false, starComment = false;
   while (!input.eof()) {
      getline(input, buffer); if (input.eof()) break;
      for (i = j = 0; i < buffer.size(); ++i) {
         if (starComment) {
            // NOTE: We do not check if "/*" lies in starComment, although we can check here !!
            if (buffer[i] == '*' && ((1 + i) != buffer.size()) && buffer[1 + i] == '/') {
               starComment = false;
               ++i; j = i + 1;
            }
         }
         else if (buffer[i] == ' ' || buffer[i] == '\t' || isCharMatchToken(buffer[i], split)) {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; lineComment = false;
         }
         else if (buffer[i] == ';' || buffer[i] == '(' || buffer[i] == ')') {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            tokens.push_back(buffer.substr(i, 1)); j = i + 1; lineComment = false;
         }
         else if (buffer[i] == '\n') {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; break;
         }
         else if (buffer[i] == '/') {
            if (!lineComment) { lineComment = true; continue; }
            if (--i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; break;
         }
         else if (buffer[i] == '*') {
            if (!lineComment) continue;
            lineComment = false; starComment = true;
            if (--i > j) tokens.push_back(buffer.substr(j, i - j));
            j = V3NtkUD; ++i;
         }
      }
      lineComment = false; if (starComment) continue;
      if (i > j) tokens.push_back(buffer.substr(j));
      if (tokens.size()) return true;
   }
   if (starComment) Msg(MSG_ERR) << "Missing \"*/\" After \"/*\" in the File !!" << endl;
   return false;
}

bool readRTLModuleHeader(ifstream& input, string& moduleName, V3StrSet& ioNames) {
   assert (input.is_open()); moduleName = ""; ioNames.clear();
   V3StrVec tokens; bool module = false; V3RTLSplitToken split; split.clear();
   split.push_back('('); split.push_back(')'); split.push_back(',');

   while (true) {
      if (!lexLineRTL(input, tokens, split)) return false;
      for (uint32_t i = 0; i < tokens.size(); ++i) {
         if (!module) {
            if ("module" != tokens[i]) {
               Msg(MSG_ERR) << "Missing \"module\" in RTL File !!" << endl;
               return false;
            }
            module = true;
         }
         else if (!moduleName.size()) {
            // NOTE: We do not check if the syntax for module name is correct !!
            //       We simply adopt the token after "module" for moduleName.
            moduleName = tokens[i];
         }
         else if (";" == tokens[i]) {
            if ((1 + i) != tokens.size()) {
               Msg(MSG_ERR) << "Missing End-of-Line After \";\" !!" << endl;
               return false;
            }
            else return true;
         }
         else {
            if (ioNames.end() != ioNames.find(tokens[i])) {
               Msg(MSG_ERR) << "Repeated Module I/O Name: " << tokens[i] << endl;
               return false;
            }
            ioNames.insert(tokens[i]);
         }
      }
   }
   return true;
}

bool readRTLModule(ifstream& input, V3StrSet& ioNames, V3NtkInput* const handler) {
   assert (input.is_open()); assert (ioNames.size()); assert (handler);
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(handler->getNtk()); assert (ntk);
   V3StrVec tokens; V3RTLSplitToken split; split.clear();
   split.push_back(':'); split.push_back('['); split.push_back(']'); split.push_back(',');

   string name, typeName = ""; uint32_t msb = V3NtkUD, lsb = V3NtkUD; int temp;
   V3NetId id, id1, id2; V3StrVec inputs; inputs.clear();
   V3GateType type; V3StrSet::iterator it; bool ok, inv, fi;

   while (true) {
      if (!lexLineRTL(input, tokens, split)) return false;
      for (uint32_t i = 0; i < tokens.size(); ++i) {
         if ("input" == tokens[i] || "output" == tokens[i] || "wire" == tokens[i] || "reg" == tokens[i]) {
            if (inputs.size()) {
               Msg(MSG_ERR) << "Missing \";\" at the end of \"" << typeName << "\" !!" << endl;
               return false;
            }
            typeName = tokens[i]; msb = lsb = V3NtkUD;
         }
         else if ("and" == tokens[i] || "or" == tokens[i] || "xor" == tokens[i] || "buf" == tokens[i] || 
                  "nand" == tokens[i] || "nor" == tokens[i] || "xnor" == tokens[i] || "not" == tokens[i]) {
            if (inputs.size()) {
               Msg(MSG_ERR) << "Missing \";\" at the end of \"" << typeName << "\" !!" << endl;
               return false;
            }
            inputs.clear(); inputs.push_back(tokens[i]);
            typeName = tokens[i]; msb = lsb = V3NtkUD;
         }
         else if ("endmodule" == tokens[i]) {
            if (inputs.size()) {
               Msg(MSG_ERR) << "Missing \";\" at the end of \"" << typeName << "\" !!" << endl;
               return false;
            }
            else return true;
         }
         else if (V3NtkUD == msb && v3Str2Int(tokens[i], temp)) {
            if (temp < 0) {
               Msg(MSG_ERR) << "Illegal Negative MSB = " << temp << " !!" << endl;
               return false;
            }
            msb = temp;
         }
         else if (V3NtkUD == lsb && v3Str2Int(tokens[i], temp)) {
            if (temp < 0) {
               Msg(MSG_ERR) << "Illegal Negative LSB = " << temp << " !!" << endl;
               return false;
            }
            else if (temp > 0) {
               Msg(MSG_ERR) << "Unexpected Non-Zero LSB = " << temp << " !!" << endl;
               return false;
            }
            lsb = temp;
         }
         else if (";" == tokens[i]) {
            if (inputs.size()) {
               assert (typeName == inputs[0]); id = id2 = V3NetUD;
               // Combine Bits with Signal Names
               V3StrVec inputNames = inputs; inputs.clear();
               for (uint32_t j = 0; j < inputNames.size(); ++j) {
                  if ("(" == inputNames[j]) {
                     assert (inputs.size()); assert (j == 1 || j == 2);
                     if (j == 2) inputs.pop_back();
                  }
                  else if (")" == inputNames[j]) {
                     assert ((1 + j) == inputNames.size());
                     break;
                  }
                  else if (!v3Str2Int(inputNames[j], temp)) inputs.push_back(inputNames[j]);
                  else {
                     assert (inputs.size());
                     inputs.back() = inputs.back() + "[" + inputNames[j] + "]";
                  }
               }
               for (uint32_t j = 1; j < inputs.size(); ++j)
                  if (string::npos == inputs[j].find('[')) inputs[j] += "[0]";
               if (1 == inputs.size()) {
                  Msg(MSG_ERR) << "Missing Output for Primitive Instance !!" << endl;
                  return false;
               }
               if (2 == inputs.size()) {
                  Msg(MSG_ERR) << "Missing Input for Primitive Instance !!" << endl;
                  return false;
               }
               if ("buf" == typeName || "not" == typeName) {
                  if (3 != inputs.size()) {
                     Msg(MSG_ERR) << "Extra Input \"" << inputs[2] << "\" for \"" << type << "\" !!" << endl;
                     return false;
                  }
                  type = BV_AND; inv = ("not" == typeName); fi = ('!' == inputs[2][0] || '~' == inputs[2][0]);
                  name = fi ? inputs[2].substr(1) : inputs[2];
                  if ("1'b0[0]" == name || "1'd0[0]" == name || "1'h0[0]" == name) id1 = V3NetId::makeNetId(0);
                  else if ("1'b1[0]" == name || "1'd1[0]" == name || "1'h1[0]" == name) id1 = ~V3NetId::makeNetId(0);
                  else id1 = handler->getNetId(name);
                  if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                  id2 = fi ? ~id1 : id1;
               }
               else {
                  if ("and" == typeName) { type = BV_AND; inv = false; }
                  else if ("nand" == typeName) { type = BV_AND; inv = true; }
                  else if ("or" == typeName) { type = BV_OR; inv = false; }
                  else if ("nor" == typeName) { type = BV_OR; inv = true; }
                  else if ("xor" == typeName) { type = BV_XOR; inv = false; }
                  else if ("xnor" == typeName) { type = BV_XOR; inv = true; }
                  for (uint32_t j = 2; j < inputs.size(); ++j) {
                     fi = ('!' == inputs[j][0] || '~' == inputs[j][0]);
                     name = fi ? inputs[j].substr(1) : inputs[j];
                     if ("1'b0[0]" == name || "1'd0[0]" == name || "1'h0[0]" == name) id1 = V3NetId::makeNetId(0);
                     else if ("1'b1[0]" == name || "1'd1[0]" == name || "1'h1[0]" == name) id1 = ~V3NetId::makeNetId(0);
                     else id1 = handler->getNetId(name);
                     if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                     if (V3NetUD == id2) id2 = fi ? ~id1 : id1;
                     else {
                        id = createNet(ntk); assert (V3NetUD != id);
                        ok = createBvPairGate(ntk, type, id, id1, id2); assert (ok);
                        id2 = id;
                     }
                  }
               }
               // Create Output
               assert (V3NetUD != id2); fi = ('!' == inputs[1][0] || '~' == inputs[1][0]);
               name = fi ? inputs[1].substr(1) : inputs[1]; id = handler->getNetId(name);
               assert (V3NetUD == id || V3_PI == ntk->getGateType(id));
               if (V3NetUD == id) id = handler->createNet(name); assert (V3NetUD != id);
               ok = createV3BufGate(ntk, id, ((inv ^ fi) ? ~id2 : id2));
            }
            inputs.clear(); typeName = ""; msb = lsb = V3NtkUD;
         }
         else if (typeName.size()) {
            if (V3NtkUD != msb && V3NtkUD == lsb) {
               Msg(MSG_ERR) << "Missing LSB under MSB = " << msb << " !!" << endl;
               return false;
            }
            else if (V3NtkUD == msb) { assert (V3NtkUD == lsb); msb = lsb = 0; }
            if (msb < lsb) {
               Msg(MSG_ERR) << "MSB (" << msb << ") < LSB (" << lsb << ") !!" << endl;
               return false;
            }
            // Create BV for I/O Signals
            if ("input" == typeName) {
               it = ioNames.find(tokens[i]);
               if (ioNames.end() != it) ioNames.erase(it);
               else {
                  Msg(MSG_ERR) << "Missing \"" << tokens[i] << "\" After \"module\" !!" << endl;
                  return false;
               }
               id = handler->createNet(tokens[i], (1 + msb - lsb)); assert (V3NetUD != id);
               ok = createInput(ntk, id); assert (ok);
               if (msb == lsb) {
                  name = tokens[i] + "[" + v3Int2Str(lsb) + "]"; id1 = handler->getNetId(name);
                  assert (V3NetUD == id1 || V3_PI == ntk->getGateType(id1));
                  if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                  ok = createV3BufGate(ntk, id1, id); assert (ok);
               }
               else {
                  // Create Split Signals
                  for (uint32_t j = lsb; j <= msb; ++j) {
                     name = tokens[i] + "[" + v3Int2Str(j) + "]"; id1 = handler->getNetId(name);
                     assert (V3NetUD == id1 || V3_PI == ntk->getGateType(id1));
                     if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                     ok = createBvSliceGate(ntk, id1, id, j - lsb, j - lsb); assert (ok);
                  }
               }
            }
            else if ("output" == typeName) {
               it = ioNames.find(tokens[i]);
               if (ioNames.end() != it) ioNames.erase(it);
               else {
                  Msg(MSG_ERR) << "Missing \"" << tokens[i] << "\" After \"module\" !!" << endl;
                  return false;
               }
               // Create Merge Signals
               assert (V3NetUD == handler->getNetId(tokens[i])); id2 = V3NetUD;
               for (uint32_t j = lsb; j <= msb; ++j) {
                  name = tokens[i] + "[" + v3Int2Str(j) + "]"; id1 = handler->getNetId(name);
                  if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                  if (V3NetUD == id2) id2 = id1;
                  else {
                     id = createNet(ntk, 1 + j - lsb); assert (V3NetUD != id);
                     ok = createBvPairGate(ntk, BV_MERGE, id, id1, id2); assert (ok);
                     id2 = id;
                  }
               }
               assert (V3NetUD != id2); assert ((1 + msb - lsb) == ntk->getNetWidth(id2));
               id = handler->createNet(tokens[i], ntk->getNetWidth(id2)); assert (V3NetUD != id);
               ok = createV3BufGate(ntk, id, id2); assert (ok);
               ok = createOutput(ntk, id); assert (ok);
               handler->resetOutName(ntk->getOutputSize() - 1, tokens[i]);
            }
            else if ("wire" == typeName || "reg" == typeName) {
               // Create Merge Signals
               id = handler->getNetId(tokens[i]); if (V3NetUD != id) continue; id2 = V3NetUD;
               for (uint32_t j = lsb; j <= msb; ++j) {
                  name = tokens[i] + "[" + v3Int2Str(j) + "]"; id1 = handler->getNetId(name);
                  if (V3NetUD == id1) id1 = handler->createNet(name); assert (V3NetUD != id1);
                  if (V3NetUD == id2) id2 = id1;
                  else {
                     id = createNet(ntk, 1 + j - lsb); assert (V3NetUD != id);
                     ok = createBvPairGate(ntk, BV_MERGE, id, id1, id2); assert (ok);
                     id2 = id;
                  }
               }
               assert (V3NetUD != id2); assert ((1 + msb - lsb) == ntk->getNetWidth(id2));
               id = handler->createNet(tokens[i], ntk->getNetWidth(id2)); assert (V3NetUD != id);
               ok = createV3BufGate(ntk, id, id2); assert (ok);
            }
            else {
               if (msb || lsb) {
                  Msg(MSG_ERR) << "Unexpected BV [" << msb << ":" << lsb << "] for Primitive \"" 
                               << typeName << "\" !!" << endl; return false;
               }
               inputs.push_back(tokens[i]);
            }
         }
      }
   }
   return true;
}

// Main RTL Parsing Function
V3NtkInput* const V3RTLParser(const char* fileName) {
   // NOTE: This Parser ONLY Deals with Designs Written into Primitives.
   //       And Currently We Assume NO Sequential Elements.
   //       Unfortunately this parser do not properly handle RTL syntax errors !!
   // Open Input RTL File
   assert (fileName); ifstream input; input.open(fileName);
   if (!input.is_open()) { Msg(MSG_ERR) << "RTL Input File \"" << fileName << "\" Not Found !!" << endl; return 0; }
   // Parsing Module Header  (i.e. module moduleName (io1, io2, ..., ioN);)
   string moduleName; V3StrSet ioNames; ioNames.clear();
   if (!readRTLModuleHeader(input, moduleName, ioNames)) return 0;
   if (!moduleName.size()) { Msg(MSG_ERR) << "Missing module Name !!" << endl; return 0; }
   if (!ioNames.size()) { Msg(MSG_ERR) << "Missing I/O signals after \"module\" !!" << endl; return 0; }
   // Create Network Handler
   V3NtkInput* rtlHandler = new V3NtkInput(false, moduleName);
   if (!rtlHandler) { Msg(MSG_ERR) << "Create RTL Parser Failed !!" << endl; return 0; }
   if (!rtlHandler->getNtk()) { Msg(MSG_ERR) << "Create RTL Network Failed !!" << endl; return 0; }
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(rtlHandler->getNtk()); assert (ntk);
   // Parsing RTL Design
   if (!readRTLModule(input, ioNames, rtlHandler)) { delete rtlHandler; return 0; }
   for (V3StrSet::const_iterator it = ioNames.begin(); it != ioNames.end(); ++it)
      Msg(MSG_ERR) << "Missing \"input\" or \"output\" for I/O \"" << *it << "\" !!" << endl;
   if (ioNames.size()) { delete rtlHandler; return 0; }
   input.close(); return rtlHandler;
}

#endif

