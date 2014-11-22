/****************************************************************************
  FileName     [ v3Ntk.cpp ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_C
#define V3_NTK_C

#include "v3Ntk.h"
#include "v3Msg.h"
#include "v3StrUtil.h"

/* -------------------------------------------------- *\
 * Class V3Ntk Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3Ntk::V3Ntk() {
   for (uint32_t i = 0; i < 3; ++i) _IOList[i].clear();
   _FFList.clear(); _ConstList.clear(); _typeMisc.clear();
   _inputData.clear(); _cutSignals.clear(); _ntkModule.clear();
   _globalMisc = 0; _globalClk = V3NetUD;
}

V3Ntk::~V3Ntk() {
}

// Ntk Construction Functions
void
V3Ntk::initialize() {
   assert (!_inputData.size());
   // Create Constant AIG_FALSE
   const V3NetId id = createNet(1); assert (!id.id); createConst(id);
}

const V3NetId
V3Ntk::createNet(const uint32_t& width) {
   // Validation Check
   if (width != 1) {
      Msg(MSG_ERR) << "Unexpected Net width = " << width << " in Base / AIG Network !!" << endl;
      return V3NetUD;
   }
   assert (_inputData.size() == _typeMisc.size());
   // Create New V3NetId
   V3NetId id = V3NetId::makeNetId(_inputData.size()); assert (!isV3NetInverted(id));
   _typeMisc.push_back(V3MiscType()); _inputData.push_back(V3InputVec());
   assert (V3_PI == _typeMisc.back().type); assert (!_typeMisc.back().misc);
   assert (!_inputData.back().size()); return id;
}

void
V3Ntk::createModule(V3NtkModule* const module) {
   assert (module); assert (module->getNtkRef());
   _ntkModule.push_back(module);
}

void
V3Ntk::createInput(const V3NetId& id) {
   assert (validNetId(id)); assert (!isV3NetInverted(id));
   assert (!reportMultipleDrivenNet(V3_PI, id));
   _IOList[0].push_back(id);
}

void
V3Ntk::createOutput(const V3NetId& id) {
   assert (validNetId(id)); _IOList[1].push_back(id);
}

void
V3Ntk::createInout(const V3NetId& id) {
   assert (validNetId(id)); assert (!isV3NetInverted(id));
   createGate(V3_PIO, id); _IOList[2].push_back(id);
}

void
V3Ntk::createLatch(const V3NetId& id) {
   assert (validNetId(id)); assert (!isV3NetInverted(id));
   createGate(V3_FF, id); _FFList.push_back(id);
}

void
V3Ntk::createConst(const V3NetId& id) {
   assert (validNetId(id)); assert (!isV3NetInverted(id));
   createGate(dynamic_cast<const V3BvNtk*>(this) ? BV_CONST : AIG_FALSE, id); _ConstList.push_back(id);
}

void
V3Ntk::createClock(const V3NetId& id) {
   assert (validNetId(id)); assert (V3_PI == getGateType(id));
   assert (V3NetUD == _globalClk); _globalClk = id;
}

void
V3Ntk::setInput(const V3NetId& id, const V3InputVec& inputs) {
   assert (validNetId(id)); assert (V3_PI == getGateType(id));
   V3InputVec& fanInVec = _inputData[id.id]; assert (!fanInVec.size());
   for (uint32_t i = 0; i < inputs.size(); ++i) fanInVec.push_back(inputs[i]);
}

void
V3Ntk::createGate(const V3GateType& type, const V3NetId& id) {
   // Check Validation
   assert (validNetId(id)); assert (type < V3_XD); assert (type > V3_PI);
   assert (dynamic_cast<const V3BvNtk*>(this) || type <= AIG_FALSE);
   assert (!(dynamic_cast<const V3BvNtk*>(this)) || (type <= V3_MODULE || type > AIG_FALSE));
   assert (!reportMultipleDrivenNet(type, id)); assert (!reportUnexpectedFaninSize(type, id));
   // Set Gate Type
   _typeMisc[id.id].type = type;
}

// Ntk Reconstruction Functions
void
V3Ntk::replaceFanin(const V3RepIdHash& repIdHash) {
   assert (repIdHash.size());
   uint32_t i, inSize; V3GateType type;
   V3RepIdHash::const_iterator it;
   for (V3NetId id = V3NetId::makeNetId(1); id.id < _inputData.size(); ++id.id) {
      type = getGateType(id);
      if (V3_MODULE == type) {
         V3NtkModule* const module = getModule(_inputData[id.id][0].value); assert (module);
         for (i = 0; i < module->getInputList().size(); ++i) {
            it = repIdHash.find(module->getInputList()[i].id); if (repIdHash.end() == it) continue;
            module->updateInput(i, module->getInputList()[i].cp ? ~(it->second) : it->second);
         }
      }
      else {
         inSize = (AIG_NODE == type || isV3PairType(type)) ? 2 : (BV_MUX == type) ? 3 : 
                  (V3_FF == type || BV_SLICE == type || isV3ReducedType(type)) ? 1 : 0;
         for (i = 0; i < inSize; ++i) {
            it = repIdHash.find(_inputData[id.id][i].id.id); if (repIdHash.end() == it) continue;
            _inputData[id.id][i] = V3NetType(_inputData[id.id][i].id.cp ? ~(it->second) : it->second);
         }
      }
   }
}

void
V3Ntk::replaceOutput(const uint32_t& index, const V3NetId& id) {
   assert (index < getOutputSize()); assert (validNetId(id)); _IOList[1][index] = id;
}

// Ntk Structure Functions
const uint32_t
V3Ntk::getNetWidth(const V3NetId& id) const {
   assert (validNetId(id)); return 1;
}

// Helper Functions for Ntk Construction Validation Check
const bool
V3Ntk::reportInvertingNet(const V3NetId& id) const {
   assert (validNetId(id));
   if (isV3NetInverted(id))
      Msg(MSG_ERR) << "Unexpected Inverting Net = " << id.id << endl;
   else return false;
   return true;
}

const bool
V3Ntk::reportMultipleDrivenNet(const V3GateType& type, const V3NetId& id) const {
   assert (type < V3_GATE_TOTAL); assert (validNetId(id));
   if (V3_PI != getGateType(id))
      Msg(MSG_ERR) << "Multiple-Driven @ " << id.id << "(" << V3GateTypeStr[type] << ")"
                   << ", Exist " << V3GateTypeStr[getGateType(id)] << endl;
   else if (_globalClk.id == id.id)
      Msg(MSG_ERR) << "Clock Signal \"" << _globalClk.id << "\" Cannot be Driven, found @ " 
                   << id.id << "(" << V3GateTypeStr[type] << ")" << endl;
   else return false;
   return true;
}

const bool
V3Ntk::reportUnexpectedFaninSize(const V3GateType& type, const V3NetId& id) const {
   assert (type < V3_XD); assert (validNetId(id));
   if (((isV3ReducedType(type) || BV_CONST == type || V3_PIO == type) && (getInputNetSize(id) != 1)) || 
       ((isV3PairType(type) || AIG_NODE == type || V3_FF == type || BV_SLICE == type) && (getInputNetSize(id) != 2)) || 
       ((BV_MUX == type) && (getInputNetSize(id) != 3)) || ((V3_MODULE == type) && (getInputNetSize(id) != 1)))
      Msg(MSG_ERR) << "Expecting Fanin Size is One while " << getInputNetSize(id) 
                   << " is Found @ " << id.id << " for Gate Type = " << V3GateTypeStr[type] << endl;
   else return false;
   return true;
}

const bool
V3Ntk::reportNetWidthInconsistency(const V3NetId& id1, const V3NetId& id2, const string& s) const {
   if (getNetWidth(id1) != getNetWidth(id2))
      Msg(MSG_ERR) << "Width of " << s << " are inconsistent : " << id1.id << "(" << getNetWidth(id1) << ") != " 
                   << id2.id << "(" << getNetWidth(id2) << ")" << endl;
   else return false;
   return true;
}

const bool
V3Ntk::reportUnexpectedNetWidth(const V3NetId& id, const uint32_t& width, const string& s) const {
   if (width != getNetWidth(id))
      Msg(MSG_ERR) << "Width of " << s << " is unexpected : " << id.id << "(" << getNetWidth(id) << ") != " 
                   << width << endl;
   else return false;
   return true;
}

/* -------------------------------------------------- *\
 * Class V3BvNtk Implementations
\* -------------------------------------------------- */
// Initialization to Static Members
V3ConstHash  V3BvNtk::_V3ConstHash = V3ConstHash();
V3BusIdHash  V3BvNtk::_V3BusIdHash = V3BusIdHash();
V3BitVecXVec V3BvNtk::_V3ValueVec  = V3BitVecXVec();
V3BusPairVec V3BvNtk::_V3BusIdVec  = V3BusPairVec();

