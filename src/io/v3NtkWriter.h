/****************************************************************************
  FileName     [ v3NtkWriter.h ]
  PackageName  [ v3/src/io ]
  Synopsis     [ Generic Writers for V3 Ntk Output. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_WRITER_H
#define V3_NTK_WRITER_H

#include "v3NtkUtil.h"
#include "v3NtkHandler.h"

#include <fstream>

// Defines
#define V3RTLName(n)          ((n.cp ? "~" : "") + rtlName[n.id])
#define V3RTLNameBase(h, n)   (getVerilogName(h, n))
#define V3RTLNameOrId(h, n)   (getVerilogName(h, h->getNetNameOrFormedWithId(n)))
#define V3RTLNameById(h, n)   (getVerilogName(h, h->applyAuxNetNamePrefix(v3Int2Str(n.id))))

// Writer Helper Functions
void writeV3GeneralHeader(const string&, ofstream&, const string&);
const string getVerilogName(const V3NtkHandler* const, const string&);

// Generic Network Plot Functions
void V3PlotNtkByLevel(const V3NtkHandler* const, const char*, const uint32_t&, const V3NetVec&, const bool&);
void V3PlotNtkByDepth(const V3NtkHandler* const, const char*, const uint32_t&, const V3NetVec&, const bool&);

// Generic Writer Main Functions
void V3BtorWriter(const V3NtkHandler* const, const char*, const bool& = false);
void V3AigWriter(const V3NtkHandler* const, const char*, const bool& = false);
void V3RTLWriter(const V3NtkHandler* const, const char*, const bool& = true, const bool& = false);

// Specific Writer Main Functions
void splitAigFromProperties(const string&, const bool& = false);

#endif

