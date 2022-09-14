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
#include "v3VrfSEC.h"
#include "v3VrfSIM.h"
#include "v3VrfBMC.h"
#include "v3VrfUMC.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3VrfCITP.h"
#include "v3VrfFITP.h"
#include "v3VrfIPDR.h"
#include "v3VrfMPDR.h"
#include "v3VrfKLive.h"
#include "v3DfxTrace.h"
#include "v3VrfResult.h"
#include "v3NtkWriter.h"
#include "v3StgExtract.h"
#include "v3NtkElaborate.h"

#include <iomanip>

bool initVrfCmd() {
   // Reset Verification Settings
   V3VrfBase::resetReportSettings(); V3VrfBase::resetSolverSettings();
   return (
         // Verification Verbosity Manipulation Commands
         v3CmdMgr->regCmd("SET REport",         3, 2, new V3SetReportCmd     ) &&
         v3CmdMgr->regCmd("PRInt REport",       3, 2, new V3PrintReportCmd   ) &&
         v3CmdMgr->regCmd("SET SOlver",         3, 2, new V3SetSolverCmd     ) &&
         v3CmdMgr->regCmd("PRInt SOlver",       3, 2, new V3PrintSolverCmd   ) &&
         // Verification Property Setting Commands
         v3CmdMgr->regCmd("SET SAFEty",         3, 4, new V3SetSafetyCmd     ) &&
         v3CmdMgr->regCmd("SET LIVEness",       3, 4, new V3SetLivenessCmd   ) &&
         v3CmdMgr->regCmd("ELAborate PRoperty", 3, 2, new V3ElaboratePrptyCmd) &&
         // Verification Main Commands
         v3CmdMgr->regCmd("VERify SIM",         3, 3, new V3SIMVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify BMC",         3, 3, new V3BMCVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify UMC",         3, 3, new V3UMCVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify PDR",         3, 3, new V3PDRVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify ITP",         3, 3, new V3ITPVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify SEC",         3, 3, new V3SECVrfCmd        ) &&
         v3CmdMgr->regCmd("VERify KLIVE",       3, 5, new V3KLiveVrfCmd      ) &&
         // Verification Result Reporting Commands
         v3CmdMgr->regCmd("CHEck REsult",       3, 2, new V3CheckResultCmd   ) &&
         v3CmdMgr->regCmd("WRIte REsult",       3, 2, new V3WriteResultCmd   ) &&
         v3CmdMgr->regCmd("PLOt REsult",        3, 2, new V3PlotResultCmd    )
   );
}

//----------------------------------------------------------------------
// SET REport [-All] [-RESUlt] [-Endline] [-Solver] [-Usage]
//            [-Profile] [-ON |-OFF |-RESET] 
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetReportCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool all = false, result = false, endline = false, solver = false, usage = false, profile = false;
   bool reset = false, on = false, off = false;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-All", token, 2) == 0) {
         if (all) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else all = true;
      }
      else if (v3StrNCmp("-RESUlt", token, 5) == 0) {
         if (result) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else result = true;
      }
      else if (v3StrNCmp("-Endline", token, 2) == 0) {
         if (endline) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else endline = true;
      }
      else if (v3StrNCmp("-Solver", token, 2) == 0) {
         if (solver) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else solver = true;
      }
      else if (v3StrNCmp("-Usage", token, 2) == 0) {
         if (usage) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else usage = true;
      }
      else if (v3StrNCmp("-Profile", token, 2) == 0) {
         if (profile) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else profile = true;
      }
      else if (v3StrNCmp("-ON", token, 3) == 0) {
         if (reset || on || off) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else on = true;
      }
      else if (v3StrNCmp("-OFF", token, 4) == 0) {
         if (reset || on || off) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else off = true;
      }
      else if (v3StrNCmp("-RESET", token, 6) == 0) {
         if (reset || on || off) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else reset = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(on || off || reset)) on = true;
   if (reset) all = true;
   if (all) result = endline = solver = usage = profile = true;

   // Set Verification Verbosities
   if (reset) V3VrfBase::resetReportSettings();
   else {
      if (result)  V3VrfBase::setRstOnly(off);
      if (all)     V3VrfBase::setReport (on);
      if (endline) V3VrfBase::setEndline(on);
      if (solver)  V3VrfBase::setSolver (on);
      if (usage)   V3VrfBase::setUsage  (on);
      if (profile) V3VrfBase::setProfile(on);
   }
   // Print Verification Report Verbosities
   V3PrintReportCmd printReport; return printReport.exec("");
}

void
V3SetReportCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SET REport [-All] [-RESUlt] [-Endline] [-Solver] [-Usage] [-Profile]" << endl;
   Msg(MSG_IFO) << "                  [-ON |-OFF |-RESET]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: -RESET  : Reset everything to default." << endl;
      Msg(MSG_IFO) << "       -ON     : Turn specified attributes on." << endl;
      Msg(MSG_IFO) << "       -OFF    : Turn specified attributes off." << endl;
      Msg(MSG_IFO) << "       -All    : Toggle all the following attributes." << endl;
      Msg(MSG_IFO) << "       -RESUlt : Toggle interactive verification status. (default = on)" << endl;
      Msg(MSG_IFO) << "       -Endline: Toggle endline or carriage return. (default = off)" << endl;
      Msg(MSG_IFO) << "       -Solver : Toggle solver information. (default = off)" << endl;
      Msg(MSG_IFO) << "       -Usage  : Toggle verification time usage. (default = on)" << endl;
      Msg(MSG_IFO) << "       -Profile: Toggle checker specific profiling. (default = off)" << endl;
   }
}

