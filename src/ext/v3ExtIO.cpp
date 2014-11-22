/****************************************************************************
  FileName     [ v3ExtIO.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Extended I/O Functions for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_EXT_IO_C
#define V3_EXT_IO_C

#include "v3Msg.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkWriter.h"
#include "v3AlgSimulate.h"

#include <cstring>
#include <fstream>

// Counterexample Trace Parser Helper Functions
inline const bool getNextLineInCexTrace(ifstream& input, string& inputStr) {
   assert (input.is_open()); assert (!input.eof());
   do { getline(input, inputStr); if (input.eof()) return false; } while (!inputStr.size());
   assert (inputStr.size()); return true;
}

inline const bool lexHeaderInCexTrace(const string& header, uint32_t& pC, uint32_t& pS, uint32_t& iS) {
   assert (header.size()); pC = pS = iS = V3NtkUD;
   uint32_t i = 0; int x; string str = "";
   for (; i < header.size(); ++i) if (' ' != header[i]) str += header[i]; else if (str.size()) break;
   if (!str.size() || !v3Str2Int(str, x) || x < 0) return false; pC = x; str = "";
   for (; i < header.size(); ++i) if (' ' != header[i]) str += header[i]; else if (str.size()) break;
   if (!str.size() || !v3Str2Int(str, x) || x < 0) return false; pS = x; str = "";
   for (; i < header.size(); ++i) if (' ' != header[i]) str += header[i]; else if (str.size()) break;
   if (str.size()) { if (!v3Str2Int(str, x) || x < 0) return false; iS = x; } return true;
}

// Counterexample Trace Recording and Recovering Functions
V3CexTrace* const V3CexTraceParser(const V3NtkHandler* const handler, const string& fileName) {
   assert (handler);
   // Open Input File
   assert (fileName.size()); ifstream input; input.open(fileName.c_str());
   if (!input.is_open()) { Msg(MSG_ERR) << "Input File \"" << fileName << "\" Not Found !!" << endl; return 0; }
   // Get Number of Patterns, Size of Patterns, and Size of Initial States
   uint32_t patternCount = 0, patternSize = 0, initSize = 0; string inputStr = "";
   if (!getNextLineInCexTrace(input, inputStr)) {
      Msg(MSG_ERR) << "Missing Header in the Trace Input !!" << endl; input.close(); return 0; }
   if (!lexHeaderInCexTrace(inputStr, patternCount, patternSize, initSize) || !patternCount) {
      Msg(MSG_ERR) << "Unexpected Header in the Trace Input !!" << endl; input.close(); return 0; }
   // Check Input Pattern Size Validation
   const V3Ntk* const ntk = handler->getNtk(); assert (ntk); uint32_t inputSize = 0;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) inputSize += ntk->getNetWidth(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) inputSize += ntk->getNetWidth(ntk->getInout(i));
   if (patternSize < inputSize) {
      Msg(MSG_ERR) << "Input pattern size (" << patternSize << ") is smaller than total PI and PIO bits ("
                   << inputSize << ") in Current Ntk !!" << endl;
      input.close(); return 0;
   }
   patternSize = inputSize;  // Read ONLY inputSize Bits in Every Input Pattern
   // Check Initial State Size Validation
   if (V3NtkUD != initSize) {
      inputSize = 0;
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) inputSize += ntk->getNetWidth(ntk->getLatch(i));
      if (initSize < inputSize) {
         Msg(MSG_ERR) << "Initial state pattern size (" << initSize << ") is smaller than total FF bits (" 
                      << inputSize << ") in Current Ntk !!" << endl;
         input.close(); return 0;
      }
      initSize = inputSize;  // Read ONLY inputSize Bits in Initial State Pattern
   }
   // Get Pattern Cycle by Cycle
   V3CexTrace* const cex = new V3CexTrace(patternCount); assert (cex);
   if (initSize && (V3NtkUD != initSize)) {  // Get Initial State Pattern
      if (!getNextLineInCexTrace(input, inputStr)) {
         Msg(MSG_ERR) << "Missing Initial State in the Trace Input !!" << endl;
         input.close(); delete cex; return 0;
      }
      if (initSize > inputStr.size()) {
         Msg(MSG_ERR) << "Initial state pattern size (" << inputStr.size() 
                      << ") Does NOT Match the size specified in the Header !!" << endl;
         input.close(); return 0;
      }
      V3BitVecX initValue(initSize); assert (initSize <= inputStr.size());
      for (uint32_t j = inputStr.size() - initSize, k = initSize; j < inputStr.size(); ++j) {
         if ('0' == inputStr[j]) initValue.set0(--k);
         else if ('1' == inputStr[j]) initValue.set1(--k);
         else if ('X' == inputStr[j]) initValue.setX(--k);
      }
      cex->setInit(initValue);
   }
   if (patternSize) {  // Get Input Patterns
      V3BitVecX patternValue(patternSize);
      for (uint32_t i = 0; i < patternCount; ++i) {
         if (!getNextLineInCexTrace(input, inputStr)) {
            Msg(MSG_ERR) << "Expecting Input Pattern " << i << " in the Trace Input !!" << endl;
            input.close(); delete cex; return 0;
         }
         if (patternSize > inputStr.size()) {
            Msg(MSG_ERR) << "Input pattern size (" << inputStr.size() 
                         << ") Does NOT Match the size specified in the Header !!" << endl;
            input.close(); return 0;
         }
         assert (patternSize <= inputStr.size());
         for (uint32_t j = inputStr.size() - patternSize, k = patternSize; j < inputStr.size(); ++j) {
            if ('0' == inputStr[j]) patternValue.set0(--k);
            else if ('1' == inputStr[j]) patternValue.set1(--k);
            else if ('X' == inputStr[j]) patternValue.setX(--k);
         }
         cex->pushData(patternValue);
      }
   }
   input.close(); assert (cex); return cex;
}

void V3CexTraceWriter(const V3NtkHandler* const handler, const V3CexTrace* const cex, const string& fileName) {
   assert (handler); assert (cex); assert (cex->getTraceSize());
   // Open Output File
   assert (fileName.size()); ofstream output; output.open(fileName.c_str());
   if (!output.is_open()) { Msg(MSG_ERR) << "Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Restrict to Output PI and Initial State Only
   const V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   uint32_t patternSize = 0;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) patternSize += ntk->getNetWidth(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) patternSize += ntk->getNetWidth(ntk->getInout(i));
   uint32_t initSize = 0;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) initSize += ntk->getNetWidth(ntk->getLatch(i));
   assert ((!cex->getInit()) || (cex->getInit()->size() >= initSize));
   // Write Number of Patterns and PatternSize
   output << cex->getTraceSize() << " " << patternSize;
   if (cex->getInit()) output << " " << initSize; output << endl;
   // Write Initial State
   if (cex->getInit()) {
      V3BitVecX* const initValue = cex->getInit(); assert (initValue);
      uint32_t j = initSize; assert (initSize <= initValue->size());
      while (j--) { output << (*initValue)[j]; } output << endl;
   }
   // Write Pattern Cycle by Cycle
   if (patternSize) {
      V3BitVecX patternValue(patternSize);
      for (uint32_t i = 0; i < cex->getTraceDataSize(); ++i) {
         patternValue = cex->getData(i).bv_slice(patternSize - 1, 0);
         uint32_t j = patternSize; assert (patternSize == patternValue.size());
         while (j--) { output << patternValue[j]; } output << endl;
      }
      output << endl;
   }
   // Write Footer
   writeV3GeneralHeader("Counterexample Trace Output", output, "#"); output.close();
}

void V3CexTraceVisualizer(const V3NtkHandler* const handler, const V3CexTrace* const cex, const string& fileName) {
   assert (handler); assert (cex); assert (cex->getTraceSize());
   // Open Output File
   assert (fileName.size()); ofstream output; output.open(fileName.c_str());
   if (!output.is_open()) { Msg(MSG_ERR) << "Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Restrict to Output PI Only  (Left FF Absent)
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Write Header
   time_t cur_time; time (&cur_time);
   string curTimeStr = asctime(localtime(&cur_time));
   output << "$comment" << endl;
   output << "Counterexample Trace Waveform Generated by V3 compiled @ $version" << endl;
   output << "$end" << endl << endl;
   output << "$date " << curTimeStr.substr(0, curTimeStr.size() - 1) << " $end" << endl;
   output << "$version " <<  __DATE__ << " " << __TIME__ << " $end" << endl << endl;
   // Write Module
   output << "$timescale 1 ps $end" << endl;
   output << "$scope module module $end" << endl << endl;
   // Write Clock
   const string clockName = ((V3NetUD == ntk->getClock()) ? "v3_clock" : V3RTLNameOrId(handler, ntk->getClock()));
   output << "$var wire 1 " << clockName << " " << clockName << " $end" << endl;
   V3BoolVec writtenNet(ntk->getNetSize(), false);
   V3StrVec nameNet(ntk->getNetSize(), "");
   // Write Primary Inputs
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      assert (!writtenNet[ntk->getInput(i).id]); assert (!nameNet[ntk->getInput(i).id].size());
      nameNet[ntk->getInput(i).id] = V3RTLNameOrId(handler, ntk->getInput(i));
      assert (nameNet[ntk->getInput(i).id].size()); writtenNet[ntk->getInput(i).id] = true;
      output << "$var wire " << ntk->getNetWidth(ntk->getInput(i)) << " " << nameNet[ntk->getInput(i).id] 
             << " " << nameNet[ntk->getInput(i).id] << " $end" << endl;
   }
   // Write Primary Inouts
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      assert (!writtenNet[ntk->getInout(i).id]); assert (!nameNet[ntk->getInout(i).id].size());
      nameNet[ntk->getInout(i).id] = V3RTLNameOrId(handler, ntk->getInout(i));
      assert (nameNet[ntk->getInout(i).id].size()); writtenNet[ntk->getInout(i).id] = true;
      output << "$var wire " << ntk->getNetWidth(ntk->getInout(i)) << " " << nameNet[ntk->getInout(i).id] 
             << " " << nameNet[ntk->getInout(i).id] << " $end" << endl;
   }
   // Write Flip-Flops
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      assert (!writtenNet[ntk->getLatch(i).id]); assert (!nameNet[ntk->getLatch(i).id].size());
      nameNet[ntk->getLatch(i).id] = V3RTLNameOrId(handler, ntk->getLatch(i));
      assert (nameNet[ntk->getLatch(i).id].size()); writtenNet[ntk->getLatch(i).id] = true;
      output << "$var wire " << ntk->getNetWidth(ntk->getLatch(i)) << " " << nameNet[ntk->getLatch(i).id] 
             << " " << nameNet[ntk->getLatch(i).id] << " $end" << endl;
   }
   // Write Primary Outputs
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      if (writtenNet[ntk->getOutput(i).id]) continue; assert (!nameNet[ntk->getOutput(i).id].size());
      nameNet[ntk->getOutput(i).id] = V3RTLNameOrId(handler, ntk->getOutput(i));
      assert (nameNet[ntk->getOutput(i).id].size()); writtenNet[ntk->getOutput(i).id] = true;
      output << "$var wire " << ntk->getNetWidth(ntk->getOutput(i)) << " " << nameNet[ntk->getOutput(i).id] 
             << " " << nameNet[ntk->getOutput(i).id] << " $end" << endl;
   }
   // Write Internal Signals
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) {
      if (writtenNet[id.id]) continue; assert (!nameNet[id.id].size());
      nameNet[id.id] = V3RTLNameOrId(handler, id);
      assert (nameNet[id.id].size()); writtenNet[id.id] = true;
      output << "$var wire " << ntk->getNetWidth(id) << " " << nameNet[id.id] 
             << " " << nameNet[id.id] << " $end" << endl;
   }
   // Write Timeframe
   const string timeFrameName = "v3_timeframe", timeFrameRegExPrefix = "64'd";
   output << "$var wire 64 " << timeFrameName << " " << timeFrameName << " $end" << endl;
   output << "$upscope $end" << endl;
   output << "$enddefinitions $end" << endl;
   // Check Counterexample Directly on Ntk
   V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(ntk)) simulator = new V3AlgBvSimulate(handler);
   else simulator = new V3AlgAigSimulate(handler); assert (simulator);
   // For each time-frame, set pattern from counterexample
   V3SimTrace signalValue(ntk->getNetSize());
   V3BitVecX pattern, value;
   for (uint32_t i = 0, k; i < cex->getTraceSize(); ++i) {
      // Reset Tables
      for (k = 0; k < writtenNet.size(); ++k) writtenNet[i] = false;
      // Write New Timeframe Info
      output << "#" << (5 * (i << 1)) << endl;  // 5 is the default half-cycle
      output << "b1" << " " << clockName << endl;
      // Update FF Next State Values
      simulator->updateNextStateValue();
      // Set Initial State Values
      if (!i && cex->getInit()) {
         V3BitVecX* const initValue = cex->getInit(); k = 0;
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
            simulator->setSource(ntk->getLatch(j), initValue->bv_slice(k + ntk->getNetWidth(ntk->getLatch(j)) - 1, k));
            k += ntk->getNetWidth(ntk->getLatch(j)); assert (k <= initValue->size());
         }
      }
      // Set PI Values and Record Pattern Values
      k = 0; if (cex->getTraceDataSize()) pattern = cex->getData(i);
      for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
         simulator->setSource(ntk->getInput(j), pattern.bv_slice(k + ntk->getNetWidth(ntk->getInput(j)) - 1, k));
         k += ntk->getNetWidth(ntk->getInput(j)); assert (k <= pattern.size());
      }
      for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
         simulator->setSource(ntk->getInout(j), pattern.bv_slice(k + ntk->getNetWidth(ntk->getInout(j)) - 1, k));
         k += ntk->getNetWidth(ntk->getInout(j)); assert (k <= pattern.size());
      }
      // Simulate Ntk for a Cycle
      simulator->simulate();
      // Record Signal Values
      for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) {
         value = simulator->getSimValue(id).bv_slice(ntk->getNetWidth(id) - 1, 0);
         if (i && (value == signalValue[id.id])) continue; signalValue[id.id] = value;
         output << "b" << value.regEx() << " " << nameNet[id.id] << endl;
      }
      // Write New Timeframe Info
      output << "b" << V3BitVec((timeFrameRegExPrefix + v3Int2Str(1 + i)).c_str()).regEx() 
             << " " << timeFrameName << endl;
      output << "#" << (5 * (1 + (i << 1))) << endl;  // 5 is the default half-cycle
      output << "b0" << " " << clockName << endl;
   }
   output.close();
}

#endif

