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
#include "v3NtkMiter.h"
#include "v3TransCmd.h"
#include "v3NtkExpand.h"
#include "v3BvBlastBv.h"
#include "v3BvBlastAig.h"
#include "v3NtkFlatten.h"
#include "v3NtkHandler.h"

#include <iomanip>

bool initTransCmd() {
   return (
         v3CmdMgr->regCmd("EXPand NTk",   3, 2, new V3ExpandNtkCmd)  &&
         v3CmdMgr->regCmd("BLAst NTk",    3, 2, new V3BlastNtkCmd)   &&
         v3CmdMgr->regCmd("MITer NTk",    3, 2, new V3MiterNtkCmd)   &&
         v3CmdMgr->regCmd("FLAtten NTk",  3, 2, new V3FlattenNtkCmd)
   );
}

//----------------------------------------------------------------------
// EXPand NTk <(unsigned cycle)> [-Initial]
//----------------------------------------------------------------------
V3CmdExecStatus
V3ExpandNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);
   
   bool init = false; uint32_t cycle = 0;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Initial", token, 2) == 0) {
         if (init) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else init = true;
      }
      else if (!cycle) {
         int temp;
         if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         if (temp <= 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
         cycle = uint32_t(temp);
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (!cycle) return V3CmdExec::errorOption(CMD_OPT_MISSING, "<(unsigned cycle)>");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (!handler->getNtk()->getModuleSize()) {
         V3NtkExpand* const expandHandler = new V3NtkExpand(handler, cycle, init); assert (expandHandler);
         v3Handler.pushAndSetCurHandler(expandHandler);
      }
      else Msg(MSG_ERR) << "Currently Networks with Module Instances Cannot be Expanded !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3ExpandNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: EXPand NTk <(unsigned cycle)>" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned cycle): The number of time-frames to be considered." << endl;
   }
}

void
V3ExpandNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "EXPand NTk: " << "Perform Time-frame Expansion for Networks." << endl;
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

