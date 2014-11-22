/****************************************************************************
  FileName     [ v3CmdMgr.h ]
  PackageName  [ v3/src/cmd ]
  Synopsis     [ Command Line Parser (Adopt Readline Library) ]
  Author       [ Chung-Yang (Ric) Huang, Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_CMD_MGR_H
#define V3_CMD_MGR_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <map>
#include <set>

using namespace std;

// Forward Declarations
class V3CmdExec;
class V3CmdMgr;

// Global Variables
extern V3CmdMgr*  v3CmdMgr;

// Command Categories to String
const string V3CmdTypeString[] = {
   "Revealed",
   "Common",
   "I/O",
   "Print",
   "Synthesis",
   "Manipulation",
   "Extraction",
   "Simulation",
   "Verification",
   "Model Checking",
   "Debugging"
};

// Command Categories Enum
enum V3CmdType
{
   // Revealed command
   CMD_TYPE_REVEALED    = 0,
   CMD_TYPE_COMMON      = 1,
   CMD_TYPE_IO          = 2,
   CMD_TYPE_PRINT       = 3,
   CMD_TYPE_SYNTHESIS   = 4,
   CMD_TYPE_MANIP       = 5,
   CMD_TYPE_EXTRACT     = 6,
   CMD_TYPE_SIMULATE    = 7,
   CMD_TYPE_VERIFY      = 8,
   CMD_TYPE_MODELCHKING = 9,
   CMD_TYPE_DEBUGFIX    = 10,
   CMD_TYPE_TOTAL       = 11
};

// Command Execution Status
enum V3CmdExecStatus
{  
   CMD_EXEC_DONE  = 0,
   CMD_EXEC_ERROR = 1,
   CMD_EXEC_QUIT  = 2,
   CMD_EXEC_NOP   = 3
};

// Command Execution Error Types
enum V3CmdOptionError
{
   CMD_OPT_MISSING    = 0,
   CMD_OPT_EXTRA      = 1,
   CMD_OPT_ILLEGAL    = 2,
   CMD_OPT_FOPEN_FAIL = 3
};

// Define MACRO for V3 Command
#define V3_COMMAND(cmd, type)                      \
class cmd : public V3CmdExec                       \
{                                                  \
   public:                                         \
      cmd() : V3CmdExec(type) {}                   \
      ~cmd() {}                                    \
      V3CmdExecStatus exec(const string&);         \
      void usage(const bool& = false) const;       \
      void help() const;                           \
};                                                 \

class V3CmdExec
{
   public :
      // Constructor and Destructor
      V3CmdExec(V3CmdType t) : _cmdType(t) {}
      virtual ~V3CmdExec() {}
      // Command Availability Check
      virtual V3CmdExecStatus exec(const string&) = 0;
      virtual void usage(const bool& = false) const = 0;
      virtual void help() const = 0;
      // Command Attributes
      inline V3CmdType getV3CmdType() const { return _cmdType; }
      inline void setCmdLen(unsigned n) { _cmdLen = n; }
      inline unsigned getCmdLen() const { return _cmdLen; }
      inline void setMandCmd(const string& str) { _mandCmd.push_back(str);}
      inline const string& getMandCmd(size_t idx) const { return _mandCmd[idx]; }
      inline void setOptCmd(const string& str) { _optCmd.push_back(str); }
      inline const string& getOptCmd(size_t idx) const { return _optCmd[idx]; }
      bool checkCmd(const string&) const;
      bool checkCmd(const string&, size_t) const;
      bool checkMandCmd(const string&, size_t) const;
      bool checkOptCmd(const string&, size_t) const;
      // Operator Overloads
      bool operator < (const V3CmdExec&) const;
   protected :
      void lexOptions(const string&, vector<string>&) const;
      V3CmdExecStatus errorOption(V3CmdOptionError, const string&) const;
   private :
      V3CmdType         _cmdType;
      unsigned          _cmdLen;
      vector<string>    _mandCmd;
      vector<string>    _optCmd;
};

struct V3CmdCompare { bool operator() (const V3CmdExec* a, const V3CmdExec* b) const { return (*a < *b); } };
typedef set<V3CmdExec*, V3CmdCompare>     V3CmdExecSubSet;
typedef map<V3CmdType, V3CmdExecSubSet*>  V3CmdExecSet;

class V3CmdMgr 
{
   public :
      // Constructor and Destructor
      V3CmdMgr(const string&);
      ~V3CmdMgr();
      // Command Registration  (At Most Two Mandatories)
      bool regCmd(const string&, unsigned, V3CmdExec*);
      bool regCmd(const string&, unsigned, unsigned, V3CmdExec*);
      // Command Retrieval
      V3CmdExec* getCmd(const string&) const;
      V3CmdExecSubSet getCmdListFromPart(const string&) const;
      // Command Parse and Exec
      V3CmdExecStatus execOneCmd();
      // Command Dofile Support
      bool openDofile(const string& dof) { _dofile.open(dof.c_str()); return _dofile.is_open(); }
      void closeDofile() { _dofile.close(); _dofile.clear(); }
      // Command Help Support
      void printHelps(bool = false) const;
      // Command History Support
      void printHistory(int = -1) const;
      // Command Prompt Settings
      inline const string& getPrompt() const { return _prompt; }
      inline void setPrompt() { _prompt = _defaultPrompt + "> "; }
   private :
      // Command Helper Functions
      bool addHistory(char*);
      V3CmdExec* parseCmd(string&);
      // Command Data members
      const string   _defaultPrompt;         // Default Command Prompt
      string         _prompt;                // Current Command Prompt
      ifstream       _dofile;                // For Script Parsing
      V3CmdExecSet   _cmdLib;                // All Available Commands
      vector<string> _history;               // Command History Copy
};

#endif