void
V3SetReportCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SET REport: " << "Set Verbosities for Verification Report." << endl;
}

//----------------------------------------------------------------------
// PRInt REport
//----------------------------------------------------------------------
V3CmdExecStatus
V3PrintReportCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   if (options.size()) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);
   Msg(MSG_IFO) << "Verification Report : \"";
   V3VrfBase::printReportSettings();
   Msg(MSG_IFO) << "\" ON" << endl;
   return CMD_EXEC_DONE;
}

void
V3PrintReportCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PRInt REport" << endl;
}

void
V3PrintReportCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PRInt REport: " << "Print Verbosities for Verification Report." << endl;
}

//----------------------------------------------------------------------
// SET SOlver [|-Default | -Minisat | -Boolector]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetSolverCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool dft = false, minisat = false, boolector = false;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Default", token, 2) == 0) {
         if (dft || minisat || boolector) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else dft = true;
      }
      else if (v3StrNCmp("-Minisat", token, 2) == 0) {
         if (dft || minisat || boolector) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else minisat = true;
      }
      else if (v3StrNCmp("-Boolector", token, 2) == 0) {
         if (dft || minisat || boolector) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else boolector = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(dft || minisat || boolector)) dft = true;

   // Set Verification Solvers
   if (minisat) V3VrfBase::setDefaultSolver(V3_SVR_MINISAT);
   else if (boolector) V3VrfBase::setDefaultSolver(V3_SVR_BOOLECTOR);
   else V3VrfBase::resetSolverSettings();
   // Print Verification Solvers
   V3PrintSolverCmd printSolver; return printSolver.exec("");

   return CMD_EXEC_DONE;
}

void
V3SetSolverCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SET SOlver [|-Default | -Minisat | -Boolector]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: -Default  : Enable default solver. (default = minisat)" << endl;
      Msg(MSG_IFO) << "       -Minisat  : Enable MiniSat as the active solver." << endl;
      Msg(MSG_IFO) << "       -Boolector: Enable Boolector as the active solver." << endl;
   }
}

void
V3SetSolverCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SET SOlver: " << "Set Active Solver for Verification." << endl;
}

//----------------------------------------------------------------------
// PRInt SOlver
//----------------------------------------------------------------------
V3CmdExecStatus
V3PrintSolverCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   if (options.size()) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);
   Msg(MSG_IFO) << "Active Solvers: \"";
   V3VrfBase::printSolverSettings();
   Msg(MSG_IFO) << "\"" << endl;
   return CMD_EXEC_DONE;
}

void
V3PrintSolverCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PRInt SOlver" << endl;
}

void
V3PrintSolverCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PRInt SOlver: " << "Print Active Solver for Verification." << endl;
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
// SET LIVEness [-Name <(string propertyName)>]
//              [-INVAriant <(string invName)*>]
//              [-INVConstraint <(string constrName)*>]
//              [-FAIRnessConstraint <(string constrName)*>]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetLivenessCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool name = false, nameON = false;
   bool invA = false, invAON = false;
   bool invC = false, invCON = false;
   bool fair = false, fairON = false;
   string propertyName = "";
   V3Vec<string>::Vec invNameList[3];  // [0]: INVA, [1]: INVC, [2]: FAIR
   invNameList[0].clear(); invNameList[1].clear(); invNameList[2].clear();

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Name", token, 2) == 0) {
         if (name) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else name = nameON = true; invAON = invCON = fairON = false;
      }
      else if (v3StrNCmp("-INVAriant", token, 5) == 0) {
         if (invA) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
         else invA = invAON = true; invCON = fairON = false;
      }
      else if (v3StrNCmp("-INVConstraint", token, 5) == 0) {
         if (invC) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
         else invC = invCON = true; invAON = fairON = false;
      }
      else if (v3StrNCmp("-FAIRnessConstraint", token, 5) == 0) {
         if (fair) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
         else fair = fairON = true; invAON = invCON = false;
      }
      else if (nameON) { propertyName = token; nameON = false; }
      else if (invAON) invNameList[0].push_back(token);
      else if (invCON) invNameList[1].push_back(token);
      else if (fairON) invNameList[2].push_back(token);
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
   if (invAON && !invNameList[0].size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string invName)");
   if (invCON && !invNameList[1].size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string constrName)");
   if (fairON && !invNameList[2].size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string constrName)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      // Create LTL Formula
      V3LTLFormula* f = 0;
      if (!propertyName.size()) propertyName = handler->getAuxPropertyName();
      if (handler->existProperty(propertyName))
         Msg(MSG_ERR) << "Property \"" << propertyName << "\" Already Exists !!" << endl;
      else {
         assert (propertyName.size()); assert (!handler->existProperty(propertyName));
         f = new V3LTLFormula(handler, V3NetId::makeNetId(0), false, propertyName);
      }
      // Add to property data base
      if (f && f->isValid()) {
         V3Property* const p = new V3Property(f); assert (p); uint32_t k;
         // Set Invariants and Constraints
         for (k = 0; k < 3; ++k) {
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
                     else if (k == 1) p->setInvConstr(handler, start, end);  // INVC
                     else p->setFairConstr(handler, start, end);  // FAIR
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
                           else if (k == 1) p->setInvConstr(handler, fsmExtract->getFSM(x));  // INVC
                           else p->setFairConstr(handler, fsmExtract->getFSM(x));  // FAIR
                     }
                     else {  // Rev-order Constraint Construction
                        uint32_t x = 1 + start; assert (x > start);
                        while (x-- > end) {
                           if (k == 0) p->setInvariant(handler, fsmExtract->getFSM(x));  // INVA
                           else if (k == 1) p->setInvConstr(handler, fsmExtract->getFSM(x));  // INVC
                           else p->setFairConstr(handler, fsmExtract->getFSM(x));  // FAIR
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
               Msg(MSG_ERR) << (k == 0 ? "Invariants " : k == 1 ? "Invariant Constraints " : "Fairness Constraints ") 
                            << invalidInvName << " Not Found !!"   << endl;
            if (invalidOutputs.size())
               Msg(MSG_ERR) << (k == 0 ? "Invariants " : k == 1 ? "Invariant Constraints " : "Fairness Constraints ") 
                            << invalidOutputs << " Not Boolean !!" << endl;
            if (invalidInvName.size() || invalidOutputs.size()) break;
         }
         // Set Property if NO Invalid Inputs
         if (k == 3) {
            handler->setProperty(p);
            Msg(MSG_IFO) << "Liveness Property " << propertyName << " has been set successfully !!" << endl;
         }
         else delete p;
      }
      else {
         Msg(MSG_ERR) << "Failed to Create Property " << propertyName << " !!" << endl;
         if (f) delete f;
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3SetLivenessCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SET LIVEness [-Name <(string propertyName)>]" << endl;
   Msg(MSG_IFO) << "                    [-INVAriant <(string invName)*>]" << endl;
   Msg(MSG_IFO) << "                    [-INVConstraint <(string constrName)*>]" << endl;
   Msg(MSG_IFO) << "                    [-FAIRnessConstraint <(string constrName)*>]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be set." << endl;
      Msg(MSG_IFO) << "       (string invName)*     : List of names of invariants." << endl;
      Msg(MSG_IFO) << "       (string constrName)*  : List of names of (either invariant or fairness) constraints." << endl;
      Msg(MSG_IFO) << "       -Name                 : Indicate the following token is the name of a property." << endl;
      Msg(MSG_IFO) << "       -INVAriant            : Indicate the starting of a list of invariants." << endl;
      Msg(MSG_IFO) << "       -INVConstraint        : Indicate the starting of a list of invariant constraints." << endl;
      Msg(MSG_IFO) << "       -FAIRnessConstraint   : Indicate the starting of a list of fairness constraints." << endl;
   }
}

