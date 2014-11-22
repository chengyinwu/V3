/****************************************************************************
  FileName     [ v3CmdMgr.cpp ]
  PackageName  [ v3/src/cmd ]
  Synopsis     [ Command Line Parser ]
  Author       [ Chung-Yang (Ric) Huang, Cheng-Yin Wu, and Steggie Chen ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_CMD_MGR_C
#define V3_CMD_MGR_C

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "v3StrUtil.h"
#include "v3CmdMgr.h"
#include "v3Msg.h"

extern "C" {
#include <readline/readline.h>
#include <readline/history.h>
}

/* -------------------------------------------------- *\
 * Class V3CmdExec Implementations
\* -------------------------------------------------- */
bool
V3CmdExec::operator < (const V3CmdExec& rhs) const {
   string lstr, rstr;
   for (size_t idx = 0; idx < _cmdLen; ++idx) {
      if (idx != 0) lstr.append(" ");
      lstr.append(_mandCmd[idx]);
      lstr.append(_optCmd[idx]);
   }
   for (size_t idx = 0; idx < rhs._cmdLen; ++idx) {
      if (idx != 0) rstr.append(" ");
      rstr.append(rhs._mandCmd[idx]);
      rstr.append(rhs._optCmd[idx]);
   }

   return (lstr < rstr);
}

void
V3CmdExec::lexOptions(const string& option, vector<string>& tokens) const {
   string token;
   size_t n = v3StrGetTok(option, token);
   while (token.size()) {
      tokens.push_back(token);
      n = v3StrGetTok(option, token, n);
   }
}

V3CmdExecStatus
V3CmdExec::errorOption(V3CmdOptionError err, const string& opt) const {
   switch (err) {
      case CMD_OPT_MISSING:
         if (opt.size()) Msg(MSG_ERR) << "Missing option \"" << opt << "\" !!" << endl;
         else Msg(MSG_ERR) << "Missing option" << "!!" << endl;
         break;
      case CMD_OPT_EXTRA:
         Msg(MSG_ERR) << "Extra option \"" << opt << "\" !!" << endl;
         break;
      case CMD_OPT_ILLEGAL:
         Msg(MSG_ERR) << "Illegal option \"" << opt << "\" !!" << endl;
         break;
      case CMD_OPT_FOPEN_FAIL:
         Msg(MSG_ERR) << "Error: cannot open file \"" << opt << "\" !!" << endl;
         break;
      default:
         Msg(MSG_ERR) << "Unknown option error type \"" << err << "\" !!" << endl;
         break;
   }
   return CMD_EXEC_ERROR;
}

bool
V3CmdExec::checkCmd(const string& check) const {
   unsigned len = this->getCmdLen();
   bool result = false;
   size_t space = 0, nxt_space; 

   for (unsigned word = 1; word <= len; ++word)
   {
      nxt_space = check.find(' ', space + 1); 
      if (nxt_space == string::npos) nxt_space = check.size();

      for (unsigned i = space + 1; i <= nxt_space; ++i)
      {
         string checkMand = check.substr(space, i - space);
         string checkOpt = check.substr(i, nxt_space - space - i);

         if (checkMandCmd(checkMand, word) &&
             (checkOpt.empty() || checkOptCmd(checkOpt, word)))
         {
            result = true;
            break;
         }
      }
      if (result == false) return false;

      result = false;
      space = nxt_space + 1;
   }

   return true;
}

bool
V3CmdExec::checkCmd(const string& check, size_t idx) const {
   if (idx >= this->getCmdLen()) return false;

   bool result = false;

   for (unsigned i = 1, n = check.size(); i <= n; ++i)
   {
      string checkMand = check.substr(0, i);
      string checkOpt = check.substr(i, n + 1 - i);

      if (checkMandCmd(checkMand, idx) &&
          (checkOpt.empty() || checkOptCmd(checkOpt, idx)))
      {
         result = true;
         break;
      }
   }

   return result;
}

bool
V3CmdExec::checkMandCmd(const string& check, size_t idx) const {
   if (check.size() != _mandCmd[idx - 1].size()) return false;
   for (unsigned i = 0, n = _mandCmd[idx - 1].size(); i < n; ++i) {
      if (!check[i]) return true;
      char ch1 = tolower(_mandCmd[idx - 1].at(i));
      char ch2 = tolower(check[i]);
      if (ch1 != ch2) return false;
   }
   return true;
}

bool
V3CmdExec::checkOptCmd(const string& check, size_t idx) const {
   if (check.size() > _optCmd[idx - 1].size()) return false;
   for (unsigned i = 0, n = _optCmd[idx - 1].size(); i < n; ++i) {
      if (!check[i]) return true;
      char ch1 = tolower(_optCmd[idx - 1].at(i));
      char ch2 = tolower(check[i]);
      if (ch1 != ch2) return false;
   }
   return true;
}