// Constructors for BV Network Gates
V3BvNtk::V3BvNtk() : V3Ntk() {
   _netWidth.clear();
}

V3BvNtk::V3BvNtk(const V3BvNtk& ntk) : V3Ntk(ntk) {
   _netWidth = ntk._netWidth;
}

V3BvNtk::~V3BvNtk() {
};

// Ntk Construction Functions
void
V3BvNtk::initialize() {
   assert (!_inputData.size());
   // Create Constant BV_CONST = 1'b0 for Sync with AIG_FALSE
   const V3NetId id = createNet(1); assert (!id.id);
   _inputData.back().push_back(0); createConst(id);
}

const V3NetId
V3BvNtk::createNet(const uint32_t& width) {
   // Validation Check
   if (!width) {
      Msg(MSG_ERR) << "Unexpected Net width = " << width << " in BV Network !!" << endl;
      return V3NetUD;
   }
   assert (_inputData.size() == _netWidth.size()); _netWidth.push_back(width);
   assert (_inputData.size() == _typeMisc.size());
   // Create New V3NetId
   V3NetId id = V3NetId::makeNetId(_inputData.size()); assert (!isV3NetInverted(id));
   _typeMisc.push_back(V3MiscType()); _inputData.push_back(V3InputVec());
   assert (V3_PI == _typeMisc.back().type); assert (!_typeMisc.back().misc);
   assert (!_inputData.back().size()); return id;
}

