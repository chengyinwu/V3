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
#include "v3NtkSimplify.h"

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
         v3CmdMgr->regCmd("REAd Aig",           3, 1, new V3ReadAIGCmd        ) &&
         v3CmdMgr->regCmd("REAd Btor",          3, 1, new V3ReadBTORCmd       ) &&
         // Network Output Commands
         v3CmdMgr->regCmd("WRIte Rtl",          3, 1, new V3WriteRTLCmd       ) &&
         v3CmdMgr->regCmd("WRIte Aig",          3, 1, new V3WriteAIGCmd       ) &&
         v3CmdMgr->regCmd("WRIte Btor",         3, 1, new V3WriteBTORCmd      ) &&
         // Network Report Commands
         v3CmdMgr->regCmd("PRInt NTk",          3, 2, new V3PrintNtkCmd       ) &&
         v3CmdMgr->regCmd("PRInt NEt",          3, 2, new V3PrintNetCmd       ) &&
         v3CmdMgr->regCmd("PLOt NTk",           3, 2, new V3PlotNtkCmd        ) &&
         // Network Hierarchy Manipulation Commands
         v3CmdMgr->regCmd("@CD",                3,    new V3SetNtkCmd         ) &&
         v3CmdMgr->regCmd("@LS",                3,    new V3ListNtkCmd        ) &&
         v3CmdMgr->regCmd("@PWD",               4,    new V3WorkNtkCmd        ) &&
         v3CmdMgr->regCmd("@LN",                3,    new V3RefNtkCmd         ) &&
         // Network Simplification Commands
         v3CmdMgr->regCmd("STRash NTk",         3, 2, new V3StrashNtkCmd      ) &&
         v3CmdMgr->regCmd("REDuce NTk",         3, 2, new V3ReduceNtkCmd      ) &&
         v3CmdMgr->regCmd("REWrite NTk",        3, 2, new V3RewriteNtkCmd     ) &&
         v3CmdMgr->regCmd("DUPlicate NTk",      3, 2, new V3DuplicateNtkCmd   ) &&
         // Network Verbosity Setting Commands
         v3CmdMgr->regCmd("SET NTKVerbosity",   3, 4, new V3SetVerbosityCmd   ) &&
         v3CmdMgr->regCmd("PRInt NTKVerbosity", 3, 4, new V3PrintVerbosityCmd )
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
// REAd Aig <(string fileName)> [-Symbol]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ReadAIGCmd::exec(const string& option) {
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

   V3NtkInput* inputHandler = V3AigParser(fileName.c_str(), symbol);
   
   if (!inputHandler) Msg(MSG_ERR) << "Parse Failed !!" << endl;
   else v3Handler.pushAndSetCurHandler(inputHandler);

   return CMD_EXEC_DONE;
}

void
V3ReadAIGCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: REAd Aig <(string fileName)> [-Symbol]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name of the input AIGER design." << endl;
      Msg(MSG_IFO) << "       -Symbol          : Enable reading of symbolic tables." << endl;
   }
}

void
V3ReadAIGCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "REAd Aig: " << "Read AIGER Designs." << endl;
}

//----------------------------------------------------------------------
// REAd Btor <(string fileName)> [-Symbol]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ReadBTORCmd::exec(const string& option) {
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

   V3NtkInput* inputHandler = V3BtorParser(fileName.c_str(), symbol);
   
   if (!inputHandler) Msg(MSG_ERR) << "Parse Failed !!" << endl;
   else v3Handler.pushAndSetCurHandler(inputHandler);
   return CMD_EXEC_DONE;
}

void
V3ReadBTORCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: REAd BTOR <(string fileName)> [-Symbol]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name of the input BTOR design." << endl;
      Msg(MSG_IFO) << "       -Symbol          : Enable the reading of symbols (i.e. name of vars)." << endl;
   }
}

void
V3ReadBTORCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "REAd Btor: " << "Read BTOR Designs." << endl;
}

//----------------------------------------------------------------------
// WRIte Rtl <(string fileName)> [-Symbol] [-Initial]
//----------------------------------------------------------------------
V3CmdExecStatus
V3WriteRTLCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   bool symbol = false, initial = false;
   string fileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Symbol", token, 2) == 0) {
         if (symbol) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else symbol = true;
      }
      else if (v3StrNCmp("-Initial", token, 2) == 0) {
         if (initial) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else initial = true;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) V3RTLWriter(handler, fileName.c_str(), symbol, initial);
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WriteRTLCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRIte Rtl <(string fileName)> [-Symbol]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name for the RTL output." << endl;
      Msg(MSG_IFO) << "       -Symbol          : Enable using signal names specified in the input design." << endl;
   }
}