/* -------------------------------------------------- *\
 * Class V3CmdMgr Implementations
\* -------------------------------------------------- */
V3CmdMgr::V3CmdMgr(const string& p) : _defaultPrompt(p) {
   _prompt = _defaultPrompt;
   _cmdLib.clear();
   _history.clear();
}

V3CmdMgr::~V3CmdMgr() {
   if (_dofile.is_open()) _dofile.close();
}

bool
V3CmdMgr::regCmd(const string& cmd, unsigned nCmp, V3CmdExec* e) {
   assert(e); assert (cmd.size()); assert (nCmp);
   // check if cmd contains space
   if (cmd.find(' ') != string::npos) return false;

   unsigned s = cmd.size();
   if (s < nCmp) return false;

   string str = cmd;
   for (unsigned i = 0; i < nCmp; ++i) str[i] = toupper(str[i]);

   e->setCmdLen(1);
   e->setMandCmd(str.substr(0, nCmp)); e->setOptCmd(str.substr(nCmp));

   V3CmdExecSet::iterator it = _cmdLib.find(e->getV3CmdType());
   V3CmdExecSubSet* cmdSet = 0;
   if (it == _cmdLib.end()) {
      cmdSet = new V3CmdExecSubSet();
      _cmdLib.insert(make_pair(e->getV3CmdType(), cmdSet));
   }
   else cmdSet = it->second;
   cmdSet->insert(e);

   return true;
}

bool
V3CmdMgr::regCmd(const string& cmd, unsigned nCmp1, unsigned nCmp2, V3CmdExec* e) {
   assert(e); assert (cmd.size()); assert (nCmp1); assert (nCmp2);
   // check if cmd contains space
   size_t idx = cmd.find(' ');
   if (idx == string::npos) return false;

   string str1 = cmd.substr(0, idx);
   string str2 = cmd.substr(idx + 1);

   // check if cmd2 contains space
   if (str2.find(' ', idx) != string::npos) return false;

   unsigned s1 = str1.size();
   unsigned s2 = str2.size();
   if ((s1 < nCmp1) || (s2 < nCmp2)) return false;

   for (unsigned i = 0; i < nCmp1; ++i) str1[i] = toupper(str1[i]);
   for (unsigned i = 0; i < nCmp2; ++i) str2[i] = toupper(str2[i]);

   e->setCmdLen(2);
   e->setMandCmd(str1.substr(0, nCmp1)); e->setOptCmd(str1.substr(nCmp1));
   e->setMandCmd(str2.substr(0, nCmp2)); e->setOptCmd(str2.substr(nCmp2));

   V3CmdExecSet::iterator it = _cmdLib.find(e->getV3CmdType());
   V3CmdExecSubSet* cmdSet = 0;
   if (it == _cmdLib.end()) {
      cmdSet = new V3CmdExecSubSet();
      _cmdLib.insert(make_pair(e->getV3CmdType(), cmdSet));
   }
   else cmdSet = it->second;
   cmdSet->insert(e);
   return true;
}

V3CmdExecStatus
V3CmdMgr::execOneCmd() {
   // Read User Command Input
   char* execCmd = 0;
   if (_dofile.is_open()) {
      execCmd = new char[1024]; _dofile.getline(execCmd, 1024);
      cout << getPrompt() << execCmd << endl;
      if (_dofile.eof()) closeDofile();
   }
   else execCmd = readline(getPrompt().c_str());
   assert (execCmd);

   if (addHistory(execCmd)) {
      add_history(_history.back().c_str());
      string option = "";
      V3CmdExec* e = parseCmd(option);
      if (e) return e->exec(option);
   }
   delete[] execCmd;
   return CMD_EXEC_NOP;
}

void
V3CmdMgr::printHelps(bool revealed) const {
   V3CmdExecSet::const_iterator it;
   V3CmdExecSubSet::iterator is;
   Msg(MSG_IFO) << endl;
   for (it = _cmdLib.begin(); it != _cmdLib.end(); ++it) {
      if ((revealed) ^ (it->first != CMD_TYPE_REVEALED)) {
         Msg(MSG_IFO) << "========== " << V3CmdTypeString[it->first] << " Commands : ==========" << endl;
         for (is = it->second->begin(); is != it->second->end(); ++is) (*is)->help();
         Msg(MSG_IFO) << endl;
      }
   }
}

