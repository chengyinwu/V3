/****************************************************************************
  FileName     [ v3CmdComm.h ]
  PackageName  [ v3/src/cmd ]
  Synopsis     [ Global Commands Implementation ]
  Author       [ Chung-Yang (Ric) Huang, Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_CMD_COMM_H
#define V3_CMD_COMM_H

#include "v3CmdMgr.h"

V3_COMMAND(V3HelpCmd,      CMD_TYPE_COMMON);
V3_COMMAND(V3QuitCmd,      CMD_TYPE_COMMON);
V3_COMMAND(V3HistoryCmd,   CMD_TYPE_COMMON);
V3_COMMAND(V3DofileCmd,    CMD_TYPE_COMMON);
V3_COMMAND(V3UsageCmd,     CMD_TYPE_COMMON);
V3_COMMAND(V3LogFileCmd,   CMD_TYPE_COMMON);

#endif