void
V3WriteRTLCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRIte Rtl: " << "Write RTL (Verilog) Designs." << endl;
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

//----------------------------------------------------------------------
// WRIte Btor <(string fileName)> [-Symbol]
//----------------------------------------------------------------------
V3CmdExecStatus
V3WriteBTORCmd::exec(const string& option) {
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
      if (dynamic_cast<const V3BvNtk*>(handler->getNtk())) V3BtorWriter(handler, fileName.c_str(), symbol);
      else Msg(MSG_ERR) << "Current Network is NOT a BV Ntk !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WriteBTORCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRIte Btor <(string fileName)> [-Symbol]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name for the BTOR output." << endl;
      Msg(MSG_IFO) << "       -Symbol          : Enable using signal names specified in the input design." << endl;
   }
}

void
V3WriteBTORCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRIte Btor: " << "Write BTOR Network." << endl;
}

//----------------------------------------------------------------------
// PRInt NTk [| -Summary | -Primary | -Verbose | -Netlist | 
//              -CombLoops | -Floating | -Unreachable]
//----------------------------------------------------------------------
V3CmdExecStatus
V3PrintNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool summary = false, primary = false, verbose = false, netlist = false;
   bool combLoops = false, floating = false, unreachable = false;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Summary", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else summary = true;
      }
      else if (v3StrNCmp("-Primary", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else primary = true;
      }
      else if (v3StrNCmp("-Verbose", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else verbose = true;
      }
      else if (v3StrNCmp("-Netlist", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else netlist = true;
      }
      else if (v3StrNCmp("-CombLoops", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else combLoops = true;
      }
      else if (v3StrNCmp("-Floating", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else floating = true;
      }
      else if (v3StrNCmp("-Unreachable", token, 2) == 0) {
         if (summary || primary || verbose || netlist || combLoops || floating || unreachable) 
            return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else unreachable = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(summary || primary || verbose || netlist || combLoops || floating || unreachable)) summary = true;

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (verbose) handler->printVerbose();
      else if (primary) handler->printPrimary();
      else if (netlist) handler->printNetlist();
      else if (combLoops) handler->printCombLoops();
      else if (floating) handler->printFloatings();
      else if (unreachable) handler->printUnreachables();
      else handler->printSummary();
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PrintNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PRInt NTk [| -Summary | -Primary | -Verbose | -Netlist]" << endl;
   Msg(MSG_IFO) << "                    -CombLoops | -Floating | -Unreachable]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: -Summary: Print network summary." << endl;
      Msg(MSG_IFO) << "       -Primary: Print primary ports." << endl;
      Msg(MSG_IFO) << "       -Verbose: Print statistics of gates." << endl;
      Msg(MSG_IFO) << "       -Netlist: Print network topology." << endl;
   }
}

void
V3PrintNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PRInt NTk: " << "Print Network Information." << endl;
}

//----------------------------------------------------------------------
// PRInt NEt <(unsigned netId)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3PrintNetCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   string netIdStr = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (netIdStr == "") netIdStr = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (netIdStr == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned netId)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      bool inv = false; int value;
      if ('~' == netIdStr[0]) { netIdStr = netIdStr.substr(1); inv = true; }
      if (v3Str2Int(netIdStr, value) && value >= 0) {
         if (handler->getNtk()->getNetSize() > (uint32_t)value) 
            handler->printNet(V3NetId::makeNetId(value, inv));
         else Msg(MSG_ERR) << "Net with Id = " << value << " NOT Found !!" << endl;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, netIdStr);
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PrintNetCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PRInt NEt <(unsigned netId)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned netId): The index of a net to be reported." << endl;
   }
}

void
V3PrintNetCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PRInt NEt: " << "Print Net Information." << endl;
}