void
V3CmdMgr::printHistory(int nPrint) const {
   int historySize = _history.size();
   if (historySize == 0) {
      Msg(MSG_IFO) << "Empty command history!!" << endl;
      return;
   }
   if ((nPrint < 0) || (nPrint > historySize)) nPrint = historySize;
   assert (historySize >= nPrint);
   for (int i = historySize - nPrint; i < historySize; ++i)
      Msg(MSG_IFO) << "   " << i << ": " << _history[i] << endl;
}

V3CmdExec*
V3CmdMgr::parseCmd(string& option) {
   assert(_history.size());
   string str = _history.back();
   assert(str[0] != 0 && str[0] != ' ');

   V3CmdExec* e = 0;
   string cmd;

   // normalize: keep only one space between two words
   unsigned delCount = 0;
   for (size_t i = 2, n = str.size(); i < n; ++i)
   {
      if ((str[i - 2] == ' ') && (str[i - 1] == ' '))
      {
         for (size_t j = i; j < n; ++j)
	    str[j - 1] = str[j];
         --i;
	 ++delCount;
      }
   }
   str.erase(str.length() - delCount);

   // space count: indicates how many words there are in cmd
   unsigned spCount = 0;
   for (size_t i = 0, n = str.size(); i < n; ++i)
      if (str[i] == ' ') ++spCount;

   // try to match commands
   size_t idx = 0;
   for (unsigned i = 0; (i < spCount + 1) && i < 2; ++i)
   {
      idx = str.find(' ', idx + 1);
      cmd = str.substr(0, idx);

      e = getCmd(cmd);
      if (e) break;
   }


   // can't match any command
   if (!e)
   {
      // Chengyin added : Support linux commands
      string _cmd = "";
      if (str.size() >= 2) {
         // Support comments in dofile, NO execution
         if ((str[0] == '/') && (str[1] == '/')) return e;
      }
      for (idx = 0; idx < str.size(); ++idx) { if (str[idx] == ' ') break; }
      _cmd = str.substr(0, idx);
      if ((_cmd == "ls") || (_cmd == "vi") || (_cmd == "vim") || (_cmd == "echo") || (_cmd == "cat")) 
         system(str.c_str());
      else Msg(MSG_ERR) << "Illegal command!! (" << str << ")" << endl;
   }
   else if (idx != string::npos)
   {
      size_t opt = str.find_first_not_of(' ', idx);
      if (opt != string::npos)
         option = str.substr(opt);
   }

   return e;
}

V3CmdExec*
V3CmdMgr::getCmd(const string& cmd) const {
   V3CmdExec* e = 0;

   // space count: indicates how many words there are in cmd
   unsigned spCount = 0;
   for (size_t i = 0, n = cmd.size(); i < n; ++i)
      if (cmd[i] == ' ') ++spCount;

   V3CmdExecSet::const_iterator it;
   V3CmdExecSubSet::iterator is;
   for (it = _cmdLib.begin(); it != _cmdLib.end(); ++it) {
      for (is = it->second->begin(); is != it->second->end(); ++is) {
         if (((*is)->getCmdLen() == spCount + 1) && (*is)->checkCmd(cmd)) {
            e = *is;
            break;
         }
      }
   }

   return e;
}

V3CmdExecSubSet
V3CmdMgr::getCmdListFromPart(const string& cmd) const {
   V3CmdExecSubSet result;

   // space count: indicates how many words there are in cmd
   unsigned spCount = 0;
   for (size_t i = 0, n = cmd.size(); i < n; ++i)
      if (cmd[i] == ' ') ++spCount;

   V3CmdExecSet::const_iterator it;
   V3CmdExecSubSet::iterator is;
   for (it = _cmdLib.begin(); it != _cmdLib.end(); ++it) {
      if (it->first != CMD_TYPE_REVEALED) {
         for (is = it->second->begin(); is != it->second->end(); ++is) {
            if (((*is)->getCmdLen() > spCount)) {
               bool check = true;
               for (unsigned i = 1; i <= spCount + 1; ++i) {
                  if (!(*is)->checkCmd(cmd, i)) {
                     check = false;
                     break;
                  }
               }
               if (check == true) result.insert(*is);
            }
         }
      }
   }

   return result;
}

bool
V3CmdMgr::addHistory(char* cmd) {
   // remove ' ' at the end
   char* tmp = &(cmd[strlen(cmd) - 1]);
   while ((tmp >= cmd) && (*tmp == ' ')) *(tmp--) = 0;

   // remove ' ' in the beginning
   tmp = cmd;
   while (*tmp == ' ') ++tmp;

   // add to _history
   if (*tmp != 0) {
      _history.push_back(tmp);
      Msg(MSG_LOG) << tmp << endl;
      return true;
   }
   return false;
}

#endif
