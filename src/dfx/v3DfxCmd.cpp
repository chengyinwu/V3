/****************************************************************************
  FileName     [ v3DfxCmd.cpp ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Debugging and Fix Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_CMD_C
#define V3_DFX_CMD_C

#include "v3Msg.h"
#include "v3DfxCmd.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3DfxTrace.h"
#include "v3BvBlastAig.h"
#include "v3DfxSimplify.h"
#include "v3NtkElaborate.h"

#include <iomanip>

bool initDfxCmd() {
   return (
         // Counterexample Simplification Commands
         v3CmdMgr->regCmd("SIMplify TRace",     3, 2, new V3TraceSimplifyCmd) &&
         // Counterexample Manipulation Commands
         v3CmdMgr->regCmd("OPTimize TRace",     3, 2, new V3OptTraceCmd     )
   );
}

//----------------------------------------------------------------------
// SIMplify TRace <(string propertyName)> [(unsigned maxNoFrames)]
//                [| -Care | -Transition]
//----------------------------------------------------------------------
V3CmdExecStatus
V3TraceSimplifyCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool care = false, trans = false;
   string propertyName = ""; uint32_t maxNoFrames = V3NtkUD;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Care", token, 2) == 0) {
         if (care) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else care = true;
      }
      else if (v3StrNCmp("-Transition", token, 2) == 0) {
         if (trans) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else trans = true;
      }
      else if ("" == propertyName) { propertyName = token; }
      else if (V3NtkUD == maxNoFrames) {
         int temp;
         if (!v3Str2Int(token, temp) || temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxNoFrames = (uint32_t)temp;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(care || trans)) care = true;
   if ("" == propertyName) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");

   // Get Properties to be Verified
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existProperty(propertyName)) {
         // Elaborate Property
         V3Property* const property = handler->getProperty(propertyName); assert (property);
         if (property->isFired()) {
            V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
            V3NetVec constrList; pNtk->elaborateProperty(property, constrList);
            // Set Constraints to Primary Outputs for Optimizations
            if (constrList.size()) pNtk->createConstrOutputs(constrList);
            // Create Engine for Trace Simplification
            V3TraceSimplify* const eng = new V3TraceSimplify(pNtk, *(property->getCexTrace())); assert (eng);
            if (V3VrfBase::reportON()) { Msg(MSG_IFO) << "Before Simplification = "; eng->printTraceInfo(); }
            eng->simplifyInputPatterns(trans, maxNoFrames);
            if (V3VrfBase::reportON()) { Msg(MSG_IFO) << "After Simplification = "; eng->printTraceInfo(); }
            // Put the Simplified Counterexample Back
            V3VrfResult cex; cex.setCexTrace(eng->getTrace()); property->setResult(cex);
            // Free Engine and Elaborated Ntk
            delete eng; delete pNtk;
         }
         else if (property->isProven()) Msg(MSG_ERR) << "Property \"" << propertyName << "\" is True !!" << endl;
         else Msg(MSG_ERR) << "Property \"" << propertyName << "\" has NOT been Verified !!" << endl;
      }
      else Msg(MSG_ERR) << "Property Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3TraceSimplifyCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SIMplify TRace <(string propertyName)> [(unsigned maxNoFrames)]" << endl
                << "                      [| -Care | -Transition]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName) : The name of a fired property." << endl;
      Msg(MSG_IFO) << "       (unsigned maxNoFrames): Upper bound frame numbers in a sub-problem. (default = inf)" << endl;
      Msg(MSG_IFO) << "       -Care                 : Start the configuration of minimizing care signals." << endl;
      Msg(MSG_IFO) << "       -Transition           : Start the configuration of minimizing transitions." << endl;
   }
}

void
V3TraceSimplifyCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SIMplify TRace: " << "Simplify Counterexample Traces." << endl;
}

//----------------------------------------------------------------------
// OPTimize TRace <(string propertyName)> [-NOReduce | -NOGeneralize]
//----------------------------------------------------------------------
V3CmdExecStatus
V3OptTraceCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool noReduce = false, noGeneralize = false;
   string propertyName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-NOReduce", token, 4) == 0) {
         if (noReduce || noGeneralize) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else noReduce = true;
      }
      else if (v3StrNCmp("-NOGeneralize", token, 4) == 0) {
         if (noReduce || noGeneralize) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else noGeneralize = true;
      }
      else if (propertyName == "") propertyName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (propertyName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string propertyName)");

   // Get Properties for Output
   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existProperty(propertyName)) {
         // Elaborate Property
         V3Property* const property = handler->getProperty(propertyName); assert (property);
         V3NtkElaborate* const pNtk = new V3NtkElaborate(handler); assert (pNtk);
         V3NetVec constrList; pNtk->elaborateProperty(property, constrList);
         // Currently Support NO Constraints
         if (!constrList.size()) {
            if (property->isFired()) {
               // Start Trace Reduction
               if (!noReduce) performTraceReduction(pNtk, property);
               // Start Trace Generalization
               if (!noGeneralize) performTraceGeneralization(pNtk, property);
            }
            else Msg(MSG_ERR) << "Property \"" << property->getLTLFormula()->getName() 
                              << "\" has no counterexample trace !!" << endl;
         }
         else Msg(MSG_ERR) << "Currently Constraints are Not Supported !!" << endl;
         delete pNtk;  // Destruct Elaborated Ntk
      }
      else Msg(MSG_ERR) << "Property Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3OptTraceCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: OPTimize TRace <(string propertyName)> [-NOReduce | -NOGeneralize]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string propertyName): The name of a failing property." << endl;
      Msg(MSG_IFO) << "       -NOReduce            : Disable counterexample reduction." << endl;
      Msg(MSG_IFO) << "       -NOGeneralize        : Disable counterexample generalization." << endl;
   }
}

void
V3OptTraceCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "OPTimize TRace: " << "Optimize a Counterexample Trace." << endl;
}

#endif