//----------------------------------------------------------------------
// PLOt NTk [| -DOT | -PNG | -PS] <(string fileName)>
//          <(-Level | -Depth) (unsigned size)> [-Monochrome]
//----------------------------------------------------------------------
V3CmdExecStatus
V3PlotNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool dot = false, png = false, ps = false;
   bool level = false, depth = false, inSize = false;
   bool monochrome = false;
   string fileName = ""; int size = 0;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-DOT", token, 4) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else if (dot || png || ps) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else dot = true;
      }
      else if (v3StrNCmp("-PNG", token, 4) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else if (dot || png || ps) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else png = true;
      }
      else if (v3StrNCmp("-PS", token, 3) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else if (dot || png || ps) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else ps = true;
      }
      else if (v3StrNCmp("-Level", token, 2) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else if (level || depth) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else level = inSize = true;
      }
      else if (v3StrNCmp("-Depth", token, 2) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else if (level || depth) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else depth = inSize = true;
      }
      else if (v3StrNCmp("-Monochrome", token, 2) == 0) {
         if (inSize) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
         else monochrome = true;
      }
      else if (inSize) {
         if (!v3Str2Int(token, size)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned size)");
         if (size <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned size)");
         inSize = false;
      }
      else if (fileName == "") fileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if ("" == fileName) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fileName)");
   if (!size) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned size)");
   if (!(dot || png || ps)) dot = true;

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      // Set Outputs
      V3NetVec outputNets; outputNets.reserve(handler->getNtk()->getOutputSize());
      for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i) 
         outputNets.push_back(handler->getNtk()->getOutput(i));
      if (level) V3PlotNtkByLevel(handler, fileName.c_str(), size, outputNets, monochrome);
      else V3PlotNtkByDepth(handler, fileName.c_str(), size, outputNets, monochrome);
      if (png) dotToPng(fileName);
      else if (ps) dotToPs(fileName);
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PlotNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PLOt NTk [| -DOT | -PNG | -PS] <(string fileName)>" << endl;
   Msg(MSG_IFO) << "                <(-Level | -Depth) (unsigned size)> [-Monochrome]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName): The file name for output." << endl;
      Msg(MSG_IFO) << "       (unsigned size)  : The number of levels or depths to be reported." << endl;
      Msg(MSG_IFO) << "       -Monochrome      : Plot with only black and white colors." << endl;
      Msg(MSG_IFO) << "       -Level           : Enable plotting networks under a specified number of levels." << endl;
      Msg(MSG_IFO) << "       -Depth           : Enable plotting networks under a specified number of time-frames." << endl;
      Msg(MSG_IFO) << "       -DOT             : Output into *.dot files." << endl;
      Msg(MSG_IFO) << "       -PNG             : Output into *.png files (an executable dot required)." << endl;
      Msg(MSG_IFO) << "       -PS              : Output into *.ps files (an executable dot required)." << endl;
   }
}

void
V3PlotNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PLOt NTk: " << "Plot Network Topology." << endl;
}

//----------------------------------------------------------------------
// @CD [/ | - | . | .. | (Path Format)]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool cdRoot = false, cdLast = false, cdCur = false, cdPrev = false;
   string path = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if ("/" == token) {
         if (cdRoot || cdLast || cdCur || cdPrev || path.size()) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cdRoot = true;
      }
      else if ("-" == token) {
         if (cdRoot || cdLast || cdCur || cdPrev || path.size()) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cdLast = true;
      }
      else if ("." == token) {
         if (cdRoot || cdLast || cdCur || cdPrev || path.size()) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cdCur = true;
      }
      else if (".." == token) {
         if (cdRoot || cdLast || cdCur || cdPrev || path.size()) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cdPrev = true;
      }
      else if (!path.size()) path = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (cdRoot) v3Handler.setRootHandler();  // Change to Root Ntk
      else if (cdLast) v3Handler.setLastHandler();  // Change to Last Ntk
      else if (cdPrev) v3Handler.setPrevHandler();  // Change to Topper Ntk  (Stall if current Ntk is the Base Ntk)
      else if (path.size()) v3Handler.setCurHandlerFromPath(path);  // Change to Ntk with specified path
      else if (!cdCur) v3Handler.setBaseHandler();  // Change to Base (Top) Ntk
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3SetNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: @CD [/ | .. | (Path Format)]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: Path Format: [/(unsigned ntkID)][/(unsigned subModuleIndex)]*" << endl;
      Msg(MSG_IFO) << "       e.g. @CD 0/1 will change Ntk to be the second submodule of the first submodule." << endl;
      Msg(MSG_IFO) << "       e.g. @CD /5/1 will change Ntk to be the second submodule of Ntk with id = 5." << endl;
   }
}

void
V3SetNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "@CD: " << "Change Design for Current Network." << endl;
}

