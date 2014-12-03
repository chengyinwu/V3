/****************************************************************************
  FileName     [ v3NtkHandler.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ V3 Ntk Handler. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_HANDLER_C
#define V3_NTK_HANDLER_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3Property.h"
#include "v3NtkHandler.h"
#include "v3StgExtract.h"

#include <iomanip>

/* -------------------------------------------------- *\
 * Class V3NtkHandler Implementations
\* -------------------------------------------------- */
// Initialization to Static Members
string V3NtkHandler::V3AuxHierSeparator   = V3HierSeparator;
string V3NtkHandler::V3AuxNameInvPrefix   = V3NameInvPrefix;
string V3NtkHandler::V3AuxNameBitPrefix   = V3NameBitPrefix;
string V3NtkHandler::V3AuxNameBitSuffix   = V3NameBitSuffix;
string V3NtkHandler::V3AuxExpansionName   = V3ExpansionName;
string V3NtkHandler::V3AuxNetNamePrefix   = V3NetNamePrefix;
unsigned char V3NtkHandler::_extVerbosity = 0;
unsigned char V3NtkHandler::_intVerbosity = 0;

// Constructor and Destructor
V3NtkHandler::V3NtkHandler(V3NtkHandler* const p, V3Ntk* const ntk) : _handler(p) {
   if (_handler) _handler->incRefCount(); _ntk = ntk;
   _fsm.clear(); _property.clear(); _ntkName = "";
   _nameHash.clear(); _netHash.clear(); _instRef = 0; _refCount = 0;
}

V3NtkHandler::~V3NtkHandler() {
   for (V3FSMMap::iterator it = _fsm.begin(); it != _fsm.end(); ++it) delete it->second;
   for (V3PropertyMap::iterator it = _property.begin(); it != _property.end(); ++it) delete it->second;
   if (_handler) _handler->decRefCount(); if (_ntk) delete _ntk; _fsm.clear(); _property.clear();
   _nameHash.clear(); _netHash.clear();
}

// Ntk Ancestry Functions
const string
V3NtkHandler::getNtkName() const {
   return (_ntkName.size() || !_handler) ? _ntkName : _handler->getNtkName();
}

V3NtkHandler* const
V3NtkHandler::getPrimitive() const {
   return (_handler) ? _handler->getPrimitive() : const_cast<V3NtkHandler* const>(this);
}

// I/O Ancestry Functions
const string
V3NtkHandler::getInputName(const uint32_t& index) const {
   assert (_ntk); if (index >= _ntk->getInputSize()) return "";
   return _handler ? _handler->getInputName(index) : getNetNameOrFormedWithId(_ntk->getInput(index));
}

const string
V3NtkHandler::getOutputName(const uint32_t& index) const {
   // Current Network
   assert (_ntk); if (index >= _ntk->getOutputSize()) return "";
   V3IdxStrHash::const_iterator it = _outIndexHash.find(index);
   if (it != _outIndexHash.end()) return it->second;
   // Parent Networks
   return _handler ? _handler->getOutputName(index) : "";
}

const string
V3NtkHandler::getInoutName(const uint32_t& index) const {
   assert (_ntk); if (index >= _ntk->getInoutSize()) return "";
   return _handler ? _handler->getInoutName(index) : getNetNameOrFormedWithId(_ntk->getInout(index));
}

// Net Ancestry Functions
const bool
V3NtkHandler::isLegalNetName(const string& name) {
   for (uint32_t i = 1; i < name.size(); ++i) if (!isalnum(name[i]) && '_' != name[i]) return false;
   return name.size() && (isalpha(name[0]) || '_' == name[0]);
}

const string
V3NtkHandler::getNetNameOrFormedWithId(const V3NetId& id) const {
   const string name = getNetName(id); if (name.size()) return name;
   return (id.cp ? V3AuxNameInvPrefix : "") + V3AuxNetNamePrefix + v3Int2Str(id.id);
}

const string
V3NtkHandler::getNetExpression(const V3NetId& id) const {
   V3NetId netId; string name = "";
   // Search for the name of  id in Original Network
   getNetName(netId =  id, name); if (name.size()) return (netId.cp ? V3AuxNameInvPrefix : "") + name;
   // Search for the name of ~id in Original Network
   getNetName(netId = ~id, name); if (name.size()) return (netId.cp ? "" : V3AuxNameInvPrefix) + name;
   // Return the Expression in terms of Gate Operations
   return getNetRecurExpression(id);
}

