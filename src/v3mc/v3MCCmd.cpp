/****************************************************************************
  FileName     [ v3MCCmd.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ Model Checking Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_CMD_C
#define V3_MC_CMD_C

#include "v3Msg.h"
#include "v3MCCmd.h"
#include "v3MCMain.h"

#include <iomanip>

bool initV3MCCmd() {
   return (
         // Model Checking Property Commands
         v3CmdMgr->regCmd("READ PROperty",      4, 3, new V3MCReadPropertyCmd ) &&
         v3CmdMgr->regCmd("WRITE PROperty",     5, 3, new V3MCWritePropertyCmd) &&
         // Model Checking Verification Commands
         v3CmdMgr->regCmd("RUN",                3,    new V3MCRunCmd          )
   );
}

//----------------------------------------------------------------------
// READ PROperty <(string fileName)> <-Aiger | -Prop>
//----------------------------------------------------------------------
V3CmdExecStatus
V3MCReadPropertyCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool aiger = false, prop = false;
   string fileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Aiger", token, 2) == 0) {
         if (aiger || prop) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else aiger = true;
      }
      else if (v3StrNCmp("-Prop", token, 2) == 0) {
         if (aiger || prop) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else prop = true;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");
   if (!aiger && !prop) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-Aiger | -Prop>");

   if (aiger) {
      // Read Design and Properties From AIGER
      V3NetVec badList, invList; V3NetTable fairList; badList.clear(); invList.clear(); fairList.clear();
      V3NtkInput* const pNtk = readAiger(fileName, badList, invList, fairList, false);
      if (pNtk) {
         // Put Constraints to Primary Outputs
         V3Ntk* const ntk = pNtk->getNtk(); assert (ntk); assert (!ntk->getOutputSize());
         for (uint32_t i = 0; i < invList.size(); ++i) ntk->createOutput(invList[i]);
         for (uint32_t i = 0; i < fairList.size(); ++i)
            for (uint32_t j = 0; j < fairList[i].size(); ++j) ntk->createOutput(fairList[i][j]);
         // Record Safety Properties
         for (uint32_t i = 0; i < badList.size(); ++i) {
            V3LTLFormula* const f = new V3LTLFormula(pNtk, ~(badList[i]), true, "b" + v3Int2Str(i)); assert (f);
            V3Property* const p = new V3Property(f); assert (p); pNtk->setProperty(p);
            if (invList.size()) p->setInvConstr(pNtk, 0, invList.size() - 1);
         }
         // Record Liveness Properties
         for (uint32_t i = 0, k = invList.size(); i < fairList.size(); ++i) {
            V3LTLFormula* const f = new V3LTLFormula(pNtk, V3NetId::makeNetId(0), false, "j" + v3Int2Str(i)); assert (f);
            V3Property* const p = new V3Property(f); assert (p); pNtk->setProperty(p);
            if (invList.size()) p->setInvConstr(pNtk, 0, invList.size() - 1);
            if (fairList[i].size()) p->setFairConstr(pNtk, k, k + fairList[i].size() - 1); k += fairList[i].size();
         }
         v3Handler.pushAndSetCurHandler(pNtk);
      }
   }
   else {
      V3NtkHandler* const handler = v3Handler.getCurHandler();
      if (handler) {
         V3NtkHandler* const pNtk = readProperty(handler, fileName);
         if (pNtk) v3Handler.pushAndSetCurHandler(pNtk);
      }
      else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   }

   return CMD_EXEC_DONE;
}

void
V3MCReadPropertyCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: READ PROperty <(string fileName)> <-Aiger | -Prop>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name of the property input." << endl;
      Msg(MSG_IFO) << "       -Aiger           : Indicate fileName is an AIGER input." << endl;
      Msg(MSG_IFO) << "       -Prop            : Indicate fileName is a PROP input (build on top of current ntk)." << endl;
   }
}

void
V3MCReadPropertyCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "READ PROperty: " << "Read property specification from external file." << endl;
}

//----------------------------------------------------------------------
// WRITE PROperty <(string fileName)> <-Aiger | -Prop> [-L2S] [-C2P]
//----------------------------------------------------------------------
V3CmdExecStatus
V3MCWritePropertyCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool aiger = false, prop = false, l2s = false, c2p;
   string fileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Aiger", token, 2) == 0) {
         if (aiger || prop) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else aiger = true;
      }
      else if (v3StrNCmp("-Prop", token, 2) == 0) {
         if (aiger || prop) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else prop = true;
      }
      else if (v3StrNCmp("-L2S", token, 4) == 0) {
         if (l2s) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else l2s = true;
      }
      else if (v3StrNCmp("-C2P", token, 4) == 0) {
         if (c2p) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else c2p = true;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");
   if (!aiger && !prop) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-Aiger | -Prop>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->getPropertyList().size()) {
         if (prop) writeProperty(handler, fileName, l2s, c2p);
         else writeAiger(handler, fileName, l2s, c2p);
      }
      else Msg(MSG_ERR) << "NO Property Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;

   return CMD_EXEC_DONE;
}

void
V3MCWritePropertyCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRITE PROperty <(string fileName)> <-Aiger | -Prop>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name of the property output." << endl;
      Msg(MSG_IFO) << "       -Aiger           : Output network and properties into AIGER format." << endl;
      Msg(MSG_IFO) << "       -Prop            : Output properties into PROP format (in terms of current network)." << endl;
   }
}

void
V3MCWritePropertyCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRITE PROperty: " << "Write property specification into file." << endl;
}

//----------------------------------------------------------------------
// RUN <-TIMEout (unsigned maxTime)>
//     <-MEMoryout (unsigned maxMemory)>
//     <-THReadout (unsigned maxThread)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3MCRunCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool time = false, mem = false, thrd = false;
   bool timeON = false, memON = false, thrdON = false;
   uint32_t maxTime = 0, maxMem = 0, maxThrd = 0;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-TIMEout", token, 5) == 0) {
         if (time) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (memON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxMemory)");
         else if (thrdON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxThread)");
         else time = timeON = true;
      }
      else if (v3StrNCmp("-MEMoryout", token, 4) == 0) {
         if (mem) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (thrdON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxThread)");
         else mem = memON = true;
      }
      else if (v3StrNCmp("-THReadout", token, 4) == 0) {
         if (thrd) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (memON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxMemory)");
         else thrd = thrdON = true;
      }
      else if (timeON || memON || thrdON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (timeON) { maxTime = (uint32_t)temp; timeON = false; }
         else if (memON) { maxMem = (uint32_t)temp; memON = false; }
         else { maxThrd = (uint32_t)temp; thrdON = false; }
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (maxTime == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-TIMEout (unsigned maxTime)>");
   if (maxMem == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-MEMoryout (unsigned maxMemory)>");
   if (maxThrd == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-THReadout (unsigned maxThread)>");

   if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
   if (memON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxMemory)");
   if (thrdON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxThread)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->getPropertyList().size())
         startVerificationFromProp(handler, maxThrd, (double)maxTime, (double)maxMem);
      else Msg(MSG_ERR) << "NO Property Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;

   return CMD_EXEC_DONE;
}

void
V3MCRunCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: RUN <-TIMEout (unsigned maxTime)>" << endl;
   Msg(MSG_IFO) << "           <-MEMoryout (unsigned maxMemory)>" << endl;
   Msg(MSG_IFO) << "           <-THReadout (unsigned maxThread)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned maxTime)  : Wall Timeout limit in seconds." << endl;
      Msg(MSG_IFO) << "       (unsigned maxMemory): Memoryout limit in Mega Bytes." << endl;
      Msg(MSG_IFO) << "       (unsigned maxThread): The number of available CPU cores." << endl;
      Msg(MSG_IFO) << "       -TIMEout            : Indicate the next number is the timeout limit." << endl;
      Msg(MSG_IFO) << "       -MEMoryout          : Indicate the next number is the memoryout limit." << endl;
      Msg(MSG_IFO) << "       -THReadout          : Indicate the next number is the CPU core limit." << endl;
   }
}

void
V3MCRunCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "RUN: " << "Run V3 Model Checker." << endl;
}

#endif