//----------------------------------------------------------------------
// @LS [(unsigned level)]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ListNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   uint32_t level = 0;
   
   if (options.size() > 1) return V3CmdExec::errorOption(CMD_OPT_EXTRA, options[1]);
   else if (options.size()) {
      const string& token = options[0]; int temp;
      if (!v3Str2Int(token, temp) || (temp <= 0)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
      level = (uint32_t)temp; --level;
   }

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) v3Handler.printRecurHierarchy(handler, level, 0);
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ListNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: @LS [(unsigned level)]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned level): The number of levels of instances to be printed. (default = 1)" << endl;
   }
}

void
V3ListNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "@LS: " << "List Network Instances of Current Network." << endl;
}

//----------------------------------------------------------------------
// @PWD [| -Base | -Root ]
//----------------------------------------------------------------------
V3CmdExecStatus
V3WorkNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool base = false, root = false;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Base", token, 2) == 0) {
         if (root || base) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else base = true;
      }
      else if (v3StrNCmp("-Root", token, 2) == 0) {
         if (root || base) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else root = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(base || root)) base = true;

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (base) v3Handler.printNtkInHierarchy();
      else v3Handler.printNtkInAncestry();
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WorkNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: @PWD" << endl;
}

void
V3WorkNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "@PWD: " << "Print the Path of Current Network." << endl;
}

//----------------------------------------------------------------------
// @LN <(unsigned ntkID)> <(unsigned subModuleIndex)> [-Blackbox]
//----------------------------------------------------------------------
V3CmdExecStatus
V3RefNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool isBlackBox = false;
   uint32_t ntkId = V3NtkUD, index = V3NtkUD;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Blackbox", token, 2) == 0) {
         if (isBlackBox) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else isBlackBox = true;
      }
      else if (V3NtkUD == ntkId) {
         int temp;
         if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkID)");
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkID)");
         ntkId = temp;
      }
      else if (V3NtkUD == index) {
         int temp;
         if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned subModuleIndex)");
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned subModuleIndex)");
         index = temp;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (V3NtkUD == ntkId) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned ntkID)");
   if (V3NtkUD == index) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned subModuleIndex)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (ntkId >= v3Handler.getHandlerCount()) 
         Msg(MSG_ERR) << "Ntk with ID = " << ntkId << " Does NOT Exist !!" << endl;
      else if (index >= handler->getNtk()->getModuleSize()) 
         Msg(MSG_ERR) << "Ntk with ID = " << ntkId << " has NO Sub-Module with Index = " << index << " !!" << endl;
      else {
         assert (handler->getNtk()->getModule(index)); assert (v3Handler.getHandler(ntkId));
         updateModule(handler->getNtk(), handler->getNtk()->getModule(index), v3Handler.getHandler(ntkId), isBlackBox);
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3RefNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: @LN <(unsigned ntkID)> <(unsigned subModuleIndex)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned ntkID)         : The index of a network." << endl;
      Msg(MSG_IFO) << "       (unsigned subModuleIndex): The index of an instance of current network." << endl;
   }
}

void
V3RefNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "@LN: " << "Link a Network with an instance of Current Network." << endl;
}

//----------------------------------------------------------------------
// STRash NTk
//----------------------------------------------------------------------
V3CmdExecStatus
V3StrashNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NtkHandler::setExternalVerbosity(); V3NtkHandler::setStrash(true);
      v3Handler.pushAndSetCurHandler(new V3NtkSimplify(handler));
      V3NtkHandler::setInternalVerbosity();
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3StrashNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: STRash NTk" << endl;
}

void
V3StrashNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "STRash NTk: " << "Perform Structural Hashing on Current Network." << endl;
}

//----------------------------------------------------------------------
// REDuce NTk
//----------------------------------------------------------------------
V3CmdExecStatus
V3ReduceNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NtkHandler::setExternalVerbosity(); V3NtkHandler::setReduce(true);
      v3Handler.pushAndSetCurHandler(new V3NtkSimplify(handler));
      V3NtkHandler::setInternalVerbosity();
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ReduceNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: REDuce NTk" << endl;
}

void
V3ReduceNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "REDuce NTk: " << "Perform COI Reduction on Current Network." << endl;
}

//----------------------------------------------------------------------
// REWrite NTk
//----------------------------------------------------------------------
V3CmdExecStatus
V3RewriteNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NtkHandler::setExternalVerbosity(); V3NtkHandler::setRewrite(true);
      v3Handler.pushAndSetCurHandler(new V3NtkSimplify(handler));
      V3NtkHandler::setInternalVerbosity();
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3RewriteNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: REWrite NTk" << endl;
}