const string
V3NtkHandler::getNetRecurExpression(const V3NetId& id) const {
   assert (_ntk); assert (id.id < _ntk->getNetSize()); string name = "";
   const V3GateType& type = _ntk->getGateType(id); assert (V3_XD > type);
   if (V3_FF >= type) return (id.cp ? V3AuxNameInvPrefix : "") + v3Int2Str(id.id);
   else if (V3_MODULE == type) {
      Msg(MSG_WAR) << "Currently Expression Over Module Instances has NOT Been Implemented !!" << endl;
   }
   else {
      if (dynamic_cast<V3BvNtk*>(_ntk)) {
         assert (AIG_FALSE < type);
         if (isV3PairType(type)) {
            const string name1 = getNetExpression(_ntk->getInputNetId(id, 0)); assert (name1.size());
            const string name2 = getNetExpression(_ntk->getInputNetId(id, 1)); assert (name2.size());
            if (BV_MERGE == type) name = "{" + name1 + ", " + name2 + "}";
            else {
               switch (type) {
                  case BV_AND      : name = "(" + name1 + " & "  + name2 + ")"; break;
                  case BV_XOR      : name = "(" + name1 + " ^ "  + name2 + ")"; break;
                  case BV_ADD      : name = "(" + name1 + " + "  + name2 + ")"; break;
                  case BV_SUB      : name = "(" + name1 + " - "  + name2 + ")"; break;
                  case BV_MULT     : name = "(" + name1 + " * "  + name2 + ")"; break;
                  case BV_SHL      : name = "(" + name1 + " << " + name2 + ")"; break;
                  case BV_SHR      : name = "(" + name1 + " >> " + name2 + ")"; break;
                  case BV_DIV      : name = "(" + name1 + " / "  + name2 + ")"; break;
                  case BV_MODULO   : name = "(" + name1 + " % "  + name2 + ")"; break;
                  case BV_EQUALITY : return "(" + name1 + (id.cp ? " != " : " == ") + name2 + ")";
                  case BV_GEQ      : return "(" + name1 + (id.cp ? " < "  : " >= ") + name2 + ")";
                  default          : assert (0);
               }
            }
         }
         else if (isV3ReducedType(type)) {
            const string name1 = getNetExpression(_ntk->getInputNetId(id, 0)); assert (name1.size());
            switch (type) {
               case BV_RED_AND : name = "(&(" + name1 + "))"; break;
               case BV_RED_OR  : name = "(|(" + name1 + "))"; break;
               case BV_RED_XOR : name = "(^(" + name1 + "))"; break;
               default         : assert (0);
            }
         }
         else if (BV_MUX == type) {
            const string fName = getNetExpression(_ntk->getInputNetId(id, 0)); assert (fName.size());
            const string tName = getNetExpression(_ntk->getInputNetId(id, 1)); assert (tName.size());
            const string sName = getNetExpression(_ntk->getInputNetId(id, 2)); assert (sName.size());
            name = "(" + sName + " ? " + tName + " : " + fName + ")";
         }
         else if (BV_SLICE == type) {
            const string name1 = getNetExpression(_ntk->getInputNetId(id, 0)); assert (name1.size());
            const uint32_t msb = dynamic_cast<V3BvNtk*>(_ntk)->getInputSliceBit(id, true);
            const uint32_t lsb = dynamic_cast<V3BvNtk*>(_ntk)->getInputSliceBit(id, false);
            const uint32_t width = _ntk->getNetWidth(_ntk->getInputNetId(id, 0)); assert (width);
            name = ((msb >= lsb) && (width == (1 + msb - lsb))) ? name1 : 
                   (msb == lsb) ? ("(" + name1 + ")[" + v3Int2Str(msb) + "]") : 
                   ("(" + name1 + ")[" + v3Int2Str(msb) + ":" + v3Int2Str(lsb) + "]");
         }
         else {
            assert (BV_CONST == type);
            const V3BitVecX value = dynamic_cast<V3BvNtk*>(_ntk)->getInputConstValue(id);
            return v3Int2Str(value.size()) + "'b" + value.regEx();
         }
      }
      else {
         assert (AIG_FALSE >= type);
         if (AIG_NODE == type) {
            const string name1 = getNetExpression(_ntk->getInputNetId(id, 0)); assert (name1.size());
            const string name2 = getNetExpression(_ntk->getInputNetId(id, 1)); assert (name2.size());
            name = "(" + name1 + " && " + name2 + ")";
         }
         else {
            assert (AIG_FALSE == type);
            return (id.cp ? "AIGER_TRUE" : "AIGER_FALSE");
         }
      }
   }
   return (id.cp ? V3AuxNameInvPrefix : "") + name;
}

const string
V3NtkHandler::getNetName(const V3NetId& id) const {
   V3NetId netId = id; string name = ""; getNetName(netId, name);
   return name.size() ? ((netId.cp ? V3AuxNameInvPrefix : "") + name) : "";
}

