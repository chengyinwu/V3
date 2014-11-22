/****************************************************************************
  FileName     [ v3AlgGeneralize.cpp ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ V3 Generalization Utility. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_GENERALIZE_C
#define V3_ALG_GENERALIZE_C

#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3AlgGeneralize.h"

#define V3_USE_NEW_SIM_GENERALIZATION
#define V3_SIM_GENERALIZATION_EARLY_QUIT

/* -------------------------------------------------- *\
 * Class V3GenBucket Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3GenBucket::V3GenBucket(const uint32_t& maxBucket, const uint32_t& maxData) {
   // NOTE: V3NtkUD for Empty Data, Null Pointer
   _prevData  = V3UI32Vec(maxData, V3NtkUD);
   _nextData  = V3UI32Vec(maxData, V3NtkUD);
   _buckets   = V3UI32Vec(maxBucket, V3NtkUD);
   _bucketIdx = V3UI32Vec(maxData, V3NtkUD);
   _bestIndex = V3NtkUD;
}

V3GenBucket::~V3GenBucket() {
   _prevData.clear(); _nextData.clear(); _buckets.clear(); _bucketIdx.clear();
}

// Basic Manipulation Functions
void
V3GenBucket::noticePushComplete() {
   assert (V3NtkUD == _bestIndex);
   for (_bestIndex = 0; _bestIndex < _buckets.size(); ++_bestIndex)
      if (V3NtkUD != _buckets[_bestIndex]) return;
}

void
V3GenBucket::push(const uint32_t& data, const uint32_t& bucketIndex) {
   assert (V3NtkUD == _bestIndex); assert (_buckets.size() > bucketIndex);
   assert (_prevData.size() > data); assert (V3NtkUD == _prevData[data]);
   assert (_nextData.size() > data); assert (V3NtkUD == _nextData[data]);
   assert (_bucketIdx.size() > data); assert (V3NtkUD == _bucketIdx[data]);
   if (V3NtkUD != _buckets[bucketIndex]) {
      // Push the data to the front of the list
      assert (V3NtkUD == _prevData[_buckets[bucketIndex]]);
      _nextData[data] = _buckets[bucketIndex];
      _prevData[_buckets[bucketIndex]] = data;
   }
   _buckets[bucketIndex] = data; _bucketIdx[data] = bucketIndex;
}

const uint32_t
V3GenBucket::pop(uint32_t& data) {
   assert (!isEmpty()); assert (V3NtkUD != _buckets[_bestIndex]);
   assert (V3NtkUD == _prevData[_buckets[_bestIndex]]);
   assert (_bestIndex == _bucketIdx[_buckets[_bestIndex]]);
   const uint32_t i = _bestIndex; data = _buckets[_bestIndex];
   if (V3NtkUD == _nextData[_buckets[_bestIndex]]) {
      while (++_bestIndex < _buckets.size())
         if (V3NtkUD != _buckets[_bestIndex]) break;
   }
   else {
      _prevData[_nextData[_buckets[_bestIndex]]] = V3NtkUD;
      _buckets[_bestIndex] = _nextData[_buckets[_bestIndex]];
   }
   return i;
}

void
V3GenBucket::inc(const uint32_t& data, const uint32_t& amount) {
   assert (!isEmpty()); assert (amount);
   assert (V3NtkUD != _bucketIdx[data]);
   assert (_bestIndex <= _bucketIdx[data]);
   assert (_buckets.size() > (amount + _bucketIdx[data]));
   // Delete data from current bucket
   bool updateBest = false;
   if (data == _buckets[_bucketIdx[data]]) {
      assert (V3NtkUD == _prevData[data]);
      if (V3NtkUD != _nextData[data]) _buckets[_bucketIdx[data]] = _nextData[data];
      else if (_bestIndex == _bucketIdx[data]) updateBest = true;
   }
   else {
      assert (V3NtkUD != _prevData[data]);
      _nextData[_prevData[data]] = _nextData[data];
   }
   if (V3NtkUD != _nextData[data]) _prevData[_nextData[data]] = _prevData[data];
   // Push data to higher bucket
   const uint32_t bucketIndex = amount + _bucketIdx[data];
   if (V3NtkUD != _buckets[bucketIndex]) {
      // Push the data to the front of the list
      assert (V3NtkUD == _prevData[_buckets[bucketIndex]]);
      _nextData[data] = _buckets[bucketIndex];
      _prevData[_buckets[bucketIndex]] = data;
   }
   _buckets[bucketIndex] = data; _bucketIdx[data] = bucketIndex;
   // Update bestIndex if Needed
   if (updateBest)
      for (; _bestIndex < _buckets.size(); ++_bestIndex)
         if (V3NtkUD != _buckets[_bestIndex]) break;
   assert (bucketIndex >= _bestIndex);
}

/* -------------------------------------------------- *\
 * Class V3GenStruct Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3GenStruct::V3GenStruct(const uint32_t& varSize, const V3NetVec& baseCube) {
   _cube2Var.clear(); _varOrder.clear();
   _varValue = V3UI32Vec(varSize, 0);
   _var2Cube = V3UI32Table(varSize, V3UI32Vec());
   _baseCube = baseCube; assert (isValidCube(_baseCube));
}

V3GenStruct::~V3GenStruct() {
   clear(V3NetVec());
}

// Maintanence Functions
const bool
V3GenStruct::pushCube(const V3NetVec& cube, const bool& invert) {
   // Record the cube to _var2Cube[var]
   assert (!_varOrder.size()); assert (isValidCube(cube));
   V3UI32Set conflictVars; conflictVars.clear();
   uint32_t i = 0, j = 0;
   while (i < _baseCube.size() && j < cube.size()) {
      if (_baseCube[i].id == cube[j].id) {
         if (invert ^ _baseCube[i].cp ^ cube[j].cp) {
            _var2Cube[_baseCube[i].id].push_back(_cube2Var.size());
            conflictVars.insert(_baseCube[i].id);
         }
         ++i; ++j;
      }
      else if (_baseCube[i].id < cube[j].id) ++i;
      else ++j;
   }
   // Push the cube (with conflicting vars retained) into _cube2Var
   if (conflictVars.size()) _cube2Var.push_back(conflictVars);
   return conflictVars.size();
}

void
V3GenStruct::startVarOrder(const V3NetVec& careVars) {
   // Construct Ordered List for all Variables
   // NOTE: The cost of a variable is the decrease of #conflicting cubes if the variable is generalized
   assert (!_varOrder.size());
   // Special Handlings for Variables with Zero Cost
   for (uint32_t i = 0; i < careVars.size(); ++i) {
      assert (_var2Cube.size() > careVars[i].id);
      if (!_var2Cube[careVars[i].id].size()) {
         assert (_varOrder.end() == _varOrder.find(make_pair(careVars[i], 0)));
         assert (!_varValue[careVars[i].id]); _varOrder.insert(make_pair(careVars[i], 0));
      }
      else {
         uint32_t dec = 1;
         for (uint32_t j = 0; j < _var2Cube[careVars[i].id].size(); ++j) {
            assert (_cube2Var[_var2Cube[careVars[i].id][j]].end() != 
                    _cube2Var[_var2Cube[careVars[i].id][j]].find(careVars[i].id));
            if (1 == _cube2Var[_var2Cube[careVars[i].id][j]].size()) ++dec;
         }
         assert (_varOrder.end() == _varOrder.find(make_pair(careVars[i], dec)));
         _varValue[careVars[i].id] = dec; _varOrder.insert(make_pair(careVars[i], dec));
      }
   }
}

const V3NetId
V3GenStruct::getNextVar() {
   // Set the Best Var from VarOrder
   if (!_varOrder.size()) return V3NetUD;
   return (*_varOrder.begin()).first;
}

const V3NetId
V3GenStruct::getNextVar(uint32_t& cost) {
   // Set the Best Var from VarOrder
   if (!_varOrder.size()) return V3NetUD;
   cost = (*_varOrder.begin()).second; return (*_varOrder.begin()).first;
}

void
V3GenStruct::updateVarOrder(const bool& succeed) {
   // Update Data According to Success or Failure
   assert (_varOrder.size()); V3NetId id;
   const uint32_t var = (*_varOrder.begin()).first.id; assert (_var2Cube.size() > var);
   V3GenVarOrder::iterator it; _varOrder.erase(_varOrder.begin());
   if (succeed) {
      for (uint32_t i = 0; i < _var2Cube[var].size(); ++i) {
         assert (_cube2Var.size() > _var2Cube[var][i]);
         assert (_cube2Var[_var2Cube[var][i]].end() != _cube2Var[_var2Cube[var][i]].find(var));
         _cube2Var[_var2Cube[var][i]].erase(_cube2Var[_var2Cube[var][i]].find(var));
         if (1 == _cube2Var[_var2Cube[var][i]].size()) {  // Increase dec for the only variable
            if (!_var2Cube[*(_cube2Var[_var2Cube[var][i]].begin())].size()) continue;  // Attempted
            it = _varOrder.find(make_pair(V3NetId::makeNetId(*(_cube2Var[_var2Cube[var][i]].begin())), 
                                          _varValue[*(_cube2Var[_var2Cube[var][i]].begin())]));
            assert (_varOrder.end() != it); id = it->first; _varOrder.erase(it);
            ++_varValue[*(_cube2Var[_var2Cube[var][i]].begin())];
            _varOrder.insert(make_pair(id, _varValue[*(_cube2Var[_var2Cube[var][i]].begin())]));
         }
      }
   }
   _var2Cube[var].clear();
}

void
V3GenStruct::clear(const V3NetVec& baseCube) {
   for (uint32_t i = 0; i < _varValue.size(); ++i) _varValue[i] = 0;
   for (uint32_t i = 0; i < _var2Cube.size(); ++i) _var2Cube[i].clear();
   for (uint32_t i = 0; i < _cube2Var.size(); ++i) _cube2Var[i].clear();
   _cube2Var.clear(); _varOrder.clear();
   _baseCube = baseCube; assert (isValidCube(_baseCube));
}

// Private Member Functions
const bool
V3GenStruct::isValidCube(const V3NetVec& cube) const {
   for (uint32_t i = 0, j = V3NtkUD; i < cube.size(); ++i) {
      if (getVarSize() <= cube[i].id) return false;
      if (V3NtkUD == j) j = cube[i].id;
      else if (cube[i].id <= j) return false;
   }
   return true;
}

void
V3GenStruct::printCube(const V3NetVec& cube) const {
   for (uint32_t i = 0; i < cube.size(); ++i)
      Msg(MSG_IFO) << (i ? " " : "") << (cube[i].cp ? "~" : "") << cube[i].id;
}

void
V3GenStruct::printConfl(const V3UI32Set& cube) const {
   for (V3UI32Set::const_iterator it = cube.begin(); it != cube.end(); ++it)
      Msg(MSG_IFO) << ((cube.begin() == it) ? "" : " ") << *it;
}

void
V3GenStruct::printVar2Cube() const {
   Msg(MSG_IFO) << "========== Var2Cube Table ==========" << endl;
   for (V3GenVarOrder::const_iterator it = _varOrder.begin(); it != _varOrder.end(); ++it) {
      Msg(MSG_IFO) << "Var[" << (it->first.cp ? "~" : "") << it->first.id << "] = ";
      for (uint32_t i = 0; i < _var2Cube[it->first.id].size(); ++i)
         Msg(MSG_IFO) << (i ? " " : "") << _var2Cube[it->first.id][i];
      Msg(MSG_IFO) << ", cost = " << it->second << endl;
   }
}

void
V3GenStruct::printCube2Var() const {
   Msg(MSG_IFO) << "========== Cube2Var Table ==========" << endl;
   for (uint32_t i = 0; i < _cube2Var.size(); ++i) {
      Msg(MSG_IFO) << "Cube[" << i << "] = ";
      for (V3UI32Set::const_iterator it = _cube2Var[i].begin(); it != _cube2Var[i].end(); ++it)
         Msg(MSG_IFO) << ((it == _cube2Var[i].begin()) ? "" : " ") << *it;
      Msg(MSG_IFO) << endl;
   }
}

/* -------------------------------------------------- *\
 * Class V3AlgGeneralize Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgGeneralize::V3AlgGeneralize() {
   _genResult.clear(); _undecided.clear();
}

V3AlgGeneralize::~V3AlgGeneralize() {
   _genResult.clear(); _undecided.clear();
}

// Generalization Main Functions
void
V3AlgGeneralize::setTargetNets(const V3NetVec& curTargets, const V3NetVec& nextTargets) {
   Msg(MSG_ERR) << "Calling virtual function V3AlgGeneralize::setTargetNets() !!" << endl;
}

const V3NetVec
V3AlgGeneralize::getUndecided() const {
   V3NetVec result; result.clear(); result.reserve(_undecided.size());
   for (V3NetList::const_iterator it = _undecided.begin(); it != _undecided.end(); ++it) result.push_back(*it);
   return result;
}

const V3NetVec
V3AlgGeneralize::getGeneralizationResult() const {
   V3NetVec result; result.clear(); result.reserve(_genResult.size());
   for (V3NetList::const_iterator it = _genResult.begin(); it != _genResult.end(); ++it) result.push_back(*it);
   return result;
}

// Preprocessing Techniques
void
V3AlgGeneralize::performSetXForNotCOIVars() {
   Msg(MSG_ERR) << "Calling virtual function V3AlgGeneralize::performSetXForNotCOIVars() !!" << endl;
}

void
V3AlgGeneralize::performFixForControlVars(const bool& parallelSim) {
   Msg(MSG_ERR) << "Calling virtual function V3AlgGeneralize::performFixForControlVars() !!" << endl;
}

// Generalization Heuristics
void
V3AlgGeneralize::performXPropForExtensibleVars(const V3UI32Vec& generalizeOrder) {
   Msg(MSG_ERR) << "Calling virtual function V3AlgGeneralize::performXPropForExtensibleVars() !!" << endl;
}

void
V3AlgGeneralize::performXPropForMinimizeTransitions(const uint32_t& frameSize, const V3NetVec& targetValue) {
   Msg(MSG_ERR) << "Calling virtual function V3AlgGeneralize::performXPropForMinimizeTransitions() !!" << endl;
}

/* -------------------------------------------------- *\
 * Class V3AlgAigGeneralize Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgAigGeneralize::V3AlgAigGeneralize(const V3NtkHandler* const handler) : V3AlgAigSimulate(handler), V3AlgGeneralize() {
   assert (_handler); assert (_handler->getNtk());
   assert (!dynamic_cast<V3BvNtk*>(_handler->getNtk()));
   _isFrozen = V3BoolVec(_handler->getNtk()->getNetSize());
   _traverse = V3BoolVec(_handler->getNtk()->getNetSize());
}

V3AlgAigGeneralize::~V3AlgAigGeneralize() {
   _targetId.clear(); _isFrozen.clear(); _traverse.clear();
}

// Generalization Main Functions
void
V3AlgAigGeneralize::setTargetNets(const V3NetVec& curTargets, const V3NetVec& nextTargets) {
   // Note that curTargets are NetIds while nextTargets are Flop Indices
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); _targetId.clear();
   // Set New Target Nets
   for (uint32_t i = 0; i < nextTargets.size(); ++i) {
      assert (nextTargets[i].id < ntk->getLatchSize());
      assert (V3_FF == ntk->getGateType(ntk->getLatch(nextTargets[i].id)));
      _targetId.push_back(ntk->getInputNetId(ntk->getLatch(nextTargets[i].id), 0));
      if (nextTargets[i].cp) _targetId.back() = ~(_targetId.back());
      assert (_targetId.back().cp == ('0' == _simValue[_targetId.back().id][0]));
   }
   for (uint32_t i = 0; i < curTargets.size(); ++i) {
      assert (curTargets[i].id < ntk->getNetSize()); _targetId.push_back(curTargets[i]);
      assert (_targetId.back().cp == ('0' == _simValue[_targetId.back().id][0]));
   }
   // Reset Generalization Data
   _genResult.clear(); _undecided.clear();
   for (uint32_t i = 0, j = ntk->getLatchSize(); i < j; ++i) {
      if ('X' == _simValue[ntk->getLatch(i).id][0]) continue;
      _genResult.push_back(V3NetId::makeNetId(i, ('0' == _simValue[ntk->getLatch(i).id][0])));
      _undecided.push_back(_genResult.back()); assert (_genResult.size() == _undecided.size());
   }
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) _isFrozen[i] = ('X' == _simValue[i][0]);
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) _traverse[i] = false;
   initializeEventList(); assert (generalizationValid());
}

// Preprocessing Techniques
void
V3AlgAigGeneralize::performSetXForNotCOIVars() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); if (!_undecided.size()) return;
   // Mark Fanin Cone from Targets
   for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
   for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
   // Preprocessing by Removing Variables NOT in COI
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      // Retain Variables in COI
      if (_traverse[ntk->getLatch((*it).id).id]) { ++it; ++is; }
      else { _undecided.erase(it++); _genResult.erase(is++); }
   }
   assert (generalizationValid());
}

void
V3AlgAigGeneralize::performFixForControlVars(const bool& parallelSim) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); if (!_undecided.size()) return;
   // Mark Fanin Cone from Targets
   for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
   for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
   // Set Controlling Signals
   V3NetVec newForwardFrozen; newForwardFrozen.clear();
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      if (_isFrozen[ntk->getInput(i).id]) continue; _isFrozen[ntk->getInput(i).id] = true;
      if (_traverse[ntk->getInput(i).id]) newForwardFrozen.push_back(ntk->getInput(i));
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) {
      if (_isFrozen[ntk->getInout(i).id]) continue; _isFrozen[ntk->getInout(i).id] = true;
      if (_traverse[ntk->getInout(i).id]) newForwardFrozen.push_back(ntk->getInout(i));
   }
   for (uint32_t i = 0; i < ntk->getConstSize(); ++i) {
      if (_isFrozen[ntk->getConst(i).id]) continue; _isFrozen[ntk->getConst(i).id] = true;
      if (_traverse[ntk->getConst(i).id]) newForwardFrozen.push_back(ntk->getConst(i));
   }
   for (uint32_t i = 0; i < _targetId.size(); ++i) {
      if (_isFrozen[_targetId[i].id]) continue; _isFrozen[_targetId[i].id] = true;
      assert (_traverse[_targetId[i].id]); newForwardFrozen.push_back(_targetId[i]);
   }
   // Perform Initial Frozen Propagation
   do {
      // Propagate Frozen by Forward Simulation
      performSimulationForFrozenPropagation(newForwardFrozen); newForwardFrozen.clear();
      // Update Target Nets by Backward Implication
      if (performImplicationForTargetUpdate(newForwardFrozen)) {
         for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
         for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
      }
   } while (newForwardFrozen.size());
   // Perform Full Pattern Simulation if parallelSim is Enabled
   if (parallelSim) {
      V3NetVec inputs; inputs.clear();
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) inputs.push_back(ntk->getInput(i));
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) inputs.push_back(ntk->getInout(i));
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) inputs.push_back(ntk->getLatch(i));
      for (uint32_t i = 0; i < ntk->getConstSize(); ++i) inputs.push_back(ntk->getConst(i));
      for (uint32_t i = 0; i < inputs.size(); ++i) {
         V3BitVecS& value = _simValue[inputs[i].id]; assert (!isV3NetInverted(inputs[i]));
         switch (value[0]) {
            case '1' : value.setZeros( 0ul); value.setOnes(~0ul); assert ('1' == value[0]); break;
            case '0' : value.setZeros(~0ul); value.setOnes( 0ul); assert ('0' == value[0]); break;
            default  : value.setZeros( 0ul); value.setOnes( 0ul); assert ('X' == value[0]); break;
         }
      }
      simulate();
   }
   // Check Controlling (i.e. inextensible) Variables
   V3AigSimDataVec recoverValue(ntk->getNetSize());
   V3BoolVec recoverable(ntk->getNetSize(), false);
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   uint32_t testIndex = 0; V3BitVecS value;
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      // Set X for the Undecided Variable
      if (!_traverse[ntk->getLatch((*it).id).id]) { _undecided.erase(it++); _genResult.erase(is++); }
      else if (_isFrozen[ntk->getLatch((*it).id).id]) { _undecided.erase(it++); ++is; }
      else {
         V3BitVecS value = _simValue[ntk->getLatch((*it).id).id]; value.setX(testIndex);
         simulateForGeneralization(ntk->getLatch((*it).id), value); ++testIndex; ++it; ++is;
         // Record Simulation Value for Recover
         for (uint32_t i = 0; i < _simRecord.size(); ++i) {
            if (recoverable[_simRecord[i].first.id]) continue;
            recoverable[_simRecord[i].first.id] = true;
            recoverValue[_simRecord[i].first.id] = _simRecord[i].second;
         }
      }
      // Collect a Maximum of 64 Patterns in a Simulation Run
      if (parallelSim && (_undecided.end() != it) && (testIndex < 64)) continue;
      // Reveal Generalized Target Nets
      value.setZeros(~0ul); value.setOnes(0ul);  // Set All 0 or All 1
      for (uint32_t i = 0; i < _targetId.size(); ++i) value ^= _simValue[_targetId[i].id];
      // Back to the First Variable in Current Run
      for (uint32_t i = 0; i < testIndex; ++i) --it;
      // Check the Inextensibility of Variables
      for (uint32_t i = 0; i < testIndex; ++i) {
         assert (!_isFrozen[ntk->getLatch((*it).id).id]);
         if ('X' != value[i]) { ++it; continue; }
         _isFrozen[ntk->getLatch((*it).id).id] = true;
         newForwardFrozen.push_back(ntk->getLatch((*it).id)); _undecided.erase(it++);
      }
      // Reset Data for the Next Run
      for (uint32_t i = 0; i < recoverable.size(); ++i) 
         if (recoverable[i]) { _simValue[i] = recoverValue[i]; recoverable[i] = false; }
      // Perform Controlling Frozen Propagation
      do {
         // Propagate Frozen by Forward Simulation
         performSimulationForFrozenPropagation(newForwardFrozen); newForwardFrozen.clear();
         // Update Target Nets by Backward Implication
         if (performImplicationForTargetUpdate(newForwardFrozen)) {
            for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
            for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
         }
      } while (newForwardFrozen.size()); testIndex = 0;
   }
   assert (generalizationValid());
}

// Generalization Heuristics
void
V3AlgAigGeneralize::performXPropForExtensibleVars(const V3UI32Vec& generalizeOrder) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   if (!_undecided.size() || !generalizeOrder.size()) return;
   // Mark Fanin Cone from Targets
   for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
   for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
   // Mark Target Nets
   V3BoolVec isTarget = V3BoolVec(_handler->getNtk()->getNetSize(), false);
   for (uint32_t i = 0; i < _targetId.size(); ++i) isTarget[_targetId[i].id] = true;
   // Mark Undecided Variables
   V3BoolVec varUndecided(ntk->getLatchSize(), false);
   V3BoolVec varGeneralized(ntk->getLatchSize(), false);
   V3NetList::const_iterator ix = _undecided.begin();
   while (ix != _undecided.end()) {
      assert (varUndecided.size() > (*ix).id);
      assert (!varUndecided[(*ix).id]);
      varUndecided[(*ix).id] = true; ++ix;
   }
   // Generalize According to a Given Variable Order
   V3BitVecS value;
   for (uint32_t i = 0; i < generalizeOrder.size(); ++i) {
      assert (varUndecided.size() > generalizeOrder[i]);
      if (!varUndecided[generalizeOrder[i]]) continue;
      varUndecided[generalizeOrder[i]] = false;  // The variable will be decided
      if (!_traverse[ntk->getLatch(generalizeOrder[i]).id]) continue;
      assert ('X' != _simValue[ntk->getLatch(generalizeOrder[i]).id][0]);
      // Put X for this Latch
      value = _simValue[ntk->getLatch(generalizeOrder[i]).id]; value.setX(0);
      // If Generalization Failed : The Variable is a Controlling (i.e. inextensible) Variable
#ifdef V3_SIM_GENERALIZATION_EARLY_QUIT
      if (!simulateForGeneralization(ntk->getLatch(generalizeOrder[i]), value, isTarget)) {
         recoverForGeneralization(); varGeneralized[generalizeOrder[i]] = true;
      }
#else
      simulateForGeneralization(ntk->getLatch(generalizeOrder[i]), value);
      for (uint32_t x = 0; x < _targetId.size(); ++x) {
         if ('X' != _simValue[_targetId[x].id][0]) continue;
         recoverForGeneralization(); varGeneralized[generalizeOrder[i]] = true;
         assert ('X' != _simValue[_targetId[x].id][0]); break;
      }
#endif
   }
   // Update Generalization Data
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      assert (varUndecided.size() > (*it).id);
      if (varUndecided[(*it).id]) ++it;  // The Variable is Undecided
      else {
         if (!varGeneralized[(*is).id]) _genResult.erase(is++);  // The Variable is Generalized
         else ++is; _undecided.erase(it++);
      }
   }
   assert (generalizationValid());
}

void
V3AlgAigGeneralize::performXPropForMinimizeTransitions(const uint32_t& frameSize, const V3NetVec& targetValue) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   if (!_undecided.size()) return;
   // Initialize Storage
   V3UI32Vec varId(ntk->getLatchSize(), V3NtkUD);  // maps a latch to a node in the bucket
   V3UI32Table updateList; updateList.clear();  // list of nodes to be updated
   V3NetVec latchId; latchId.clear();  // index of the latch a node represents
   V3IntVec cost; cost.clear();  // the difference on the number of transitions of a node
   const uint32_t ffSize = ntk->getLatchSize() / frameSize;
   assert (ntk->getLatchSize() == (frameSize * ffSize));
   V3NetVec id(ffSize, V3NetUD); V3BoolVec frozen(ffSize, false);
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   uint32_t index = 0, refIndex = 0;
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is); assert ((*it).id >= (*is).id);
         assert ((ffSize * frameSize) > (*is).id); index = (*is).id % ffSize;
         if (V3NetUD != id[index] && !frozen[index]) {
            if (V3NtkUD == varId[id[index].id]) {
               updateList.push_back(V3UI32Vec()); updateList.back().clear();
               varId[id[index].id] = latchId.size(); latchId.push_back(id[index]); cost.push_back(0);
            }
            refIndex = varId[id[index].id]; assert (V3NtkUD != refIndex);
            if (id[index].cp == (*is).cp) ++cost[refIndex]; else --cost[refIndex];
         }
         id[index] = *is; frozen[index] = true; assert (V3NetUD != id[index]); ++is;
      }
      // Add Constraints into Solver
      assert ((ffSize * frameSize) > (*it).id); index = (*it).id % ffSize;
      if (V3NetUD != id[index]) {
         if (frozen[index]) {
            updateList.push_back(V3UI32Vec()); updateList.back().clear();
            varId[(*it).id] = latchId.size(); latchId.push_back(*it); cost.push_back(0);
            if (id[index].cp == (*it).cp) ++cost.back(); else --cost.back();
         }
         else {
            if (V3NtkUD == varId[id[index].id]) {
               updateList.push_back(V3UI32Vec()); updateList.back().clear();
               varId[id[index].id] = latchId.size(); latchId.push_back(id[index]); cost.push_back(0);
            }
            refIndex = varId[id[index].id]; assert (V3NtkUD != refIndex);
            updateList.push_back(V3UI32Vec()); updateList.back().clear();
            varId[(*it).id] = latchId.size(); latchId.push_back(*it); cost.push_back(0);
            updateList[refIndex].push_back(latchId.size() - 1);
            updateList[latchId.size() - 1].push_back(refIndex);
            if (id[index].cp == (*it).cp) { ++cost[refIndex]; ++cost.back(); }
            else { --cost[refIndex]; --cost.back(); }
         }
      }
      id[index] = *it; frozen[index] = false; assert (V3NetUD != id[index]); ++it; ++is;
   }
   while (_genResult.end() != is) {
      assert (_genResult.end() != is); assert ((*it).id >= (*is).id);
      assert ((ffSize * frameSize) > (*is).id); index = (*is).id % ffSize;
      if (V3NetUD != id[index] && !frozen[index]) {
         if (V3NtkUD == varId[id[index].id]) {
            updateList.push_back(V3UI32Vec()); updateList.back().clear();
            varId[id[index].id] = latchId.size(); latchId.push_back(id[index]); cost.push_back(0);
         }
         refIndex = varId[id[index].id]; assert (V3NtkUD != refIndex);
         if (id[index].cp == (*is).cp) ++cost[refIndex]; else --cost[refIndex];
      }
      id[index] = *is; frozen[index] = true; assert (V3NetUD != id[index]); ++is;
   }
   for (uint32_t i = 0; i < targetValue.size(); ++i) {
      index = targetValue[i].id; assert (ffSize > index);
      if (V3NetUD != id[index] && !frozen[index]) {
         if (V3NtkUD == varId[id[index].id]) {
            updateList.push_back(V3UI32Vec()); updateList.back().clear();
            varId[id[index].id] = latchId.size(); latchId.push_back(id[index]); cost.push_back(0);
         }
         refIndex = varId[id[index].id]; assert (V3NtkUD != refIndex);
         if (id[index].cp == targetValue[i].cp) ++cost[refIndex]; else --cost[refIndex];
      }
   }
   // Initialize BucketList Like Storage
   V3UI32Vec bucketHead(5, V3NtkUD);  // first elements of buckets
   V3UI32Vec bucketSize(5, 0);  // sizes of buckets with costs -2, -1, 0, 1, and 2
   V3UI32Vec prev(latchId.size(), V3NtkUD);  // forward pointer of a node
   V3UI32Vec next(latchId.size(), V3NtkUD);  // backward pointer of a node
   uint32_t bestBucket = 0;  // index to the best bucket
   for (uint32_t i = 0; i < latchId.size(); ++i) {
      assert (cost[i] >= -2 && cost[i] <= 2);
      if (V3NtkUD != bucketHead[2 + cost[i]]) {
         assert (V3NtkUD == prev[bucketHead[2 + cost[i]]]);
         prev[bucketHead[2 + cost[i]]] = i; next[i] = bucketHead[2 + cost[i]];
      }
      bucketHead[2 + cost[i]] = i; ++bucketSize[2 + cost[i]];
   }
   // Mark Target Nets
   V3BoolVec isTarget = V3BoolVec(_handler->getNtk()->getNetSize(), false);
   for (uint32_t i = 0; i < _targetId.size(); ++i) isTarget[_targetId[i].id] = true;
   // Start Heuristic to Minimize Transitions
   V3UI32Vec genOrder; genOrder.clear(); V3IntVec genCost; genCost.clear();
   uint32_t bestIndex; bool generalizeOK; const int disableCost = -3;
   while (true) {
      bestBucket = 0;
      while (bestBucket < bucketSize.size()) { if (bucketSize[bestBucket]) break; else ++bestBucket; }
      if (bestBucket == bucketSize.size()) break;
      // 1. Pop a latch in the bestBucket (and update bestBucket, bucketSize, etc)
      bestIndex = bucketHead[bestBucket]; assert (V3NtkUD != bestIndex);
      bucketHead[bestBucket] = next[bestIndex]; --bucketSize[bestBucket];
      if (V3NtkUD != next[bestIndex]) prev[next[bestIndex]] = V3NtkUD;
      // 2. Try Generalize the latch
      assert ('X' != _simValue[ntk->getLatch(latchId[bestIndex].id).id][0]);
      // Put X for this Latch
      V3BitVecS value = _simValue[ntk->getLatch(latchId[bestIndex].id).id]; value.setX(0);
      // If Generalization Failed : The Variable is a Controlling (i.e. inextensible) Variable
#ifdef V3_SIM_GENERALIZATION_EARLY_QUIT
      generalizeOK = simulateForGeneralization(ntk->getLatch(latchId[bestIndex].id), value, isTarget);
      if (!generalizeOK) recoverForGeneralization();
#else
      simulateForGeneralization(ntk->getLatch(latchId[bestIndex].id), value); generalizeOK = true;
      for (uint32_t x = 0; x < _targetId.size(); ++x) {
         if ('X' != _simValue[_targetId[x].id][0]) continue;
         recoverForGeneralization(); generalizeOK = false;
         assert ('X' != _simValue[_targetId[x].id][0]); break;
      }
#endif
      // 3. Update costs of updateList
      if (generalizeOK) {  // Generalize Succeed
         genOrder.push_back(bestIndex);
         genCost.push_back((genCost.size() ? genCost.back() : 0) + cost[bestIndex]);
         for (uint32_t i = 0; i < updateList[bestIndex].size(); ++i) {
            refIndex = updateList[bestIndex][i]; assert (bestIndex != refIndex);
            if (disableCost == cost[refIndex]) continue;
            // Delete latch refIndex
            if (bucketHead[2 + cost[refIndex]] == refIndex) bucketHead[2 + cost[refIndex]] = next[refIndex];
            else next[prev[refIndex]] = next[refIndex]; --bucketSize[2 + cost[refIndex]];
            if (V3NtkUD != next[refIndex]) prev[next[refIndex]] = prev[refIndex];
            // Update cost of latch refIndex
            if (latchId[bestIndex].cp == latchId[refIndex].cp) cost[refIndex] -= 2;
            else cost[refIndex] += 2;
            // Add latch refIndex with new cost
            /* (to the front of the bucket)
            if (V3NtkUD != bucketHead[2 + cost[refIndex]]) {
               next[refIndex] = bucketHead[2 + cost[refIndex]];
               prev[bucketHead[2 + cost[refIndex]]] = refIndex;
            }
            else next[refIndex] = V3NtkUD; ++bucketSize[2 + cost[refIndex]];
            bucketHead[2 + cost[refIndex]] = refIndex; prev[refIndex] = V3NtkUD;
            */
            ///* (to the back of the bucket)
            if (V3NtkUD != bucketHead[2 + cost[refIndex]]) {
               index = bucketHead[2 + cost[refIndex]];
               while (V3NtkUD != next[index]) { index = next[index]; }
               next[index] = refIndex; prev[refIndex] = index;
            }
            else { bucketHead[2 + cost[refIndex]] = refIndex; prev[refIndex] = V3NtkUD; }
            ++bucketSize[2 + cost[refIndex]]; next[refIndex] = V3NtkUD;
            //*/
         }
      }
      cost[bestIndex] = disableCost;  // disable the latch
   }
   // Determine the Best Generalization
   V3BoolVec genVarId(ntk->getLatchSize(), false); bestIndex = 0;
   for (uint32_t i = 0; i < genCost.size(); ++i) if (genCost[i] < genCost[bestIndex]) bestIndex = i;
   if ((genCost.size() == 0) || (genCost[bestIndex] >= 0)) bestIndex = 0; else ++bestIndex;
   for (uint32_t i = 0; i < bestIndex; ++i) genVarId[latchId[genOrder[i]].id] = true;
   // Update Generalization Data
   it = _undecided.begin(), is = _genResult.begin();
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      if (genVarId[(*it).id]) { _undecided.erase(it++); _genResult.erase(is++); }
      else { ++it; ++is; }
   }
   assert (generalizationValid());
}