//----------------------------------------------------------------------
// MITer NTk <(unsigned ntkId1)> <(unsigned ntkId2)>
//           [-Name <(string miterNtkName)>] [| -SEC | -CEC] [-Merge]
//----------------------------------------------------------------------
V3CmdExecStatus
V3MiterNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   uint32_t id1 = V3NtkUD, id2 = V3NtkUD;
   bool miterName = false, miterNameON = false;
   bool sec = false, cec = false, merge = false;
   string miterNtkName = ""; int temp;
   
   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (v3StrNCmp("-Name", token, 2) == 0) {
         if (miterName) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else miterName = miterNameON = true;
      }
      else if (v3StrNCmp("-SEC", token, 4) == 0) {
         if (sec || cec) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else sec = true;
      }
      else if (v3StrNCmp("-CEC", token, 4) == 0) {
         if (sec || cec) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else cec = true;
      }
      else if (v3StrNCmp("-Merge", token, 2) == 0) {
         if (merge) return V3CmdExec::errorOption(CMD_OPT_EXTRA, token);
         else merge = true;
      }
      else if (miterNameON) { miterNameON = false; miterNtkName = token; }
      else if (V3NtkUD == id1) {
         if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkId1)");
         else if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkId1)");
         id1 = (uint32_t)temp;
      }
      else if (V3NtkUD == id2) {
         if (!v3Str2Int(token, temp)) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkId2)");
         else if (temp < 0) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned ntkId2)");
         id2 = (uint32_t)temp;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   if (V3NtkUD == id1) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned ntkId1)");
   if (V3NtkUD == id2) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(unsigned ntkId2)");
   if (miterNameON) return V3CmdExec::errorOption(CMD_OPT_MISSING, "(string miterNtkName)");

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (id1 >= v3Handler.getHandlerCount()) 
         Msg(MSG_ERR) << "Network with id = " << id1 << " Does NOT Exist !!" << endl;
      else if (id2 >= v3Handler.getHandlerCount()) 
         Msg(MSG_ERR) << "Network with id = " << id2 << " Does NOT Exist !!" << endl;
      else {
         V3NtkHandler* const handler1 = v3Handler.getHandler(id1); assert (handler);
         V3NtkHandler* const handler2 = v3Handler.getHandler(id2); assert (handler);
         if (!(reportIncompatibleModule(handler1, handler2) || reportIncompatibleModule(handler1, handler2))) {
            if (!cec) {
               V3NtkMiter* const miterHandler = new V3NtkMiter(handler1, handler2, merge, miterNtkName);
               assert (miterHandler); v3Handler.pushAndSetCurHandler(miterHandler);
            }
            else {
               // Compute Name Mapping for Latches Between Two Networks
               if (handler1->getNtk()->getLatchSize() == handler2->getNtk()->getLatchSize()) {
                  V3Map<string, uint32_t>::Map nameMap; nameMap.clear();
                  for (uint32_t i = 0; i < handler2->getNtk()->getLatchSize(); ++i)
                     nameMap.insert(make_pair(handler2->getNetName(handler2->getNtk()->getLatch(i)), i));
                  if (handler2->getNtk()->getLatchSize() == nameMap.size()) {
                     V3Map<string, uint32_t>::Map::const_iterator it; V3UI32Vec latchMap;
                     latchMap.clear(); latchMap.reserve(handler1->getNtk()->getLatchSize());
                     for (uint32_t i = 0; i < nameMap.size(); ++i) {
                        it = nameMap.find(handler1->getNetName(handler1->getNtk()->getLatch(i)));
                        if ((nameMap.end() == it) || 
                            (handler1->getNtk()->getNetWidth(handler1->getNtk()->getLatch(i)) != 
                             handler2->getNtk()->getNetWidth(handler2->getNtk()->getLatch(it->second)))) break;
                        else latchMap.push_back(it->second);
                     }
                     if (handler1->getNtk()->getLatchSize() == latchMap.size()) {
                        V3NtkMiter* const miterHandler = 
                           new V3NtkMiter(handler1, handler2, latchMap, merge, miterNtkName);
                        assert (miterHandler); v3Handler.pushAndSetCurHandler(miterHandler);
                     }
                     else Msg(MSG_ERR) << "Unmapped Latch " << latchMap.size() 
                                       << " Found in Network " << id1 << " !!" << endl;
                  }
                  else Msg(MSG_ERR) << "Repeated or Missing Latch Names Exist !!" << endl;
               }
               else Msg(MSG_ERR) << "Number of Latches in Networks NOT Matched : " << id1 << "(" 
                                 << handler1->getNtk()->getLatchSize() << ") != " << id2 << "(" 
                                 << handler2->getNtk()->getLatchSize() << ") !!" << endl;
            }
         }
      }
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3MiterNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: MITer NTk <(unsigned ntkId1)> <(unsigned ntkId2)>" << endl;
   Msg(MSG_IFO) << "                 [-Name <(string miterNtkName)>] [| -SEC | -CEC] [-Merge]" << endl;
   if (verbose) {
      Msg(MSG_IFO) << "Param: (unsigned ntkId1)    : The index of the first network." << endl;
      Msg(MSG_IFO) << "       (unsigned ntkId2)    : The index of the second network." << endl;
      Msg(MSG_IFO) << "       (string miterNtkName): The name of the resulting miter." << endl;
      Msg(MSG_IFO) << "       -Name                : Indicate the following token is the name of the miter." << endl;
      Msg(MSG_IFO) << "       -SEC                 : Construct miter for sequential equivalence checking." << endl;
      Msg(MSG_IFO) << "       -CEC                 : Construct miter for combinational equivalence checking." << endl;
      Msg(MSG_IFO) << "                              Please notice that latches are mapped by their names." << endl;
      Msg(MSG_IFO) << "       -Merge               : Merge miter outputs into a representative." << endl;
   }
}

void
V3MiterNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "MITer NTk: " << "Miter Two Networks." << endl;
}

//----------------------------------------------------------------------
// FLAtten NTk [(unsigned level)]
//----------------------------------------------------------------------
V3CmdExecStatus
V3FlattenNtkCmd::exec(const string& option) {
   vector<string> options;
   V3CmdExec::lexOptions(option, options);

   uint32_t level = V3NtkUD; int temp;

   size_t n = options.size();
   for (size_t i = 0; i < n; ++i) {
      const string& token = options[i];
      if (V3NtkUD == level) {
         if (!v3Str2Int(token, temp) || !temp) return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, "(unsigned level)");
         level = (uint32_t)temp;
      }
      else return V3CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
   }

   V3NtkHandler* const handler = v3Handler.getCurHandler();
   if (handler) {
      if (handler->getNtk()->getModuleSize()) {
         if (!reportInconsistentRefNtk(handler->getNtk())) {
            V3NtkFlatten* const flattenHandler = new V3NtkFlatten(handler, level);
            assert (flattenHandler); v3Handler.pushAndSetCurHandler(flattenHandler);
         }
      }
      else Msg(MSG_ERR) << "Current Network is Already a Flattened Network !!" << endl;
   }
   else Msg(MSG_ERR) << "Empty Ntk !!" << endl;
   return CMD_EXEC_DONE;
}

void
V3FlattenNtkCmd::usage(const bool& verbose) const {
   Msg(MSG_IFO) << "Usage: FLAtten NTk" << endl;
}

void
V3FlattenNtkCmd::help() const {
   Msg(MSG_IFO) << setw(20) << left << "FLAtten NTk: " << "Flatten Hierarchical Networks." << endl;
}

#endif

