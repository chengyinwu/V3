/****************************************************************************
  FileName     [ v3VrfCmd.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Verification Commands. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_CMD_H
#define V3_VRF_CMD_H

#include "v3CmdMgr.h"

// Verification Verbosity Manipulation Commands
V3_COMMAND(V3SetReportCmd,       CMD_TYPE_VERIFY);
V3_COMMAND(V3PrintReportCmd,     CMD_TYPE_VERIFY);
V3_COMMAND(V3SetSolverCmd,       CMD_TYPE_VERIFY);
V3_COMMAND(V3PrintSolverCmd,     CMD_TYPE_VERIFY);
// Verification Property Setting Commands
V3_COMMAND(V3SetSafetyCmd,       CMD_TYPE_VERIFY);
V3_COMMAND(V3SetLivenessCmd,     CMD_TYPE_VERIFY);
V3_COMMAND(V3ElaboratePrptyCmd,  CMD_TYPE_VERIFY);
// Verification Main Commands
V3_COMMAND(V3SIMVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3BMCVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3UMCVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3PDRVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3ITPVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3SECVrfCmd,          CMD_TYPE_VERIFY);
V3_COMMAND(V3KLiveVrfCmd,        CMD_TYPE_VERIFY);
// Verification Result Reporting Commands
V3_COMMAND(V3CheckResultCmd,     CMD_TYPE_VERIFY);
V3_COMMAND(V3WriteResultCmd,     CMD_TYPE_VERIFY);
V3_COMMAND(V3PlotResultCmd,      CMD_TYPE_VERIFY);

#endif