// Generalization Techniques
// Private Generalization Functions
void
V3AlgAigGeneralize::simulateForGeneralization(const V3NetId& generalizeId, const V3BitVecS& generalizeValue) {
#ifdef V3_USE_NEW_SIM_GENERALIZATION
   // Clear Generalization Record and Set Source Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]); assert (_traverse[generalizeId.id]);
   initializeEventList(); _simRecord.clear(); _simRecord.reserve(ntk->getNetSize());
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue;
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3NetId id, id1, id2;
   for (uint32_t i = 0; i < _fanoutVec[generalizeId.id].size(); ++i) {
      id1 = _fanoutVec[generalizeId.id][i]; assert (_levelData[id1.id]);
      if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
      _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
   }
   V3BitVecS value; uint32_t level;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (AIG_NODE == ntk->getGateType(id)); assert (_hasEvent[id.id]);
      _hasEvent[id.id] = false; value = _simValue[id.id];
      // Simulate for the New Value
      id1 = ntk->getInputNetId(id, 0); assert (_traverse[id1.id]);
      id2 = ntk->getInputNetId(id, 1); assert (_traverse[id2.id]);
      _simValue[id.id].bv_and(_simValue[id1.id], id1.cp, _simValue[id2.id], id2.cp);
      if (value == _simValue[id.id]) continue; _simRecord.push_back(make_pair(id, value));
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (level < _levelData[id1.id]);
         if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#else
   // Clear Generalization Record and Set Source Event
   initializeEventList(); _simRecord.clear();
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (V3_FF == ntk->getGateType(generalizeId)); assert (generalizeValue != _simValue[generalizeId.id]);
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue; if (!_traverse[generalizeId.id]) return;
   _hasEvent[generalizeId.id] = true; _eventList.add(_levelData[generalizeId.id], generalizeId);
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecS value; uint32_t level;
   V3NetId id, id1, id2, id3;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]);
         if (!_traverse[id1.id]) continue; value = _simValue[id1.id];
         assert (AIG_NODE == ntk->getGateType(id1)); assert (level < _levelData[id1.id]);
         id2 = ntk->getInputNetId(id1, 0); id3 = ntk->getInputNetId(id1, 1);
         _simValue[id1.id].bv_and(_simValue[id2.id], id2.cp, _simValue[id3.id], id3.cp);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _simRecord.push_back(make_pair(id1, value));
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#endif
}

