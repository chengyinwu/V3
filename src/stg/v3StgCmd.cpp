/****************************************************************************
  FileName     [ v3StgCmd.cpp ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ State-Transition-Graph Commands ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_CMD_C
#define V3_STG_CMD_C

#include "v3Msg.h"
#include "v3StgCmd.h"
#include "v3StrUtil.h"
#include "v3FileUtil.h"
#include "v3NtkWriter.h"
#include "v3StgExtract.h"
#include "v3NtkElaborate.h"

#include <iomanip>

bool initStgCmd() {
   return (
         // FSM Extraction Commands
         v3CmdMgr->regCmd("ELAborate FSM",      3, 3, new V3ElaborateFSMCmd) &&
         v3CmdMgr->regCmd("EXTract FSM",        3, 3, new V3ExtractFSMCmd  ) &&
         v3CmdMgr->regCmd("WRIte FSM",          3, 3, new V3WriteFSMCmd    ) &&
         v3CmdMgr->regCmd("PLOT FSM",           4, 3, new V3PlotFSMCmd     )
   );
}

//----------------------------------------------------------------------
// ELAborate FSM [-Input (string inputName)] [-Name <(string fsmName)>]
//               [-Output <(string outputIndex)>] [-CONFIRM]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ElaborateFSMCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool name = false, nameON = false;
   bool input = false, inputON = false;
   bool output = false, outputON = false;
   bool confirm = false;
   string fsmName = "", inputName = "", sdgFileName = "";
   uint32_t outputIndex = V3NtkUD;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Input", token, 2) == 0) {
         if (input) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else input = inputON = true;
      }
      else if (v3StrNCmp("-Name", token, 2) == 0) {
         if (name) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string inputName)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else name = nameON = true;
      }
      else if (v3StrNCmp("-Output", token, 2) == 0) {
         if (output) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string inputName)");
         else output = outputON = true;
      }
      else if (v3StrNCmp("-CONFIRM", token, 8) == 0) {
         if (confirm) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string inputName)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else confirm  = true;
      }
      else if (nameON) { fsmName = token; nameON = false; }
      else if (inputON) { inputName = token; inputON = false; }
      else if (outputON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         outputIndex = (uint32_t)temp; outputON = false;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fsmName)>");
   if (inputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string inputName)");
   if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string outputIndex)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      V3NetVec terminal; terminal.clear(); V3FSMExtract* fsmExtract = 0;
      if (output) {  // Extract (Property) Directed FSM
         if (outputIndex < handler->getNtk()->getOutputSize())
            terminal.push_back(handler->getNtk()->getOutput(outputIndex));
         else Msg(MSG_ERR) << "Output with Index " << outputIndex << " does NOT Exist in Current Ntk !!" << endl;
      }
      if (input) {
         if (v3ExistDir(inputName.c_str())) fsmExtract = new V3FSMExtract(handler, inputName, terminal);
         else Msg(MSG_ERR) << "Input Specification File " << inputName << " does NOT Exist !!" << endl;
      }
      else {
         V3NetVec outputs = terminal;
         if (!outputs.size())
            for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i)
               outputs.push_back(handler->getNtk()->getOutput(i));
         fsmExtract = new V3FSMExtract(handler, outputs, terminal);
      }
      if (fsmExtract && fsmExtract->getFSMSDGDBSize()) {
         if (!fsmName.size()) fsmName = fsmExtract->getNtkHandler()->getAuxFSMName();
         if (fsmExtract->getNtkHandler()->existFSM(fsmName))
            Msg(MSG_ERR) << "FSM or Constraint has Already Exists !!" << endl;
         else if (!fsmExtract->getNtkHandler()->isLegalFSMName(fsmName))
            Msg(MSG_ERR) << "Illegal Name for FSM !!" << endl;
         else {
            assert (fsmExtract->getNtkHandler());
            // Start Extracting FSMs
            fsmExtract->startExtractFSMs();
            // Confirm if Required
            if (confirm) fsmExtract->checkCompleteFSMs();
            // Add to constraint data base
            if (fsmExtract->getFSMListSize()) {
               assert (fsmExtract->getNtkHandler()->getNtk());
               v3Handler.pushAndSetCurHandler(fsmExtract->getNtkHandler());
               v3Handler.getCurHandler()->setFSM(fsmName, fsmExtract);
               Msg(MSG_IFO) << "FSM " << fsmName;
               if (fsmExtract->getFSMListSize() > 1) Msg(MSG_IFO) << "[" << fsmExtract->getFSMListSize() - 1 << ":0]";
               Msg(MSG_IFO) << " has been extracted successfully !!" << endl;
            }
            else {
               delete fsmExtract->getNtkHandler(); delete fsmExtract;
               Msg(MSG_IFO) << "NO FSM Found or All Extracted FSMs are Disposed!!" << endl;
            }
         }
      }
      else Msg(MSG_ERR) << "Failed to Extract FSM !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ElaborateFSMCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: ELAborate FSM <-Input (string inputName)> [-Name <(string fsmName)>]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string inputName): The name of FSM specification input file." << endl;
      Msg(MSG_IFO) << "       (string fsmName)  : The name of FSMs to be extracted." << endl;
      Msg(MSG_IFO) << "       -Input            : Indicate that the following token is the name for the input file." << endl;
      Msg(MSG_IFO) << "       -Name             : Indicate that the following token is the name for FSMs." << endl;
   }
}

void
V3ElaborateFSMCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "ELAborate FSM: "
                                    << "Elaborate Network and Construct FSM from Input Specification." << endl;
}

//----------------------------------------------------------------------
// EXTract FSM [-Name <(string fsmName)>] [-Output <(string outputIndex)>]
//             [-Time <(unsigned maxTime)>] [-SCC | -NONE | -INVdirected]
//             [-CONFIRM] [-PLOTSDG <(string sdgFileName)>]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ExtractFSMCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   bool scc = false, none = false, invD = false;
   bool name = false, nameON = false;
   bool output = false, outputON = false;
   bool time = false, timeON = false;
   bool plotSDG = false, plotSDGON = false;
   bool confirm = false; uint32_t maxTime = 0;
   string fsmName = "", sdgFileName = "";
   uint32_t outputIndex = V3NtkUD;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-SCC", token, 4) == 0) {
         if (scc || none || invD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else scc = true;
      }
      else if (v3StrNCmp("-NONE", token, 5) == 0) {
         if (scc || none || invD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else none = true;
      }
      else if (v3StrNCmp("-INVdirected", token, 5) == 0) {
         if (scc || none || invD) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else invD = true;
      }
      else if (v3StrNCmp("-CONFIRM", token, 8) == 0) {
         if (confirm) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else confirm  = true;
      }
      else if (v3StrNCmp("-Name", token, 2) == 0) {
         if (name) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else name = nameON = true;
      }
      else if (v3StrNCmp("-Time", token, 2) == 0) {
         if (time) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else time = timeON = true;
      }
      else if (v3StrNCmp("-Output", token, 2) == 0) {
         if (output) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");
         else output = outputON = true;
      }
      else if (v3StrNCmp("-PLOTSDG", token, 8) == 0) {
         if (plotSDG) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string fsmName)");
         else if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned maxTime)");
         else if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string outputIndex)");
         else plotSDG = plotSDGON = true;
      }
      else if (nameON) { fsmName = token; nameON = false; }
      else if (timeON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         maxTime = (uint32_t)temp; timeON = false;
      }
      else if (outputON) {
         int temp; if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         outputIndex = (uint32_t)temp; outputON = false;
      }
      else if (plotSDGON) { sdgFileName = token; plotSDGON = false; }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (nameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fsmName)>");
   if (timeON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(unsigned maxTime)>");
   if (outputON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string outputIndex)>");
   if (plotSDGON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string sdgFileName)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (!fsmName.size()) fsmName = handler->getAuxFSMName();
      if (handler->existFSM(fsmName))
         Msg(MSG_ERR) << "FSM or Constraint has Already Exists !!" << endl;
      else if (!handler->isLegalFSMName(fsmName))
         Msg(MSG_ERR) << "Illegal Name for FSM !!" << endl;
      else {
         V3NetVec terminal; terminal.clear();
         V3FSMExtract* fsmExtract = 0;
         if (output) {
            if (outputIndex < handler->getNtk()->getOutputSize()) {  // Extract (Property) Directed FSM
               terminal.push_back(handler->getNtk()->getOutput(outputIndex));
               fsmExtract = new V3FSMExtract(handler, terminal);
            }
            else Msg(MSG_ERR) << "Output with Index " << outputIndex << " does NOT Exist in Current Ntk !!" << endl;
         }
         else {  // Extract Network FSM
            terminal.reserve(handler->getNtk()->getOutputSize() + handler->getNtk()->getLatchSize());
            for (uint32_t i = 0; i < handler->getNtk()->getOutputSize(); ++i) 
               terminal.push_back(handler->getNtk()->getOutput(i));
            for (uint32_t i = 0; i < handler->getNtk()->getLatchSize(); ++i) 
               terminal.push_back(handler->getNtk()->getLatch(i));
            fsmExtract = new V3FSMExtract(handler, terminal); terminal.clear();
         }
         if (fsmExtract) {
            // Set FSM Extraction Configurations
            if (scc) fsmExtract->setVariableClusterBySCC();
            else if (invD) fsmExtract->setInvariantDirectedCluster();
            else if (!none) fsmExtract->enableConstructStandardFSM();
            if (time) fsmExtract->setMaxTimeUsage((double)maxTime);
            // Set Currently User-Uncontrollable Configurations
            fsmExtract->filterOutNoMuxVariable();
            fsmExtract->filterOutNonSeqVariable();
            fsmExtract->enableOmitNodesAllCombVars();
            //fsmExtract->enableOmitNodesIndepToVars();
            fsmExtract->enableRenderNonEmptyStates();
            //fsmExtract->enableStopOnAllInitOrTerms();
            // Set Terminal States if Property is Specified
            for (uint32_t i = 0; i < terminal.size(); ++i) fsmExtract->pushTerminalState(terminal[i]);
            // Set Maximum State Size for Each FSM
            fsmExtract->setMaxStateSize(50);
            // Start Extracting FSMs
            fsmExtract->startExtractFSMs();
            // Confirm if Required
            if (confirm) fsmExtract->checkCompleteFSMs();
            // Output Figures if Required
            if (plotSDG) fsmExtract->writeClusterResult(sdgFileName);
            // Add to constraint data base
            if (fsmExtract->getFSMListSize()) {
               handler->setFSM(fsmName, fsmExtract);
               Msg(MSG_IFO) << "FSM " << fsmName;
               if (fsmExtract->getFSMListSize() > 1) Msg(MSG_IFO) << "[" << fsmExtract->getFSMListSize() - 1 << ":0]";
               Msg(MSG_IFO) << " has been extracted successfully !!" << endl;
            }
            else {
               delete fsmExtract;
               Msg(MSG_IFO) << "NO FSM Found or All Extracted FSMs are Disposed!!" << endl;
            }
         }
         else Msg(MSG_ERR) << "Failed to Extract FSM !!" << endl;
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ExtractFSMCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: EXTract FSM [-Name <(string fsmName)>] [-SCC | -NONE]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fsmName): The name of FSMs to be extracted." << endl;
      Msg(MSG_IFO) << "       -Name           : Indicate that the following token is the name for FSMs." << endl;
      Msg(MSG_IFO) << "       -SCC            : Cluster variables by strongly connected components." << endl;
      Msg(MSG_IFO) << "       -NONE           : Disable clustering of variables." << endl;
   }
}

void
V3ExtractFSMCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "EXTract FSM: " << "Extract Finite State Machines from Current Network." << endl;
}

//----------------------------------------------------------------------
// WRIte FSM <(string fsmName)> <(string outputFile)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3WriteFSMCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fsmName)>");
   if (n == 1) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string outputFile)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existFSM(options[0])) {
         V3FSMExtract* const fsmExtract = handler->getFSM(options[0]);
         assert (fsmExtract); assert (fsmExtract->getFSMListSize());
         // Open File to Output
         ofstream output; output.open(options[1].c_str());
         if (output.is_open()) {
            // Collect State Variables
            V3UI32Set stateNets; stateNets.clear(); uint32_t sdgDBSize = 0;
            V3UI32Vec id2State(handler->getNtk()->getNetSize(), V3NtkUD);
            for (uint32_t i = 0; i < fsmExtract->getFSMListSize(); ++i) {
               assert (fsmExtract->getFSM(i) && fsmExtract->getFSM(i)->isCompleteFSM());
               fsmExtract->getFSM(i)->collectStateNets(stateNets);
               sdgDBSize += fsmExtract->getFSM(i)->getFSMSDGList().size();
            }
            output << "fsm " << stateNets.size() << " " << sdgDBSize << " " << fsmExtract->getFSMListSize() << endl;
            // Write State Variables
            output << "\n// State Variables" << endl; uint32_t j = 0;
            for (V3UI32Set::const_iterator it = stateNets.begin(); it != stateNets.end(); ++it) {
               output << handler->getNetExpression(V3NetId::makeNetId(*it)) << endl;
               assert (id2State.size() > (*it)); id2State[*it] = j++;
            }
            // Write FSMSDG
            output << "\n// Topology of State Variables" << endl; string exp = "";
            for (uint32_t i = 0; i < fsmExtract->getFSMListSize(); ++i) {
               for (j = 0; j < fsmExtract->getFSM(i)->getFSMSDGList().size(); ++j) {
                  fsmExtract->getFSM(i)->getFSMSDGList()[j]->getSDGExpr(id2State, exp);
                  output << exp.substr(1) << endl; exp = "";
               }
            }
            // Write FSM
            output << "\n// Abstract State Transition Graphs" << endl;
            for (uint32_t i = 0, k = 0; i < fsmExtract->getFSMListSize(); ++i) {
               for (j = 0; j < fsmExtract->getFSM(i)->getFSMSDGList().size(); ++j) output << (j ? " " : "") << k++;
               output << endl;
            }
            // Footer in FSM Output
            output << endl;
            writeV3GeneralHeader("FSM with Name = " + string(options[0]), output, "//");
            output.close();
         }
         else Msg(MSG_ERR) << "FSM Output File \"" << options[1] << "\" Not Found !!" << endl;
      }
      else Msg(MSG_ERR) << "FSM \"" << options[0] << "\" Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3WriteFSMCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: WRIte FSM <(string fsmName)> <(string outputFile)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fsmName)   : The name of FSMs to be plotted." << endl;
      Msg(MSG_IFO) << "       (string outputFile): The name of a file for FSM outputs." << endl;
   }
}

void
V3WriteFSMCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "WRIte FSM: " << "Output Finite State Machines Specifications." << endl;
}

//----------------------------------------------------------------------
// PLOT FSM <(string fsmName)> <(string fsmDirName)>
//----------------------------------------------------------------------
V3CmdExecStatus
V3PlotFSMCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   size_t n = options.size();
   if (n == 0) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fsmName)>");
   if (n == 1) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(string fsmDirName)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->existFSM(options[0])) {
         V3FSMExtract* const fsmExtract = handler->getFSM(options[0]);
         assert (fsmExtract); assert (fsmExtract->getFSMListSize());
         // Create Directory for FSM Outputs
         if (v3CreateDir(options[1].c_str())) {
            for (uint32_t i = 0; i < fsmExtract->getFSMListSize(); ++i) {
               const string fileName = options[1] + "/" + v3Int2Str(i);
               fsmExtract->getFSM(i)->writeFSM(".v3.fsm_output.png");
               system(("mv .v3.fsm_output.png " + fileName + ".png").c_str());
            }
         }
         else Msg(MSG_ERR) << "Creating Output Directory \"" << options[1] << "\" Failed !!" << endl;
      }
      else Msg(MSG_ERR) << "FSM \"" << options[0] << "\" Not Found !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3PlotFSMCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: PLOT FSM <(string fsmName)> <(string fsmDirName)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (string fsmName)   : The name of FSMs to be plotted." << endl;
      Msg(MSG_IFO) << "       (string fsmDirName): The name of a directory for FSM outputs." << endl;
   }
}

void
V3PlotFSMCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "PLOT FSM: " << "Plot Finite State Machines into *.png files." << endl;
}

#endif

