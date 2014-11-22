/****************************************************************************
  FileName     [ v3RTLWriter.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ RTL (Verilog) Writer for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_RTL_WRITER_H
#define V3_RTL_WRITER_H

#include "v3Msg.h"
#include "v3StrUtil.h"
#include "v3NtkWriter.h"

// RTL Operators
const string V3TypeRTL[] = {
   "", "", "", "", " & ", "",                            // PI, PIO, FF, MODULE, AIG GATES
   "&", "|", "^", "", " & ", " ^ ",                      // BV_(RED, LOGIC)
   " + ", " - ", " * ", " / ", " % ", " << ",  " >> ",   // BV_(ARITH)
   "", " == ", " >= ", "", ""                            // BV_(MODEL), BV_(COMP)
};

const string getVerilogName(const V3NtkHandler* const handler, const string& name) {
   assert (handler); if (!name.size()) return name;
   string rtlName = (isdigit(name[0])) ? "id" : "";//handler->applyAuxNetNamePrefix("id") : "";
   //for (uint32_t i = 0; i < name.size(); ++i) 
   //   if ('[' == name[i]) rtlName += "_v3_slice_";
   //   else if (':' == name[i]) rtlName += "_";
   //   else if (']' == name[i]) rtlName += "";
   //   else rtlName += name[i];
   return rtlName + name;
}

const string getVerilogModuleName(const V3NtkHandler* const handler) {
   assert (handler); string name = handler->getNtkName(); if (!name.size()) return "v3_rtl";
   size_t pos = name.find_last_of("/\\"); if (string::npos != pos) name = name.substr(pos + 1);
   pos = name.find_first_of(".\\"); if (string::npos != pos) name = name.substr(0, pos);
   return name;
}

// RTL (Verilog) Writer Helper Function
const bool V3RTLWriter(const V3NtkHandler* const handler, ofstream& output, const bool& symbol, const bool& initial, string prefix, V3StrVec& ioName) {
   assert (handler); assert (output.is_open());
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Output Module Instances First
   V3Vec<V3NtkModule*>::Vec instList; instList.clear();
   V3StrTable instIoList; instIoList.clear();
   string instPrefix = ""; bool existInstClock = false;
   prefix += (prefix.size() ? "_" : "") + getVerilogModuleName(handler);
   for (uint32_t i = 0; i < ntk->getModuleSize(); ++i) {
      V3NtkModule* const module = ntk->getModule(i); assert (module);
      V3NtkHandler* const moduleHandler = module->getNtkRef();
      if (!moduleHandler|| module->isNtkRefBlackBoxed()) continue; assert (moduleHandler->getNtk());
      if (moduleHandler->getNtk()->getInoutSize()) {
         Msg(MSG_ERR) << "Not Support Module Instance with Inout Ports !!" << endl;
         Msg(MSG_ERR) << "Output Verilog Terminated Unexpectedly at Network \"" 
                      << moduleHandler->getNtkName() << "\" !!" << endl; return false;
      }
      instPrefix = prefix + v3Int2Str(instList.size()); instList.push_back(module); instIoList.push_back(V3StrVec());
      if (!V3RTLWriter(moduleHandler, output, symbol, initial, instPrefix, instIoList.back())) return false;
      existInstClock |= (instIoList.back().size() > 
                        (moduleHandler->getNtk()->getInputSize() + moduleHandler->getNtk()->getOutputSize()));
      output << endl;
   }
   // Check if Clock is Necessary but Missing
   const string clockName = ((V3NetUD == ntk->getClock() || isV3NetInverted(ntk->getClock())) ? 
                            "v3_clock" : V3RTLNameOrId(handler, ntk->getClock())); assert (clockName.size());
   // Compute Name Mapping Table
   string name; V3StrVec rtlName(ntk->getNetSize(), ""); ioName.clear();
   ioName.reserve(ntk->getInputSize() + ntk->getOutputSize() + ntk->getInoutSize());
   if (ntk->getLatchSize() || existInstClock) {
      if (V3NetUD != ntk->getClock() && !isV3NetInverted(ntk->getClock())) 
         rtlName[ntk->getClock().id] = clockName; ioName.push_back(clockName);
   }
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      assert (!rtlName[ntk->getInput(i).id].size()); name = getVerilogName(handler, handler->getInputName(i));
      if (!name.size()) name = symbol ? V3RTLNameOrId(handler, ntk->getInput(i)) : 
                                        V3RTLNameById(handler, ntk->getInput(i)); assert (name.size());
      ioName.push_back(name); rtlName[ntk->getInput(i).id] = ioName.back();
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      Msg(MSG_WAR) << "Inout Port has not been Completely Supported !!" << endl;
      assert (!rtlName[ntk->getInout(i).id].size()); name = getVerilogName(handler, handler->getInoutName(i));
      if (!name.size()) name = symbol ? V3RTLNameOrId(handler, ntk->getInout(i)) : 
                                        V3RTLNameById(handler, ntk->getInout(i)); assert (name.size());
      ioName.push_back(name); rtlName[ntk->getInout(i).id] = ioName.back();
   }
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      name = getVerilogName(handler, handler->getOutputName(i));
      if (!name.size()) name = symbol ? V3RTLNameOrId(handler, ntk->getOutput(i)) : 
                                        V3RTLNameById(handler, ntk->getOutput(i)); assert (name.size());
      ioName.push_back(name);
   }
   if (symbol) {
      for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) 
         if (!rtlName[id.id].size()) rtlName[id.id] = V3RTLNameOrId(handler, id);
   }
   else {
      for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) 
         if (!rtlName[id.id].size()) rtlName[id.id] = V3RTLNameById(handler, id);
   }
   // Output Verilog Module
   output << "// Module " << prefix << endl; output << "module " << prefix << "\n(" << endl;
   for (uint32_t i = 0; i < ioName.size(); ++i) output << (i ? ",\n" : "") << V3_INDENT << ioName[i];
   output << "\n);" << endl;
   // Output Verilog I/O Declarations
   uint32_t ioIndex = 0;
   if (ntk->getLatchSize() || existInstClock) {
      output << "\n" << V3_INDENT << "// Clock Signal for Synchronous DFF" << endl;
      output << V3_INDENT << "input " << ioName[ioIndex++] << ";" << endl;
   }
   output << "\n" << V3_INDENT << "// I/O Declarations" << endl;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      output << V3_INDENT << "input ";
      if (ntk->getNetWidth(ntk->getInput(i)) > 1) output << "[" << ntk->getNetWidth(ntk->getInput(i)) - 1 << ":0] ";
      assert (ioIndex < ioName.size()); output << ioName[ioIndex++] << ";" << endl;
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      output << V3_INDENT << "inout ";
      if (ntk->getNetWidth(ntk->getInout(i)) > 1) output << "[" << ntk->getNetWidth(ntk->getInout(i)) - 1 << ":0] ";
      assert (ioIndex < ioName.size()); output << ioName[ioIndex++] << ";" << endl;
   }
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      output << V3_INDENT << "output ";
      if (ntk->getNetWidth(ntk->getOutput(i)) > 1) output << "[" << ntk->getNetWidth(ntk->getOutput(i)) - 1 << ":0] ";
      assert (ioIndex < ioName.size()); output << ioName[ioIndex++] << ";" << endl;
   }
   assert (ioIndex == ioName.size());
   // Output Verilog Wire and Reg Declarations
   V3HashSet<string>::Hash nameHash; nameHash.clear();
   output << "\n" << V3_INDENT << "// Wire and Reg Declarations" << endl;
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) {
      if (nameHash.end() != nameHash.find(rtlName[id.id])) continue; nameHash.insert(rtlName[id.id]);
      output << V3_INDENT << ((V3_FF == ntk->getGateType(id)) ? "reg " : "wire ");
      if (ntk->getNetWidth(id) > 1) output << "[" << ntk->getNetWidth(id) - 1 << ":0] ";
      output << rtlName[id.id] << ";" << endl;
   }
   // Output Verilog Output Names if Eliminated
   V3Vec<string>::Vec outAssign; outAssign.clear();
   for (uint32_t i = 0, j = ioName.size() - ntk->getOutputSize(); i < ntk->getOutputSize(); ++i, ++j) {
      const V3NetId id = ntk->getOutput(i); name = rtlName[id.id];
      assert (name.size()); if (ioName[j] == name) continue; 
      if (nameHash.end() == nameHash.find(ioName[j])) {
         nameHash.insert(ioName[j]);
         if (!outAssign.size()) output << "\n" << V3_INDENT << "// Output Net Declarations" << endl;
         output << V3_INDENT << "wire ";
         if (ntk->getNetWidth(id) > 1) output << "[" << ntk->getNetWidth(id) - 1 << ":0] ";
         output << ioName[j] << ";" << endl;
      }
      outAssign.push_back("assign " + ioName[j] + " = " + V3RTLName(id));
   }
   // Output Verilog Inout Assignments
   if (ntk->getInoutSize()) {
      output << "\n" << V3_INDENT << "// Inout Assignments" << endl;
      Msg(MSG_WAR) << "Inout Assignment has Not been Supported Yet !!" << endl;
   }
   // Output Verilog Combinational Assignments
   output << "\n" << V3_INDENT << "// Combinational Assignments" << endl;
   V3BvNtk* const bvNtk = dynamic_cast<V3BvNtk*>(ntk);
   V3GateType type; V3NetId id1, id2, id3;
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) {
      type = ntk->getGateType(id); assert (V3_XD > type); if (V3_MODULE >= type) continue;
      output << V3_INDENT << "assign " << rtlName[id.id] << " = ";
      if (AIG_NODE == type || isV3PairType(type)) {
         id1 = ntk->getInputNetId(id, 0); id2 = ntk->getInputNetId(id, 1);
         if (type == BV_MERGE) output << "{" << V3RTLName(id1) << ", " << V3RTLName(id2) << "};" << endl;
         else output << V3RTLName(id1) << V3TypeRTL[type] << V3RTLName(id2) << ";" << endl;
      }
      else if (isV3ReducedType(type)) {
         id1 = ntk->getInputNetId(id, 0); if (id1.cp) { output << "~"; id1.cp = 0; }
         output << V3TypeRTL[type] << V3RTLName(id1) << ";" << endl;
      }
      else if (BV_MUX == type) {
         id1 = ntk->getInputNetId(id, 0); id2 = ntk->getInputNetId(id, 1); id3 = ntk->getInputNetId(id, 2);
         output << V3RTLName(id3) << " ? " << V3RTLName(id2) << " : " << V3RTLName(id1) << ";" << endl;
      }
      else if (BV_SLICE == type) {
         id1 = ntk->getInputNetId(id, 0); assert (bvNtk);
         if (ntk->getNetWidth(id) == ntk->getNetWidth(id1)) output << V3RTLName(id1) << ";" << endl;
         else if (bvNtk->getInputSliceBit(id, true) == bvNtk->getInputSliceBit(id, false))
            output << V3RTLName(id1) << "[" << bvNtk->getInputSliceBit(id, true) << "];" << endl;
         else output << V3RTLName(id1) << "[" << bvNtk->getInputSliceBit(id, true) << ":"
                     << bvNtk->getInputSliceBit(id, false) << "];" << endl;
      }
      else if (BV_CONST == type) { assert (bvNtk); output << bvNtk->getInputConstValue(id) << "; " << endl; }
      else { assert (AIG_FALSE == type); output << "1'b0;" << endl; }
   }
   // Output Verilog Eliminated Output Assignments
   if (outAssign.size()) output << "\n" << V3_INDENT << "// Output Net Assignments" << endl;
   for (uint32_t i = 0; i < outAssign.size(); ++i) output << V3_INDENT << outAssign[i] << ";" << endl;
   // Output Verilog Initial State
   if (initial && ntk->getLatchSize()) {
      output << "\n" << V3_INDENT << "// Initial State" << endl;
      output << V3_INDENT << "initial begin" << endl;
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
         id2 = ntk->getInputNetId(ntk->getLatch(i), 1);
         output << V3_INDENT << V3_INDENT << rtlName[ntk->getLatch(i).id] << " = " << V3RTLName(id2) << ";" << endl;
      }
      output << V3_INDENT << "end" << endl;
   }
   // Output Verilog Non-blocking Assignments
   if (ntk->getLatchSize()) {
      output << "\n" << V3_INDENT << "// Non-blocking Assignments" << endl;
      output << V3_INDENT << "always @ (posedge " << clockName << ") begin" << endl;
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
         id1 = ntk->getInputNetId(ntk->getLatch(i), 0);
         output << V3_INDENT << V3_INDENT << rtlName[ntk->getLatch(i).id] << " <= " << V3RTLName(id1) << ";" << endl;
      }
      output << V3_INDENT << "end" << endl;
   }
   // Output Verilog Module Instances
   if (instList.size()) output << "\n" << V3_INDENT << "// Module Instances" << endl;
   for (uint32_t i = 0; i < instList.size(); ++i) {
      V3NtkHandler* const moduleHandler = instList[i]->getNtkRef(); assert (moduleHandler);
      name = prefix + v3Int2Str(i) + "_" + getVerilogModuleName(moduleHandler); assert (name.size());
      if (i) output << endl; output << V3_INDENT << name << " " << name << "(" << endl;
      // Clock Signal
      existInstClock = (instIoList[i].size() > 
                       (moduleHandler->getNtk()->getInputSize() + moduleHandler->getNtk()->getOutputSize()));
      uint32_t index = 0; assert (clockName.size()); assert (!existInstClock || instIoList[i].size());
      if (existInstClock) output << V3_INDENT << V3_INDENT << "." << instIoList[i][index++] << "(" << clockName << ")";
      // Primary Inputs
      const V3NetVec& inputList = instList[i]->getInputList();
      assert (inputList.size() == moduleHandler->getNtk()->getInputSize());
      for (uint32_t j = 0; j < inputList.size(); ++j, ++index) {
         if (index) output << ","; output << endl; assert (index < instIoList[i].size());
         output << V3_INDENT << V3_INDENT << "." << instIoList[i][index] << "(" << V3RTLName(inputList[j]) << ")";
      }
      // Primary Outputs
      const V3NetVec& outputList = instList[i]->getOutputList();
      assert (outputList.size() == moduleHandler->getNtk()->getOutputSize());
      for (uint32_t j = 0; j < outputList.size(); ++j, ++index) {
         if (index) output << ","; output << endl; assert (index < instIoList[i].size());
         output << V3_INDENT << V3_INDENT << "." << instIoList[i][index] << "(" << V3RTLName(outputList[j]) << ")";
      }
      if (index) output << endl; output << V3_INDENT << ");" << endl;
   }
   output << "endmodule" << endl; const_cast<V3NtkHandler*>(handler)->resetAuxRenaming();
   return true;
}

// RTL (Verilog) Writer Main Function
void V3RTLWriter(const V3NtkHandler* const handler, const char* fileName, const bool& symbol, const bool& initial) {
   assert (handler); assert (handler->getNtk()); assert (handler->getNtk());
   // Open RTL (Verilog) Output File
   assert (fileName); ofstream output; output.open(fileName);
   if (!output.is_open()) { Msg(MSG_ERR) << "RTL Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // V3 Header for RTL Output
   const_cast<V3NtkHandler*>(handler)->setAuxRenaming(); writeV3GeneralHeader("RTL (Verilog)", output, "//");
   output << "// Internal nets are renamed with prefix \"" << handler->applyAuxNetNamePrefix("") << "\".\n" << endl;
   V3StrVec ioName; ioName.clear(); V3RTLWriter(handler, output, symbol, initial, "", ioName); output.close();
}

#endif