const bool
V3AlgAigGeneralize::simulateForGeneralization(const V3NetId& generalizeId, const V3BitVecS& generalizeValue, 
                                              const V3BoolVec& isTarget) {
#ifdef V3_USE_NEW_SIM_GENERALIZATION
   // Clear Generalization Record and Set Source Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]); assert (_traverse[generalizeId.id]);
   assert (generalizeId.id < isTarget.size()); _simRecord.clear();
   initializeEventList(); if (isTarget[generalizeId.id]) return false;
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue;
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3NetId id, id1, id2;
   for (uint32_t i = 0; i < _fanoutVec[generalizeId.id].size(); ++i) {
      id1 = _fanoutVec[generalizeId.id][i]; assert (_levelData[id1.id]);
      if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
      _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
   }
   V3BitVecS value; uint32_t level;
   bool result = true;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (AIG_NODE == ntk->getGateType(id)); assert (_hasEvent[id.id]);
      _hasEvent[id.id] = false; value = _simValue[id.id]; assert (result);
      // Simulate for the New Value
      id1 = ntk->getInputNetId(id, 0); assert (_traverse[id1.id]);
      id2 = ntk->getInputNetId(id, 1); assert (_traverse[id2.id]);
      _simValue[id.id].bv_and(_simValue[id1.id], id1.cp, _simValue[id2.id], id2.cp);
      if (value == _simValue[id.id]) continue; _simRecord.push_back(make_pair(id, value));
      assert (id.id < isTarget.size()); if (isTarget[id.id]) { result = false; break; }
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (level < _levelData[id1.id]);
         if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#else
   // Clear Generalization Record and Set Source Event
   initializeEventList(); _simRecord.clear();
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (generalizeId.id < isTarget.size());
   if (isTarget[generalizeId.id]) return false;
   assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]);
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue; assert (_traverse[generalizeId.id]);
   _hasEvent[generalizeId.id] = true; _eventList.add(_levelData[generalizeId.id], generalizeId);
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecS value; uint32_t level;
   V3NetId id, id1, id2, id3;
   bool result = true;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (id.id < isTarget.size()); assert (!isTarget[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]);
         if (!_traverse[id1.id] || _simValue[id1.id].allX()) continue;
         value = _simValue[id1.id]; assert (level < _levelData[id1.id]);
         // Simulate for the New Value
         assert (AIG_NODE == ntk->getGateType(id1));
         id2 = ntk->getInputNetId(id1, 0); assert (_traverse[id2.id]);
         id3 = ntk->getInputNetId(id1, 1); assert (_traverse[id3.id]);
         _simValue[id1.id].bv_and(_simValue[id2.id], id2.cp, _simValue[id3.id], id3.cp);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _simRecord.push_back(make_pair(id1, value));
         assert (id1.id < isTarget.size()); if (isTarget[id1.id]) { result = false; break; }
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#endif
   if (!result) { while (_eventList.size()) { _eventList.pop(level, id); _hasEvent[id.id] = false; } }
   return result;
}

