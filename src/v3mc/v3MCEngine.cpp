/****************************************************************************
  FileName     [ v3MCEngine.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Verification Engines. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_ENGINE_C
#define V3_MC_ENGINE_C

#include "v3Msg.h"
#include "v3MCMain.h"
#include "v3VrfBMC.h"
#include "v3VrfSEC.h"
#include "v3VrfSIM.h"
#include "v3VrfUMC.h"
#include "v3VrfBase.h"
#include "v3VrfCITP.h"
#include "v3VrfFITP.h"
#include "v3VrfIPDR.h"
#include "v3VrfMPDR.h"
#include "v3VrfKLive.h"
#include "v3MCEngine.h"
#include "v3StgExtract.h"

// V3 Verification Engine Configuration Functions
V3VrfBase* getChecker(const V3MCEngineType& type, V3NtkHandler* const handler, const uint32_t& ntkSize) {
   assert (type < V3MC_TOTAL); assert (handler); assert (handler->getNtk());
   const double ratio = (ntkSize <= 100000) ? 1.00 : (1.00 / pow(2, log10(((double)ntkSize) / 100000.00)));
   // BMC / UMC
   if (V3MC_BMC == type) {
      V3VrfBMC* checker = new V3VrfBMC(handler); assert (checker);
      checker->setPreDepth(ceil(20.00 * ratio));
      checker->setIncDepth(ceil(10.00 * ratio));
      checker->setSolver(V3_SVR_MINISAT);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_UBC == type) {
      V3VrfUMC* checker = new V3VrfUMC(handler); assert (checker);
      checker->setPreDepth(ceil(30.00 * ratio));
      checker->setIncDepth(ceil(5.00 * ratio));
      checker->setFireOnly(true);
      checker->setUniqueness(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_UMC == type) {
      V3VrfUMC* checker = new V3VrfUMC(handler); assert (checker);
      checker->setPreDepth(ceil(30.00 * ratio));
      checker->setIncDepth(ceil(5.00 * ratio));
      checker->setUniqueness(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_IND == type) {
      V3VrfUMC* checker = new V3VrfUMC(handler); assert (checker);
      checker->setIncDepth(ceil(20.00 * ratio));
      checker->setProveOnly(true);
      checker->setUniqueness(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   // PDR
   else if (V3MC_MPDR == type) {
      V3VrfMPDR* checker = new V3VrfMPDR(handler); assert (checker);
      checker->setRecycle(300);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_IPDR == type) {
      V3VrfIPDR* checker = new V3VrfIPDR(handler); assert (checker);
      checker->setRecycle(300);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   // ITP
   else if (V3MC_CITP == type) {
      V3VrfCITP* checker = new V3VrfCITP(handler); assert (checker);
      checker->setNoIncBySharedBound(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_CITP_I == type) {
      V3VrfCITP* checker = new V3VrfCITP(handler); assert (checker);
      checker->setNoIncBySharedBound(true);
      checker->setIncrementDepth(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_CITP_F_I == type) {
      V3VrfCITP* checker = new V3VrfCITP(handler); assert (checker);
      checker->setNoIncBySharedBound(true);
      checker->setForceUnreachable(true);
      checker->setIncrementDepth(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_CITP_REC == type) {
      V3VrfCITP* checker = new V3VrfCITP(handler); assert (checker);
      checker->setRecycleInterpolants(true);
      checker->setNoIncBySharedBound(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_FITP == type) {
      V3VrfFITP* checker = new V3VrfFITP(handler); assert (checker);
      checker->setNoIncBySharedBound(true);
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   // PROVE
   else if (V3MC_SEC_BMC == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfUMC* secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker);
      secChecker->setFireOnly(true);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_UMC == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfUMC* secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_IPDR == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfIPDR* secChecker = new V3VrfIPDR(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_MPDR == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfMPDR* secChecker = new V3VrfMPDR(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_SYN_BMC == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfUMC* secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker);
      secChecker->setFireOnly(true);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      checker->setSynthesisMode(true);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_SYN_UMC == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfUMC* secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      checker->setSynthesisMode(true);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_SYN_IPDR == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfIPDR* secChecker = new V3VrfIPDR(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      checker->setSynthesisMode(true);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   else if (V3MC_SEC_SYN_MPDR == type) {
      V3VrfSEC* checker = new V3VrfSEC(handler); assert (checker);
      V3VrfMPDR* secChecker = new V3VrfMPDR(checker->getSECHandler()); assert (secChecker);
      checker->setSolver(V3_SVR_MINISAT);
      checker->setChecker(secChecker);
      checker->setSynthesisMode(true);
      if (ntkSize > 10000) {
         if (ntkSize < 100000) checker->setAssumeSECMiter();
         else checker->setAssumeCECMiter();
      }
      checker->setMaxDepth(ceil(64.00 * ratio));
      return checker;
   }
   // K-LIVENESS
   else if (V3MC_KLIVE_MPDR == type) {
      V3VrfKLive* checker = new V3VrfKLive(handler); assert (checker);
      V3VrfMPDR* klChecker = new V3VrfMPDR(checker->getKLHandler()); assert (klChecker);
      klChecker->setRecycle(300);
      checker->setChecker(klChecker);
      checker->setIncDepth(ceil(10.00 * ratio));
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_KLIVE_IPDR == type) {
      V3VrfKLive* checker = new V3VrfKLive(handler); assert (checker);
      V3VrfIPDR* klChecker = new V3VrfIPDR(checker->getKLHandler()); assert (klChecker);
      klChecker->setRecycle(300);
      checker->setChecker(klChecker);
      checker->setIncDepth(ceil(10.00 * ratio));
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_KLIVE_CITP == type) {
      V3VrfKLive* checker = new V3VrfKLive(handler); assert (checker);
      V3VrfCITP* klChecker = new V3VrfCITP(checker->getKLHandler()); assert (klChecker);
      klChecker->setIncrementDepth(true);
      checker->setChecker(klChecker);
      checker->setIncDepth(ceil(10.00 * ratio));
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_KLIVE_CITP_REC == type) {
      V3VrfKLive* checker = new V3VrfKLive(handler); assert (checker);
      V3VrfCITP* klChecker = new V3VrfCITP(checker->getKLHandler()); assert (klChecker);
      klChecker->setRecycleInterpolants(true);
      checker->setChecker(klChecker);
      checker->setIncDepth(ceil(4.00 * ratio));
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   else if (V3MC_KLIVE_FITP == type) {
      V3VrfKLive* checker = new V3VrfKLive(handler); assert (checker);
      V3VrfFITP* klChecker = new V3VrfFITP(checker->getKLHandler()); assert (klChecker);
      klChecker->setIncrementDepth(true);
      checker->setChecker(klChecker);
      checker->setIncDepth(ceil(4.00 * ratio));
      checker->setMaxDepth(V3NtkUD);
      return checker;
   }
   // SIM
   else if (V3MC_SIM == type) {
      V3VrfSIM* checker = new V3VrfSIM(handler); assert (checker);
      checker->setMaxDepth(ceil(128 * ratio));
      return checker;
   }
   // IDLE or TOTAL
   else return 0;  // V3MC_IDLE or V3MC_TOTAL
}

void getDefaultCheckerList(const V3MCMainType& type, const uint32_t& pSize, V3MCEngineList& checkers) {
   checkers.clear();
   switch (type) {
      case V3MC_SAFE : 
         checkers.push_back(V3MC_IPDR);
         checkers.push_back(V3MC_UMC);
         if (pSize == 1) checkers.push_back(V3MC_BMC);
         checkers.push_back(V3MC_CITP_REC);
         checkers.push_back(V3MC_MPDR);
         if (pSize > 1) checkers.push_back(V3MC_SEC_BMC);
         checkers.push_back(V3MC_CITP);
         checkers.push_back(V3MC_SIM);
         checkers.push_back(V3MC_FITP);
         if (pSize == 1) checkers.push_back(V3MC_CITP_F_I);
         if (pSize > 1) checkers.push_back(V3MC_SEC_MPDR);
         break;
      case V3MC_LIVE :
         checkers.push_back(V3MC_KLIVE_IPDR);
         checkers.push_back(V3MC_KLIVE_CITP);
         checkers.push_back(V3MC_KLIVE_MPDR);
         checkers.push_back(V3MC_KLIVE_CITP_REC);
         if (pSize > 1) checkers.push_back(V3MC_SEC_SYN_BMC);
         checkers.push_back(V3MC_KLIVE_FITP);
         break;
      case V3MC_L2S  : 
         checkers.push_back(V3MC_IPDR);
         checkers.push_back(V3MC_UBC);
         if (pSize == 1) checkers.push_back(V3MC_BMC);
         checkers.push_back(V3MC_MPDR);
         checkers.push_back(V3MC_CITP_REC);
         if (pSize > 1) checkers.push_back(V3MC_SEC_BMC);
         checkers.push_back(V3MC_CITP);
         checkers.push_back(V3MC_SIM);
         checkers.push_back(V3MC_FITP);
         if (pSize == 1) checkers.push_back(V3MC_CITP_F_I);
         if (pSize > 1) checkers.push_back(V3MC_SEC_MPDR);
         break;
      default : break;
   }
}

#endif

