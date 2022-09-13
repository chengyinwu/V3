/****************************************************************************
  FileName     [ v3NtkCmd.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Ntk Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_CMD_C
#define V3_NTK_CMD_C

#include "v3Msg.h"
#include "v3NtkCmd.h"
#include "v3StrUtil.h"
#include "v3FileUtil.h"
#include "v3NtkInput.h"
#include "v3NtkParser.h"
#include "v3NtkWriter.h"

#include <iomanip>

bool initNtkCmd() {
   // Set 1'b0 and [0:0] 0 to V3BVXId and V3BusId, respectively
   const V3BVXId zeroId = V3BvNtk::hashV3ConstBitVec("1'b0"); assert (!zeroId);
   const V3BusId boolId = V3BvNtk::hashV3BusId(0, 0); assert (!boolId);
   // Reset Verbosity Settings
   V3NtkHandler::resetVerbositySettings();
   return (
         // Network Input Commands
         v3CmdMgr->regCmd("REAd Rtl",           3, 1, new V3ReadRTLCmd        ) &&
         // Network Output Commands
         v3CmdMgr->regCmd("WRIte Aig",          3, 1, new V3WriteAIGCmd       )
   );
}

//----------------------------------------------------------------------
// REAd Rtl <(string fileName)> [-Filelist] [-FLAtten]
//          [| -QuteRTL | -Primitive]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ReadRTLCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   bool quteRTL = false, primitive = false;
   bool fileList = false, flatten = false;
   string fileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-QuteRTL", token, 2) == 0) {
         if (quteRTL || primitive) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else quteRTL = true;
      }
      else if (v3StrNCmp("-Primitive", token, 2) == 0) {
         if (quteRTL || primitive) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else primitive = true;
      }
      else if (v3StrNCmp("-FLAtten", token, 4) == 0) {
         if (flatten) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else flatten = true;
      }
      else if (v3StrNCmp("-Filelist", token, 2) == 0) {
         if (fileList) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else fileList = true;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");
   if (!(quteRTL || primitive)) quteRTL = true;

   V3NtkInput* inputHandler = 0;
   if (primitive) inputHandler = V3RTLParser(fileName.c_str());
   else inputHandler = V3NtkFromQuteRTL(fileName.c_str(), fileList, flatten);
   
   if (!inputHandler) Msg(MSG_ERR) << "Parse Failed !!" << endl;
   else v3Handler.pushAndSetCurHandler(inputHandler);
   return CMD_EXEC_DONE;
}

void
V3ReadRTLCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: REAd Rtl <(string fileName)> [-Filelist] [-FLAtten] [| -QuteRTL | -Primitive]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name of the input Verilog design or a list design files." << endl;
      Msg(MSG_IFO) << "       -Filelist        : Indicate fileName is a list of design files." << endl;
      Msg(MSG_IFO) << "       -QuteRTL         : Use QuteRTL RTL front-end for design parsing and synthesis." << endl;
      Msg(MSG_IFO) << "       -Primitive       : Use V3 Primitive RTL front-end for design parsing and synthesis." << endl;
      Msg(MSG_IFO) << "       -FLAtten         : Flatten the design after parsing. (only available to -QuteRTL)" << endl;
   }
}

void
V3ReadRTLCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "REAd Rtl: " << "Read RTL (Verilog) Designs." << endl;
}

//----------------------------------------------------------------------
// WRIte Aig <(string fileName)> [-Symbol]
//----------------------------------------------------------------------
V3CmdExecStatus
V3WriteAIGCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   bool symbol = false;
   string fileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Symbol", token, 2) == 0) {
         if (symbol) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else symbol = true;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (!dynamic_cast<const V3BvNtk*>(handler->getNtk())) V3AigWriter(handler, fileName.c_str(), symbol);
      else Msg(MSG_ERR) << "Current Network is NOT an AIG Ntk !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WriteAIGCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRIte Aig <(string fileName)> [-Symbol]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name for the AIGER output." << endl;
      Msg(MSG_IFO) << "       -Symbol          : Enable using signal names specified in the input design." << endl;
   }
}

void
V3WriteAIGCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRIte Aig: " << "Write AIGER Designs." << endl;
}

#endif

