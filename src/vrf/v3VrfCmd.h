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

// Verification Property Setting Commands
V3_COMMAND(V3SetSafetyCmd,       CMD_TYPE_VERIFY);
// Verification Main Commands
V3_COMMAND(V3UMCVrfCmd,          CMD_TYPE_VERIFY);

#endif