void
V3SetLivenessCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SET LIVEness: " << "Set Liveness Properties on Current Network." << endl;
}

//----------------------------------------------------------------------
// ELAborate PRoperty [(string propertyName)]*
//----------------------------------------------------------------------
V3CmdExecStatus
V3ElaboratePrptyCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   if (!options.size()) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");

   // Get Properties to be Verified
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NtkElaborate* elaboratedNtk = new V3NtkElaborate(handler); assert (elaboratedNtk);
      if (elaboratedNtk) {
         V3NetVec constrList; constrList.clear();
         for (uint32_t i = 0; i < options.size(); ++i) {
            if (!handler->existProperty(options[i])) {
               Msg(MSG_ERR) << "Property \"" << options[i] << "\" Not Found !!" << endl;
               delete elaboratedNtk; elaboratedNtk = 0; break;
            }
            V3Property* const property = handler->getProperty(options[i]); assert (property);
            elaboratedNtk->elaborateProperty(property, constrList);
            // Set Constraints to Primary Outputs
            if (constrList.size()) elaboratedNtk->createConstrOutputs(constrList);
         }
         if (elaboratedNtk && elaboratedNtk->getNtk()) v3Handler.pushAndSetCurHandler(elaboratedNtk);
      }
      else Msg(MSG_ERR) << "Property Elaboration Failed !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ElaboratePrptyCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: ELAborate PRoperty [(string propertyName)]*" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property." << endl;
   }
}

void
V3ElaboratePrptyCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "ELAborate PRoperty: " << "Elaborate Properties on a Duplicated Network." << endl;
}

//----------------------------------------------------------------------
// VERify SIM [(string propertyName)]
//            [<-Time (unsigned MaxTime)>] [<-Cycle (unsigned MaxCycle)>]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SIMVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool time = false, cycle = false;
   bool timeON = false, cycleON = false;
   string propertyName = "";
   uint32_t maxTime = 0, maxCycle = 0;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Time", token, 2) == 0) {
         if (time) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else time = timeON = true;
      }
      else if (v3StrNCmp("-Cycle", token, 2) == 0) {
         if (cycle) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cycle = cycleON = true;
      }
      else if (timeON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxTime = (uint32_t)temp; assert (maxTime); timeON = false;
      }
      else if (cycleON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxCycle = (uint32_t)temp; assert (maxCycle); cycleON = false;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }
   
   if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxTime)");
   if (cycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCycle)");

   // Get Properties to be Verified
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (propertyName.size()) {
         if (handler->existProperty(propertyName)) {
            // Elaborate Property
            V3Property* const property = handler->getProperty(propertyName); assert (property);
            V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
            V3NetVec constrList; pNtk->elaborateProperty(property, constrList);
            // Initialize Checker
            V3VrfSIM* const checker = new V3VrfSIM(pNtk); assert (checker);
            if (constrList.size()) checker->setConstraint(constrList, 0);
            // SIM Specific Settings
            if (maxTime) checker->setMaxTime(maxTime);
            if (maxCycle) checker->setMaxDepth(maxCycle);
            checker->verifyInOrder();
            // Set Verification Result
            if (checker->getResult(0).isCex())
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
V3SIMVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify SIM [(string propertyName)]" << endl;
   Msg(MSG_IFO) << "                  [<-Time (unsigned MaxTime)>] [<-Cycle (unsigned MaxCycle)>]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxTime)   : The upper bound of simulation runtime." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxCycle)  : The upper bound of simulation cycle." << endl;
      Msg(MSG_IFO) << "       -Time                : Enable setting of runtime limit." << endl;
      Msg(MSG_IFO) << "       -Cycle               : Enable setting of cycle limit." << endl;
   }
}

