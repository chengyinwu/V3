/****************************************************************************
  FileName     [ v3VrfCmd.cpp ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Verification Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_CMD_C
#define V3_VRF_CMD_C

#include "v3Msg.h"
#include "v3VrfCmd.h"
#include "v3VrfUMC.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3VrfResult.h"
#include "v3NtkWriter.h"
#include "v3StgExtract.h"
#include "v3NtkElaborate.h"

#include <iomanip>

bool initVrfCmd() {
   // Reset Verification Settings
   V3VrfBase::resetReportSettings(); V3VrfBase::resetSolverSettings();
   return (
         // Verification Property Setting Commands
         v3CmdMgr->regCmd("SET SAFEty",         3, 4, new V3SetSafetyCmd     ) &&
         // Verification Main Commands
         v3CmdMgr->regCmd("VERify UMC",         3, 3, new V3UMCVrfCmd        ) 
   );
}

//----------------------------------------------------------------------
// SET SAFEty [-Name <(string propertyName)>] <(unsigned outputIndex)>
//            [-INVAriant <(string invName)*>]
//            [-INVConstraint <(string constrName)*>]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetSafetyCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool name = false, nameON = false;
   bool invA = false, invAON = false;
   bool invC = false, invCON = false;
   string propertyName = "";
   uint32_t outputIndex = V3NtkUD;
   V3Vec<string>::Vec invNameList[2];  // [0]: INVA, [1]: INVC
   invNameList[0].clear(); invNameList[1].clear();

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Name", token, 2) == 0) {
         if (name) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else name = nameON = true; invAON = invCON = false;
      }
      else if (v3StrNCmp("-INVAriant", token, 5) == 0) {
         if (invA) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
         else invA = invAON = true; invCON = false;
      }
      else if (v3StrNCmp("-INVConstraint", token, 5) == 0) {
         if (invC) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
         else invC = invCON = true; invAON = false;
      }
      else if (nameON) { propertyName = token; nameON = false; }
      else if (invAON) invNameList[0].push_back(token);
      else if (invCON) invNameList[1].push_back(token);
      else if (V3NtkUD == outputIndex) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         outputIndex = (uint32_t)temp;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
   if (invAON && !invNameList[0].size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string invName)");
   if (invCON && !invNameList[1].size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string constrName)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      // Create LTL Formula
      V3LTLFormula* f = 0;
      if (!propertyName.size()) propertyName = handler->getAuxPropertyName();
      if (handler->existProperty(propertyName))
         Msg(MSG_ERR) << "Property \"" << propertyName << "\" Already Exists !!" << endl;
      else {
         assert (propertyName.size()); assert (!handler->existProperty(propertyName));
         if (outputIndex < handler->getNtk()->getOutputSize())
            f = new V3LTLFormula(handler, ~(handler->getNtk()->getOutput(outputIndex)), true, propertyName);
         else Msg(MSG_ERR) << "Output with Index " << outputIndex << " does NOT Exist in Current Ntk !!" << endl;
      }
      // Add to property data base
      if (f && f->isValid()) {
         V3Property* const p = new V3Property(f); assert (p); uint32_t k;
         // Set Invariants and Constraints
         for (k = 0; k < 2; ++k) {
            string invalidInvName = "", invalidOutputs = "";
            for (uint32_t i = 0; i < invNameList[k].size(); ++i) {
               // Tokenize Constraint Expression
               uint32_t start = V3NtkUD, end = V3NtkUD;
               const string constrName = v3Str2BVExpr(invNameList[k][i], start, end);
               if ("PO" == constrName) {  // Synthesized Constraint (i.e. Primary Outputs)
                  if (V3NtkUD == start && V3NtkUD == end) start = 0;
                  if (V3NtkUD == start) start = handler->getNtk()->getOutputSize() - 1;
                  if (V3NtkUD == end) end = handler->getNtk()->getOutputSize() - 1;
                  if (handler->getNtk()->getOutputSize() <= start)
                     invalidInvName += (invalidInvName.size() ? ", \"PO[" : "\"PO[") + v3Int2Str(start) + "]\"";
                  else if (handler->getNtk()->getOutputSize() <= end)
                     invalidInvName += (invalidInvName.size() ? ", \"PO[" : "\"PO[") + 
                                       v3Int2Str(handler->getNtk()->getOutputSize()) + "]\"";
                  else if (start <= end) {  // Fwd-order Constraint Construction
                     for (uint32_t j = start; j <= end; ++j)
                        if (1 == handler->getNtk()->getNetWidth(handler->getNtk()->getOutput(j))) continue;
                        else invalidOutputs += (invalidOutputs.size() ? ", \"PO[" : "\"PO[") + v3Int2Str(j) + "]\"";
                  }
                  else {  // Rev-order Constraint Construction
                     uint32_t j = 1 + start; assert (j > start);
                     while (j-- > end) {
                        if (1 == handler->getNtk()->getNetWidth(handler->getNtk()->getOutput(j))) continue;
                        else invalidOutputs += (invalidOutputs.size() ? ", \"PO[" : "\"PO[") + v3Int2Str(j) + "]\"";
                     }
                  }
                  if (!invalidOutputs.size() && !invalidInvName.size()) {
                     if (k == 0) p->setInvariant(handler, start, end);  // INVA
                     else p->setInvConstr(handler, start, end);  // INVC
                  }
               }
               else if (handler->existFSM(constrName)) {  // FSM Constraints
                  V3FSMExtract* const fsmExtract = handler->getFSM(constrName);
                  assert (fsmExtract); assert (fsmExtract->getFSMListSize());
                  if (V3NtkUD == start && V3NtkUD == end) start = 0;
                  if (V3NtkUD == start) start = fsmExtract->getFSMListSize() - 1;
                  if (V3NtkUD == end) end = fsmExtract->getFSMListSize()- 1;
                  if (fsmExtract->getFSMListSize() <= start)
                     invalidInvName += (invalidInvName.size() ? ", \"" : "\"") + constrName + "\[" + 
                                       v3Int2Str(start) + "]\"";
                  else if (fsmExtract->getFSMListSize() <= end)
                     invalidInvName += (invalidInvName.size() ? ", \"" : "\"") + constrName + "\[" + 
                                       v3Int2Str(fsmExtract->getFSMListSize()) + "]\"";
                  if (!invalidOutputs.size() && !invalidInvName.size()) {
                     if (start <= end) {  // Fwd-order Constraint Construction
                        for (uint32_t x = start; x <= end; ++x)
                           if (k == 0) p->setInvariant(handler, fsmExtract->getFSM(x));  // INVA
                           else p->setInvConstr(handler, fsmExtract->getFSM(x));  // INVC
                     }
                     else {  // Rev-order Constraint Construction
                        uint32_t x = 1 + start; assert (x > start);
                        while (x-- > end) {
                           if (k == 0) p->setInvariant(handler, fsmExtract->getFSM(x));  // INVA
                           else p->setInvConstr(handler, fsmExtract->getFSM(x));  // INVC
                        }
                     }
                  }
               }
               else {
                  invalidInvName += (invalidInvName.size() ? ", \"" : "\"") + 
                                    (constrName.size() ? constrName : invNameList[k][i]) + "\"";
               }
            }
            // Report Invalid Inputs
            if (invalidInvName.size())
               Msg(MSG_ERR) << (k == 0 ? "Invariants " : "Invariant Constraints ") << invalidInvName << " Not Found !!"   << endl;
            if (invalidOutputs.size())
               Msg(MSG_ERR) << (k == 0 ? "Invariants " : "Invariant Constraints ") << invalidOutputs << " Not Boolean !!" << endl;
            if (invalidInvName.size() || invalidOutputs.size()) break;
         }
         // Set Property if NO Invalid Inputs
         if (k == 2) {
            handler->setProperty(p);
            Msg(MSG_IFO) << "Safety Property " << propertyName << " has been set successfully !!" << endl;
         }
         else delete p;
      }
      else {
         Msg(MSG_ERR) << "Failed to Create Safety Property " << propertyName << " !!" << endl;
         if (f) delete f;
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3SetSafetyCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SET SAFEty [-Name <(string propertyName)>] <(unsigned outputIndex)>" << endl;
   Msg(MSG_IFO) << "                  [-INVAriant <(string invName)*>]" << endl;
   Msg(MSG_IFO) << "                  [-INVConstraint <(string constrName)*>]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be set." << endl;
      Msg(MSG_IFO) << "       (unsigned outputIndex): The index of a primary output serving as a bad signal." << endl;
      Msg(MSG_IFO) << "       (string invName)*     : List of names of invariants." << endl;
      Msg(MSG_IFO) << "       (string constrName)*  : List of names of (either invariant or fairness) constraints." << endl;
      Msg(MSG_IFO) << "       -Name                 : Indicate the following token is the name of a property." << endl;
      Msg(MSG_IFO) << "       -INVAriant            : Indicate the starting of a list of invariants." << endl;
      Msg(MSG_IFO) << "       -INVConstraint        : Indicate the starting of a list of invariant constraints." << endl;
   }
}

void
V3SetSafetyCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SET SAFEty: " << "Set Safety Properties on Current Network." << endl;
}

//----------------------------------------------------------------------
// VERify UMC [(string propertyName)]
//            [-Max-depth (unsigned MaxDepth)]
//            [-Pre-depth (unsigned PreDepth)]
//            [-Inc-depth (unsigned IncDepth)]
//            [-NOProve | -NOFire]
//            [-Uniqueness] [-Dynamic]
//----------------------------------------------------------------------
V3CmdExecStatus
V3UMCVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   string propertyName = "";
   bool maxD = false, preD = false, incD = false;
   bool maxDON = false, preDON = false, incDON = false;
   uint32_t maxDepth = 0, preDepth = 0, incDepth = 0;
   bool noProve = false, noFire = false;
   bool unique = false, dynamic = false;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-NOProve", token, 4) == 0) {
         if (noProve || noFire) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else noProve = true;
      }
      else if (v3StrNCmp("-NOFire", token, 4) == 0) {
         if (noProve || noFire) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else noFire = true;
      }
      else if (v3StrNCmp("-Uniqueness", token, 2) == 0) {
         if (unique) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else unique = true;
      }
      else if (v3StrNCmp("-Dynamic", token, 2) == 0) {
         if (dynamic) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else dynamic = true;
      }
      else if (v3StrNCmp("-Max-depth", token, 2) == 0) {
         if (maxD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else maxD = maxDON = true;
      }
      else if (v3StrNCmp("-Pre-depth", token, 2) == 0) {
         if (preD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else preD = preDON = true;
      }
      else if (v3StrNCmp("-Inc-depth", token, 2) == 0) {
         if (incD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
         else incD = incDON = true;
      }
      else if (maxDON || preDON || incDON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (maxDON) { maxDepth = (uint32_t)temp; maxDON = false; }
         else if (preDON) { preDepth = (uint32_t)temp; preDON = false; }
         else { incDepth = (uint32_t)temp; incDON = false; }
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }
   
   if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
   if (preDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned PreDepth)");
   if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");

   // Get Properties to be Verified
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (propertyName.size()) {
         if (handler->existProperty(propertyName)) {
            // Elaborate Property
            V3Property* const property = handler->getProperty(propertyName); assert (property);
            V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
            V3NetVec constrList; pNtk->elaborateProperty(property, constrList);
            if (constrList.size()) pNtk->combineConstraintsToOutputs(0, constrList);
            // Initialize Checker
            V3VrfUMC* const checker = new V3VrfUMC(pNtk); assert (checker);
            // UMC Specific Settings
            checker->setFireOnly(noProve);
            checker->setProveOnly(noFire);
            checker->setUniqueness(unique);
            checker->setDynamicInc(dynamic);
            if (maxD) checker->setMaxDepth(maxDepth);
            if (preD) checker->setPreDepth(preDepth);
            if (incD) checker->setIncDepth(incDepth);
            checker->verifyInOrder();
            // Set Verification Result
            if (checker->getResult(0).isCex() || checker->getResult(0).isInv())
               property->setResult(checker->getResult(0));
            // Free Checker and Elaborated Ntk
            delete checker; delete pNtk;
         }
         else Msg(MSG_ERR) << "Property Not Found !!" << endl;
      }
      else {
         Msg(MSG_IFO) << "Totally " << handler->getPropertyList().size() << " Properties to be Verified !!" << endl;
         Msg(MSG_WAR) << "Currently do not support multiple properties under verification !!" << endl;
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3UMCVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify UMC [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-Pre-depth (unsigned PreDepth)] [-Inc-depth (unsigned IncDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-NOProve | -NOFire] [-Uniqueness]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxDepth)  : The upper bound of time-frames to be reached. (default = 100)" << endl;
      Msg(MSG_IFO) << "       (unsigned PreDepth)  : The number of frames at initial. (default = 0)" << endl;
      Msg(MSG_IFO) << "       (unsigned IncDepth)  : The number of frames to be increased in each iteration. (default = 1)"<< endl;
      Msg(MSG_IFO) << "       -Max-depth           : Indicate the following token is the time-frame limit." << endl;
      Msg(MSG_IFO) << "       -Pre-depth           : Indicate the following token is the number of frames at initial." << endl;
      Msg(MSG_IFO) << "       -Inc-depth           : Indicate the following token is the number of frames to be increased." << endl;
      Msg(MSG_IFO) << "       -NOProve             : Disable running k-induction in UMC." << endl;
      Msg(MSG_IFO) << "       -NOFire              : Disable performing bounded model checking (BMC) in UMC." << endl;
      Msg(MSG_IFO) << "       -Uniqueness          : Enable adding uniqueness constraints." << endl;
   }
}

void
V3UMCVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify UMC: " << "Perform Unbounded Model Checking." << endl;
}

#endif

