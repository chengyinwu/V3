/****************************************************************************
  FileName     [ v3AlgSim.cpp ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ V3 Ntk Simulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_SIM_C
#define V3_ALG_SIM_C

#include "v3Msg.h"
#include "v3AlgSim.h"

#include <fstream>
#include <iomanip>

// General Simulation Functions
const bool performInputFileSimulation(const V3NtkHandler* const handler, const string& fileName, 
                                      const bool& event, const string& outFileName) {
   assert (handler); assert (handler->getNtk());
   assert (!handler->getNtk()->getModuleSize()); assert (fileName.size());
   // Open Pattern File
   ifstream input; input.open(fileName.c_str());
   if (!input) { Msg(MSG_ERR) << "Pattern Input File \"" << fileName << "\" Not Found !!" << endl; return false; }
   // Open Output File
   ofstream output;
   if (outFileName.size()) {
      output.open(outFileName.c_str());
      if (!output) {
         Msg(MSG_ERR) << "Simulation Output File \"" << outFileName << "\" Not Found !!" << endl;
         input.close(); return false;
      }
   }
   // Create Simulation Handler
   V3AlgSimulate* simHandler = 0;
   if (dynamic_cast<V3BvNtk*>(handler->getNtk())) simHandler = new V3AlgBvSimulate(handler);
   else simHandler = new V3AlgAigSimulate(handler); assert (simHandler);
   // Get Number of Patterns
   uint32_t patternCount = 0, patternSize = 0; input >> patternCount >> patternSize; assert (patternCount);
   // Check Input Pattern Size Validation
   const V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   uint32_t inputSize = 0;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) inputSize += ntk->getNetWidth(ntk->getInput(i));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) inputSize += ntk->getNetWidth(ntk->getInout(i));
   if (patternSize != inputSize) {
      Msg(MSG_ERR) << "Input pattern size (" << patternSize << ") does NOT match total PI and PIO bits ("
                   << inputSize << ") in Current Ntk !!" << endl; input.close(); return false;
   }
   // Start Simulation
   double runtime = clock(); uint32_t j = 0;
   V3BitVecX value; string valueStr = "";
   if (patternSize == 0) {
      if (!event) {  // Primitive Simulation
         for (; j < patternCount; ++j) {
            simHandler->updateNextStateValue(); simHandler->simulate();
            // Output Value
            if (!outFileName.size()) continue; valueStr = "";
            for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
               value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                               ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            output << valueStr << endl;
         }
      }
      else {  // Event-Driven Simulation
         for (j = 0; j < patternCount; ++j) {
            simHandler->updateNextStateEvent();
            // Output Value
            if (!outFileName.size()) continue; valueStr = "";
            for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
               value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                               ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            output << valueStr << endl;
         }
      }
   }
   else {
      if (!event) {  // Primitive Simulation
         for (; j < patternCount; ++j) {
            do { getline(input, valueStr); assert (!input.eof()); } while (!valueStr.size());
            assert (patternSize == valueStr.size()); simHandler->updateNextStateValue();
            for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
               value.resize(ntk->getNetWidth(ntk->getInput(i)));
               for (uint32_t x = ntk->getNetWidth(ntk->getInput(i)), k = valueStr.size() - x; k < valueStr.size(); ++k)
                  if ('0' == valueStr[k]) value.set0(--x);
                  else if ('1' == valueStr[k]) value.set1(--x);
                  else if ('X' == valueStr[k]) value.setX(--x);
               valueStr = valueStr.substr(0, valueStr.size() - ntk->getNetWidth(ntk->getInput(i)));
               simHandler->setSource(ntk->getInput(i), value);
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value.resize(ntk->getNetWidth(ntk->getInout(i)));
               for (uint32_t x = ntk->getNetWidth(ntk->getInout(i)), k = valueStr.size() - x; k < valueStr.size(); ++k)
                  if ('0' == valueStr[k]) value.set0(--x);
                  else if ('1' == valueStr[k]) value.set1(--x);
                  else if ('X' == valueStr[k]) value.setX(--x);
               valueStr = valueStr.substr(0, valueStr.size() - ntk->getNetWidth(ntk->getInout(i)));
               simHandler->setSource(ntk->getInout(i), value);
            }
            simHandler->simulate();
            // Output Value
            if (!outFileName.size()) continue; valueStr = "";
            for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
               value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                               ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            output << valueStr << endl;
         }
      }
      else {  // Event-Driven Simulation
         for (j = 0; j < patternCount; ++j) {
            do { getline(input, valueStr); assert (!input.eof()); } while (!valueStr.size());
            assert (patternSize == valueStr.size()); simHandler->updateNextStateEvent();
            for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
               value.resize(ntk->getNetWidth(ntk->getInput(i)));
               for (uint32_t x = ntk->getNetWidth(ntk->getInput(i)), k = valueStr.size() - x; k < valueStr.size(); ++k)
                  if ('0' == valueStr[k]) value.set0(--x);
                  else if ('1' == valueStr[k]) value.set1(--x);
                  else if ('X' == valueStr[k]) value.setX(--x);
               valueStr = valueStr.substr(0, valueStr.size() - ntk->getNetWidth(ntk->getInput(i)));
               simHandler->setSourceEvent(ntk->getInput(i), value);
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value.resize(ntk->getNetWidth(ntk->getInout(i)));
               for (uint32_t x = ntk->getNetWidth(ntk->getInout(i)), k = valueStr.size() - x; k < valueStr.size(); ++k)
                  if ('0' == valueStr[k]) value.set0(--x);
                  else if ('1' == valueStr[k]) value.set1(--x);
                  else if ('X' == valueStr[k]) value.setX(--x);
               valueStr = valueStr.substr(0, valueStr.size() - ntk->getNetWidth(ntk->getInout(i)));
               simHandler->setSourceEvent(ntk->getInout(i), value);
            }
            // Output Value
            if (!outFileName.size()) continue; valueStr = "";
            for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
               value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
               value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                               ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
               valueStr = value.regEx() + valueStr;
            }
            output << valueStr << endl;
         }
      }
   }
   runtime = (clock() - runtime) / CLOCKS_PER_SEC;
   Msg(MSG_IFO) << j << " Patterns are Simulated from " << fileName 
                << "  (time = " << setprecision(5) << runtime << "  sec)" << endl;
   delete simHandler; input.close(); if (outFileName.size()) output.close(); return true;
}

const bool performRandomSimulation(const V3NtkHandler* const handler, const uint32_t& patternCount, 
                                   const bool& event, const string& outFileName) {
   assert (handler); assert (handler->getNtk());
   assert (!handler->getNtk()->getModuleSize()); assert (patternCount);
   // Open Output File
   ofstream output;
   if (outFileName.size()) {
      output.open(outFileName.c_str());
      if (!output) {
         Msg(MSG_ERR) << "Simulation Output File \"" << outFileName << "\" Not Found !!" << endl;
         return false;
      }
   }
   // Create Simulation Handler
   V3AlgSimulate* simHandler = 0;
   if (dynamic_cast<V3BvNtk*>(handler->getNtk())) simHandler = new V3AlgBvSimulate(handler);
   else simHandler = new V3AlgAigSimulate(handler); assert (simHandler);
   // Start Simulation
   const V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   double runtime = clock(); uint32_t j = 0;
   V3BitVecX value; string valueStr = "";
   if (!event) {  // Primitive Simulation
      for (; j < patternCount; ++j) {
         simHandler->updateNextStateValue(); simHandler->setSourceFree(V3_PI, true);
         simHandler->setSourceFree(V3_PIO, true); simHandler->simulate();
         // Output Value
         if (!outFileName.size()) continue; valueStr = "";
         for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
            value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
            valueStr = value.regEx() + valueStr;
         }
         for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
            value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                            ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
            valueStr = value.regEx() + valueStr;
         }
         output << valueStr << endl;
      }
   }
   else {  // Event-Driven Simulation
      for (; j < patternCount; ++j) {
         simHandler->updateNextStateEvent(); simHandler->setSourceFreeEvent(V3_PI, true);
         simHandler->setSourceFreeEvent(V3_PIO, true);
         // Output Value
         if (!outFileName.size()) continue; valueStr = "";
         for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
            value = simHandler->getSimValue(ntk->getOutput(i)).bv_slice(ntk->getNetWidth(ntk->getOutput(i)) - 1, 0);
            valueStr = value.regEx() + valueStr;
         }
         for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
            value = simHandler->getSimValue(ntk->getInputNetId(ntk->getInout(i), 0)).bv_slice(
                                            ntk->getNetWidth(ntk->getInout(i)) - 1, 0);
            valueStr = value.regEx() + valueStr;
         }
         output << valueStr << endl;
      }
   }
   runtime = (clock() - runtime) / CLOCKS_PER_SEC;
   Msg(MSG_IFO) << j << " Patterns are Simulated"
                << "  (time = " << setprecision(5) << runtime << "  sec)" << endl;
   delete simHandler; if (outFileName.size()) output.close(); return true;
}

#endif

