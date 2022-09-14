/****************************************************************************
  FileName     [ v3DfxCmd.h ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Debugging and Fix Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_CMD_H
#define V3_DFX_CMD_H

#include "v3CmdMgr.h"

// Counterexample Simplification Commands
V3_COMMAND(V3TraceSimplifyCmd,   CMD_TYPE_DEBUGFIX);
// Counterexample Manipulation Commands
V3_COMMAND(V3OptTraceCmd,        CMD_TYPE_DEBUGFIX);

#endif

