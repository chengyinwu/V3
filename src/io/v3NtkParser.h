/****************************************************************************
  FileName     [ v3NtkParser.h ]
  PackageName  [ v3/src/io ]
  Synopsis     [ Generic Parsers for V3 Ntk Construction. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_PARSER_H
#define V3_NTK_PARSER_H

#include "v3NtkInput.h"

// Front-End Framework Integration Main Functions
V3NtkInput* const V3NtkFromQuteRTL(const char*, const bool&, const bool&, const bool& = false);

// Generic Parser Main Functions
V3NtkInput* const V3BtorParser(const char*, const bool& = true);
V3NtkInput* const V3AigParser(const char*, const bool& = true);
V3NtkInput* const V3RTLParser(const char*);

#endif