void
V3SIMVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify SIM: " << "Perform (Constrained) Random Simulation." << endl;
}

//----------------------------------------------------------------------
// VERify BMC [(string propertyName)]
//            [-Max-depth (unsigned MaxDepth)]
//            [-Pre-depth (unsigned PreDepth)]
//            [-Inc-depth (unsigned IncDepth)]
//----------------------------------------------------------------------
V3CmdExecStatus
V3BMCVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   string propertyName = "";
   bool maxD = false, preD = false, incD = false;
   bool maxDON = false, preDON = false, incDON = false;
   uint32_t maxDepth = 0, preDepth = 0, incDepth = 0;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Max-depth", token, 2) == 0) {
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
            V3VrfBMC* const checker = new V3VrfBMC(pNtk); assert (checker);
            // BMC Specific Settings
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
V3BMCVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify BMC [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-Pre-depth (unsigned PreDepth)] [-Inc-depth (unsigned IncDepth)]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxDepth)  : The upper bound of time-frames to be reached. (default = 100)" << endl;
      Msg(MSG_IFO) << "       (unsigned PreDepth)  : The number of frames at initial. (default = 0)" << endl;
      Msg(MSG_IFO) << "       (unsigned IncDepth)  : The number of frames to be increased in each iteration. (default = 1)"<< endl;
      Msg(MSG_IFO) << "       -Max-depth           : Indicate the following token is the time-frame limit." << endl;
      Msg(MSG_IFO) << "       -Pre-depth           : Indicate the following token is the number of frames at initial." << endl;
      Msg(MSG_IFO) << "       -Inc-depth           : Indicate the following token is the number of frames to be increased." << endl;
   }
}

void
V3BMCVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify BMC: " << "Perform Bounded Model Checking." << endl;
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

//----------------------------------------------------------------------
// VERify PDR [(string propertyName)] [-Max-depth (unsigned MaxDepth)]
//            [-Recycle (unsigned MaxCount)] [-Incremental]
//            [-FWDSATGen] [-FWDUNSATGen]
//----------------------------------------------------------------------
V3CmdExecStatus
V3PDRVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool maxD = false, maxDON = false, recycle = false, recycleON = false;
   bool incremental = false, fwdSATGen = false, fwdUNSATGen = false;
   uint32_t maxDepth = 0, recycleCount = 0;
   string propertyName = "";
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Max-depth", token, 2) == 0) {
         if (maxD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (recycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCount)");
         else maxD = maxDON = true;
      }
      else if (v3StrNCmp("-Recycle", token, 2) == 0) {
         if (recycle) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else recycle = recycleON = true;
      }
      else if (v3StrNCmp("-Incremental", token, 2) == 0) {
         if (incremental) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (recycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCount)");
         else incremental = true;
      }
      else if (v3StrNCmp("-FWDSATGen", token, 8) == 0) {
         if (fwdSATGen) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (recycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCount)");
         else fwdSATGen = true;
      }
      else if (v3StrNCmp("-FWDUNSATGen", token, 10) == 0) {
         if (fwdUNSATGen) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (recycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCount)");
         else fwdUNSATGen = true;
      }
      else if (maxDON || recycleON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (maxDON) { maxDepth = (uint32_t)temp; assert (maxDepth); maxDON = false; }
         else { recycleCount = (uint32_t)temp; assert (recycleCount); recycleON = false; }
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }
   
   if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
   if (recycleON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxCount)");

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
            V3VrfBase* const checker = (incremental) ? (V3VrfBase*)(new V3VrfIPDR(pNtk)) 
                                                     : (V3VrfBase*)(new V3VrfMPDR(pNtk)); assert (checker);
            // PDR Specific Settings
            if (maxD) checker->setMaxDepth(maxDepth);
            if (incremental) {
               V3VrfIPDR* const pdrChecker = dynamic_cast<V3VrfIPDR*>(checker);
               if (recycle) pdrChecker->setRecycle(recycleCount);
               pdrChecker->setForwardSATGen(fwdSATGen);
               pdrChecker->setForwardUNSATGen(fwdUNSATGen);
            }
            else {
               V3VrfMPDR* const pdrChecker = dynamic_cast<V3VrfMPDR*>(checker);
               if (recycle) pdrChecker->setRecycle(recycleCount);
               pdrChecker->setForwardSATGen(fwdSATGen);
               pdrChecker->setForwardUNSATGen(fwdUNSATGen);
            }
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
V3PDRVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify PDR [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-Recycle (unsigned MaxCount)] [-Incremental]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxDepth)  : The upper bound of time-frames to be reached." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxCount)  : The upper bound of temporary assumption literals in solvers." << endl;
      Msg(MSG_IFO) << "       -Max-depth           : Indicate the following token is the limit of time-frames." << endl;
      Msg(MSG_IFO) << "       -Recycle             : Enable setting the limit of assumption literals for recycle." << endl;
      Msg(MSG_IFO) << "       -Incremental         : Implement with multiple solvers. (c.f. Monolithic)" << endl;
   }
}

