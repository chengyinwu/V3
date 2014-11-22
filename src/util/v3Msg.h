/****************************************************************************
  FileName     [ v3Msg.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ V3 File and Directory Manipulation Functions ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MSG_H
#define V3_MSG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class V3MsgMgr;
extern V3MsgMgr Msg;

class V3Msg
{
   public : 
      V3Msg(ostream& os) : _default(os) { _defaultON = true; _filename = ""; }
      ~V3Msg() {}
      bool isOutFileON() { return _outfile.is_open(); }
      void setOutFile(const string filename) { _filename = filename; }
      void stopOutFile() { if (isOutFileON()) _outfile.close(); }
      void startOutFile(bool append) {
         stopOutFile();
         if (append) _outfile.open(_filename.c_str(), ios::app);
         else _outfile.open(_filename.c_str());
      }
      void stopDefault() { _defaultON = false; }
      void startDefault() { _defaultON = true; }

      // operator overload
      template<class T> friend V3Msg& operator << (V3Msg& m, const T& s) {
         if (m._defaultON) m._default << s;
         if (m.isOutFileON()) m._outfile << s;
         if (V3Msg::_allout.is_open()) V3Msg::_allout << s;
         return m;
      }

      typedef ostream& (*StdEndLine)(ostream&);  // overload operator << for std::endl
      friend V3Msg& operator << (V3Msg& m, StdEndLine e) {
         if (m._defaultON) e(m._default);
         if (m.isOutFileON()) e(m._outfile);
         if (V3Msg::_allout.is_open()) e(V3Msg::_allout);
         return m;
      }
      static ofstream   _allout;
      static string     _allName;
   private : 
      bool              _defaultON;
      string            _filename;
      ofstream          _outfile;
      ostream&          _default;
};

enum V3MsgType
{
   MSG_LOG = 0,
   MSG_ERR = 1,
   MSG_WAR = 2,
   MSG_DBG = 3,
   MSG_IFO = 4
};

class V3MsgMgr
{
   public :
      V3MsgMgr() {
         Msgs.clear();
         MsgTypeString.clear();
         Msgs.push_back(new V3Msg(cout)); MsgTypeString.push_back("");              // 0 : CmdLogMsg
         Msgs.push_back(new V3Msg(cerr)); MsgTypeString.push_back("[ERROR]: ");     // 1 : ErrorMsg
         Msgs.push_back(new V3Msg(cerr)); MsgTypeString.push_back("[WARNING]: ");   // 2 : WarningMsg
         Msgs.push_back(new V3Msg(cout)); MsgTypeString.push_back("");              // 3 : DebugMsg
         Msgs.push_back(new V3Msg(cout)); MsgTypeString.push_back("");              // 4 : InfoMsg
         Msgs[0]->stopDefault();  // NO std output needed for command Log
      }
      ~V3MsgMgr() { Msgs.clear(); }
      V3Msg& operator () (const V3MsgType type) {
         (*Msgs[type]) << MsgTypeString[type];
         return *Msgs[type];
      }
      void setOutFile(const string filename, const V3MsgType type) { Msgs[type]->setOutFile(filename); }
      void startOutFile(const V3MsgType type, bool append = false) { Msgs[type]->startOutFile(append); }
      void stopOutFile(const V3MsgType type) { Msgs[type]->stopOutFile(); }
      void startDefault(const V3MsgType type) { Msgs[type]->startDefault(); }
      void stopDefault(const V3MsgType type) { Msgs[type]->stopDefault(); }
      
      bool isAllOutFileON() { return V3Msg::_allout.is_open(); }
      void setAllOutFile(const string filename) { V3Msg::_allName = filename; }
      void startAllOutFile(bool append = false) {
         stopAllOutFile();
         if (append) V3Msg::_allout.open(V3Msg::_allName.c_str(), ios::app);
         else V3Msg::_allout.open(V3Msg::_allName.c_str());
      }
      void stopAllOutFile() { if (isAllOutFileON()) V3Msg::_allout.close(); }
      void startAllDefault() { for (unsigned i = 1; i < 5; ++i) Msgs[i]->startDefault(); }
      void stopAllDefault() { for (unsigned i = 1; i < 5; ++i) Msgs[i]->stopDefault(); }
   private : 
      vector<string> MsgTypeString;
      vector<V3Msg*> Msgs;
      pthread_mutex_t msgMutex;
};

// V3 Pre-Defined Template Messages
static inline const int callEmptyVirtual(const string& functionName) {
   Msg(MSG_ERR) << "Calling virtual function " << functionName << " !!" << endl; return 0;
}

#endif