void
V3RewriteNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "REWrite NTk: " << "Perform Rule-based Rewriting on Current Network." << endl;
}

//----------------------------------------------------------------------
// DUPlicate NTk
//----------------------------------------------------------------------
V3CmdExecStatus
V3DuplicateNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NtkSimplify* const duplicatedNtk = new V3NtkSimplify(handler);
      assert (handler); v3Handler.pushAndSetCurHandler(duplicatedNtk);
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3DuplicateNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: DUPlicate NTk" << endl;
}

void
V3DuplicateNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "DUPlicate NTk: " << "Duplicate Current Ntk from Verbosity Settings." << endl;
}

//----------------------------------------------------------------------
// SET NTKVerbosity [-All] [-REDuce] [-Strash] [-REWrite]
//                  [-Fwd-map] [-Bwd-map] [-ON |-OFF |-RESET]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SetVerbosityCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool all = false, reduce = false, strash = false, rewrite = false, fwdMap = false, bwdMap = false;
   bool reset = false, on = false, off = false;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-All", token, 2) == 0) {
         if (all) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else all = true;
      }
      else if (v3StrNCmp("-REDuce", token, 4) == 0) {
         if (reduce) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else reduce = true;
      }
      else if (v3StrNCmp("-Strash", token, 2) == 0) {
         if (strash) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else strash = true;
      }
      else if (v3StrNCmp("-REWrite", token, 4) == 0) {
         if (rewrite) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else rewrite = true;
      }
      else if (v3StrNCmp("-Fwd-map", token, 2) == 0) {
         if (fwdMap) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else fwdMap = true;
      }
      else if (v3StrNCmp("-Bwd-map", token, 2) == 0) {
         if (bwdMap) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else bwdMap = true;
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
   if (all) reduce = strash = rewrite = fwdMap = bwdMap = true;

   // Set Verification Verbosities
   if (reset) V3NtkHandler::resetVerbositySettings();
   else {
      if (reduce)  V3NtkHandler::setReduce (on);
      if (strash)  V3NtkHandler::setStrash (on);
      if (rewrite) V3NtkHandler::setRewrite(on);
      if (fwdMap)  V3NtkHandler::setP2CMap (on);
      if (bwdMap)  V3NtkHandler::setC2PMap (on);
   }
   // Print Verification Report Verbosities
   V3PrintVerbosityCmd printNtkVerbosity; return printNtkVerbosity.exec("");
}

void
V3SetVerbosityCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SET NTKVerbosity [-All] [-REDuce] [-Strash] [-REWrite] [-Fwd-map] [-Bwd-map]" << endl;
   Msg(MSG_IFO) << "                        [-ON |-OFF |-RESET]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: -RESET  : Reset everything to default." << endl;
      Msg(MSG_IFO) << "       -ON     : Turn specified attributes on." << endl;
      Msg(MSG_IFO) << "       -OFF    : Turn specified attributes off." << endl;
      Msg(MSG_IFO) << "       -All    : Toggle all the following attributes." << endl;
      Msg(MSG_IFO) << "       -REDuce : Toggle COI Reduction. (default = on)" << endl;
      Msg(MSG_IFO) << "       -Strash : Toggle Structural Hashing. (default = off)" << endl;
      Msg(MSG_IFO) << "       -REWrite: Toggle Rule-based Rewriting. (default = off)" << endl;
      Msg(MSG_IFO) << "       -Fwd-map: Toggle Preservation of Forward (to Sucessor) ID Maps. (default = on)" << endl;
      Msg(MSG_IFO) << "       -Bwd-map: Toggle Preservation of Backward (to Ancestor) ID Maps. (default = on)" << endl;
   }
}

void
V3SetVerbosityCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SET NTKVerbosity: " << "Set Verbosities for Network Duplication." << endl;
}

//----------------------------------------------------------------------
// PRInt NTKVerbosity
//----------------------------------------------------------------------
V3CmdExecStatus
V3PrintVerbosityCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   if (options.size()) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, options[0]);
   Msg(MSG_IFO) << "Ntk Verbosity Setting : \"";
   V3NtkHandler::printVerbositySettings();
   Msg(MSG_IFO) << "\" ON" << endl;
   return CMD_EXEC_DONE;
}

void
V3PrintVerbosityCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PRInt NTKVerbosity" << endl;
}

void
V3PrintVerbosityCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PRInt NTKVerbosity: " << "Print Verbosities for Network Duplication." << endl;
}

#endif