const bool
V3NtkHandler::resetNetName(const uint32_t& index, const string& name) {
   assert (index < _ntk->getNetSize());
   if (name.size() && !isLegalNetName(name)) {
      Msg(MSG_ERR) << "Illegal Net Name \"" << name << "\" !!" << endl;
      return false;
   }
   if (existNetName(name)) {
      Msg(MSG_ERR) << "Net Name \"" << name << "\" Already Exists !!" << endl;
      return false;
   }
   V3NetStrHash::iterator it = _netHash.find(index);
   if (_netHash.end() != it) {
      _nameHash.erase(_nameHash.find(it->second));
      _netHash[index] = name;
   }
   else if (name.size()) _netHash.insert(make_pair(index, name));
   if (name.size()) _nameHash.insert(make_pair(name, V3NetId::makeNetId(index)));
   assert (!name.size() || existNetName(name)); return true;
}

const bool
V3NtkHandler::resetOutName(const uint32_t& index, const string& name) {
   // There is an unimplemented optimization: stores the mapping only if name != getNetName(PO[index])
   assert (index < _ntk->getOutputSize());
   if (name == getOutputName(index)) return true;
   if (name.size() && _outNameHash.end() != _outNameHash.find(name)) {
      Msg(MSG_ERR) << "Output Name \"" << name << "\" Already Exists !!" << endl;
      return false;
   }
   V3IdxStrHash::iterator it = _outIndexHash.find(index);
   if (_outIndexHash.end() != it) {
      if (it->second.size()) _outNameHash.erase(_outNameHash.find(it->second));
      _outIndexHash[index] = name;
   }
   else if (name.size()) _outIndexHash.insert(make_pair(index, name));
   if (name.size()) _outNameHash.insert(make_pair(name, index)); return true;
}

const bool
V3NtkHandler::existNetName(const string& s) const {
   // Current Network
   if (_nameHash.end() != _nameHash.find(s)) return true;
   if (_outNameHash.end() != _outNameHash.find(s)) return true;
   // Parent Networks
   return _handler ? _handler->existNetName(s) : false;
}

const V3NetId
V3NtkHandler::getNetFromName(const string& s) const {
   // Current Network
   V3StrNetHash::const_iterator it = _nameHash.find(s);
   if (it != _nameHash.end()) return it->second;
   V3StrIdxHash::const_iterator is = _outNameHash.find(s);
   if (is != _outNameHash.end() && is->second < _ntk->getOutputSize()) return _ntk->getOutput(is->second);
   // Parent Networks
   if (!_handler) return V3NetUD;
   const V3NetId id = _handler->getNetFromName(s);
   return (V3NetUD == id) ? id : getCurrentNetId(id);
}

void
V3NtkHandler::getNetName(V3NetId& id, string& name) const {
   if (V3NetUD == id) return;
   // Current Network
   if (!id.cp) {
      V3NetStrHash::const_iterator it = _netHash.find(id.id);
      if (it != _netHash.end()) { name = it->second; return; }
   }
   // Parent Networks
   if (_handler) _handler->getNetName(id = getParentNetId(id), name);
}

const V3NetId
V3NtkHandler::getParentNetId(const V3NetId& id) const {
   return V3NetUD;
}

const V3NetId
V3NtkHandler::getCurrentNetId(const V3NetId& id, const uint32_t& index) const {
   return V3NetUD;
}

// FSM (STG) Maintanence Functions
void
V3NtkHandler::setFSM(const string& fsmName, V3FSMExtract* const fsmList) {
   assert (fsmList && fsmList->getFSMListSize() && isLegalFSMName(fsmName) && !existFSM(fsmName));
   _fsm.insert(make_pair(fsmName, fsmList));
}

const string
V3NtkHandler::getAuxFSMName() const {
   const string fsmNamePrefix = "fsm";
   string fsmName = ""; uint32_t i = 0;
   while (true) {
      fsmName = fsmNamePrefix + v3Int2Str(++i);
      if (!existFSM(fsmName)) break;
   }
   assert (isLegalFSMName(fsmName));
   return fsmName;
}

const bool
V3NtkHandler::isLegalFSMName(const string& fsmName) const {
   for (uint32_t i = 0; i < fsmName.size(); ++i)
      if (' ' == fsmName[i] || '[' == fsmName[i] || ']' == fsmName[i] || '-' == fsmName[i] || 
          '$' == fsmName[i] || !isprint(fsmName[i]) || (!i && !isalpha(fsmName[i]))) return false;
   if ("PI" == fsmName || "PO" == fsmName || "PIO" == fsmName || "FF" == fsmName) return false;
   return fsmName.size();
}

// Property Maintanence Functions
void
V3NtkHandler::setProperty(V3Property* const p) {
   assert (p); assert (p->getLTLFormula()->getName().size());
   assert (!existProperty(p->getLTLFormula()->getName()));
   _property.insert(make_pair(p->getLTLFormula()->getName(), p));
}

