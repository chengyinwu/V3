/****************************************************************************
  FileName     [ v3AlgSimulate.cpp ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ Algorithm for V3 Network Simulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_SIMULATE_C
#define V3_ALG_SIMULATE_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3AlgSimulate.h"

/* -------------------------------------------------- *\
 * Class V3AlgSimulate Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgSimulate::V3AlgSimulate(const V3NtkHandler* const handler) : _handler(handler) {
   assert (handler->getNtk()); assert (!handler->getNtk()->getModuleSize()); resetSimulator();
}

V3AlgSimulate::~V3AlgSimulate() {
   _orderMap.clear();
}

// Simulation Main Functions
void
V3AlgSimulate::simulate() {}

void
V3AlgSimulate::reset(const V3NetVec& targetNets) { resetSimulator(); }

// Simulation Data Functions
const V3BitVecX
V3AlgSimulate::getSimValue(const V3NetId& id) const { return V3BitVecX(); }

// Simulation Setting Functions
void
V3AlgSimulate::updateNextStateValue() {}

void
V3AlgSimulate::setSource(const V3NetId& id, const V3BitVecX& value) {}

void
V3AlgSimulate::setSourceFree(const V3GateType& type, const bool& random) {}

void
V3AlgSimulate::clearSource(const V3NetId& id, const bool& random) {}

// Simulation with Event-Driven Functions
void
V3AlgSimulate::updateNextStateEvent() {}

void
V3AlgSimulate::setSourceEvent(const V3NetId& id, const V3BitVecX& value) {}

void
V3AlgSimulate::setSourceFreeEvent(const V3GateType& type, const bool& random) {}

// Simulation Record Functions
void
V3AlgSimulate::recordSimValue() {}

void
V3AlgSimulate::getSimRecordData(uint32_t cycle, V3SimTrace& traceData) const { traceData.clear(); }

// Simulation Output Functions
void
V3AlgSimulate::printResult() const { }

// Simulation Private Helper Function
void
V3AlgSimulate::resetSimulator() {
   _cycle = 0; _level = 0; _init[0] = _init[1] = V3NtkUD; _orderMap.clear();
   _hasEvent.clear(); _levelData.clear(); _fanoutVec.clear(); _eventList.clear();
}

void
V3AlgSimulate::initializeEventList() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   if (!_eventList.buckets()) {
      if (!_hasEvent.size()) _hasEvent = V3BoolVec(ntk->getNetSize(), false);
      if (!_levelData.size()) _level = computeLevel(ntk, _levelData, _targets);
      if (!_fanoutVec.size()) computeFanout(ntk, _fanoutVec, _targets);
      assert (_level); _eventList.init(_level);
   }
   else _eventList.clear();
   assert (_eventList.buckets() == _level); assert (!_eventList.size());
}

/* -------------------------------------------------- *\
 * Class V3AlgAigSimulate Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgAigSimulate::V3AlgAigSimulate(const V3NtkHandler* const handler) : V3AlgSimulate(handler) {
   assert (!dynamic_cast<V3BvNtk*>(handler->getNtk())); reset();
}

V3AlgAigSimulate::~V3AlgAigSimulate() {
   _dffValue.clear(); _traceData.clear(); _simValue.clear();
}

// Simulation Main Functions
void
V3AlgAigSimulate::simulate() {
   const V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Perform Combinational Simulation
   V3NetId id1, id2;
   for (uint32_t i = (_cycle ? _init[1] : _init[0]); i < _orderMap.size(); ++i) {
      assert (AIG_NODE == ntk->getGateType(_orderMap[i]));
      id1 = ntk->getInputNetId(_orderMap[i], 0); id2 = ntk->getInputNetId(_orderMap[i], 1);
      _simValue[_orderMap[i].id].bv_and(_simValue[id1.id], id1.cp, _simValue[id2.id], id2.cp);
   }
   // Record DFF Next State Value
   const uint32_t index = (_cycle) ? 0 : 1; ++_cycle;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      id1 = ntk->getInputNetId(ntk->getLatch(i), index);
      _dffValue[i] = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
   }
}

void
V3AlgAigSimulate::reset(const V3NetVec& targetNets) {
   // Initialize Simulation Order
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); resetSimulator(); _targets = targetNets;
   _init[0] = ntk->getConstSize() + ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize();
   _init[1] = dfsNtkForSimulationOrder(_handler->getNtk(), _orderMap, _targets); assert (_init[1] >= _init[0]);
   // Initialize Simulation Data and Reset Value
   _simValue = V3AigSimDataVec(ntk->getNetSize()); assert (_orderMap.size() <= _simValue.size());
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) {
      assert (_orderMap[i] == ntk->getConst(i)); _simValue[ntk->getConst(i).id].setZeros(~0ul);
   }
   _dffValue = V3AigSimDataVec(ntk->getLatchSize()); _traceData.clear(); _simRecord.clear(); _cycle = 0;
   // Initial Simulation : PI, PIO, DFF = X
   setSourceFree(V3_PI, false); setSourceFree(V3_PIO, false); setSourceFree(V3_FF, false); simulate();
}

// Simulation Data Functions
const V3BitVecX
V3AlgAigSimulate::getSimValue(const V3NetId& id) const {
   assert (id.id < _simValue.size()); return id.cp ? ~_simValue[id.id] : _simValue[id.id];
}

// Simulation Setting Functions
void
V3AlgAigSimulate::updateNextStateValue() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) _simValue[getV3NetIndex(ntk->getLatch(i))] = _dffValue[i];
}

void
V3AlgAigSimulate::setSource(const V3NetId& id, const V3BitVecX& value) {
   assert (id.id < _simValue.size()); _simValue[id.id] = id.cp ? ~value : value;
   if (value.size() == 1) {
      if ('0' == value[0]) _simValue[id.id].setZeros(-1);
      if ('1' == value[0]) _simValue[id.id].setOnes(-1);
      assert (_simValue[id.id][0] == (id.cp ? ~value : value)[0]);
   }
}

void
V3AlgAigSimulate::setSourceFree(const V3GateType& type, const bool& random) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (type <= V3_FF);
   if (V3_PI == type)
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
         if (!random) _simValue[getV3NetIndex(ntk->getInput(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInput(i))].random();
   else if (V3_FF == type)
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
         if (!random) _simValue[getV3NetIndex(ntk->getLatch(i))].clear();
         else _simValue[getV3NetIndex(ntk->getLatch(i))].random();
   else
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
         if (!random) _simValue[getV3NetIndex(ntk->getInout(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInout(i))].random();
}

void
V3AlgAigSimulate::clearSource(const V3NetId& id, const bool& random) {
   assert (id.id < _simValue.size());
   if (!random) _simValue[id.id].clear(); else _simValue[id.id].random();
}

// Simulation with Event-Driven Functions
void
V3AlgAigSimulate::updateNextStateEvent() {
   // Initialize Event List
   initializeEventList(); assert (_cycle);
   // Record DFF Next State Value
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   const uint32_t index = (_cycle > 1) ? 0 : 1;
   V3NetId id1;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      id1 = ntk->getInputNetId(ntk->getLatch(i), index);
      _dffValue[i] = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
   }
   // Set Events ON DFF
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      if (_dffValue[i] == _simValue[getV3NetIndex(ntk->getLatch(i))]) continue;
      _simValue[getV3NetIndex(ntk->getLatch(i))] = _dffValue[i];
      assert (!_hasEvent[getV3NetIndex(ntk->getLatch(i))]);
      _hasEvent[getV3NetIndex(ntk->getLatch(i))] = true;
      _eventList.add(_levelData[ntk->getLatch(i).id], ntk->getLatch(i));
   }
   // Simulate According To Event List
   simulateEventList(); ++_cycle;
}

void
V3AlgAigSimulate::setSourceEvent(const V3NetId& id, const V3BitVecX& value) {
   // Initialize Event List
   assert (id.id < _simValue.size()); initializeEventList();
   // Set Event
   const V3BitVecS v = id.cp ? ~value : value;
   if (_simValue[id.id] == v) return; _simValue[id.id] = v;
   assert (!_hasEvent[id.id]); _hasEvent[id.id] = true;
   _eventList.add(_levelData[id.id], id);
   // Simulate According To Event List
   simulateEventList();
}

void
V3AlgAigSimulate::setSourceFreeEvent(const V3GateType& type, const bool& random) {
   // Initialize Event List
   initializeEventList();
   // Set Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (type <= V3_FF);
   V3BitVecS clearValue;
   if (V3_PI == type)
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
         if (!random) {
            if (clearValue == _simValue[getV3NetIndex(ntk->getInput(i))]) continue;
            _simValue[getV3NetIndex(ntk->getInput(i))].clear();
            assert (!_hasEvent[getV3NetIndex(ntk->getInput(i))]);
            _hasEvent[getV3NetIndex(ntk->getInput(i))] = true;
            _eventList.add(_levelData[ntk->getInput(i).id], ntk->getInput(i));
         }
         else {
            clearValue = _simValue[getV3NetIndex(ntk->getInput(i))];
            _simValue[getV3NetIndex(ntk->getInput(i))].random();
            if (clearValue == _simValue[getV3NetIndex(ntk->getInput(i))]) continue;
            assert (!_hasEvent[getV3NetIndex(ntk->getInput(i))]);
            _hasEvent[getV3NetIndex(ntk->getInput(i))] = true;
            _eventList.add(_levelData[ntk->getInput(i).id], ntk->getInput(i));
         }
   else if (V3_FF == type)
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
         if (!random) {
            if (clearValue == _simValue[getV3NetIndex(ntk->getLatch(i))]) continue;
            _simValue[getV3NetIndex(ntk->getLatch(i))].clear();
            assert (!_hasEvent[getV3NetIndex(ntk->getLatch(i))]);
            _hasEvent[getV3NetIndex(ntk->getLatch(i))] = true;
            _eventList.add(_levelData[ntk->getLatch(i).id], ntk->getLatch(i));
         }
         else {
            clearValue = _simValue[getV3NetIndex(ntk->getLatch(i))];
            _simValue[getV3NetIndex(ntk->getLatch(i))].random();
            if (clearValue == _simValue[getV3NetIndex(ntk->getLatch(i))]) continue;
            assert (!_hasEvent[getV3NetIndex(ntk->getLatch(i))]);
            _hasEvent[getV3NetIndex(ntk->getLatch(i))] = true;
            _eventList.add(_levelData[ntk->getLatch(i).id], ntk->getLatch(i));
         }
   else
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
         if (!random) {
            if (clearValue == _simValue[getV3NetIndex(ntk->getInout(i))]) continue;
            _simValue[getV3NetIndex(ntk->getInout(i))].clear();
            assert (!_hasEvent[getV3NetIndex(ntk->getInout(i))]);
            _hasEvent[getV3NetIndex(ntk->getInout(i))] = true;
            _eventList.add(_levelData[ntk->getInout(i).id], ntk->getInout(i));
         }
         else {
            clearValue = _simValue[getV3NetIndex(ntk->getInout(i))];
            _simValue[getV3NetIndex(ntk->getInout(i))].random();
            if (clearValue == _simValue[getV3NetIndex(ntk->getInout(i))]) continue;
            assert (!_hasEvent[getV3NetIndex(ntk->getInout(i))]);
            _hasEvent[getV3NetIndex(ntk->getInout(i))] = true;
            _eventList.add(_levelData[ntk->getInout(i).id], ntk->getInout(i));
         }
   // Simulate According To Event List
   simulateEventList();
}

// Simulation Record Functions
void
V3AlgAigSimulate::recordSimValue() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3AigSimDataVec trace; trace.reserve(ntk->getInputSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) trace.push_back(_simValue[getV3NetIndex(ntk->getInput(i))]);
   assert (ntk->getInputSize() == trace.size()); _traceData.push_back(trace);
}

void
V3AlgAigSimulate::getSimRecordData(uint32_t cycle, V3SimTrace& traceData) const {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (cycle < _traceData.size()); assert (ntk->getInputSize() == _traceData[cycle].size());
   if (ntk->getInputSize() != traceData.size()) traceData.reserve(ntk->getInputSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) traceData[i] = _traceData[cycle][i];
}

// Simulation Output Functions
void
V3AlgAigSimulate::printResult() const {
   V3AigNtk* const ntk = _handler->getNtk(); assert (ntk);
   V3GateType type;
   for (uint32_t i = 0; i < _orderMap.size(); ++i) {
      type = ntk->getGateType(_orderMap[i]);
      Msg(MSG_DBG) << "Order [" << i << "] = " << _orderMap[i].id << ", value = " << _simValue[_orderMap[i].id]
                   << ", type = " << V3GateTypeStr[type];
      if (AIG_NODE == type)
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputNetId(_orderMap[i], 0).id << ", " 
                      << ntk->getInputNetId(_orderMap[i], 1).id << ")" << endl;
      else Msg(MSG_DBG) << endl;
   }
}

// Private Event-Driven Simulation Functions
void
V3AlgAigSimulate::simulateEventList() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3BitVecS value; uint32_t level;
   V3NetId id, id1, id2, id3;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]); value = _simValue[id1.id];
         assert (AIG_NODE == ntk->getGateType(id1)); assert (level < _levelData[id1.id]);
         id2 = ntk->getInputNetId(id1, 0); id3 = ntk->getInputNetId(id1, 1);
         _simValue[id1.id].bv_and(_simValue[id2.id], id2.cp, _simValue[id3.id], id3.cp);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
}

/* -------------------------------------------------- *\
 * Class V3AlgBvSimulate Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgBvSimulate::V3AlgBvSimulate(const V3NtkHandler* const handler) : V3AlgSimulate(handler) {
   assert (dynamic_cast<V3BvNtk*>(handler->getNtk())); reset();
}

V3AlgBvSimulate::~V3AlgBvSimulate() {
   _dffValue.clear(); _traceData.clear(); _simValue.clear();
}

// Simulation Main Functions
void
V3AlgBvSimulate::simulate() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Perform Combinational Simulation
   for (uint32_t i = (_cycle ? _init[1] : _init[0]); i < _orderMap.size(); ++i) simulateGate(_orderMap[i]);
   // Record DFF Next State Value
   const uint32_t index = (_cycle) ? 0 : 1; ++_cycle;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      V3NetId id1 = ntk->getInputNetId(ntk->getLatch(i), index);
      _dffValue[i] = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
   }
}

void
V3AlgBvSimulate::reset(const V3NetVec& targetNets) {
   // Initialize Simulation Order
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(_handler->getNtk()); assert (ntk); resetSimulator(); _targets = targetNets;
   _init[0] = ntk->getConstSize() + ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize();
   _init[1] = dfsNtkForSimulationOrder(_handler->getNtk(), _orderMap, _targets); assert (_init[1] >= _init[0]);
   // Initialize Simulation Data and Reset Value
   _simValue = V3BvSimDataVec(ntk->getNetSize()); assert (_orderMap.size() <= _simValue.size());
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) {
      assert (_orderMap[i].id == ntk->getConst(i).id);
      _simValue[ntk->getConst(i).id] = ntk->getInputConstValue(V3NetId::makeNetId(ntk->getConst(i).id));
   }
   for (uint32_t i = ntk->getConstSize(); i < _orderMap.size(); ++i)
      _simValue[_orderMap[i].id].resize(ntk->getNetWidth(_orderMap[i]));
   _dffValue = V3BvSimDataVec(ntk->getLatchSize()); _traceData.clear(); _cycle = 0;
   // Initial Simulation : PI, PIO, DFF = X
   setSourceFree(V3_PI, false); setSourceFree(V3_PIO, false); setSourceFree(V3_FF, false); simulate();
}

// Simulation Data Functions
const V3BitVecX
V3AlgBvSimulate::getSimValue(const V3NetId& id) const {
   assert (id.id < _simValue.size()); return id.cp ? ~_simValue[id.id] : _simValue[id.id];
}

// Simulation Setting Functions
void
V3AlgBvSimulate::updateNextStateValue() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) _simValue[getV3NetIndex(ntk->getLatch(i))] = _dffValue[i];
}

void
V3AlgBvSimulate::setSource(const V3NetId& id, const V3BitVecX& value) {
   assert (id.id < _simValue.size()); assert (_simValue[id.id].size() == value.size());
   _simValue[id.id] = id.cp ? ~value : value;
}

void
V3AlgBvSimulate::setSourceFree(const V3GateType& type, const bool& random) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (V3_PI == type || V3_PIO == type || V3_FF == type);
   if (V3_PI == type)
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i)
         if (!random) _simValue[getV3NetIndex(ntk->getInput(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInput(i))].random();
   else if (V3_FF == type)
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
         if (!random) _simValue[getV3NetIndex(ntk->getLatch(i))].clear();
         else _simValue[getV3NetIndex(ntk->getLatch(i))].random();
   else
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
         if (!random) _simValue[getV3NetIndex(ntk->getInout(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInout(i))].random();
}

void
V3AlgBvSimulate::clearSource(const V3NetId& id, const bool& random) {
   assert (id.id < _simValue.size());
   if (!random) _simValue[id.id].clear(); else _simValue[id.id].random();
}

// Simulation with Event-Driven Functions
void
V3AlgBvSimulate::updateNextStateEvent() {
   // Initialize Event List
   initializeEventList(); assert (_cycle);
   // Record DFF Next State Value
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   const uint32_t index = (_cycle > 1) ? 0 : 1;
   V3NetId id1;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      id1 = ntk->getInputNetId(ntk->getLatch(i), index);
      _dffValue[i] = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
   }
   // Set Events ON DFF
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      if (_dffValue[i] == _simValue[getV3NetIndex(ntk->getLatch(i))]) continue;
      assert (!_hasEvent[getV3NetIndex(ntk->getLatch(i))]);
      _hasEvent[getV3NetIndex(ntk->getLatch(i))] = true;
      _eventList.add(_levelData[ntk->getLatch(i).id], ntk->getLatch(i));
      _simValue[getV3NetIndex(ntk->getLatch(i))] = _dffValue[i];
   }
   // Simulate According To Event List
   simulateEventList(); ++_cycle;
}

void
V3AlgBvSimulate::setSourceEvent(const V3NetId& id, const V3BitVecX& value) {
   // Initialize Event List
   assert (id.id < _simValue.size()); initializeEventList();
   assert (_simValue[id.id].size() == value.size());
   // Set Event
   const V3BitVecX v = id.cp ? ~value : value;
   if (_simValue[id.id] == v) return; _simValue[id.id] = v;
   assert (!_hasEvent[id.id]); _hasEvent[id.id] = true;
   _eventList.add(_levelData[id.id], id);
   // Simulate According To Event List
   simulateEventList();
}

void
V3AlgBvSimulate::setSourceFreeEvent(const V3GateType& type, const bool& random) {
   // Initialize Event List
   initializeEventList();
   // Set Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (type <= V3_FF);
   V3BitVecX clearValue;
   if (V3_PI == type)
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
         clearValue = _simValue[getV3NetIndex(ntk->getInput(i))];
         if (!random) _simValue[getV3NetIndex(ntk->getInput(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInput(i))].random();
         if (clearValue == _simValue[getV3NetIndex(ntk->getInput(i))]) continue;
         assert (!_hasEvent[getV3NetIndex(ntk->getInput(i))]);
         _hasEvent[getV3NetIndex(ntk->getInput(i))] = true;
         _eventList.add(_levelData[ntk->getInput(i).id], ntk->getInput(i));
      }
   else if (V3_FF == type)
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
         clearValue = _simValue[getV3NetIndex(ntk->getLatch(i))];
         if (!random) _simValue[getV3NetIndex(ntk->getLatch(i))].clear();
         else _simValue[getV3NetIndex(ntk->getLatch(i))].random();
         if (clearValue == _simValue[getV3NetIndex(ntk->getLatch(i))]) continue;
         assert (!_hasEvent[getV3NetIndex(ntk->getLatch(i))]);
         _hasEvent[getV3NetIndex(ntk->getLatch(i))] = true;
         _eventList.add(_levelData[ntk->getLatch(i).id], ntk->getLatch(i));
      }
   else
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
         clearValue = _simValue[getV3NetIndex(ntk->getInout(i))];
         if (!random) _simValue[getV3NetIndex(ntk->getInout(i))].clear();
         else _simValue[getV3NetIndex(ntk->getInout(i))].random();
         if (clearValue == _simValue[getV3NetIndex(ntk->getInout(i))]) continue;
         assert (!_hasEvent[getV3NetIndex(ntk->getInout(i))]);
         _hasEvent[getV3NetIndex(ntk->getInout(i))] = true;
         _eventList.add(_levelData[ntk->getInout(i).id], ntk->getInout(i));
      }
   // Simulate According To Event List
   simulateEventList();
}

// Simulation Record Functions
void
V3AlgBvSimulate::recordSimValue() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3BvSimDataVec trace; trace.reserve(ntk->getInputSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) trace.push_back(_simValue[getV3NetIndex(ntk->getInput(i))]);
   assert (ntk->getInputSize() == trace.size()); _traceData.push_back(trace);
}

void
V3AlgBvSimulate::getSimRecordData(uint32_t cycle, V3SimTrace& traceData) const {
   assert (cycle < _traceData.size()); assert (_handler->getNtk()->getInputSize() == _traceData[cycle].size());
   traceData = _traceData[cycle];
}

// Simulation Output Functions
void
V3AlgBvSimulate::printResult() const {
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(_handler->getNtk()); assert (ntk);
   V3GateType type;
   for (uint32_t i = 0; i < _orderMap.size(); ++i) {
      type = ntk->getGateType(_orderMap[i]);
      Msg(MSG_DBG) << "Order [" << i << "] = " << _orderMap[i].id << ", value = " << _simValue[_orderMap[i].id]
                   << ", type = " << V3GateTypeStr[type];
      if (isV3PairType(type))
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputNetId(_orderMap[i], 0).id << ", " 
                      << ntk->getInputNetId(_orderMap[i], 1).id << ")" << endl;
      else if (isV3ReducedType(type))
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputNetId(_orderMap[i], 0).id << ")" << endl;
      else if (BV_MUX == type)
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputNetId(_orderMap[i], 0).id << ", " 
                      << ntk->getInputNetId(_orderMap[i], 1).id << ", "
                      << ntk->getInputNetId(_orderMap[i], 2).id << ")" << endl;
      else if (BV_SLICE == type) 
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputNetId(_orderMap[i], 0).id << " [" 
                      << ntk->getInputSliceBit(_orderMap[i], true) << " : "
                      << ntk->getInputSliceBit(_orderMap[i], false) << "])" << endl;
      else if (BV_CONST == type)
         Msg(MSG_DBG) << ", Inputs = (" << ntk->getInputConstValue(_orderMap[i]) << ")" << endl;
      else Msg(MSG_DBG) << endl;
   }
}

// Private Event-Driven Simulation Functions
void
V3AlgBvSimulate::simulateEventList() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3BitVecX value; uint32_t level;
   V3NetId id, id1;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]); value = _simValue[id1.id];
         assert (level < _levelData[id1.id]); simulateGate(id1);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
}

// Private Simulation Helper Functions
void
V3AlgBvSimulate::simulateGate(const V3NetId& id) {
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(_handler->getNtk()); assert (ntk);
   const V3GateType type = ntk->getGateType(id); assert (AIG_FALSE < type && BV_CONST != type);
   if (isV3PairType(type)) {
      const V3NetId& id1 = ntk->getInputNetId(id, 0); V3BitVecX value1 = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
      const V3NetId& id2 = ntk->getInputNetId(id, 1); V3BitVecX value2 = id2.cp ? ~_simValue[id2.id] : _simValue[id2.id];
      if (BV_MERGE == type) _simValue[id.id] = value1.bv_concat(value2);
      else if (BV_EQUALITY == type) _simValue[id.id] = value1.bv_equal(value2);
      else if (BV_GEQ == type) _simValue[id.id] = value1.bv_geq(value2);
      else {
         assert (BV_AND == type || BV_XOR == type || BV_ADD == type || BV_SUB == type || BV_SHL == type || 
                 BV_SHR == type || BV_MULT == type || BV_DIV == type || BV_MODULO == type);
         switch (type) {
            case BV_AND  : value1  &= value2; _simValue[id.id] = value1; return;
            case BV_XOR  : value1  ^= value2; _simValue[id.id] = value1; return;
            case BV_ADD  : value1  += value2; _simValue[id.id] = value1; return;
            case BV_SUB  : value1  -= value2; _simValue[id.id] = value1; return;
            case BV_SHL  : value1 <<= value2; _simValue[id.id] = value1; return;
            case BV_SHR  : value1 >>= value2; _simValue[id.id] = value1; return;
            case BV_MULT : value1  *= value2; _simValue[id.id] = value1; return;
            case BV_DIV  : value1  /= value2; _simValue[id.id] = value1; return;
            default      : value1  %= value2; _simValue[id.id] = value1; return;
         }
      }
   }
   else if (isV3ReducedType(type)) {
      const V3NetId& id1 = ntk->getInputNetId(id, 0); V3BitVecX value1 = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
      if (BV_RED_AND == type) _simValue[id.id] = value1.bv_red_and();
      else if (BV_RED_OR == type) _simValue[id.id] = value1.bv_red_or();
      else _simValue[id.id] = value1.bv_red_xor();
   }
   else if (BV_MUX == type) {
      const V3NetId& id1 = ntk->getInputNetId(id, 0); V3BitVecX value1 = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
      const V3NetId& id2 = ntk->getInputNetId(id, 1); V3BitVecX value2 = id2.cp ? ~_simValue[id2.id] : _simValue[id2.id];
      const V3NetId& id3 = ntk->getInputNetId(id, 2); V3BitVecX value3 = id3.cp ? ~_simValue[id3.id] : _simValue[id3.id];
      _simValue[id.id] = value3.bv_select(value1, value2);
   }
   else {
      assert (BV_SLICE == type);
      const V3NetId& id1 = ntk->getInputNetId(id, 0); V3BitVecX value1 = id1.cp ? ~_simValue[id1.id] : _simValue[id1.id];
      _simValue[id.id] = value1.bv_slice(ntk->getInputSliceBit(id, true), ntk->getInputSliceBit(id, false));
   }
}

#endif