const bool
V3AlgAigGeneralize::generalizationValid() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Set Sources
   setSourceFree(V3_FF, false);
   V3NetList::const_iterator it = _genResult.begin();
   while (it != _genResult.end()) {
      if ((*it).cp) _simValue[ntk->getLatch((*it).id).id].set0(0);
      else _simValue[ntk->getLatch((*it).id).id].set1(0); ++it;
   }
   simulate();
   for (uint32_t i = 0; i < _targetId.size(); ++i) 
      if ((_targetId[i].cp ? '0' : '1') != _simValue[_targetId[i].id][0]) return false;
   return true;
}

void
V3AlgAigGeneralize::recoverForGeneralization() {
   for (uint32_t i = 0; i < _simRecord.size(); ++i) 
      _simValue[_simRecord[i].first.id] = _simRecord[i].second;
}

void
V3AlgAigGeneralize::performSimulationForFrozenPropagation(const V3NetVec& newSource) {
   if (!newSource.size()) return; initializeEventList();
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Initialize Events
   V3UI32Vec iList; iList.clear(); V3NetVec tList; tList.clear();
   for (uint32_t i = 0; i < newSource.size(); ++i) {
      assert (!_hasEvent[newSource[i].id]); _hasEvent[newSource[i].id] = true;
      iList.push_back(_levelData[newSource[i].id]); tList.push_back(newSource[i]);
   }
   _eventList.add(iList, tList);
   // Check Fanout Frozen
   V3BitVecS value; uint32_t level;
   V3NetId id, id1, id2, id3;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_isFrozen[id.id]); assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]);
         if (!_traverse[id1.id] || _hasEvent[id1.id] || _isFrozen[id1.id]) continue;
         value = _simValue[id1.id]; assert ('0' == value[0] || '1' == value[0]);
         assert (AIG_NODE == ntk->getGateType(id1)); assert (level < _levelData[id1.id]);
         id2 = ntk->getInputNetId(id1, 0); id3 = ntk->getInputNetId(id1, 1);
         assert (_isFrozen[id2.id] || _isFrozen[id3.id]);
         if ('0' == value[0]) {
            // Frozen Output 0 <--> Controlling Input is Frozen
            if (!((_isFrozen[id2.id] && ((id2.cp ? '1' : '0') == _simValue[id2.id][0])) || 
                  (_isFrozen[id3.id] && ((id3.cp ? '1' : '0') == _simValue[id3.id][0])))) continue;
         }
         else {
            // Frozen Output 1 <--> Two Inputs Are Frozen
            if (!(_isFrozen[id2.id] && _isFrozen[id3.id])) continue;
         }
         _isFrozen[id1.id] = true; _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
}

