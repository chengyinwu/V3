/****************************************************************************
  FileName     [ main.cpp ]
  PackageName  [ main ]
  Synopsis     [ Main Function ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MAIN_C
#define V3_MAIN_C

#include "v3Usage.h"
#include "v3CmdMgr.h"
#include "v3StrUtil.h"
#include "v3NtkHandler.h"

// Global Variables for V3MsgMgr
string V3Msg::_allName = "";
ofstream V3Msg::_allout;
V3MsgMgr Msg;

// Global Variable for V3Usage
V3Usage v3Usage;

// Global Variable for V3CmdMgr
V3CmdMgr* v3CmdMgr = new V3CmdMgr("v3");

// Global Variable for V3Handler
V3Handler v3Handler;

// Various Commands Registration
extern bool initAlgCmd();
extern bool initDfxCmd();
extern bool initNtkCmd();
extern bool initStgCmd();
extern bool initVrfCmd();
extern bool initV3MCCmd();
extern bool initTransCmd();
extern bool initCommonCmd();

// Program Usage
static void usage() {
   Msg(MSG_IFO) << "Usage: <v3> [ -File < dofile > ]" << endl; exit(-1);
}

int main(int argc, char** argv) {
   bool dof = false;
   string dofile = "";
   
   // Set Random Number Seed
   srand(time(NULL));

   // Register Commands
   if (!(initAlgCmd() && initDfxCmd() && initNtkCmd() && initStgCmd() && initVrfCmd() &&
         initV3MCCmd() && initTransCmd() && initCommonCmd())) {
      Msg(MSG_ERR) << "Command Register Failed !!!" << endl; exit(0);
   }
   
   for (int i = 1; i < argc; ++i) {
      if (v3StrNCmp("-File", argv[i], 2) == 0) {
         if (dof) { Msg(MSG_ERR) << "Extra Argument \"" << argv[i] << "\" !!" << endl; usage(); }
         else dof = true;
      }
      else if (dof && !dofile.size()) dofile = argv[i];
      else { Msg(MSG_ERR) << "Unknown Argument \"" << argv[i] << "\" !!" << endl; usage(); }
   }

   // Dofile
   if (dof) {
      if (dofile.size() == 0) { Msg(MSG_ERR) << "Missing Argument \"< dofile >\" !!" << endl; usage(); }
      else if (!v3CmdMgr->openDofile(dofile.c_str())) {
         Msg(MSG_ERR) << "Cannot Open File \"" << dofile << "\" !!" << endl; usage();
      }
   }
   // Start Program
   V3CmdExecStatus status = CMD_EXEC_DONE;
   while (status != CMD_EXEC_QUIT) {
      v3CmdMgr->setPrompt();
      status = v3CmdMgr->execOneCmd();
      cout << endl;
   }
   return 0;
}

#endif