const string
V3NtkHandler::getAuxPropertyName() const {
   const string pNamePrefix = "p";
   string pName = ""; uint32_t i = 0;
   while (true) {
      pName = pNamePrefix + v3Int2Str(++i);
      if (!existProperty(pName)) break;
   }
   return pName;
}

// Ntk Verbosity Functions
void
V3NtkHandler::printVerbositySettings() {
   if (reduceON())  Msg(MSG_IFO) << "-REDuce ";
   if (strashON())  Msg(MSG_IFO) << "-Strash ";
   if (rewriteON()) Msg(MSG_IFO) << "-REWrite ";
   if (P2CMapON())  Msg(MSG_IFO) << "-Fwd_map ";
   if (C2PMapON())  Msg(MSG_IFO) << "-Bwd_map ";
}

// Ntk Printing Functions
void
V3NtkHandler::printSummary() const {
   assert (_ntk);
   Msg(MSG_IFO) << "#PI = " << _ntk->getInputSize() << ", #PO = " << _ntk->getOutputSize() << ", "
                << "#PIO = " << _ntk->getInoutSize() << ", #FF = " << _ntk->getLatchSize() << ", "
                << "#Const = " << _ntk->getConstSize() << ", #Net = " << _ntk->getNetSize() - 1 << endl;
}

void
V3NtkHandler::printPrimary() const {
   assert (_ntk); Msg(MSG_IFO) << "Primary Inputs  = ";
   for (uint32_t i = 0; i < _ntk->getInputSize(); ++i) {
      if (i && !(i % 5)) Msg(MSG_IFO) << endl << string(strlen("Primary Inputs  = "), ' ');
      else if (i) Msg(MSG_IFO) << ", "; Msg(MSG_IFO) << getInputName(i);
      if (_ntk->getNetWidth(_ntk->getInput(i)) > 1) Msg(MSG_IFO) << "(" << _ntk->getNetWidth(_ntk->getInput(i)) << ")";
   }
   Msg(MSG_IFO) << endl;
   Msg(MSG_IFO) << "Primary Outputs = ";
   for (uint32_t i = 0; i < _ntk->getOutputSize(); ++i) {
      if (i && !(i % 5)) Msg(MSG_IFO) << endl << string(strlen("Primary Outputs = "), ' ');
      else if (i) Msg(MSG_IFO) << ", "; Msg(MSG_IFO) << getOutputName(i);
      if (_ntk->getNetWidth(_ntk->getOutput(i)) > 1) Msg(MSG_IFO) << "(" << _ntk->getNetWidth(_ntk->getOutput(i)) << ")";
   }
   Msg(MSG_IFO) << endl;
   Msg(MSG_IFO) << "Primary Inouts = ";
   for (uint32_t i = 0; i < _ntk->getInoutSize(); ++i) {
      if (i && !(i % 5)) Msg(MSG_IFO) << endl << string(strlen("Primary Inouts = "), ' ');
      else if (i) Msg(MSG_IFO) << ", "; Msg(MSG_IFO) << getInoutName(i);
      if (_ntk->getNetWidth(_ntk->getInout(i)) > 1) Msg(MSG_IFO) << "(" << _ntk->getNetWidth(_ntk->getInout(i)) << ")";
   }
   Msg(MSG_IFO) << endl;
}

void
V3NtkHandler::printVerbose() const {
   // Collect Gates
   assert (_ntk); V3UI32Vec gateCount(V3_XD, 0);
   for (uint32_t i = 1, j = _ntk->getNetSize(); i < j; ++i) ++gateCount[_ntk->getGateType(V3NetId::makeNetId(i, 0))];
   Msg(MSG_IFO) << "==================================================" << endl;
   // Print Gates
   uint32_t total = _ntk->getInputSize() + _ntk->getInoutSize() + _ntk->getLatchSize();
   for (V3GateType type = AIG_NODE; type < V3_XD; type = (V3GateType)(type + 1)) {
      if (!gateCount[type]) continue; total += gateCount[type];
      Msg(MSG_IFO) << "   " << left << setw(12) << V3GateTypeStr[type] << right << setw(32) << gateCount[type] << endl;
   }
   Msg(MSG_IFO) << "--------------------------------------------------" << endl;
   if (_ntk->getInputSize())
      Msg(MSG_IFO) << "   " << left << setw(12) << "V3_PI" << right << setw(32) << _ntk->getInputSize() << endl;
   if (_ntk->getOutputSize())
      Msg(MSG_IFO) << "   " << left << setw(12) << "V3_PO" << right << setw(32) << _ntk->getOutputSize() << endl;
   if (_ntk->getInoutSize())
      Msg(MSG_IFO) << "   " << left << setw(12) << "V3_PIO" << right << setw(32) << _ntk->getInoutSize() << endl;
   if (_ntk->getLatchSize())
      Msg(MSG_IFO) << "   " << left << setw(12) << "V3_FF" << right << setw(32) << _ntk->getLatchSize() << endl;
   if (_ntk->getModuleSize())
      Msg(MSG_IFO) << "   " << left << setw(12) << "V3_MODULE" << right << setw(32) << _ntk->getModuleSize() << endl;
   Msg(MSG_IFO) << "==================================================" << endl;
   Msg(MSG_IFO) << "   " << left << setw(12) << "TOTAL" << right << setw(32) << total << endl;
   total = gateCount[V3_PI] - _ntk->getInputSize(); assert (gateCount[V3_PI] >= _ntk->getInputSize());
   if (total) Msg(MSG_IFO) << "   " << left << setw(12) << "Floating" << right << setw(32) << total << endl;
}