const bool
V3AlgAigGeneralize::performImplicationForTargetUpdate(V3NetVec& newSource) {
   // Main Implication Procedure
   V3BoolVec updated(_handler->getNtk()->getNetSize(), false);
   const V3NetVec targetNets = _targetId; _targetId.clear();
   bool isUpdated = false; newSource.clear();
   for (uint32_t i = 0; i < targetNets.size(); ++i)
      isUpdated |= performImplicationForTargetUpdate(targetNets[i], newSource, updated);
   return isUpdated;
}

const bool
V3AlgAigGeneralize::performImplicationForTargetUpdate(const V3NetId& id, V3NetVec& newSource, V3BoolVec& updated) {
   // Recursive Implication Procedure
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (_traverse[id.id]);
   assert (_isFrozen[id.id]); assert ('0' == _simValue[id.id][0] || '1' == _simValue[id.id][0]);
   assert (updated.size() > id.id); if (updated[id.id]) return false; updated[id.id] = true;
   if (AIG_NODE == ntk->getGateType(id)) {
      const V3NetId id1 = ntk->getInputNetId(id, 0), id2 = ntk->getInputNetId(id, 1);
      if ('0' == _simValue[id.id][0]) {
         const bool control1 = (id1.cp ? '1' : '0') == _simValue[id1.id][0];  // 0-Controlling
         const bool control2 = (id2.cp ? '1' : '0') == _simValue[id2.id][0];  // 0-Controlling
         if (control1) {
            if (!control2) {
               if (!_isFrozen[id1.id]) { newSource.push_back(id1); _isFrozen[id1.id] = true; }
               performImplicationForTargetUpdate(id1, newSource, updated);
            }
            else {
               _targetId.push_back(V3NetId::makeNetId(id.id, ('0' == _simValue[id.id][0])));
               return false;
            }
         }
         else {
            if (!_isFrozen[id2.id]) { newSource.push_back(id2); _isFrozen[id2.id] = true; }
            assert (control2); performImplicationForTargetUpdate(id2, newSource, updated);
         }
      }
      else {
         assert ((id1.cp ? '0' : '1') == _simValue[id1.id][0]);  // 1-Controlling
         assert ((id2.cp ? '0' : '1') == _simValue[id2.id][0]);  // 1-Controlling
         if (!_isFrozen[id1.id]) { newSource.push_back(id1); _isFrozen[id1.id] = true; }
         if (!_isFrozen[id2.id]) { newSource.push_back(id2); _isFrozen[id2.id] = true; }
         performImplicationForTargetUpdate(id1, newSource, updated);
         performImplicationForTargetUpdate(id2, newSource, updated);
      }
   }
   else if (V3_FF == ntk->getGateType(id)) {
      _targetId.push_back(V3NetId::makeNetId(id.id, ('0' == _simValue[id.id][0])));
      return false;
   }
   return true;
}

