/****************************************************************************
  FileName     [ quteRTL.cpp ]
  Synopsis     [ Interface Provided by QuteRTL. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyleft(c) 2012 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef QUTERTL_SOURCE
#define QUTERTL_SOURCE

#include "quteRTL.h"
#include "cktModule.h"
#include "cmdDesign.h"
#include "vlpDesign.h"

#include "cktCell.h"
#include "cktArithCell.h"
#include "cktLogicCell.h"
#include "cktIoCell.h"
#include "cktModuleCell.h"
#include "cktModelCell.h"
#include "cktSeqCell.h"

#include "util.h"

// Global Members
ofstream MsgOutput::_allout;        // Set Default Standard Output
string MsgOutput::_allName;
DesignMgr quteMgr("QuteRTL");       // QuteRTL Global Manager
CmdMsgMgr Msg;                      // Message Output Manager
VlpDesign& VLDesign = quteMgr.VLDesign;
CmdParser* cmdMgr = quteMgr.cmdMgr;

// Global Defines
#define PREFIX_CHAR "k"

/* -------------------------------------------------- *\
 * QuteRTL API Message Functions
\* -------------------------------------------------- */
const bool quteSetLogFile(const char* fileName, const bool& append) {
   Msg.setAllOutFile(fileName);
   Msg.startAllOutFile(append);
   Msg.stopAllDefault();
   return Msg.isAllOutFileON();
}

/* -------------------------------------------------- *\
 * QuteRTL API Read Design Functions
\* -------------------------------------------------- */
CktModule* const quteReadRTL(const char* fileName, const bool& isFileList, const bool& toFlatten) {
   // Read RTL by QuteRTL
   VLDesign.resetCkt();
   const bool ok = isFileList ? VLDesign.ParseFileList(fileName) : VLDesign.ParseVerilog(fileName, false);
   if (!ok) return 0; VLDesign.resolveHier();
   // Perform Synthesis on Verilog
   VLDesign.synthesis();
   // Perform Flatten on Verilog if Specified
   if (!toFlatten) return VLDesign.getCktModule(VLDesign.getTopModName());
   VLDesign.completeFlatten();
   return VLDesign.getFltModule();
}

CktModule* const quteReadRTL2(const char* fileName, const bool& isFileList, const bool& toFlatten) {
   /*
   // Read RTL by QuteRTL with VIA
   VLDesign.resetCkt();
   const bool ok = isFileList ? VLDesign.ParseFileList(fileName) : VLDesign.CreateDesignFromVIA(fileName, false);
   if (!ok) return 0; VLDesign.resolveHier();
   // Perform Flatten on Verilog if Specified
   if (!toFlatten) return VLDesign.getCktModule(VLDesign.getTopModName());
   VLDesign.completeFlatten();
   return VLDesign.getFltModule();
   */
   return quteReadRTL(fileName, isFileList, toFlatten);
}

/* -------------------------------------------------- *\
 * QuteRTL API Design Traversal Functions
\* -------------------------------------------------- */
const unsigned quteGetDesignIoSize(CktModule* const m, const QuteRTL_API_CellType& type) {
   assert (m); assert (type < QUTE_TOTAL_CELL);
   assert (QUTE_PI_CELL == type  || QUTE_PO_CELL == type || QUTE_PIO_CELL == type || QUTE_DFF_CELL == type);
   if (QUTE_PI_CELL == type) return m->getIoSize(CKT_IN);
   else if (QUTE_PO_CELL == type) return m->getIoSize(CKT_OUT);
   else if (QUTE_PIO_CELL == type) return m->getIoSize(CKT_IO);
   else return m->getDffSize();
}

CktCell* const quteGetDesignIoCell(CktModule* const m, const QuteRTL_API_CellType& type, const unsigned& i) {
   assert (i < quteGetDesignIoSize(m, type));
   if (QUTE_PI_CELL == type) return m->getIoCell(CKT_IN, i);
   else if (QUTE_PO_CELL == type) return m->getIoCell(CKT_OUT, i);
   else if (QUTE_PIO_CELL == type) return m->getIoCell(CKT_IO, i);
   else return m->getSeqCell(i);
}

