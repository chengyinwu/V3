/****************************************************************************
  FileName     [ v3NtkCmd.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Ntk Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_CMD_H
#define V3_NTK_CMD_H

#include "v3CmdMgr.h"

// Network Input Commands
V3_COMMAND(V3ReadRTLCmd,         CMD_TYPE_IO);
// Network Output Commands
V3_COMMAND(V3WriteAIGCmd,        CMD_TYPE_IO);

#endif