/* -------------------------------------------------- *\
 * Class V3AlgBvGeneralize Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3AlgBvGeneralize::V3AlgBvGeneralize(const V3NtkHandler* const handler) : V3AlgBvSimulate(handler), V3AlgGeneralize() {
   assert (_handler); V3Ntk* const ntk = _handler->getNtk();
   assert (ntk); assert (dynamic_cast<V3BvNtk*>(ntk));
   _traverse = V3BoolVec(_handler->getNtk()->getNetSize());
}

V3AlgBvGeneralize::~V3AlgBvGeneralize() {
   _targetId.clear(); _traverse.clear();
}

// Generalization Main Functions
void
V3AlgBvGeneralize::setTargetNets(const V3NetVec& curTargets, const V3NetVec& nextTargets) {
   // Note that curTargets are NetIds while nextTargets are Flop Indices
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); _targetId.clear();
   // Set New Target Nets
   for (uint32_t i = 0; i < nextTargets.size(); ++i) {
      assert (nextTargets[i].id < ntk->getLatchSize());
      assert (V3_FF == ntk->getGateType(ntk->getLatch(nextTargets[i].id)));
      _targetId.push_back(ntk->getInputNetId(ntk->getLatch(nextTargets[i].id), 0));
      if (nextTargets[i].cp) _targetId.back() = ~(_targetId.back());
      assert (_targetId.back().cp == ('0' == _simValue[_targetId.back().id][0]));
   }
   for (uint32_t i = 0; i < curTargets.size(); ++i) {
      assert (curTargets[i].id < ntk->getNetSize()); _targetId.push_back(curTargets[i]);
      if (!(_targetId.back().cp == ('0' == _simValue[_targetId.back().id][0])))
      assert (_targetId.back().cp == ('0' == _simValue[_targetId.back().id][0]));
   }
   // Reset Generalization Data
   _genResult.clear(); _undecided.clear();
   for (uint32_t i = 0, j = ntk->getLatchSize(); i < j; ++i) {
      if ('X' == _simValue[ntk->getLatch(i).id][0]) continue;
      _genResult.push_back(V3NetId::makeNetId(i, ('0' == _simValue[ntk->getLatch(i).id][0])));
      _undecided.push_back(_genResult.back()); assert (_genResult.size() == _undecided.size());
   }
   //for (uint32_t i = 0; i < ntk->getNetSize(); ++i) _isFrozen[i] = ('X' == _simValue[i][0]);
   initializeEventList(); assert (generalizationValid());
}

// Preprocessing Techniques
void
V3AlgBvGeneralize::performSetXForNotCOIVars() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); if (!_undecided.size()) return;
   // Mark Fanin Cone from Targets
   for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
   for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
   // Preprocessing by Removing Variables NOT in COI
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      // Retain Variables in COI
      if (_traverse[ntk->getLatch((*it).id).id]) { ++it; ++is; }
      else { _undecided.erase(it++); _genResult.erase(is++); }
   }
   assert (generalizationValid());
}

void
V3AlgBvGeneralize::performFixForControlVars(const bool& parallelSim) {
}

// Generalization Heuristics
void
V3AlgBvGeneralize::performXPropForExtensibleVars(const V3UI32Vec& generalizeOrder) {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   if (!_undecided.size() || !generalizeOrder.size()) return;
   // Mark Fanin Cone from Targets
   for (uint32_t i = 0; i < _traverse.size(); ++i) _traverse[i] = false;
   for (uint32_t i = 0; i < _targetId.size(); ++i) dfsMarkFaninCone(ntk, _targetId[i], _traverse);
   // Mark Target Nets
   V3BoolVec isTarget = V3BoolVec(_handler->getNtk()->getNetSize(), false);
   for (uint32_t i = 0; i < _targetId.size(); ++i) isTarget[_targetId[i].id] = true;
   // Mark Undecided Variables
   V3BoolVec varUndecided(ntk->getLatchSize(), false);
   V3BoolVec varGeneralized(ntk->getLatchSize(), false);
   V3NetList::const_iterator ix = _undecided.begin();
   while (ix != _undecided.end()) {
      assert (varUndecided.size() > (*ix).id);
      assert (!varUndecided[(*ix).id]);
      varUndecided[(*ix).id] = true; ++ix;
   }
   // Generalize According to a Given Variable Order
   V3BitVecX value;
   for (uint32_t i = 0; i < generalizeOrder.size(); ++i) {
      assert (varUndecided.size() > generalizeOrder[i]);
      if (!varUndecided[generalizeOrder[i]]) continue;
      varUndecided[generalizeOrder[i]] = false;  // The variable will be decided
      if (!_traverse[ntk->getLatch(generalizeOrder[i]).id]) continue;
      assert ('X' != _simValue[ntk->getLatch(generalizeOrder[i]).id][0]);
      // Put X for this Latch
      value = _simValue[ntk->getLatch(generalizeOrder[i]).id]; value.setX(0);
      // If Generalization Failed : The Variable is a Controlling (i.e. inextensible) Variable
#ifdef V3_SIM_GENERALIZATION_EARLY_QUIT
      if (!simulateForGeneralization(ntk->getLatch(generalizeOrder[i]), value, isTarget)) {
         recoverForGeneralization(); varGeneralized[generalizeOrder[i]] = true;
      }
#else
      simulateForGeneralization(ntk->getLatch(generalizeOrder[i]), value);
      for (uint32_t x = 0; x < _targetId.size(); ++x) {
         if ('X' != _simValue[_targetId[x].id][0]) continue;
         recoverForGeneralization(); varGeneralized[generalizeOrder[i]] = true;
         assert ('X' != _simValue[_targetId[x].id][0]); break;
      }
#endif
   }
   // Update Generalization Data
   V3NetList::iterator it = _undecided.begin();
   V3NetList::iterator is = _genResult.begin();
   while (_undecided.end() != it) {
      // Align Variables
      while ((*it).id != (*is).id) {
         assert (_genResult.end() != is);
         assert ((*it).id >= (*is).id); ++is;
      }
      assert (varUndecided.size() > (*it).id);
      if (varUndecided[(*it).id]) ++it;  // The Variable is Undecided
      else {
         if (!varGeneralized[(*is).id]) _genResult.erase(is++);  // The Variable is Generalized
         else ++is; _undecided.erase(it++);
      }
   }
   assert (generalizationValid());
}

void
V3AlgBvGeneralize::performXPropForMinimizeTransitions(const uint32_t& frameSize, const V3NetVec& targetValue) {
}

// Private Generalization Functions
void
V3AlgBvGeneralize::simulateForGeneralization(const V3NetId& generalizeId, const V3BitVecX& generalizeValue) {
#ifdef V3_USE_NEW_SIM_GENERALIZATION
   // Clear Generalization Record and Set Source Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]); assert (_traverse[generalizeId.id]);
   initializeEventList(); _simRecord.clear(); _simRecord.reserve(ntk->getNetSize());
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue;
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecX value; uint32_t level; V3NetId id, id1;
   for (uint32_t i = 0; i < _fanoutVec[generalizeId.id].size(); ++i) {
      id1 = _fanoutVec[generalizeId.id][i]; assert (_levelData[id1.id]);
      if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
      _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
   }
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      // Simulate for the New Value
      value = _simValue[id.id]; simulateGate(id);
      if (value == _simValue[id.id]) continue; _simRecord.push_back(make_pair(id, value));
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (level < _levelData[id1.id]);
         if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#else
   // Clear Generalization Record and Set Source Event
   initializeEventList(); _simRecord.clear();
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (V3_FF == ntk->getGateType(generalizeId)); assert (generalizeValue != _simValue[generalizeId.id]);
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue; if (!_traverse[generalizeId.id]) return;
   _hasEvent[generalizeId.id] = true; _eventList.add(_levelData[generalizeId.id], generalizeId);
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecX value; uint32_t level; V3NetId id, id1;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]);
         if (!_traverse[id1.id]) continue; value = _simValue[id1.id];
         assert (level < _levelData[id1.id]); simulateGate(id1);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _simRecord.push_back(make_pair(id1, value));
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#endif
}

const bool
V3AlgBvGeneralize::simulateForGeneralization(const V3NetId& generalizeId, const V3BitVecX& generalizeValue, 
                                             const V3BoolVec& isTarget) {
#ifdef V3_USE_NEW_SIM_GENERALIZATION
   // Clear Generalization Record and Set Source Event
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]); assert (_traverse[generalizeId.id]);
   assert (generalizeId.id < isTarget.size()); _simRecord.clear();
   initializeEventList(); if (isTarget[generalizeId.id]) return false;
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue;
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecX value; uint32_t level; bool result = true; V3NetId id, id1;
   for (uint32_t i = 0; i < _fanoutVec[generalizeId.id].size(); ++i) {
      id1 = _fanoutVec[generalizeId.id][i]; assert (_levelData[id1.id]);
      if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
      _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
   }
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false; assert (result);
      // Simulate for the New Value
      value = _simValue[id.id]; simulateGate(id);
      if (value == _simValue[id.id]) continue; _simRecord.push_back(make_pair(id, value));
      assert (id.id < isTarget.size()); if (isTarget[id.id]) { result = false; break; }
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (level < _levelData[id1.id]);
         if (!_traverse[id1.id] || _hasEvent[id1.id] || _simValue[id1.id].allX()) continue;
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#else
   // Clear Generalization Record and Set Source Event
   initializeEventList(); _simRecord.clear();
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   assert (generalizeId.id < isTarget.size());
   if (isTarget[generalizeId.id]) return false;
   assert (V3_FF == ntk->getGateType(generalizeId));
   assert (generalizeValue != _simValue[generalizeId.id]);
   _simRecord.push_back(make_pair(generalizeId, _simValue[generalizeId.id]));
   _simValue[generalizeId.id] = generalizeValue; assert (_traverse[generalizeId.id]);
   _hasEvent[generalizeId.id] = true; _eventList.add(_levelData[generalizeId.id], generalizeId);
   // Perform Event-Driven Simulation for Marked Nets with Record
   V3BitVecX value; uint32_t level; bool result = true; V3NetId id, id1;
   while (_eventList.size()) {
      _eventList.pop(level, id); assert (level == _levelData[id.id]);
      assert (id.id < isTarget.size()); assert (!isTarget[id.id]);
      assert (_hasEvent[id.id]); _hasEvent[id.id] = false;
      for (uint32_t i = 0; i < _fanoutVec[id.id].size(); ++i) {
         id1 = _fanoutVec[id.id][i]; assert (_levelData[id1.id]);
         if (!_traverse[id1.id] || _simValue[id1.id].allX()) continue;
         value = _simValue[id1.id]; assert (level < _levelData[id1.id]);
         // Simulate for the New Value
         simulateGate(id1);
         if (_hasEvent[id1.id] || value == _simValue[id1.id]) continue;
         _simRecord.push_back(make_pair(id1, value));
         assert (id1.id < isTarget.size()); if (isTarget[id1.id]) { result = false; break; }
         _hasEvent[id1.id] = true; _eventList.add(_levelData[id1.id], id1);
      }
   }
#endif
   if (!result) { while (_eventList.size()) { _eventList.pop(level, id); _hasEvent[id.id] = false; } }
   return result;
}

const bool
V3AlgBvGeneralize::generalizationValid() {
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Set Sources
   setSourceFree(V3_FF, false);
   V3NetList::const_iterator it = _genResult.begin();
   while (it != _genResult.end()) {
      if ((*it).cp) _simValue[ntk->getLatch((*it).id).id].set0(0);
      else _simValue[ntk->getLatch((*it).id).id].set1(0); ++it;
   }
   simulate();
   for (uint32_t i = 0; i < _targetId.size(); ++i) 
      if ((_targetId[i].cp ? '0' : '1') != _simValue[_targetId[i].id][0]) return false;
   return true;
}

void
V3AlgBvGeneralize::recoverForGeneralization() {
   for (uint32_t i = 0; i < _simRecord.size(); ++i) 
      _simValue[_simRecord[i].first.id] = _simRecord[i].second;
}

#endif

