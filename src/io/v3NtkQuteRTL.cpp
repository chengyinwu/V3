/****************************************************************************
  FileName     [ v3NtkQuteRTL.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ QuteRTL Front-End to V3 Network Construction. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_QUTERTL_C
#define V3_NTK_QUTERTL_C

#include "v3Msg.h"
#include "v3Set.h"
#include "v3StrUtil.h"
#include "v3FileUtil.h"
#include "v3NtkParser.h"
#include "v3NtkWriter.h"

#include "quteRTL.h"

#include <fstream>
#include <string>

// Global Variable  (Will be User Definable in Compile Time by Configure.)
const string quteRTL_MsgFile = "quteRTL.log";  // QuteRTL Log File

// Forward Declaration
V3NtkInput* const V3QuteRTLHandler(CktModule* const&, const bool&, V3NetVec&, const string& = "");

// Set QuteRTL Log File Helper Functions
void setQuteLogFile() {
   ofstream output; output.open(quteRTL_MsgFile.c_str());
   if (output.is_open()) {
      writeV3GeneralHeader("QuteRTL API Output Log", output, "//"); output.close();
      quteSetLogFile(quteRTL_MsgFile.c_str(), true);
   }
   else Msg(MSG_WAR) << "QuteRTL Log \"" << quteRTL_MsgFile << "\" Cannot be Opened !!" << endl;
}

// QuteRTL Module to V3 Ntk Transformation Helper Functions
const uint32_t getOutPinWidthFromQuteRTL(const string& name, CktOutPin* const OutPin) {
   assert (name.size()); assert (OutPin);
   const uint32_t end = quteGetPinEnd(OutPin), begin = quteGetPinBegin(OutPin);
   if (end < begin) Msg(MSG_WAR) << "Inverting Net Found : " << name << "[" << end << ":" << begin << "]" << endl;
   return (end >= begin) ? (1 + end - begin) : (1 + begin - end);
}

const V3NetId dfsBuildNtkFromQuteRTL(V3NtkInput* const quteHandler, CktOutPin* const OutPin, const bool& a2s) {
   assert (quteHandler); assert (OutPin);
   // Check if OutPin Already Exists
   const string name = quteGetOutPinName(OutPin); assert (name.size());
   V3NetId id = quteHandler->getNetId(name); if (V3NetUD != id) return id;
   // Get OutPin Info
   CktCell* const cell = quteGetCellFromPin(OutPin); assert (cell);
   const QuteRTL_API_CellType type = quteGetCellType(cell); assert (type < QUTE_TOTAL_CELL);
   assert (type != QUTE_PI_CELL && type != QUTE_PO_CELL && type != QUTE_PIO_CELL && type != QUTE_DFF_CELL);
   // Create V3NetId for OutPin
   if (QUTE_MODULE_CELL != type) {
      id = quteHandler->createNet(name, getOutPinWidthFromQuteRTL(name, OutPin));
      if (V3NetUD == id) return id;
   }
   // Compute V3 Gate Type According to QuteRTL Cell Type
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(quteHandler->getNtk()); assert (ntk);
   bool invert = false, exactTwo = false;
   V3GateType v3Type;
   switch (type) {
      // Single Input : Reduced
      case QUTE_RED_AND_CELL  : v3Type = BV_RED_AND;                   break;
      case QUTE_RED_OR_CELL   : v3Type = BV_RED_OR;                    break;
      case QUTE_RED_NAND_CELL : v3Type = BV_RED_AND;  invert = true;   break;
      case QUTE_RED_NOR_CELL  : v3Type = BV_RED_OR;   invert = true;   break;
      case QUTE_RED_XOR_CELL  : v3Type = BV_RED_XOR;                   break;
      case QUTE_RED_XNOR_CELL : v3Type = BV_RED_XOR;  invert = true;   break;
      // Single Input : Logic
      case QUTE_BUF_CELL      : v3Type = BV_BUF;                       break;
      case QUTE_INV_CELL      : v3Type = BV_BUF;      invert = true;   break;
      // One+ Input : Logic
      case QUTE_AND_CELL      : v3Type = BV_AND;                       break;
      case QUTE_OR_CELL       : v3Type = BV_OR;                        break;
      case QUTE_NAND_CELL     : v3Type = BV_AND;      invert = true;   break;
      case QUTE_NOR_CELL      : v3Type = BV_OR;       invert = true;   break;
      case QUTE_XOR_CELL      : v3Type = BV_XOR;                       break;
      case QUTE_XNOR_CELL     : v3Type = BV_XOR;      invert = true;   break;
      // One+ Input : Arithmetic
      case QUTE_ADD_CELL      : v3Type = BV_ADD;                       break;
      case QUTE_SUB_CELL      : v3Type = BV_SUB;                       break;
      case QUTE_MULT_CELL     : v3Type = BV_MULT;                      break;
      case QUTE_DIV_CELL      : v3Type = BV_DIV;                       break;
      case QUTE_MODULO_CELL   : v3Type = BV_MODULO;                    break;
      // One+ Input : Model
      case QUTE_MERGE_CELL    : v3Type = BV_MERGE;                     break;
      // Two Inputs : Arithmetic
      case QUTE_SHL_CELL      : v3Type = BV_SHL;                       break;
      case QUTE_SHR_CELL      : v3Type = BV_SHR;                       break;
      // Two Inputs : Comparator
      case QUTE_EQUALITY_CELL : v3Type = BV_EQUALITY; exactTwo = true; break;
      case QUTE_GEQ_CELL      : v3Type = BV_GEQ;      exactTwo = true; break;
      case QUTE_GREATER_CELL  : v3Type = BV_GREATER;  exactTwo = true; break;
      case QUTE_LEQ_CELL      : v3Type = BV_LEQ;      exactTwo = true; break;
      case QUTE_LESS_CELL     : v3Type = BV_LESS;     exactTwo = true; break;
      // Multiplexer
      case QUTE_MUX_CELL      : v3Type = BV_MUX;                       break;
      // Model
      case QUTE_CONST_CELL    : v3Type = BV_CONST;                     break;
      case QUTE_SPLIT_CELL    : v3Type = BV_SLICE;                     break;
      // Unsupported : Model
      case QUTE_MEMORY_CELL   : Msg(MSG_ERR) << "Memory Exists in RTL Design !!" << endl; return V3NetUD;
      // Unsupported : Module Instance
      case QUTE_MODULE_CELL   : v3Type = V3_MODULE;                    break;
      // Unsupported : Latch
      case QUTE_DLAT_CELL     : Msg(MSG_ERR) << "Latch Exists in RTL Design !!" << endl; return V3NetUD;
      default                 : Msg(MSG_ERR) << "Unexpected QuteRTL Cell Type : " << type << endl; return V3NetUD;
   }
   // Build V3 Gate
   V3NetId id1, id2, id3, id4;
   if (isV3ReducedType(v3Type)) {  // REDUCED
      assert (quteGetCellInputSize(cell) == 1); assert (V3NetUD != id);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 0), a2s); if (V3NetUD == id1) return id1;
      id2 = ntk->createNet(1); if (V3NetUD == id2) return id2;
      if (!createBvReducedGate(ntk, v3Type, id2, id1)) return V3NetUD;
      // Matching Output and Input Bit-widths
      if (ntk->getNetWidth(id) == ntk->getNetWidth(id2)) {
         if (!createV3BufGate(ntk, id, (invert ? getV3InvertNet(id2) : id2))) return V3NetUD; }
      else {
         id3 = ntk->createNet(1); if (V3NetUD == id3) return id3;
         if (!createV3BufGate(ntk, id3, (invert ? getV3InvertNet(id2) : id2))) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id3)) return V3NetUD;
      }
   }
   else if (exactTwo) {  // TWO INPUTS
      assert (quteGetCellInputSize(cell) == 2); assert (V3NetUD != id); assert (!invert);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 0), a2s); if (V3NetUD == id1) return id1;
      id2 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 1), a2s); if (V3NetUD == id2) return id2;
      // Matching Input Bit-widths
      if (ntk->getNetWidth(id1) > ntk->getNetWidth(id2)) {
         id2 = createNetExtensionGate(ntk, id2, ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2; }
      else if (ntk->getNetWidth(id1) < ntk->getNetWidth(id2)) {
         id1 = createNetExtensionGate(ntk, id1, ntk->getNetWidth(id2)); if (V3NetUD == id1) return id1; }
      // Matching Output and Input Bit-widths
      if (1 == ntk->getNetWidth(id)) {
         if (!createBvPairGate(ntk, v3Type, id, id1, id2)) return V3NetUD; }
      else {
         id3 = ntk->createNet(1); if (V3NetUD == id3) return id3;
         if (!createBvPairGate(ntk, v3Type, id3, id1, id2)) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id3)) return V3NetUD;
      }
   }
   else if (BV_MUX == v3Type) {  // MULTIPLEXER
      assert (quteGetCellInputSize(cell) == 3); assert (V3NetUD != id); assert (!invert);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 0), a2s); if (V3NetUD == id1) return id1;
      id2 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 1), a2s); if (V3NetUD == id2) return id2;
      // Matching Input Bit-widths
      if (ntk->getNetWidth(id1) > ntk->getNetWidth(id2)) {
         id2 = createNetExtensionGate(ntk, id2, ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2; }
      else if (ntk->getNetWidth(id1) < ntk->getNetWidth(id2)) {
         id1 = createNetExtensionGate(ntk, id1, ntk->getNetWidth(id2)); if (V3NetUD == id1) return id1; }
      id3 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 2), a2s); if (V3NetUD == id3) return id3;
      // Matching Output and Input Bit-widths
      if (ntk->getNetWidth(id) == ntk->getNetWidth(id1)) {
         if (!createBvMuxGate(ntk, id, id1, id2, id3)) return V3NetUD; }
      else {
         id4 = ntk->createNet(ntk->getNetWidth(id1)); if (V3NetUD == id4) return id4;
         if (!createBvMuxGate(ntk, id4, id1, id2, id3)) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id4)) return V3NetUD;
      }
   }
   else if (BV_SLICE == v3Type) {  // SLICE
      assert (quteGetCellInputSize(cell) == 1); assert (V3NetUD != id); assert (!invert);
      CktOutPin* inOutPin = quteGetCellInputPin(cell, 0); assert (inOutPin);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, inOutPin, a2s); if (V3NetUD == id1) return id1;
      const uint32_t sliceEnd = quteGetSplitOutPinEnd(OutPin), sliceBegin = quteGetSplitOutPinBegin(OutPin);
      const uint32_t end = quteGetPinEnd(inOutPin), begin = quteGetPinBegin(inOutPin);
      if (end >= begin) {
         assert (sliceEnd <= end); assert (sliceBegin >= begin);
         // Matching Output and Input Bit-widths
         if (ntk->getNetWidth(id) == (1 + sliceEnd - sliceBegin)) {
            if (!createBvSliceGate(ntk, id, id1, sliceEnd - begin, sliceBegin - begin)) return V3NetUD; }
         else {
            id2 = ntk->createNet(1 + sliceEnd - sliceBegin); if (V3NetUD == id2) return id2;
            if (!createBvSliceGate(ntk, id2, id1, sliceEnd - begin, sliceBegin - begin)) return V3NetUD;
            if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
         }
      }
      else {
         assert (sliceEnd >= end); assert (sliceBegin <= begin);
         // Matching Output and Input Bit-widths
         if (ntk->getNetWidth(id) == (1 + sliceBegin - sliceEnd)) {
            if (!createBvSliceGate(ntk, id, id1, begin - sliceEnd, begin - sliceBegin)) return V3NetUD;
         }
         else {
            id2 = ntk->createNet(1 + sliceBegin - sliceEnd); if (V3NetUD == id2) return id2;
            if (!createBvSliceGate(ntk, id2, id1, begin - sliceEnd, begin - sliceBegin)) return V3NetUD;
            if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
         }
      }
   }
   else if (BV_CONST == v3Type) {  // CONST
      assert (quteGetCellInputSize(cell) == 0); assert (V3NetUD != id); assert (!invert);
      const string value = quteGetConstCellValue(cell); assert (value.size());
      // Matching Output and Input Bit-widths
      V3BitVec bvValue(value.c_str());
      if (ntk->getNetWidth(id) == bvValue.size()) {
         if (!createBvConstGate(ntk, id, value)) return V3NetUD; }
      else {
         id2 = ntk->createNet(bvValue.size()); if (V3NetUD == id2) return id2;
         if (!createBvConstGate(ntk, id2, value)) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
      }
   }
   else if (BV_BUF == v3Type) {  // BUF / INV
      assert (quteGetCellInputSize(cell) == 1); assert (V3NetUD != id);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, 0), a2s); if (V3NetUD == id1) return id1;
      // Matching Output and Input Bit-widths
      if (ntk->getNetWidth(id) == ntk->getNetWidth(id1)) {
         if (!createV3BufGate(ntk, id, (invert ? getV3InvertNet(id1) : id1))) return V3NetUD; }
      else {
         id2 = ntk->createNet(ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2;
         if (!createV3BufGate(ntk, id2, (invert ? getV3InvertNet(id1) : id1))) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
      }
   }
   else if (BV_MERGE == v3Type) {  // MERGE
      assert (quteGetCellInputSize(cell) >= 1); assert (V3NetUD != id); id1 = V3NetUD;
      uint32_t merge_width = 0;
      for (uint32_t i = 0, j = quteGetCellInputSize(cell); i < j; ++i) {
         id2 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, i), a2s); if (V3NetUD == id2) return id2;
         merge_width += ntk->getNetWidth(id2); assert (merge_width);
         if (V3NetUD == id1) { id1 = id2; continue; }
         id3 = ntk->createNet(merge_width); if (V3NetUD == id3) return id3;
         if (!createBvPairGate(ntk, v3Type, id3, id1, id2)) return V3NetUD; id1 = id3;
      }
      // Matching Output and Input Bit-widths
      if (ntk->getNetWidth(id) == ntk->getNetWidth(id1)) {
         if (!createV3BufGate(ntk, id, id1)) return V3NetUD; }
      else {
         id2 = ntk->createNet(ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2;
         if (!createV3BufGate(ntk, id2, id1)) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
      }
   }
   else if (V3_MODULE == v3Type) {  // MODULE
      // Get Module Instance
      CktModule* const module = quteGetModuleInstance(cell); assert (module);
      // Create Output Nets for Module Instance
      V3NetVec outputs(quteGetCellOutputSize(cell), V3NetUD); assert (V3NetUD == id);
      assert (outputs.size() == quteGetDesignIoSize(module, QUTE_PO_CELL));
      for (uint32_t i = 0; i < outputs.size(); ++i) {
         CktOutPin* const outputPin = quteGetCellOutputPin(cell, i);
         if (outputPin) {  // Non-Floating Output
            const string outputName = quteGetOutPinName(outputPin); assert (outputName.size());
            assert (V3NetUD == quteHandler->getNetId(outputName));
            outputs[i] = quteHandler->createNet(outputName, getOutPinWidthFromQuteRTL(outputName, outputPin));
            if (V3NetUD == outputs[i]) return outputs[i]; if (OutPin == outputPin) id = outputs[i];
            // Matching Module Output Bit-widths
            CktCell* const outputCell = quteGetDesignIoCell(module, QUTE_PO_CELL, i); assert (outputCell);
            assert (1 == quteGetCellOutputSize(outputCell)); assert (quteGetCellOutputPin(outputCell, 0));
            if (quteGetPinWidth(quteGetCellOutputPin(outputCell, 0)) != ntk->getNetWidth(outputs[i])) {
               id1 = ntk->createNet(quteGetPinWidth(quteGetCellOutputPin(outputCell, 0))); if (V3NetUD == id1) return id1;
               if (!createIOExtensionGate(ntk, outputs[i], id1)) return V3NetUD; outputs[i] = id1;
            }
         }
         else {  // Floating Output
            CktCell* const outputCell = quteGetDesignIoCell(module, QUTE_PO_CELL, i); assert (outputCell);
            assert (1 == quteGetCellOutputSize(outputCell)); assert (quteGetCellOutputPin(outputCell, 0));
            outputs[i] = quteHandler->createNet("", quteGetPinWidth(quteGetCellOutputPin(outputCell, 0)));
            if (V3NetUD == outputs[i]) return outputs[i];
         }
      }
      // Collect Input Nets for Module Instance
      V3NetVec inputs(quteGetCellInputSize(cell), V3NetUD);
      assert (inputs.size() == quteGetDesignIoSize(module, QUTE_PI_CELL));
      for (uint32_t i = 0; i < inputs.size(); ++i) {
         inputs[i] = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, i), a2s);
         if (V3NetUD == inputs[i]) return inputs[i];
         // Matching Module Input Bit-widths
         CktCell* const inputCell = quteGetDesignIoCell(module, QUTE_PI_CELL, i); assert (inputCell);
         assert (1 == quteGetCellOutputSize(inputCell)); assert (quteGetCellOutputPin(inputCell, 0));
         if (quteGetPinWidth(quteGetCellOutputPin(inputCell, 0)) != ntk->getNetWidth(inputs[i])) {
            id1 = ntk->createNet(quteGetPinWidth(quteGetCellOutputPin(inputCell, 0))); if (V3NetUD == id1) return id1;
            if (!createIOExtensionGate(ntk, id1, inputs[i])) return V3NetUD; inputs[i] = id1;
         }
      }
      // Create Module Instance
      const V3NetVec oInputs = inputs;  // Backup for Clock Signal Identification
      V3NtkHandler* const moduleHandler = V3QuteRTLHandler(module, a2s, inputs, quteGetModuleInstanceName(cell));
      if (!moduleHandler) return V3NetUD; assert (moduleHandler->getNtk());
      // Set Clock From Module Instance
      for (uint32_t i = 0; i < inputs.size(); ++i) {
         if (V3NetUD != inputs[i]) continue; inputs.erase(inputs.begin() + i);  // Remove Clock Signal
         if (V3NetUD == ntk->getClock()) ntk->createClock(oInputs[i]);
         else if (oInputs[i] != ntk->getClock()) 
            Msg(MSG_WAR) << "Multiple Clock Domains Found in RTL Design \"" << quteHandler->getNtkName() << "\" !!" << endl;
         break;
      }
      createModule(ntk, inputs, outputs, moduleHandler, false);
   }
   else {  // ONE+ INPUT
      assert (quteGetCellInputSize(cell) >= 1); assert (V3NetUD != id);
      assert (isV3PairType(v3Type) || (BV_OR == v3Type)); id1 = V3NetUD;
      for (uint32_t i = 0, j = quteGetCellInputSize(cell); i < j; ++i) {
         id2 = dfsBuildNtkFromQuteRTL(quteHandler, quteGetCellInputPin(cell, i), a2s); if (V3NetUD == id2) return id2;
         //if (BV_ADD == v3Type || BV_SUB == v3Type) {
         //   id2 = createNetExtensionGate(ntk, id2, 1 + ntk->getNetWidth(id2)); if (V3NetUD == id2) return id2; }
         //else if (BV_MULT == v3Type) {
         //   id2 = createNetExtensionGate(ntk, id2, ntk->getNetWidth(id2) << 1); if (V3NetUD == id2) return id2; }
         if (V3NetUD == id1) { id1 = id2; continue; }
         // Matching Input Bit-widths
         if (ntk->getNetWidth(id1) > ntk->getNetWidth(id2)) {
            id2 = createNetExtensionGate(ntk, id2, ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2; }
         else if (ntk->getNetWidth(id1) < ntk->getNetWidth(id2)) {
            id1 = createNetExtensionGate(ntk, id1, ntk->getNetWidth(id2)); if (V3NetUD == id1) return id1; }
         //if (BV_SHL == v3Type) {
         //   id1 = createNetExtensionGate(ntk, id1, ntk->getNetWidth(id1) + ntk->getNetWidth(id2));
         //   if (V3NetUD == id1) return id1;
         //   id2 = createNetExtensionGate(ntk, id2, ntk->getNetWidth(id1));
         //   if (V3NetUD == id2) return id2;
         //}
         id3 = ntk->createNet(ntk->getNetWidth(id1)); if (V3NetUD == id3) return id3;
         if (!createBvPairGate(ntk, v3Type, id3, id1, id2)) return V3NetUD; id1 = id3;
      }
      if (invert) {
         id3 = ntk->createNet(1); if (V3NetUD == id3) return id3;
         if (!createV3BufGate(ntk, id3, ~id1)) return V3NetUD; id1 = id3;
      }
      // Reset Bit-Width for Internal Output Signal
      if (quteIsInternalName(name)) ntk->resetNetWidth(id, ntk->getNetWidth(id1));
      // Matching Output and Input Bit-widths
      if (ntk->getNetWidth(id) == ntk->getNetWidth(id1)) {
         if (!createV3BufGate(ntk, id, id1)) return V3NetUD; }
      else {
         id2 = ntk->createNet(ntk->getNetWidth(id1)); if (V3NetUD == id2) return id2;
         if (!createV3BufGate(ntk, id2, id1)) return V3NetUD;
         if (!createIOExtensionGate(ntk, id, id2)) return V3NetUD;
      }
   }
   // Return V3NetId for OutPin
   return id;
}

const bool V3QuteRTLInputHandler(V3NtkInput* const quteHandler, CktModule* const module, V3NetVec& piList) {
   assert (quteHandler); assert (module);
   CktCell* cell; CktOutPin* OutPin;
   V3NetId id; string name;
   // Store Inputs for Clock Signal Pruning
   piList.clear(); piList.reserve(quteGetDesignIoSize(module, QUTE_PI_CELL));
   // Build Input  (Renders the Same Order As Design Created by QuteRTL)
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(quteHandler->getNtk()); assert (ntk);
   for (uint32_t i = 0, j = quteGetDesignIoSize(module, QUTE_PI_CELL); i < j; ++i) {
      // Get Cell Info from QuteRTL
      cell = quteGetDesignIoCell(module, QUTE_PI_CELL, i); assert (cell);
      assert (quteGetCellOutputSize(cell) == 1);
      OutPin = quteGetCellOutputPin(cell, 0); assert (OutPin);
      name = quteGetOutPinName(OutPin); assert (name.size());
      // Build Input in V3 Ntk
      id = quteHandler->createNet(name, getOutPinWidthFromQuteRTL(name, OutPin)); if (V3NetUD == id) return false;
      piList.push_back(id);  //if (!createInput(ntk, id)) return false;
   }
   for (uint32_t i = 0, j = quteGetDesignIoSize(module, QUTE_PIO_CELL); i < j; ++i) {
      // Get Cell Info from QuteRTL
      cell = quteGetDesignIoCell(module, QUTE_PIO_CELL, i); assert (cell);
      assert (quteGetCellOutputSize(cell) == 1);
      OutPin = quteGetCellOutputPin(cell, 0); assert (OutPin);
      name = quteGetOutPinName(OutPin); assert (name.size());
      // Build Input in V3 Ntk
      id = quteHandler->createNet(name, getOutPinWidthFromQuteRTL(name, OutPin)); if (V3NetUD == id) return false;
      if (!createInout(ntk, id)) return false;
   }
   return true;
}

const bool V3QuteRTLFFHandler(V3NtkInput* const quteHandler, CktModule* const module, const bool& async2sync, V3NetVec& inputs) {
   assert (quteHandler); assert (module);
   CktCell* cell; CktOutPin* OutPin;
   uint32_t width; string name;
   V3NetId id, id1, id2, id3;
   // Build FF Nets  (Renders the Same Order As Design Created by QuteRTL)
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(quteHandler->getNtk()); assert (ntk);
   V3NetVec quteFFVec; quteFFVec.clear();
   V3Set<string>::Set quteFFClk; quteFFClk.clear();
   for (uint32_t i = 0, j = quteGetDesignIoSize(module, QUTE_DFF_CELL); i < j; ++i) {
      // Get Cell Info from QuteRTL
      cell = quteGetDesignIoCell(module, QUTE_DFF_CELL, i); assert (cell);
      assert (quteGetCellOutputSize(cell) == 1);
      OutPin = quteGetCellOutputPin(cell, 0); assert (OutPin);
      name = quteGetOutPinName(OutPin); assert (name.size());
      // Build DFF in V3 Ntk
      id = quteHandler->createNet(name, getOutPinWidthFromQuteRTL(name, OutPin));
      if (V3NetUD == id) return false; quteFFVec.push_back(id);
   }
   // DFS Traverse On DFF Fanin Cone
   for (uint32_t i = 0, j = quteGetDesignIoSize(module, QUTE_DFF_CELL); i < j; ++i) {
      // Get Cell Info from QuteRTL
      cell = quteGetDesignIoCell(module, QUTE_DFF_CELL, i); assert (cell);
      const uint32_t inPinSize = quteGetCellInputSize(cell); assert (inPinSize >= 2);
      // Check Clock Signal
      OutPin = quteGetCellInputPin(cell, 1); assert (OutPin);
      name = quteGetOutPinExpr(OutPin); assert (name.size()); quteFFClk.insert(name);
      if (inPinSize > 2) {  // Asynchronous Reset DFF : D, clk, reset, default
         OutPin = quteGetCellInputPin(cell, 0); assert (OutPin);
         id1 = dfsBuildNtkFromQuteRTL(quteHandler, OutPin, async2sync); if (V3NetUD == id1) return false;
         OutPin = quteGetCellInputPin(cell, 2); assert (OutPin);
         id2 = dfsBuildNtkFromQuteRTL(quteHandler, OutPin, async2sync); if (V3NetUD == id2) return false;
         OutPin = quteGetCellInputPin(cell, 3); assert (OutPin);
         id3 = dfsBuildNtkFromQuteRTL(quteHandler, OutPin, async2sync); if (V3NetUD == id3) return false;
         // Build DFF
         name = quteGetOutPinName(OutPin); assert (name.size());
         width = quteHandler->getNtk()->getNetWidth(quteFFVec[i]);
         if (async2sync) {  // Renders all Asynchronous DFF be Regarded as Synchronous DFF
            // Build MUX
            id = quteHandler->createNet(V3AsyncMuxName + name, width); if (V3NetUD == id) return false;
            if (!createBvMuxGate(ntk, id, id1, id3, id2)) return false;
            // Build DFF
            id2 = ntk->createNet(width); if (V3NetUD == id2) return false;
            if (!createBvConstGate(ntk, id2, v3Int2Str(width) + "'d0")) return false;
            if (!createV3FFGate(ntk, quteFFVec[i], id, id2)) return false;
         }
         else {  // Retain Asynchronous Behavior in V3 Ntk
            // Build MUX
            id = quteHandler->createNet(V3AsyncDFFName + name, width); if (V3NetUD == id) return false;
            if (!createBvMuxGate(ntk, quteFFVec[i], id, id3, id2)) return false;
            // Build DFF
            id2 = ntk->createNet(width); if (V3NetUD == id2) return false;
            if (!createBvConstGate(ntk, id2, v3Int2Str(width) + "'d0")) return false;
            if (!createV3FFGate(ntk, id, id1, id2)) return false;
         }
      }
      else {  // Synchronous Reset DFF : D, clk
         // Traverse DFF Input
         OutPin = quteGetCellInputPin(cell, 0); assert (OutPin);
         id1 = dfsBuildNtkFromQuteRTL(quteHandler, OutPin, async2sync);
         if (V3NetUD == id1) return false;
         // Build DFF
         width = quteHandler->getNtk()->getNetWidth(quteFFVec[i]);
         id2 = ntk->createNet(width); if (V3NetUD == id2) return false;
         if (!createBvConstGate(ntk, id2, v3Int2Str(width) + "'d0")) return false;
         if (!createV3FFGate(ntk, quteFFVec[i], id1, id2)) return false;
      }
   }
   // Report Multiple Clock Domain if Exists
   if (quteFFClk.size() > 1) {
      Msg(MSG_WAR) << "Multiple Clock Domains Found in RTL Design \"" << quteHandler->getNtkName() << "\" !!" << endl;
      Msg(MSG_IFO) << "Clock Domains (in terms of RTL signal names) are: ";
      for (V3Set<string>::Set::const_iterator it = quteFFClk.begin(); it != quteFFClk.end(); ++it)
         Msg(MSG_IFO) << (it == quteFFClk.begin() ? "" : ", ") << (*it);
      Msg(MSG_IFO) << endl;
      Msg(MSG_WAR) << "V3 Simply Treat All Clock Domains to be the Same One !!" << endl;
   }
   // Set Clock Signal and Remove Clock from Inputs
   if (V3NetUD == ntk->getClock()) {
      if (quteFFClk.size()) ntk->createClock(quteHandler->getNetId(*(quteFFClk.begin())));
   }
   else {
      if (quteFFClk.size() && (ntk->getClock() != quteHandler->getNetId(*(quteFFClk.begin()))))
         Msg(MSG_WAR) << "Multiple Clock Domains Found in RTL Design \"" << quteHandler->getNtkName() << "\" !!" << endl;
   }
   return true;
}

const bool V3QuteRTLOutputHandler(V3NtkInput* const quteHandler, CktModule* const module, const bool& async2sync) {
   assert (quteHandler); assert (module);
   CktCell* cell; CktOutPin* OutPin;
   uint32_t width; string name;
   V3NetId id, id1;
   // Build Output  (Renders the Same Order As Design Created by QuteRTL)
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(quteHandler->getNtk()); assert (ntk);
   for (uint32_t i = 0, j = quteGetDesignIoSize(module, QUTE_PO_CELL); i < j; ++i) {
      // Get Cell Info from QuteRTL
      cell = quteGetDesignIoCell(module, QUTE_PO_CELL, i); assert (cell);
      assert (quteGetCellOutputSize(cell) == 1);
      OutPin = quteGetCellOutputPin(cell, 0); assert (OutPin);
      name = quteGetOutPinName(OutPin); assert (name.size());
      width = getOutPinWidthFromQuteRTL(name, OutPin);
      // Traverse PO Input
      assert (quteGetCellInputSize(cell) == 1);
      OutPin = quteGetCellInputPin(cell, 0); assert (OutPin);
      id1 = dfsBuildNtkFromQuteRTL(quteHandler, OutPin, async2sync); if (V3NetUD == id1) return false;
      // Build Output in V3 Ntk
      id = quteHandler->createNet(name, width); if (V3NetUD == id) return false;
      if ((id != id1) && (!createV3BufGate(ntk, id, id1))) return false;
      if (!createOutput(ntk, id)) return false; quteHandler->resetOutName(ntk->getOutputSize() - 1, name);
   }
   return true;
}

const bool V3QuteRTLClockHandler(V3NtkInput* const quteHandler, const V3NetVec& piList, V3NetVec& inputs) {
   assert (quteHandler); assert (!inputs.size() || (piList.size() == inputs.size()));
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(quteHandler->getNtk()); assert (ntk);
   if (V3NetUD != ntk->getClock()) {
      for (uint32_t i = 0; i < piList.size(); ++i) 
         if (ntk->getClock().id == piList[i].id) { if (inputs.size()) inputs[i] = V3NetUD; continue; }
         else if (!createInput(ntk, piList[i])) return false;
   }
   else for (uint32_t i = 0; i < piList.size(); ++i) if (!createInput(ntk, piList[i])) return false;
   assert (ntk->getInputSize() == (piList.size() - ((V3NetUD != ntk->getClock()) ? 1 : 0))); return true;
}

V3NtkInput* const V3QuteRTLHandler(CktModule* const& module, const bool& async2sync, V3NetVec& inputs, const string& instName) {
   // Create Network Handler
   V3NtkInput* quteHandler = new V3NtkInput(false, (instName.size() ? instName : quteGetModuleName(module)));
   if (!quteHandler) { Msg(MSG_ERR) << "Create RTL Design Failed !!" << endl; return 0; }
   if (!quteHandler->getNtk()) { Msg(MSG_ERR) << "Create BV Network Failed !!" << endl; return 0; }

   V3NetVec piList; piList.clear();
   // 1. Create Input / Inout Nets
   if (!V3QuteRTLInputHandler(quteHandler, module, piList)) { delete quteHandler; return 0; }
   // 2. Create DFF and Traverse its Fanin Cones
   if (!V3QuteRTLFFHandler(quteHandler, module, async2sync, inputs)) { delete quteHandler; return 0; }
   // 3. Create Output
   if (!V3QuteRTLOutputHandler(quteHandler, module, async2sync)) { delete quteHandler; return 0; }
   // 4. Remove Clock Signal from Inputs
   if (!V3QuteRTLClockHandler(quteHandler, piList, inputs)) { delete quteHandler; return 0; }
   // 4. Remove Internal Names (Optional)
   quteHandler->removePrefixNetName(quteIsInternalName);
   
   return quteHandler;
}

// Front-End Framework Integration Main Functions
V3NtkInput* const V3NtkFromQuteRTL(const char* fileName, const bool& isFileList, const bool& toFlatten, const bool& async2sync) {
   // Set QuteRTL Log File
   setQuteLogFile();
   // Parse Verilog by QuteRTL from API and Get a Pointer to CktModule in QuteRTL
   CktModule* const module = quteReadRTL(fileName, isFileList, toFlatten);
   if (!module) {
      Msg(MSG_ERR) << "RTL Parse Failed !!  "
                   << "(See QuteRTL Log File \"" << quteRTL_MsgFile << "\" for Detailed Info)" << endl;
      return 0;
   }
   // Remove QuteRTL Log File
   v3DeleteDir(quteRTL_MsgFile.c_str());
   // Traverse CktModule and Construct Ntk in V3
   V3NetVec inputs; inputs.clear(); return V3QuteRTLHandler(module, async2sync, inputs);
}

#endif

