/****************************************************************************
  FileName     [ v3TransCmd.cpp ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Ntk Transformation Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_TRANS_CMD_C
#define V3_TRANS_CMD_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3TransCmd.h"
#include "v3BvBlastBv.h"
#include "v3BvBlastAig.h"
#include "v3NtkHandler.h"

#include <iomanip>

bool initTransCmd() {
   return (
         v3CmdMgr->regCmd("BLAst NTk",    3, 2, new V3BlastNtkCmd)   
   );
}

//----------------------------------------------------------------------
// BLAst NTk [-Primary]
//----------------------------------------------------------------------
V3CmdExecStatus
V3BlastNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool primary = false;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Primary", token, 2) == 0) {
         if (primary) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else primary = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (dynamic_cast<V3BvNtk*>(handler->getNtk())) {
         if (primary) {
            V3BvBlastBv* const blastHandler = new V3BvBlastBv(handler); assert (blastHandler);
            v3Handler.pushAndSetCurHandler(blastHandler);
         }
         else {
            V3BvBlastAig* const blastHandler = new V3BvBlastAig(handler); assert (blastHandler);
            v3Handler.pushAndSetCurHandler(blastHandler);
         }
      }
      else Msg(MSG_ERR) << "Current Network is Already an AIG Network !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3BlastNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: BLAst NTk [-Primary]" << endl;
   Msg(MSG_IFO) << "Param: [-Primary]: Bit-blast primary inputs, inouts, and latches only." << endl;
}

void
V3BlastNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "BLAst NTk: " << "Bit-blast Word-level Networks into Boolean-level Networks." << endl;
}

#endif

