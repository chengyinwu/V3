/****************************************************************************
  FileName     [ v3ExtType.h ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ Ext Types in V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_EXT_TYPE_H
#define V3_EXT_TYPE_H

#include "v3StrUtil.h"

// V3 Extensive Naming Prefix
const string V3PropertyPrefix = "P_";
const string V3SafetyPrefix   = "v3_Safety_";
const string V3LivenessPrefix = "v3_Liveness_";
const string V3L2SPrefix      = "v3_L2S_";
const string V3L2SGeneralID   = V3L2SPrefix + v3Int2Str(time(NULL));
const string V3L2SAssertName  = V3L2SPrefix + "Assertion_";
const string V3L2SShadowName  = V3L2SPrefix + "Shadow_Latch_";
const string V3L2SPCheckName  = V3L2SPrefix + "F_not_P_Latch_";
const string V3L2SLoopedName  = V3L2SGeneralID + "_Looped";
const string V3L2SOracleName  = V3L2SGeneralID + "_Loop_Oracle";
const string V3L2SInLoopName  = V3L2SGeneralID + "_InLoop_Latch";
const string V3L2SLatchONName = V3L2SGeneralID + "_Shadow_Latch_ON";

#endif