const QuteRTL_API_CellType quteGetCellType(CktCell* const cell) {
   assert (cell);
   const CktCellType type = cell->getCellType();
   switch (type) {
      // Module Instance
      case CKT_MODULE_CELL   : return QUTE_MODULE_CELL;
      // Input / Output / Inout
      case CKT_PI_CELL       : return QUTE_PI_CELL;
      case CKT_PO_CELL       : return QUTE_PO_CELL;
      case CKT_PIO_CELL      : return QUTE_PIO_CELL;
      // Flop / Latch
      case CKT_DFF_CELL      : return QUTE_DFF_CELL;
      case CKT_DLAT_CELL     : return QUTE_DLAT_CELL;
      // Logic
      case CKT_BUF_CELL      : return QUTE_BUF_CELL;
      case CKT_INV_CELL      : return QUTE_INV_CELL;
      case CKT_AND_CELL      : return (CKT_RED == dynamic_cast<CktAndCell*>(cell)->getType()) ? 
                                      QUTE_RED_AND_CELL : QUTE_AND_CELL;
      case CKT_OR_CELL       : return (CKT_RED == dynamic_cast<CktOrCell*>(cell)->getType()) ? 
                                      QUTE_RED_OR_CELL : QUTE_OR_CELL;
      case CKT_NAND_CELL     : return (CKT_RED == dynamic_cast<CktNandCell*>(cell)->getType()) ? 
                                      QUTE_RED_NAND_CELL : QUTE_NAND_CELL;
      case CKT_NOR_CELL      : return (CKT_RED == dynamic_cast<CktNorCell*>(cell)->getType()) ? 
                                      QUTE_RED_NOR_CELL : QUTE_NOR_CELL;
      case CKT_XOR_CELL      : return (CKT_RED == dynamic_cast<CktXorCell*>(cell)->getType()) ? 
                                      QUTE_RED_XOR_CELL : QUTE_XOR_CELL;
      case CKT_XNOR_CELL     : return (CKT_RED == dynamic_cast<CktXnorCell*>(cell)->getType()) ? 
                                      QUTE_RED_XNOR_CELL : QUTE_XNOR_CELL;
      case CKT_MUX_CELL      : return QUTE_MUX_CELL;
      // Arithmetic
      case CKT_ADD_CELL      : return QUTE_ADD_CELL;
      case CKT_SUB_CELL      : return QUTE_SUB_CELL;
      case CKT_MULT_CELL     : return QUTE_MULT_CELL;
      case CKT_DIV_CELL      : return QUTE_DIV_CELL;
      case CKT_MODULO_CELL   : return QUTE_MODULO_CELL;
      case CKT_SHL_CELL      : return QUTE_SHL_CELL;
      case CKT_SHR_CELL      : return QUTE_SHR_CELL;
      // Model
      case CKT_CONST_CELL    : return QUTE_CONST_CELL;
      case CKT_SPLIT_CELL    : return QUTE_SPLIT_CELL;
      case CKT_MERGE_CELL    : return QUTE_MERGE_CELL;
      case CKT_MEMORY_CELL   : return QUTE_MEMORY_CELL;
      // Comparator
      case CKT_EQUALITY_CELL : return QUTE_EQUALITY_CELL;
      case CKT_GEQ_CELL      : return QUTE_GEQ_CELL;
      case CKT_GREATER_CELL  : return QUTE_GREATER_CELL;
      case CKT_LEQ_CELL      : return QUTE_LEQ_CELL;
      case CKT_LESS_CELL     : return QUTE_LESS_CELL;
      // Unsupported
      default                : return QUTE_TOTAL_CELL;
   }
}

CktCell* const quteGetCellFromPin(CktOutPin* const p) {
   assert (p); return p->getCell();
}

const unsigned quteGetCellInputSize(CktCell* const cell) {
   assert (cell); const CktCellType type = cell->getCellType();
   if (CKT_DFF_CELL == type) return (dynamic_cast<CktDffCell*>(cell)->getReset()) ? 4 : 2;
   else return cell->getInPinSize();
}

const unsigned quteGetCellOutputSize(CktCell* const cell) {
   assert (cell); const CktCellType type = cell->getCellType();
   if (CKT_SPLIT_CELL == type) return dynamic_cast<CktSplitCell*>(cell)->getOutPinSize();
   else if (CKT_MODULE_CELL == type) return dynamic_cast<CktModuleCell*>(cell)->getOutPinSize();
   else return (cell->getOutPin()) ? 1 : 0;
}

CktOutPin* const quteGetCellInputPin(CktCell* const cell, const unsigned& i) {
   assert (i < quteGetCellInputSize(cell));
   const CktCellType type = cell->getCellType();
   if (CKT_DFF_CELL == type) {
      if (i == 1) return dynamic_cast<CktDffCell*>(cell)->getClk()->getOutPin();
      else if (i == 2) return dynamic_cast<CktDffCell*>(cell)->getReset()->getOutPin();
      else if (i == 3) return dynamic_cast<CktDffCell*>(cell)->getDefault()->getOutPin();
      else return cell->getInPin(0)->getOutPin();
   }
   else return cell->getInPin(i)->getOutPin();
}