void
V3NtkHandler::printNetlist() const {
   assert (_ntk); V3NetId id = V3NetId::makeNetId(1); V3GateType type;
   for (uint32_t i = 1; i < _ntk->getNetSize(); ++i, ++id.id) {
      assert (i == id.id); Msg(MSG_IFO) << "[" << getNetNameOrFormedWithId(id) << "]";
      type = _ntk->getGateType(id); Msg(MSG_IFO) << " = " << V3GateTypeStr[type];
      if (V3_FF == type || AIG_NODE == type || isV3PairType(type))
         Msg(MSG_IFO) << "(.A(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 0)) << ")," 
                      << " .B(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 1)) << "))" << endl;
      else if (V3_PIO == type || isV3ReducedType(type))
         Msg(MSG_IFO) << "(.A(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 0)) << "))" << endl;
      else if (BV_MUX == type)
         Msg(MSG_IFO) << "(.F(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 0)) << "),"
                      << " .T(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 1)) << "),"
                      << " .S(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 2)) << "))" << endl;
      else if (BV_SLICE == type)
         Msg(MSG_IFO) << "(.A(" << getNetNameOrFormedWithId(_ntk->getInputNetId(id, 0)) << "[" 
                      << dynamic_cast<V3BvNtk*>(_ntk)->getInputSliceBit(id, true) << " : " 
                      << dynamic_cast<V3BvNtk*>(_ntk)->getInputSliceBit(id, false) << "))" << endl;
      else if (BV_CONST == type)
         Msg(MSG_IFO) << "(.A(" << (dynamic_cast<V3BvNtk*>(_ntk)->getInputConstValue(id)) << "))" << endl;
      else if (V3_MODULE == type)
         Msg(MSG_IFO) << "(" << _ntk->getModule(id)->getNtkRef()->getNtkName() << ")" << endl;
      else Msg(MSG_IFO) << endl;
   }
}

void
V3NtkHandler::printCombLoops() const {
   const uint32_t i = reportCombinationalLoops(_ntk);
   if (i) {
      Msg(MSG_WAR) << "Totally " << i << " Combinational Loops Found !!" << endl;
      Msg(MSG_WAR) << "Please fix them to ensure expected operations on this network !!" << endl;
   }
}

void
V3NtkHandler::printFloatings() const {
   assert (_ntk); V3UI32Set inputNets; inputNets.clear();
   for (uint32_t i = 0; i < _ntk->getInputSize(); ++i) inputNets.insert(_ntk->getInput(i).id);
   assert (_ntk->getInputSize() == inputNets.size()); uint32_t j = 0;
   for (V3NetId id = V3NetId::makeNetId(0); id.id < _ntk->getNetSize(); ++id.id) {
      if (V3_PI != _ntk->getGateType(id) || id == _ntk->getClock() || 
          inputNets.end() != inputNets.find(id.id)) continue;
      if (!j) Msg(MSG_IFO) << "Floating Nets (id):"; Msg(MSG_IFO) << " " << id.id; ++j;
   }
   if (j) {
      Msg(MSG_IFO) << endl;
      Msg(MSG_WAR) << "Totally " << j << " Floating Nets Found !!" << endl;
      Msg(MSG_WAR) << "Please fix them to ensure expected operations on this network !!" << endl;
   }
}

