/****************************************************************************
  FileName     [ v3AlgCmd.cpp ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ Alg Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_CMD_C
#define V3_ALG_CMD_C

#include "v3Msg.h"
#include "v3AlgCmd.h"
#include "v3AlgSim.h"
#include "v3ExtUtil.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"

#include <iomanip>

bool initAlgCmd() {
   return (
         v3CmdMgr->regCmd("SIM NTk",      3, 2, new V3SimNtkCmd   )  &&
         v3CmdMgr->regCmd("PLOt TRace",   3, 2, new V3PlotTraceCmd)
   );
}

//----------------------------------------------------------------------
// SIM NTk <(-Input <(string fileName)>) | (-Random <(unsigned patterns)>>
//         [-Output <(string outFileName)>] [-Event]
//----------------------------------------------------------------------
V3CmdExecStatus
V3SimNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   bool input = false, random = false, output = false, event = false;
   bool inputON = false, randomON = false, outputON = false;
   string fileName = "", outFileName = "";
   int patternSize = 0;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Input", token, 2) == 0) {
         if (input || random) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fileName)");
         else if (randomON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned patterns)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outFileName)");
         else input = inputON = true;
      }
      else if (v3StrNCmp("-Random", token, 2) == 0) {
         if (input || random) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fileName)");
         else if (randomON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned patterns)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outFileName)");
         else random = randomON = true;
      }
      else if (v3StrNCmp("-Output", token, 2) == 0) {
         if (output) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fileName)");
         else if (randomON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned patterns)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outFileName)");
         else output = outputON = true;
      }
      else if (inputON) {
         if (fileName == "") fileName = token;
         else return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         inputON = false;
      }
      else if (randomON) {
         if (!patternSize) {
            if (!v3Str2Int(token, patternSize)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned patterns)");
            else if (patternSize <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned patterns)");
         }
         else return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         randomON = false;
      }
      else if (outputON) {
         if (outFileName == "") outFileName = token;
         else return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         outputON = false;
      }
      else if (v3StrNCmp("-Event", token, 2) == 0) {
         if (event) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         event = true;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!(input || random)) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<-Input | -Random>");
   if (input && fileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fileName)>");
   if (random && patternSize == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned patterns)");
   if (output && outFileName == "") return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string outFileName)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      assert (handler->getNtk());
      if (!handler->getNtk()->getModuleSize()) {
         if (!random) performInputFileSimulation(handler, fileName, event, outFileName);
         else performRandomSimulation(handler, patternSize, event, outFileName);
      }
      else Msg(MSG_ERR) << "Simulation can only be performed on Flattened Ntk !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3SimNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: SIM NTk <(-Input <(string fileName)>) | (-Random <(unsigned patterns)>)>" << endl;
   Msg(MSG_IFO) << "               [-Output <(string outFileName)>] [-Event]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fileName)   : The file name of the input pattern file." << endl;
      Msg(MSG_IFO) << "       (unsigned patterns) : The number of patterns for random simulation." << endl;
      Msg(MSG_IFO) << "       (string outFileName): The file name for simulation result output." << endl;
      Msg(MSG_IFO) << "       -Event              : Enable event-driven simulation." << endl;
      Msg(MSG_IFO) << "       -Input              : Enable simulation from input patterns." << endl;
      Msg(MSG_IFO) << "       -Random             : Enable random simulation." << endl;
      Msg(MSG_IFO) << "       -Output             : Enable dumping simulation results into a file." << endl;
   }
}

void
V3SimNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "SIM NTk: " << "Simulate on Current Network." << endl;
}

//----------------------------------------------------------------------
// PLOt TRace <(string inputPatternFileName)> <(string outputFileName)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3PlotTraceCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   string fileName = "", outFileName = "";

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (fileName == "") fileName = token;
      else if (outFileName == "") outFileName = token;
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      assert (handler->getNtk());
      V3CexTrace* const trace = V3CexTraceParser(handler, fileName);
      if (trace) {
         V3CexTraceVisualizer(handler, trace, outFileName);
         delete trace;
      }
      else Msg(MSG_ERR) << "Simulation for the Pattern File \"" << fileName << "\" Failed !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PlotTraceCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PLOt TRace <(string inputPatternFileName)> <(string outputFileName)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string inputPatternFileName): The file name of the input pattern file." << endl;
      Msg(MSG_IFO) << "       (string outputFileName)      : The file name for simulation result output." << endl;
   }
}

void
V3PlotTraceCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PLOt TRace: " << "Plot simulation or counterexample traces." << endl;
}

#endif