void
V3PDRVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify PDR: " << "Perform Property Directed Reachability." << endl;
}

//----------------------------------------------------------------------
// VERify ITP [(string propertyName)] [-Max-depth (unsigned MaxDepth)]
//            [-Reverse] [-Increment] [-Force] [-RECycle]
//            [-Block (unsigned badCount)] [-INDIVidual]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ITPVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool maxD = false, maxDON = false, reverse = false;
   bool inc = false, force = false;
   bool block = false, blockON = false, indiv = false;
   bool recycle = false;
   uint32_t maxDepth = 0, badCount = 1;
   string propertyName = "";
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Max-depth", token, 2) == 0) {
         if (maxD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else maxD = maxDON = true;
      }
      else if (v3StrNCmp("-Block", token, 2) == 0) {
         if (block) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else block = blockON = true;
      }
      else if (v3StrNCmp("-INDIVidual", token, 6) == 0) {
         if (indiv) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else indiv = true;
      }
      else if (v3StrNCmp("-RECycle", token, 4) == 0) {
         if (recycle) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else recycle = true;
      }
      else if (v3StrNCmp("-Reverse", token, 2) == 0) {
         if (reverse) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else reverse = true;
      }
      else if (v3StrNCmp("-Increment", token, 2) == 0) {
         if (inc) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else inc = true;
      }
      else if (v3StrNCmp("-Force", token, 2) == 0) {
         if (force) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");
         else force = true;
      }
      else if (maxDON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxDepth = (uint32_t)temp; assert (maxDepth); maxDON = false;
      }
      else if (blockON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         badCount = (uint32_t)temp; blockON = false;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }
   
   if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
   if (blockON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned badCount)");

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
            V3VrfBase* const checker = (reverse ? (V3VrfBase*)(new V3VrfFITP(pNtk)) 
                                                : (V3VrfBase*)(new V3VrfCITP(pNtk))); assert (checker);
            // ITP Specific Settings
            if (reverse) {
               dynamic_cast<V3VrfFITP*>(checker)->setIncrementDepth(inc);
               dynamic_cast<V3VrfFITP*>(checker)->setForceUnreachable(force);
               dynamic_cast<V3VrfFITP*>(checker)->setBlockBadCountIndep(indiv);
               dynamic_cast<V3VrfFITP*>(checker)->setRecycleInterpolants(recycle);
               if (block) dynamic_cast<V3VrfFITP*>(checker)->setBlockBadCount(badCount);
            }
            else {
               dynamic_cast<V3VrfCITP*>(checker)->setIncrementDepth(inc);
               dynamic_cast<V3VrfCITP*>(checker)->setForceUnreachable(force);
               dynamic_cast<V3VrfCITP*>(checker)->setBlockBadCountIndep(indiv);
               dynamic_cast<V3VrfCITP*>(checker)->setRecycleInterpolants(recycle);
               if (block) dynamic_cast<V3VrfCITP*>(checker)->setBlockBadCount(badCount);
            }
            if (maxD) checker->setMaxDepth(maxDepth);
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
V3ITPVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify ITP [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-Reverse] [-Increment] [-Force] [-RECycle]" << endl;
   Msg(MSG_IFO) << "                  [-Block (unsigned badCount)] [-INDIVidual]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxDepth)  : The upper bound of time-frames to be reached." << endl;
      Msg(MSG_IFO) << "       (unsigned badCount)  : The maximum number of spurious cex for refinement. (default = 1)" << endl;
      Msg(MSG_IFO) << "       -Max-depth           : Indicate the following token is the limit of time-frames." << endl;
      Msg(MSG_IFO) << "       -Block               : Indicate the following token is the limit to cex analysis." << endl;
      Msg(MSG_IFO) << "       -Reverse             : Enables the reversed implementation of the algorithm." << endl;
      Msg(MSG_IFO) << "       -Increment           : Enables incrementing BMC depth dynamically." << endl;
      Msg(MSG_IFO) << "       -Force               : Enables considering 1~k frames (instead of the k-th frame) in the BMC part." << endl;
      Msg(MSG_IFO) << "       -RECycle             : Enables cube recycling for interpolant reuse." << endl;
   }
}

void
V3ITPVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify ITP: " << "Perform Interpolation-based Model Checking." << endl;
}

