/****************************************************************************
  FileName     [ v3ExtUtil.h ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Generic Utility Functions for V3 Property Manipulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_EXT_UTIL_H
#define V3_EXT_UTIL_H

#include "v3Property.h"

/* GENERAL V3 ELABORATION FUNCTIONS : Implemented in v3ExtElaborate.cpp */
// General Elaboration Functions for V3 Ntk
const V3NetId elaborateAigFormula(V3Formula* const, const uint32_t&, V3PortableType&);
const V3NetId elaborateBvFormula(V3Formula* const, const uint32_t&, V3PortableType&);

/* GENERAL V3 EXTENDED I/O FUNCTIONS : Implemented in v3ExtIO.cpp */
// Counterexample Trace Recording and Recovering Functions
V3CexTrace* const V3CexTraceParser(const V3NtkHandler* const, const string&);
void V3CexTraceWriter(const V3NtkHandler* const, const V3CexTrace* const, const string&);
void V3CexTraceVisualizer(const V3NtkHandler* const, const V3CexTrace* const, const string&);

#endif