void
V3NtkHandler::printUnreachables() const {
   assert (_ntk); uint32_t x = 0, y = 0, z = 0; V3BoolVec m(_ntk->getNetSize(), false);
   for (uint32_t i = 0; i < _ntk->getOutputSize(); ++i) dfsMarkFaninCone(_ntk, _ntk->getOutput(i), m);
   for (uint32_t i = 0; i < _ntk->getInputSize(); ++i) {
      if (m[_ntk->getInput(i).id]) continue;
      if (!x) Msg(MSG_IFO) << "Unreachable PIs (index):"; Msg(MSG_IFO) << " " << i; ++x;
   }
   for (uint32_t i = 0; i < _ntk->getInoutSize(); ++i) {
      if (m[_ntk->getInout(i).id]) continue;
      if (!y) Msg(MSG_IFO) << "Unreachable PIOs (index):"; Msg(MSG_IFO) << " " << i; ++y;
   }
   for (uint32_t i = 0; i < _ntk->getLatchSize(); ++i) {
      if (m[_ntk->getLatch(i).id]) continue;
      if (!z) Msg(MSG_IFO) << "Unreachable Latches (index):"; Msg(MSG_IFO) << " " << i; ++z;
   }
   if (x + y + z) {
      Msg(MSG_IFO) << endl;
      Msg(MSG_WAR) << "Totally " << (x + y + z) << " PI/PIO/FF Found Unreachable from Any POs !!" << endl;
      Msg(MSG_WAR) << "Please fix them to ensure expected operations on this network !!" << endl;
   }
}

void
V3NtkHandler::printNet(const V3NetId& id) const {
   assert (_ntk); assert (_ntk->getNetSize() > id.id);
   Msg(MSG_IFO) << "Net  : Id = " << (id.cp ? "~" : "") << id.id << ", "
                << "Width = " << _ntk->getNetWidth(id) << ", "
                << "Type = " << V3GateTypeStr[_ntk->getGateType(id)] << endl;
   const V3GateType type = _ntk->getGateType(id); assert (V3_XD > type);
   V3NetId netId; string name = ""; Msg(MSG_IFO) << "Expr : ";
   if (!name.size()) {
      getNetName(netId = id, name);
      if (name.size()) Msg(MSG_IFO) << (netId.cp ? V3AuxNameInvPrefix : "") + name;
   }
   if (!name.size()) {
      getNetName(netId = ~id, name);
      if (name.size()) Msg(MSG_IFO) << (netId.cp ? "" : V3AuxNameInvPrefix) + name;
   }
   if (V3_FF <= type) {
      if (name.size()) Msg(MSG_IFO) << " = ";
      Msg(MSG_IFO) << getNetRecurExpression(V3_FF == type ? _ntk->getInputNetId(id, 0) : id);
   }
   Msg(MSG_IFO) << endl;
}

// Renaming Functions
void
V3NtkHandler::setAuxRenaming() {
   V3AuxHierSeparator = "_v3_hier_";
   V3AuxNameInvPrefix = "v3_inv_";
   V3AuxNameBitPrefix = "_v3_slice_";
   V3AuxNameBitSuffix = "";
   V3AuxExpansionName = "_v3_expand_at_";
   V3AuxNetNamePrefix = "v3_" + v3Int2Str(time(NULL)) + "_";
}

void
V3NtkHandler::resetAuxRenaming() {
   V3AuxHierSeparator = V3HierSeparator;
   V3AuxNameInvPrefix = V3NameInvPrefix;
   V3AuxNameBitPrefix = V3NameBitPrefix;
   V3AuxNameBitSuffix = V3NameBitSuffix;
   V3AuxExpansionName = V3ExpansionName;
   V3AuxNetNamePrefix = V3NetNamePrefix;
}

const string
V3NtkHandler::applyAuxNetNamePrefix(const string& name) {
   return V3AuxNetNamePrefix + name;
}

// Private Helper Functions
const V3NetId
V3NtkHandler::getPrimitiveNetId(const V3NetId& id) const {
   if (V3NetUD == id) return id;
   return (_handler) ? _handler->getPrimitiveNetId(getParentNetId(id)) : id;
}