// Ntk Structure Functions
const uint32_t
V3BvNtk::getNetWidth(const V3NetId& id) const {
   assert (validNetId(id)); return _netWidth[id.id];
}

// Ntk for BV Gate Functions
const V3BVXId
V3BvNtk::hashV3ConstBitVec(const string& input_exp) {
   V3ConstHash::const_iterator it = _V3ConstHash.find(input_exp);
   if (it != _V3ConstHash.end()) return it->second;
   V3BitVecX* const value = new V3BitVecX(input_exp.c_str()); assert (value);
   const string exp = v3Int2Str(value->size()) + "'" + value->toExp();
   if (input_exp != exp) {
      it = _V3ConstHash.find(exp);
      if (it != _V3ConstHash.end()) { delete value; return it->second; }
   }
   assert (_V3ConstHash.size() == _V3ValueVec.size());
   _V3ConstHash.insert(make_pair(exp, _V3ValueVec.size()));
   _V3ValueVec.push_back(value); return _V3ValueVec.size() - 1;
}

const V3BusId
V3BvNtk::hashV3BusId(const uint32_t& msb, const uint32_t& lsb) {
   const V3PairType msb_lsb(msb, lsb);
   assert (msb == msb_lsb.bus[0] && lsb == msb_lsb.bus[1]);
   V3BusIdHash::const_iterator it = _V3BusIdHash.find(msb_lsb.pair);
   if (it != _V3BusIdHash.end()) return it->second;
   assert (_V3BusIdHash.size() == _V3BusIdVec.size());
   _V3BusIdHash.insert(make_pair(msb_lsb.pair, _V3BusIdVec.size()));
   _V3BusIdVec.push_back(msb_lsb); return _V3BusIdVec.size() - 1;
}

const uint32_t
V3BvNtk::getConstWidth(const V3BVXId& valueId) const {
   assert (valueId < _V3ValueVec.size()); return _V3ValueVec[valueId]->size();
}

const uint32_t
V3BvNtk::getSliceWidth(const V3BusId& busId) const {
   assert (busId < _V3BusIdVec.size());
   return 1 + ((_V3BusIdVec[busId].bus[0] > _V3BusIdVec[busId].bus[1]) ? 
              (_V3BusIdVec[busId].bus[0] - _V3BusIdVec[busId].bus[1]) : 
              (_V3BusIdVec[busId].bus[1] - _V3BusIdVec[busId].bus[0]));
}

const V3BitVecX
V3BvNtk::getConstValue(const V3BVXId& valueId) const {
   assert (valueId < _V3ValueVec.size()); return *(_V3ValueVec[valueId]);
}

const uint32_t
V3BvNtk::getSliceBit(const V3BusId& busId, const bool& msb) const {
   assert (busId < _V3BusIdVec.size());
   return msb ? _V3BusIdVec[busId].bus[0] : _V3BusIdVec[busId].bus[1];
}

const V3BitVecX
V3BvNtk::getInputConstValue(const V3NetId& id) const {
   assert (validNetId(id)); assert (BV_CONST == getGateType(id));
   return id.cp ? ~getConstValue(_inputData[id.id][0].value) : getConstValue(_inputData[id.id][0].value);
}

const uint32_t
V3BvNtk::getInputSliceBit(const V3NetId& id, const bool& msb) const {
   assert (validNetId(id)); assert (BV_SLICE == getGateType(id));
   const V3BusId busId = _inputData[id.id][1].value; assert (busId < _V3BusIdVec.size());
   return msb ? _V3BusIdVec[busId].bus[0] : _V3BusIdVec[busId].bus[1];
}

// Ntk Reconstruction Functions
void
V3BvNtk::resetNetWidth(const V3NetId& id, const uint32_t& width) {
   assert (validNetId(id)); assert (V3_PI == getGateType(id));
   assert (width); _netWidth[id.id] = width;
}

#endif