//----------------------------------------------------------------------
// VERify SEC [(string propertyName)] [-Max-depth (unsigned MaxDepth)]
//            [-BMC | -UMC | -IPDR | -MPDR | -CITP | -FITP]
//            [-CEC] [-SEC]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SECVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool bmc = false, umc = false, citp = false, fitp = false, mpdr = false, ipdr = false;
   bool cec = false, sec = false; bool maxD = false, maxDON = false;
   string propertyName = ""; uint32_t maxDepth = 0;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Max-depth", token, 2) == 0) {
         if (maxD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else maxD = maxDON = true;
      }
      else if (v3StrNCmp("-BMC", token, 4) == 0) {
         if (bmc) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (umc || ipdr || mpdr || citp || fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else bmc = true;
      }
      else if (v3StrNCmp("-UMC", token, 4) == 0) {
         if (umc) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (bmc || ipdr || mpdr || citp || fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else umc = true;
      }
      else if (v3StrNCmp("-IPDR", token, 5) == 0) {
         if (ipdr) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (bmc || umc || mpdr || citp || fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else ipdr = true;
      }
      else if (v3StrNCmp("-MPDR", token, 5) == 0) {
         if (mpdr) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (bmc || umc || ipdr || citp || fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else mpdr = true;
      }
      else if (v3StrNCmp("-CITP", token, 5) == 0) {
         if (citp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (bmc || umc || ipdr || mpdr || fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else citp = true;
      }
      else if (v3StrNCmp("-FITP", token, 5) == 0) {
         if (fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (bmc || umc || ipdr || mpdr || citp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else fitp = true;
      }
      else if (v3StrNCmp("-CEC", token, 4) == 0) {
         if (cec) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cec = true;
      }
      else if (v3StrNCmp("-SEC", token, 4) == 0) {
         if (sec) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else sec = true;
      }
      else if (maxDON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxDepth = (uint32_t)temp; assert (maxDepth); maxDON = false;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");

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
            V3VrfSEC* const checker = new V3VrfSEC(pNtk); assert (checker);
            V3VrfBase* secChecker = 0;
            if (bmc) {
               secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker);
               dynamic_cast<V3VrfUMC*>(secChecker)->setFireOnly(true);
            }
            else if (umc) { secChecker = new V3VrfUMC(checker->getSECHandler()); assert (secChecker); }
            else if (citp) { secChecker = new V3VrfCITP(checker->getSECHandler()); assert (secChecker); }
            else if (fitp) { secChecker = new V3VrfFITP(checker->getSECHandler()); assert (secChecker); }
            else if (mpdr) {
               secChecker = new V3VrfMPDR(checker->getSECHandler()); assert (secChecker);
               //dynamic_cast<V3VrfMPDR*>(secChecker)->setRecycle(300);
            }
            else if (ipdr) {
               secChecker = new V3VrfIPDR(checker->getSECHandler()); assert (secChecker);
               //dynamic_cast<V3VrfIPDR*>(secChecker)->setRecycle(300);
            }
            if (secChecker) checker->setChecker(secChecker);
            if (cec) checker->setAssumeCECMiter();
            if (sec) checker->setAssumeSECMiter();
            if (maxD) checker->setMaxDepth(maxDepth);
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
V3SECVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify SEC [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                  [-UMC | -IPDR | -MPDR] [-CEC] [-SEC]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       -UMC                 : Enable UMC as a safety checker." << endl;
      Msg(MSG_IFO) << "       -MPDR                : Enable Monolithic PDR as a safety checker." << endl;
      Msg(MSG_IFO) << "       -IPDR                : Enable Incremental PDR as a safety checker." << endl;
      Msg(MSG_IFO) << "       -CEC                 : Assume that the Network could be a CEC Miter." << endl;
      Msg(MSG_IFO) << "       -SEC                 : Assume that the Network could be a SEC Miter." << endl;
   }
}

void
V3SECVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify SEC: " << "Perform Sequential Equivalence Checking." << endl;
}

//----------------------------------------------------------------------
// VERify KLIVE [(string propertyName)] [-Max-depth (unsigned MaxDepth)]
//              [-Inc-depth (unsigned IncDepth)]
//              [| -UMC | -IPDR | -MPDR | -CITP | -FITP]
//----------------------------------------------------------------------
V3CmdExecStatus
V3KLiveVrfCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool maxD = false, incD = false, maxDON = false, incDON = false;
   bool umc = false, ipdr = false, mpdr = false, citp = false, fitp = false;
   string propertyName = ""; uint32_t maxDepth = 0, incDepth = 0;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Max-depth", token, 2) == 0) {
         if (maxD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else maxD = maxDON = true;
      }
      else if (v3StrNCmp("-Inc-depth", token, 2) == 0) {
         if (incD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else incD = incDON = true;
      }
      else if (v3StrNCmp("-UMC", token, 4) == 0) {
         if (umc) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else umc = true;
      }
      else if (v3StrNCmp("-IPDR", token, 5) == 0) {
         if (ipdr) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else ipdr = true;
      }
      else if (v3StrNCmp("-MPDR", token, 5) == 0) {
         if (mpdr) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else mpdr = true;
      }
      else if (v3StrNCmp("-CITP", token, 5) == 0) {
         if (citp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else citp = true;
      }
      else if (v3StrNCmp("-FITP", token, 5) == 0) {
         if (fitp) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
         else if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
         else fitp = true;
      }
      else if (maxDON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxDepth = (uint32_t)temp; assert (maxDepth); maxDON = false;
      }
      else if (incDON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         incDepth = (uint32_t)temp; assert (incDepth); incDON = false;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }
   
   if (maxDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned MaxDepth)");
   if (incDON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned IncDepth)");
   if (!umc && !ipdr && !mpdr && !citp && !fitp) umc = true;

   // Get Properties to be Verified
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (propertyName.size()) {
         if (handler->existProperty(propertyName)) {
            // Elaborate Property
            V3Property* const property = handler->getProperty(propertyName); assert (property);
            V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
            V3NetVec constrList; pNtk->elaborateProperty(property, constrList, false);
            if (constrList.size()) pNtk->combineConstraintsToOutputs(0, constrList);
            // Elaborate Fairness Constraints
            V3NetVec fairList; fairList.clear();
            for (uint32_t i = 0; i < property->getFairConstrSize(); ++i) {
               V3NetVec constr; pNtk->elaborateFairness(property->getFairConstr(i), constr);
               fairList.insert(fairList.end(), constr.begin(), constr.end());
            }
            // Initialize Checker
            V3VrfKLive* const checker = new V3VrfKLive(pNtk); assert (checker);
            if (fairList.size()) checker->setConstraint(fairList, 0);
            V3VrfBase* klChecker = 0;
            if (ipdr) {
               klChecker = new V3VrfIPDR(checker->getKLHandler()); assert (klChecker);
               dynamic_cast<V3VrfIPDR*>(klChecker)->setRecycle(300);
            }
            else if (mpdr) {
               klChecker = new V3VrfMPDR(checker->getKLHandler()); assert (klChecker);
               dynamic_cast<V3VrfMPDR*>(klChecker)->setRecycle(300);
            }
            else if (citp) { klChecker = new V3VrfCITP(checker->getKLHandler()); assert (klChecker); }
            else if (fitp) { klChecker = new V3VrfFITP(checker->getKLHandler()); assert (klChecker); }
            else { klChecker = new V3VrfUMC(checker->getKLHandler()); assert (klChecker); }
            if (klChecker) checker->setChecker(klChecker);
            if (maxD) checker->setMaxDepth(maxDepth);
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
V3KLiveVrfCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: VERify KLIVE [(string propertyName)] [-Max-depth (unsigned MaxDepth)]" << endl;
   Msg(MSG_IFO) << "                    [-Inc-depth (unsigned IncDepth)] [-UMC | -IPDR | -MPDR]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a property to be verified." << endl;
      Msg(MSG_IFO) << "       (unsigned MaxDepth)  : The upper bound of time-frames to be reached. (default = 100)" << endl;
      Msg(MSG_IFO) << "       (unsigned IncDepth)  : The number of frames to be increased in each iteration. (default = 1)"<< endl;
      Msg(MSG_IFO) << "       -Max-depth           : Indicate the following token is the time-frame limit." << endl;
      Msg(MSG_IFO) << "       -Inc-depth           : Indicate the following token is the number of frames to be increased." << endl;
      Msg(MSG_IFO) << "       -UMC                 : Enable UMC as a safety checker." << endl;
      Msg(MSG_IFO) << "       -MPDR                : Enable Monolithic PDR as a safety checker." << endl;
      Msg(MSG_IFO) << "       -IPDR                : Enable Incremental PDR as a safety checker." << endl;
   }
}

void
V3KLiveVrfCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "VERify KLIVE: " << "Perform K-Liveness for Liveness Checking." << endl;
}

//----------------------------------------------------------------------
// CHEck REsult <(string propertyName)> [-Simulation | -Formal]
//              [[-Trace | -Invariant] <(string resultFileName)>]
//----------------------------------------------------------------------
V3CmdExecStatus
V3CheckResultCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool simulation = false, formal = false;
   bool trace = false, invariant = false, inputON = false;
   string propertyName = "", resultFileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Simulation", token, 2) == 0) {
         if (simulation || formal) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string resultFileName)");
         else simulation = true;
      }
      else if (v3StrNCmp("-Formal", token, 2) == 0) {
         if (simulation || formal) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string resultFileName)");
         else formal = true;
      }
      else if (v3StrNCmp("-Trace", token, 2) == 0) {
         if (trace || invariant) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else trace = inputON = true;
      }
      else if (v3StrNCmp("-Invariant", token, 2) == 0) {
         if (trace || invariant) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else invariant = inputON = true;
      }
      else if (inputON) {
         resultFileName = token;
         inputON = false;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (propertyName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
   if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string resultFileName)");

   // Get Properties to be Checked
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existProperty(propertyName)) {
         // Elaborate Property
         V3Property* const property = handler->getProperty(propertyName); assert (property);
         V3LTLFormula* const ltlFormula = property->getLTLFormula(); assert (ltlFormula);
         V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
         // Use this Function to Omit Invariants
         const uint32_t pIndex = pNtk->elaborateLTLFormula(ltlFormula, false); assert (!pIndex);
         assert (pNtk->getNtk()); assert (1 == pNtk->getNtk()->getOutputSize());
         // Elaborate Invariant Constraints
         V3NetVec constrList; constrList.clear();
         for (uint32_t i = 0; i < property->getInvConstrSize(); ++i) {
            V3NetVec constr; pNtk->elaborateConstraints(property->getInvConstr(i), constr);
            constrList.insert(constrList.end(), constr.begin(), constr.end());
         }
         // Elaborate Fairness Constraints
         V3NetVec fairList; fairList.clear();
         for (uint32_t i = 0; i < property->getFairConstrSize(); ++i) {
            V3NetVec constr; pNtk->elaborateConstraints(property->getFairConstr(i), constr);
            fairList.insert(fairList.end(), constr.begin(), constr.end());
         }
         // Set Constraints to Primary Outputs for Simulation
         if (constrList.size()) pNtk->createConstrOutputs(constrList);
         if (fairList.size()) pNtk->createConstrOutputs(fairList);
         // Get Property Type
         V3LTLFormula* const formula = property->getLTLFormula(); assert (formula);
         assert (V3_LTL_T_G == formula->getOpType(formula->getRoot()) || 
                 V3_LTL_T_F == formula->getOpType(formula->getRoot()));
         const bool safe = (V3_LTL_T_G == formula->getOpType(formula->getRoot()));
         // Set Verification Result from Input File if Specified
         if (trace) {  // Set Counterexample Trace from Input
            V3CexTrace* const cex = V3CexTraceParser(pNtk, resultFileName);
            if (!cex) Msg(MSG_ERR) << "Failed to Parse Counterexample Trace Data !!" << endl;
            else { V3VrfResult result; result.setCexTrace(cex); property->setResult(result); }
         }
         else if (invariant)  // Set Inductive Invariant from Input
            Msg(MSG_WAR) << "Currently Inductive Invariant cannot be recoreded !!" << endl;
         // Check Verification Result
         int result = 0;
         if (property->isFired()) {  // Check if a real counter-example is found
            if (formal) result = formalCheckFiredResult(*(property->getCexTrace()), constrList, fairList, safe, pNtk);
            else result = simulationCheckFiredResult(*(property->getCexTrace()), constrList, fairList, safe, pNtk);
            // Report Check Result
            if (result > 0) Msg(MSG_IFO) << "A real counter-example is found ";
            else if (result < 0) Msg(MSG_IFO) << "A spurious counter-example is found ";
            else Msg(MSG_IFO) << "Fail to check verification validation ";
            Msg(MSG_IFO) << "for property \"" << property->getLTLFormula()->getName() << "\"." << endl;
         }
         else if (property->isProven()) {  // Check if a real inductie invariant is found
            if (simulation) 
               Msg(MSG_WAR) << "Property \"" << property->getLTLFormula()->getName() << "\" is an invariant, "
                            << "which can not be checked by simulation (formal check is adopted) !!" << endl;
            result = formalCheckProvenResult(property->getIndInv(), handler);
            // Report Check Result
            if (result > 0) Msg(MSG_IFO) << "A real inductive invariant is found ";
            else if (result < 0) Msg(MSG_IFO) << "A spurious inductive invariant is found ";
            else Msg(MSG_IFO) << "Fail to check verification validation ";
            Msg(MSG_IFO) << "for property \"" << property->getLTLFormula()->getName() << "\"." << endl;
         }
         else Msg(MSG_ERR) << "Property \"" << property->getLTLFormula()->getName()
                           << "\" has not been verified !!" << endl;
         delete pNtk;  // Destruct Elaborated Ntk
      }
      else Msg(MSG_ERR) << "Property Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3CheckResultCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: CHEck REsult <(string propertyName)> [-Simulation | -Formal]" << endl;
   Msg(MSG_IFO) << "                    [[-Trace | -Invariant] <(string resultFileName)>]" << endl;
   Msg(MSG_IFO) << "NOTE : Confirmation of Inductive Invariants is Not Available Yet !!" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName)  : The name of a verified property." << endl;
      Msg(MSG_IFO) << "       (string resultFileName): The file name of a verification result." << endl;
      Msg(MSG_IFO) << "       -Simulation            : Enable simulation in verifying the result." << endl;
      Msg(MSG_IFO) << "       -Formal                : Enable formal in verifying the result." << endl;
      Msg(MSG_IFO) << "       -Trace                 : Indicate resultFileName is a file of counterexample." << endl;
      Msg(MSG_IFO) << "       -Invariant             : Indicate resultFileName is a file of inductive invariant." << endl;
   }
}

void
V3CheckResultCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "CHEck REsult: " << "Verify Verification Result." << endl;
}