CktOutPin* const quteGetCellOutputPin(CktCell* const cell, const unsigned& i) {
   assert (i < quteGetCellOutputSize(cell));
   const CktCellType type = cell->getCellType();
   if (CKT_SPLIT_CELL == type) return dynamic_cast<CktSplitCell*>(cell)->getOutPin(i);
   else if (CKT_MODULE_CELL == type) return dynamic_cast<CktModuleCell*>(cell)->getOutPin(i);
   else return cell->getOutPin();
}

const unsigned quteGetSplitOutPinBegin(CktOutPin* const p) {
   assert (p); assert (p->getCell()); assert (CKT_SPLIT_CELL == p->getCell()->getCellType());
   CktSplitCell* const cell = dynamic_cast<CktSplitCell*>(p->getCell()); assert (cell);
   unsigned i = 0, j = cell->getOutPinSize();
   for (; i < j; ++i) if (p == cell->getOutPin(i)) break; assert (i < j);
   return VLDesign.getBus(cell->getBusId(i))->getBegin();
}

const unsigned quteGetSplitOutPinEnd(CktOutPin* const p) {
   assert (p); assert (p->getCell()); assert (CKT_SPLIT_CELL == p->getCell()->getCellType());
   CktSplitCell* const cell = dynamic_cast<CktSplitCell*>(p->getCell()); assert (cell);
   unsigned i = 0, j = cell->getOutPinSize();
   for (; i < j; ++i) if (p == cell->getOutPin(i)) break; assert (i < j);
   return VLDesign.getBus(cell->getBusId(i))->getEnd();
}

const string quteGetConstCellValue(CktCell* const cell) {
   assert (cell); assert (CKT_CONST_CELL == cell->getCellType());
   string value = "";
   //cout << "valueStr = " << dynamic_cast<CktConstCell*>(cell)->getValueStr() << ", "
   //     << "valuePtr = " << *(dynamic_cast<CktConstCell*>(cell)->getBvValue()) << endl;
   Bv4* bv_value = dynamic_cast<CktConstCell*>(cell)->getBvValue(); assert (bv_value);
   for (unsigned i = 0; i < bv_value->bits(); ++i) {
      if (i < bv_value->bits()) {
         if (_BV4_0 == (*bv_value)[i]) value = "0" + value;
         else if (_BV4_1 == (*bv_value)[i]) value = "1" + value;
         else value = "X" + value;
      }
      else value = "0" + value;
   }
   value = (myInt2Str(bv_value->bits()) + "'b") + value;
   return value;
}

CktModule* const quteGetModuleInstance(CktCell* const cell) {
   assert (cell); assert (dynamic_cast<CktModuleCell*>(cell));
   return dynamic_cast<CktModuleCell*>(cell)->getModulePtr();
}

const string quteGetModuleInstanceName(CktCell* const cell) {
   assert (cell); assert (dynamic_cast<CktModuleCell*>(cell));
   return dynamic_cast<CktModuleCell*>(cell)->getInstName();
}

const string quteGetModuleName(CktModule* const m) {
   assert (m); return m->getModuleName();
}

/* -------------------------------------------------- *\
 * CktCell / CktOutPin Interface Functions
\* -------------------------------------------------- */
const unsigned quteGetPinBegin(CktOutPin* const p) {
   assert (p); return p->getBusBegin();
}

const unsigned quteGetPinEnd(CktOutPin* const p ) {
   assert (p); return p->getBusEnd();
}

const unsigned quteGetPinWidth(CktOutPin* const p) {
   assert (p); return p->getBusWidth();
}

const string quteGetCellName(CktCell* const cell) {
   assert (cell); const CktCellType type = cell->getCellType();
   if (CKT_SPLIT_CELL == type) return dynamic_cast<CktSplitCell*>(cell)->getOutPin(0)->getName();
   else if (CKT_MODULE_CELL == type) return dynamic_cast<CktModuleCell*>(cell)->getInstName();
   else return cell->getOutPin()->getName();
}

const string quteGetOutPinName(CktOutPin* const p) {
   assert (p); return p->getCell()->getName(p);
}

const string quteGetOutPinExpr(CktOutPin* const p) {
   assert (p); return p->getCell()->getExpr(p);
}

/* -------------------------------------------------- *\
 * Query Functions
\* -------------------------------------------------- */
const bool quteIsInternalName(const string& name) {
   return string::npos != name.find(PREFIX_CHAR);
   //return !strncmp(name.c_str(), PREFIX_CHAR, strlen(PREFIX_CHAR));
}

#endif

