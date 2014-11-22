/****************************************************************************
  FileName     [ v3MCEngine.h ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Verification Engines. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_ENGINE_H
#define V3_MC_ENGINE_H

#include "v3StrUtil.h"

// An Enumeration of Verification Engines with Configurations
// NOTE: Only Exploiting Checkers are In the List
enum V3MCEngineType
{
   // IDLE
   V3MC_IDLE = 0,
   // BMC / UMC
   V3MC_BMC,
   V3MC_UBC,
   V3MC_UMC,
   V3MC_IND,
   // PDR
   V3MC_MPDR,
   V3MC_IPDR,
   // ITP
   V3MC_CITP,
   V3MC_CITP_I,
   V3MC_CITP_F_I,
   V3MC_CITP_REC,
   V3MC_FITP,
   // PROVE
   V3MC_SEC_BMC,
   V3MC_SEC_UMC,
   V3MC_SEC_IPDR,
   V3MC_SEC_MPDR,
   V3MC_SEC_SYN_BMC,
   V3MC_SEC_SYN_UMC,
   V3MC_SEC_SYN_IPDR,
   V3MC_SEC_SYN_MPDR,
   // K-LIVENESS
   V3MC_KLIVE_MPDR,
   V3MC_KLIVE_IPDR,
   V3MC_KLIVE_CITP,
   V3MC_KLIVE_CITP_REC,
   V3MC_KLIVE_FITP,
   // SIM
   V3MC_SIM,
   // TOTAL
   V3MC_TOTAL
};

const string V3MCEngineTypeStr[] = 
{
   // IDLE
   "IDLE",
   // BMC / UMC
   "BMC",
   "UBC",
   "UMC",
   "IND",
   // PDR
   "MPDR",
   "IPDR",
   // ITP
   "CITP",
   "CITP_I",
   "CITP_F_I",
   "CITP_REC",
   "FITP",
   // PROVE
   "SEC_BMC",
   "SEC_UMC",
   "SEC_IPDR",
   "SEC_MPDR",
   "SEC_SYN_BMC",
   "SEC_SYN_UMC",
   "SEC_SYN_IPDR",
   "SEC_SYN_MPDR",
   // K-LIVENESS
   "KLIVE_MPDR",
   "KLIVE_IPDR",
   "KLIVE_CITP",
   "KLIVE_CITP_REC",
   "KLIVE_FITP",
   // SIM
   "SIM",
   // TOTAL
   "TOTAL"
};

#endif

