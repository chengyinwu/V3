/****************************************************************************
  FileName     [ quteRTL.h ]
  Synopsis     [ Interface Provided by QuteRTL. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyleft(c) 2012 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef QUTERTL_HEADER
#define QUTERTL_HEADER

#include <iostream>

using namespace std;

/* -------------------------------------------------- *\
 * Forward Declarations
\* -------------------------------------------------- */
class CktCell;
class CktModule;
class CktOutPin;

/* -------------------------------------------------- *\
 * QuteRTL API Cell Types
\* -------------------------------------------------- */
enum QuteRTL_API_CellType
{
   // Module Instance
   QUTE_MODULE_CELL,
   // Input / Output / Inout
   QUTE_PI_CELL,
   QUTE_PO_CELL,
   QUTE_PIO_CELL,
   // Flop / Latch
   QUTE_DFF_CELL,
   QUTE_DLAT_CELL,
   // Reduced
   QUTE_RED_AND_CELL,
   QUTE_RED_OR_CELL,
   QUTE_RED_NAND_CELL,
   QUTE_RED_NOR_CELL,
   QUTE_RED_XOR_CELL,
   QUTE_RED_XNOR_CELL,
   // Logic
   QUTE_BUF_CELL,
   QUTE_INV_CELL,
   QUTE_AND_CELL,
   QUTE_OR_CELL,
   QUTE_NAND_CELL,
   QUTE_NOR_CELL,
   QUTE_XOR_CELL,
   QUTE_XNOR_CELL,
   QUTE_MUX_CELL,
   // Arithmetic
   QUTE_ADD_CELL,
   QUTE_SUB_CELL,
   QUTE_MULT_CELL,
   QUTE_DIV_CELL,
   QUTE_MODULO_CELL,
   QUTE_SHL_CELL,
   QUTE_SHR_CELL,
   // Model
   QUTE_CONST_CELL,
   QUTE_SPLIT_CELL,
   QUTE_MERGE_CELL,
   QUTE_MEMORY_CELL,
   // Comparator
   QUTE_EQUALITY_CELL,
   QUTE_GEQ_CELL,
   QUTE_GREATER_CELL,
   QUTE_LEQ_CELL,
   QUTE_LESS_CELL,
   // Total
   QUTE_TOTAL_CELL
};

/* -------------------------------------------------- *\
 * QuteRTL API Message Functions
\* -------------------------------------------------- */
const bool quteSetLogFile(const char*, const bool& = false);

/* -------------------------------------------------- *\
 * QuteRTL API Read Design Functions
\* -------------------------------------------------- */
CktModule* const quteReadRTL(const char*, const bool&, const bool&);
CktModule* const quteReadRTL2(const char*, const bool&, const bool&);

/* -------------------------------------------------- *\
 * QuteRTL API Design Traversal Functions
\* -------------------------------------------------- */
const unsigned quteGetDesignIoSize(CktModule* const, const QuteRTL_API_CellType&);
CktCell* const quteGetDesignIoCell(CktModule* const, const QuteRTL_API_CellType&, const unsigned&);

const QuteRTL_API_CellType quteGetCellType(CktCell* const);
CktCell* const quteGetCellFromPin(CktOutPin* const);

const unsigned quteGetCellInputSize(CktCell* const);
const unsigned quteGetCellOutputSize(CktCell* const);
CktOutPin* const quteGetCellInputPin(CktCell* const, const unsigned&);
CktOutPin* const quteGetCellOutputPin(CktCell* const, const unsigned&);

const unsigned quteGetSplitOutPinBegin(CktOutPin* const);
const unsigned quteGetSplitOutPinEnd(CktOutPin* const);
const string quteGetConstCellValue(CktCell* const);

CktModule* const quteGetModuleInstance(CktCell* const);
const string quteGetModuleInstanceName(CktCell* const);

const string quteGetModuleName(CktModule* const);

/* -------------------------------------------------- *\
 * CktCell / CktOutPin Interface Functions
\* -------------------------------------------------- */
const unsigned quteGetPinWidth(CktOutPin* const);
const unsigned quteGetPinBegin(CktOutPin* const);
const unsigned quteGetPinEnd(CktOutPin* const);
const string quteGetCellName(CktCell* const);
const string quteGetOutPinName(CktOutPin* const);
const string quteGetOutPinExpr(CktOutPin* const);

/* -------------------------------------------------- *\
 * Query Functions
\* -------------------------------------------------- */
const bool quteIsInternalName(const string&);

#endif

