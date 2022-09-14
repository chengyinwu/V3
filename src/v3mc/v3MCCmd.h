/****************************************************************************
  FileName     [ v3MCCmd.h ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ Model Checking Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_CMD_H
#define V3_MC_CMD_H

#include "v3CmdMgr.h"

// Model Checking Property Commands
V3_COMMAND(V3MCReadPropertyCmd,     CMD_TYPE_MODELCHKING);
V3_COMMAND(V3MCWritePropertyCmd,    CMD_TYPE_MODELCHKING);
// Model Checking Verification Commands
V3_COMMAND(V3MCRunCmd,              CMD_TYPE_MODELCHKING);

#endif

