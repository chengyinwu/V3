/****************************************************************************
  FileName     [ v3StgCmd.h ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ State-Transition-Graph Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_CMD_H
#define V3_STG_CMD_H

#include "v3CmdMgr.h"

// FSM Extraction Commands
V3_COMMAND(V3ElaborateFSMCmd,    CMD_TYPE_EXTRACT);
V3_COMMAND(V3ExtractFSMCmd,      CMD_TYPE_EXTRACT);
V3_COMMAND(V3WriteFSMCmd,        CMD_TYPE_EXTRACT);
V3_COMMAND(V3PlotFSMCmd,         CMD_TYPE_EXTRACT);

#endif