//----------------------------------------------------------------------
// WRIte REsult <(string propertyName)> <(string resultFileName)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3WriteResultCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   string propertyName = "", resultFileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (propertyName == "") propertyName = token;
      else if (resultFileName == "") resultFileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (propertyName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
   if (resultFileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string resultFileName)");

   // Get Properties for Output
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existProperty(propertyName)) {
         // Write Verification Results
         V3Property* const property = handler->getProperty(propertyName); assert (property);
         if (property->isFired())  // Write Counterexample Trace
            V3CexTraceWriter(handler, property->getCexTrace(), resultFileName);
         else if (property->isProven())  // Write Inductive Invariant
            Msg(MSG_WAR) << "Currently Inductive Invariant cannot be reported !!" << endl;
         else Msg(MSG_ERR) << "Property \"" << property->getLTLFormula()->getName()
                           << "\" has not been verified !!" << endl;
      }
      else Msg(MSG_ERR) << "Property Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WriteResultCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRIte REsult <(string propertyName)> <(string resultFileName)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName)  : The name of a verified property." << endl;
      Msg(MSG_IFO) << "       (string resultFileName): The file name for the output of verification results." << endl;
   }
}

void
V3WriteResultCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRIte REsult: " << "Write Verification Results." << endl;
}

//----------------------------------------------------------------------
// PLOt REsult <(string propertyName)> <(string resultFileName)> 
//----------------------------------------------------------------------
V3CmdExecStatus
V3PlotResultCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   string propertyName = "", resultFileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (propertyName == "") propertyName = token;
      else if (resultFileName == "") resultFileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (propertyName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");
   if (resultFileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string resultFileName)");

   // Get Properties for Output
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existProperty(propertyName)) {
         // Plot Verification Results
         V3Property* const property = handler->getProperty(propertyName); assert (property);
         if (property->isFired())  // Plot Counterexample Trace
            V3CexTraceVisualizer(handler, property->getCexTrace(), resultFileName);
         else if (property->isProven())  // Write Inductive Invariant
            Msg(MSG_WAR) << "Currently Inductive Invariant cannot be reported !!" << endl;
         else Msg(MSG_ERR) << "Property \"" << property->getLTLFormula()->getName()
                           << "\" has not been verified !!" << endl;
      }
      else Msg(MSG_ERR) << "Property Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PlotResultCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PLOt REsult <(string propertyName)> <(string resultFileName)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName)  : The name of a verified property." << endl;
      Msg(MSG_IFO) << "       (string resultFileName): The file name for the output of verification results." << endl;
   }
}

void
V3PlotResultCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PLOt REsult: " << "Plot Verification Results." << endl;
}

#endif