/* -------------------------------------------------- *\
 * Class V3Handler Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3Handler::V3Handler() {
   _curHandlerId = 0; _lastHandlerId = 0;
   _ntkHandlerList.clear(); _curRefIdVec.clear(); _lastRefIdVec.clear();
}

V3Handler::~V3Handler() {
   uint32_t i = _ntkHandlerList.size(); while (i--) { delete _ntkHandlerList[i]; }
   _ntkHandlerList.clear(); _curRefIdVec.clear(); _lastRefIdVec.clear();
}

// Ntk Ancestry and Hierarchy Maintanence Functions
V3NtkHandler* const
V3Handler::getHandler(const uint32_t& ntkId) const {
   return (ntkId < getHandlerCount()) ? _ntkHandlerList[ntkId] : 0;
}

void
V3Handler::pushAndSetCurHandler(V3NtkHandler* const handler) {
   assert (handler); _ntkHandlerList.push_back(handler);
   setCurHandlerFromId(getHandlerCount() - 1); assert (handler == getCurHandler());
}

void
V3Handler::setCurHandlerFromId(const uint32_t& ntkId) {
   assert (ntkId < getHandlerCount());
   _lastHandlerId = _curHandlerId; _curHandlerId = ntkId;
   _lastRefIdVec = _curRefIdVec; _curRefIdVec.clear(); _curRefIdVec.push_back(ntkId);
}

void
V3Handler::setCurHandlerFromPath(const string& path) {
   assert (path.size());
   string curPath = path;
   uint32_t i;
   // Split Path by '/'
   V3Vec<string>::Vec idStr; idStr.clear();
   while (curPath.size()) {
      for (i = 0; i < curPath.size(); ++i) if ('/' == curPath[i]) break;
      if (!i) idStr.push_back("");
      else idStr.push_back(curPath.substr(0, i));
      if ((i + 1) >= curPath.size()) break;
      else curPath = curPath.substr(i + 1);
   }
   // Copy Current Handler and RefId
   const uint32_t curHandlerId = _curHandlerId; assert (getHandler(curHandlerId));
   V3UI32Vec curRefIdVec = _curRefIdVec; assert (curRefIdVec.size());
   V3NtkHandler* handler = 0;
   bool isRoot = false; int temp;
   for (i = 0; i < idStr.size(); ++i) {
      if (handler) {  // Expecting Sub-Module Index
         if (!idStr[i].size()) continue;
         if (v3Str2Int(idStr[i], temp)) {
            const uint32_t id = (uint32_t)temp;
            if (id < handler->getNtk()->getModuleSize()) {
               handler = handler->getNtk()->getModule(id)->getNtkRef();
               uint32_t j = 0; for (; j < _ntkHandlerList.size(); ++j) if (handler == _ntkHandlerList[j]) break;
               _curHandlerId = j; _curRefIdVec.push_back(id);
            }
            else {
               string recoverPath = "";
               for (uint32_t j = 0; j < _curRefIdVec.size(); ++j) recoverPath += ("/" + v3Int2Str(_curRefIdVec[j]));
               Msg(MSG_ERR) << "Sub-Module of " << recoverPath << " with Index = " 
                            << idStr[i] << " Does NOT Exist !!" << endl; return;
            }
         }
      }
      else {  // Expecting NtkID or Location Symbols (/, ./, ~/, .)
         if (!idStr[i].size()) isRoot = true;
         else {
            // Lex if current token is a SubModule Index
            if (v3Str2Int(idStr[i], temp)) {
               const uint32_t id = (uint32_t)temp;
               if (isRoot) {
                  if (id < getHandlerCount()) {
                     handler = getHandler(id); _curHandlerId = id;
                     _curRefIdVec.clear(); _curRefIdVec.push_back(id);
                  }
                  else {
                     Msg(MSG_ERR) << "Ntk with ID = " << idStr[i] << " Does NOT Exist !!" << endl; return;
                  }
               }
               else {
                  if (id < getCurHandler()->getNtk()->getModuleSize()) {
                     handler = getCurHandler()->getNtk()->getModule(id)->getNtkRef();
                     uint32_t j = 0; for (; j < _ntkHandlerList.size(); ++j) if (handler == _ntkHandlerList[j]) break;
                     _curHandlerId = j; _curRefIdVec.push_back(id);
                  }
                  else {
                     string recoverPath = "";
                     for (uint32_t j = 0; j < _curRefIdVec.size(); ++j) recoverPath += ("/" + v3Int2Str(_curRefIdVec[j]));
                     Msg(MSG_ERR) << "Sub-Module of " << recoverPath << " with Index = " 
                                  << idStr[i] << " Does NOT Exist !!" << endl; return;
                  }
               }
            }
            else if (idStr[i].size() > 1) {
               Msg(MSG_ERR) << "Unexpected Symbol \"" << idStr[i] << "\" in the Path !!" << endl; return;
            }
            else if ('~' == idStr[i][0]) {
               const uint32_t baseId = _curHandlerId = _curRefIdVec[0]; handler = getHandler(baseId);
               _curRefIdVec.clear(); _curRefIdVec.push_back(baseId);
            }
            else if ('.' == idStr[i][0]) handler = getCurHandler(); assert (handler);
         }
      }
   }
   // Copy Data for Last if Valid, Or Resume Data for Current
   if (_curHandlerId < _ntkHandlerList.size()) {
      _lastHandlerId = curHandlerId; _lastRefIdVec = curRefIdVec;
   }
   else {
      _curHandlerId = curHandlerId; _curRefIdVec = curRefIdVec;
      Msg(MSG_ERR) << "Network " << path << " was Implicitly Created and it is Currently Untraceable !!" << endl;
   }
}

void
V3Handler::setLastHandler() {
   assert (getHandlerCount()); if (_curHandlerId == _lastHandlerId) return;
   const uint32_t curHandlerId = _curHandlerId; assert (getHandler(curHandlerId));
   V3UI32Vec curRefIdVec = _curRefIdVec; assert (curRefIdVec.size());
   _curHandlerId = _lastHandlerId; _curRefIdVec = _lastRefIdVec;
   _lastHandlerId = curHandlerId; _lastRefIdVec = curRefIdVec;
}

void
V3Handler::setRootHandler() {
   assert (getHandlerCount());
   const V3NtkHandler* const handler = getCurHandler()->getPrimitive(); assert (handler);
   if (getCurHandler() == handler) return;
   // Compute NtkID of Root
   uint32_t i = 0; for (; i < _ntkHandlerList.size(); ++i) if (handler == _ntkHandlerList[i]) break;
   assert (i < _ntkHandlerList.size()); _lastHandlerId = _curHandlerId; _lastRefIdVec = _curRefIdVec;
   _curHandlerId = i; _curRefIdVec.clear(); _curRefIdVec.push_back(_curHandlerId);
}

void
V3Handler::setPrevHandler() {
   assert (getHandlerCount()); if (_curRefIdVec.size() == 1) return;
   if (_curRefIdVec.size() == 2) return setBaseHandler();
   _lastHandlerId = _curHandlerId; _lastRefIdVec = _curRefIdVec;
   _curRefIdVec.pop_back(); assert (_curRefIdVec.size());
   V3NtkHandler* handler = getHandler(_curRefIdVec[0]); assert (handler);
   for (uint32_t i = 1; i < _curRefIdVec.size(); ++i) {
      assert (_curRefIdVec[i] < handler->getNtk()->getModuleSize());
      handler = handler->getNtk()->getModule(i)->getNtkRef(); assert (handler);
   }
   // Compute NtkID of Prev
   uint32_t i = 0; for (; i < _ntkHandlerList.size(); ++i) if (handler == _ntkHandlerList[i]) break;
   assert (i < _ntkHandlerList.size()); _curHandlerId = i;
}

void
V3Handler::setBaseHandler() {
   assert (getHandlerCount()); if (_curRefIdVec.size() == 1) return;
   _lastHandlerId = _curHandlerId; _lastRefIdVec = _curRefIdVec;
   _curHandlerId = _curRefIdVec[0]; _curRefIdVec.clear(); _curRefIdVec.push_back(_curHandlerId);
}

// Print Functions
void
V3Handler::printNtkInAncestry() const {
   if (!getHandlerCount()) return;
   V3HandlerVec ancestry; ancestry.clear();
   V3NtkHandler* handler = getCurHandler(); assert (handler);
   while (true) {
      ancestry.push_back(handler); if (handler == handler->getPrimitive()) break;
      handler = const_cast<V3NtkHandler*>(handler->getPrimitive());
   }
   uint32_t i = ancestry.size(), j;
   while (i--) {
      for (j = 0; j < _ntkHandlerList.size(); ++j) if (ancestry[i] == _ntkHandlerList[j]) break;
      assert (j < _ntkHandlerList.size()); Msg(MSG_IFO) << "/" << j;
   }
   Msg(MSG_IFO) << endl;
}

void
V3Handler::printNtkInHierarchy() const {
   if (!_curRefIdVec.size()) return;
   for (uint32_t i = 0; i < _curRefIdVec.size(); ++i)
      Msg(MSG_IFO) << "/" << _curRefIdVec[i];
   if (_curRefIdVec.size() > 1) Msg(MSG_IFO) << " --> /" << _curHandlerId;
   Msg(MSG_IFO) << endl;
}

void
V3Handler::printRecurHierarchy(V3NtkHandler* const handler, const uint32_t& maxLevel, const uint32_t& level) const {
   assert (handler); assert (handler->getNtk());
   uint32_t i = 0; for (; i < _ntkHandlerList.size(); ++i) if (handler == _ntkHandlerList[i]) break;
   const string curPath = "/" + v3Int2Str(i);
   assert (i < _ntkHandlerList.size()); Msg(MSG_IFO) << setw(5) << left << curPath;
   if ((!handler->getNtk()->getModuleSize()) || (maxLevel == level)) { Msg(MSG_IFO) << endl; return; }
   for (i = 0; i < handler->getNtk()->getModuleSize(); ++i) {
      if (i) for (i = 0; i <= level; ++i) Msg(MSG_IFO) << "     ";  // setw(5)
      printRecurHierarchy(handler->getNtk()->getModule(i)->getNtkRef(), maxLevel, 1 + level);
   }
}

void
V3Handler::printNtkRelation(const string& fileName) const {
}

#endif

