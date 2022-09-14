/****************************************************************************
  FileName     [ v3TransCmd.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Ntk Transformation Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_TRANS_CMD_H
#define V3_TRANS_CMD_H

#include "v3CmdMgr.h"

V3_COMMAND(V3ExpandNtkCmd,    CMD_TYPE_SYNTHESIS);
V3_COMMAND(V3BlastNtkCmd,     CMD_TYPE_SYNTHESIS);
V3_COMMAND(V3MiterNtkCmd,     CMD_TYPE_SYNTHESIS);
V3_COMMAND(V3FlattenNtkCmd,   CMD_TYPE_SYNTHESIS);

#endif

