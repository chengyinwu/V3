/****************************************************************************
  FileName     [ v3StgFSM.cpp ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ FSM Data Structure for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_FSM_C
#define V3_STG_FSM_C

#include "v3StgFSM.h"
#include "v3NumUtil.h"
#include "v3StrUtil.h"
#include "v3FileUtil.h"
#include "v3NtkWriter.h"

/* -------------------------------------------------- *\
 * Class V3FSMSDG Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3FSMSDG::V3FSMSDG(const V3NetId& id) : _netId(id) {
   _falseSDG.clear(); _trueSDG.clear(); _falseSize = 0; _trueSize = 0;
}

V3FSMSDG::~V3FSMSDG() {
   for (V3FSMSDGSet::iterator it = _falseSDG.begin(); it != _falseSDG.end(); ++it) delete *it;
   for (V3FSMSDGSet::iterator it = _trueSDG.begin(); it != _trueSDG.end(); ++it) delete *it;
   _falseSDG.clear(); _trueSDG.clear();
}

// State Computation Functions
void
V3FSMSDG::addFalseSDG(V3FSMSDG* const& s) {
   assert (s); assert (s->getStateSize());
   assert (_falseSDG.end() == _falseSDG.find(s));
   uint32_t size = 0;
   if (_falseSize) {
      assert (V3SafeMult(_falseSize, s->getStateSize(), size));
      _falseSize *= s->getStateSize();
   }
   else _falseSize = s->getStateSize(); _falseSDG.insert(s);
}

void
V3FSMSDG::addTrueSDG(V3FSMSDG* const& s) {
   assert (s); assert (s->getStateSize());
   assert (_trueSDG.end() == _trueSDG.find(s));
   uint32_t size = 0;
   if (_trueSize) {
      assert (V3SafeMult(_trueSize, s->getStateSize(), size));
      _trueSize *= s->getStateSize();
   }
   else _trueSize = s->getStateSize(); _trueSDG.insert(s);
}

const uint32_t
V3FSMSDG::getStateSize() const {
   uint32_t size = _falseSize;
   if (V3SafeAdd(size, _trueSize)) return size;
   else return V3NtkUD;
}

// Output Functions
void
V3FSMSDG::getSDGExpr(const V3UI32Vec& stateMap, string& exp) const {
   assert (_netId.id < stateMap.size());
   exp += (_netId.cp ? " ~" : " ") + v3Int2Str(stateMap[_netId.id]) + " ";
   exp += (_trueSize ? v3Int2Str(_trueSDG.size()) : "X") + " " + (_falseSize ? v3Int2Str(_falseSDG.size()) : "X");
   for (V3FSMSDGSet::const_iterator it = _trueSDG.begin(); it != _trueSDG.end(); ++it) (*it)->getSDGExpr(stateMap, exp);
   for (V3FSMSDGSet::const_iterator it = _falseSDG.begin(); it != _falseSDG.end(); ++it) (*it)->getSDGExpr(stateMap, exp);
}

/* -------------------------------------------------- *\
 * Class V3FSMSDGDB Implementations
\* -------------------------------------------------- */
V3FSMSDG* const
createSDGFromSpec(V3Ntk* const ntk, const V3StrVec& tokens, uint32_t& index) {
   assert (ntk); assert (tokens.size());
   if (tokens.size() <= (index + 3)) {
      Msg(MSG_ERR) << "Line " << tokens.back() << " : Incomplete SDG Specification Found !!" << endl; return 0; }
   const bool cp = tokens[index].size() && ('~' == tokens[index][0] || '!' == tokens[index][0] || '-' == tokens[index][0]);
   uint32_t varIndex, tSize, fSize;
   if (cp) {
      if (!v3Str2UInt(tokens[index].substr(1), varIndex) || ntk->getOutputSize() <= varIndex) {
         Msg(MSG_ERR) << "Line " << tokens.back() << " : State Variable with Index = \"" << tokens[index].substr(1)
                      << "\" Not Found !!" << endl; return 0; }
   }
   else {
      if (!v3Str2UInt(tokens[index], varIndex) || ntk->getOutputSize() <= varIndex) {
         Msg(MSG_ERR) << "Line " << tokens.back() << " : State Variable with Index = \"" << tokens[index]
                      << "\" Not Found !!" << endl; return 0; }
   }
   tSize = ("X" == tokens[++index]) ? V3NtkUD : 0;
   if ((V3NtkUD != tSize) && (!v3Str2UInt(tokens[index], tSize) || tSize < 0)) {
      Msg(MSG_ERR) << "Line " << tokens.back() << " : Unexpected Number of True SDGs = \"" << tokens[index]
                   << "\" for State Variable with Index = \"" << varIndex << "\" !!" << endl; return 0; }
   fSize = ("X" == tokens[++index]) ? V3NtkUD : 0;
   if ((V3NtkUD != fSize) && (!v3Str2UInt(tokens[index], fSize) || fSize < 0)) {
      Msg(MSG_ERR) << "Line " << tokens.back() << " : Unexpected Number of False SDGs = \"" << tokens[index]
                   << "\" for State Variable with Index = \"" << varIndex << "\" !!" << endl; return 0; }
   V3FSMSDG* const sdg = new V3FSMSDG(cp ? ~(ntk->getOutput(varIndex)) : ntk->getOutput(varIndex));
   if (!sdg) {
      Msg(MSG_ERR) << "Line " << tokens.back() << " : Failed to Create SDG Node for State Variable with Index = \""
                   << varIndex << "\" !!" << endl; return 0; }
   // Recursive Construct Child SDG
   V3FSMSDG* sdgNode = 0; uint32_t size = 0;
   if (V3NtkUD != tSize) {
      if (!tSize) sdg->setTrueEmptyButExist();
      for (uint32_t i = 0; i < tSize; ++i) {
         ++index; sdgNode = createSDGFromSpec(ntk, tokens, index);
         if (!sdgNode) { delete sdg; return 0; }
         if (sdg->getTrueStateSize() && !V3SafeMult(sdg->getTrueStateSize(), sdgNode->getStateSize(), size)) {
            Msg(MSG_ERR) << "Line " << tokens.back() << " : The Number of States in the SDG Exceeds "
                         << "a Maximum Bound of Storage !!" << endl; delete sdg; return 0; }
         sdg->addTrueSDG(sdgNode);
      }
   }
   if (V3NtkUD != fSize) {
      if (!fSize) sdg->setFalseEmptyButExist();
      for (uint32_t i = 0; i < fSize; ++i) {
         ++index; sdgNode = createSDGFromSpec(ntk, tokens, index);
         if (!sdgNode) { delete sdg; return 0; }
         if (sdg->getFalseStateSize() && !V3SafeMult(sdg->getFalseStateSize(), sdgNode->getStateSize(), size)) {
            Msg(MSG_ERR) << "Line " << tokens.back() << " : The Number of States in the SDG Exceeds "
                         << "a Maximum Bound of Storage !!" << endl; delete sdg; return 0; }
         sdg->addFalseSDG(sdgNode);
      }
   }
   assert (sdg); return sdg;
}

// Constructor and Destructor
V3FSMSDGDB::V3FSMSDGDB(V3SDG* const sdgMain, const V3NetVec& depVarList, const V3SolverType& solverType) 
   : _handler(sdgMain->getNtkHandler()), _solver(allocSolver(solverType, sdgMain->getNtkHandler()->getNtk())),
     _sdgMain(sdgMain) {
   assert (_handler); assert (_solver); assert (depVarList.size());
   _sdgList.clear(); _depVars.clear(); _maxSize = V3NtkUD; _configs = 0;
   // Simplify SDG and Record into Database
   for (uint32_t i = 0; i < depVarList.size(); ++i) _depVars.insert(depVarList[i].id);
}

V3FSMSDGDB::V3FSMSDGDB(V3SDG* const sdgMain, const V3NetVec& depVarList, V3SvrBase* const solver) 
   : _handler(sdgMain->getNtkHandler()), _solver(solver), _sdgMain(sdgMain) {
   assert (_handler); assert (_solver); assert (depVarList.size());
   _sdgList.clear(); _depVars.clear(); _maxSize = V3NtkUD; _configs = 0;
   _sdgList.clear(); _depVars.clear(); _maxSize = V3NtkUD; _configs = 64ul;
   // Simplify SDG and Record into Database
   for (uint32_t i = 0; i < depVarList.size(); ++i) _depVars.insert(depVarList[i].id);
}

V3FSMSDGDB::V3FSMSDGDB(V3NtkHandler* const handler, const V3StrTable& sdgSpec)
   : _handler(handler), _solver(0), _sdgMain(0) {
   assert (_handler); assert (handler->getNtk()); assert (sdgSpec.size());
   _sdgList.clear(); _depVars.clear(); _maxSize = V3NtkUD; _configs = 0;
   // Construct V3FSMSDG for Each Specification defined in sdgSpec
   for (uint32_t i = 0; i < sdgSpec.size(); ++i) {
      uint32_t index = 0; _sdgList.push_back(createSDGFromSpec(handler->getNtk(), sdgSpec[i], index));
      if (!_sdgList.back()) { setEarlySuspended(); return; }
      if (sdgSpec[i].size() > (2 + index)) {
         Msg(MSG_ERR) << "Line " << sdgSpec[i].back() << " : Extra SDG Specification Found !!" << endl;
         setEarlySuspended(); return;
      }
   }
}

V3FSMSDGDB::~V3FSMSDGDB() {
   if (_solver && !isExternalSolver()) delete _solver; _depVars.clear();
   for (uint32_t i = 0; i < _sdgList.size(); ++i) delete _sdgList[i]; _sdgList.clear();
}

// Main Functions
void
V3FSMSDGDB::startConstructFSMSDG() {
   if (_sdgList.size() || !_depVars.size()) return;
   V3FSMSDGMap selectMap; selectMap.clear(); assert (isValid());
   V3NetVec parentNets; parentNets.clear();
   uint32_t stateSize = 0; V3NetId id; assert (_sdgMain);
   for (V3UI32Set::const_iterator it = _depVars.begin(); it != _depVars.end(); ++it) {
      id = V3NetId::makeNetId(*it); assert (_sdgMain->getSDGNode(id.id));
      assert (!dynamic_cast<V3SDGMUX*>(_sdgMain->getSDGNode(id.id)));
      if (isConstructStandardFSMEnabled()) {
         if (V3_FF != _handler->getNtk()->getGateType(id)) { setEarlySuspended(); break; }
         V3UI32Set depMuxSet = _sdgMain->getSDGNode(id.id)->getBase()->getDepMuxSet();
         if (1 != depMuxSet.size() || (*(depMuxSet.begin())) != _handler->getNtk()->getInputNetId(id, 0).id) {
            setEarlySuspended(); break; }
      }
      if (!startConstructFSMSDG(_sdgMain->getSDGNode(id.id)->getBase(), stateSize, selectMap, parentNets)) {
         assert (!isValid()); break;
      }
      assert (!parentNets.size()); assert (isValid());
   }
   if (!isValid()) for (V3FSMSDGMap::iterator is = selectMap.begin(); is != selectMap.end(); ++is) delete is->second;
   else for (V3FSMSDGMap::iterator is = selectMap.begin(); is != selectMap.end(); ++is) _sdgList.push_back(is->second);
}

const uint32_t
V3FSMSDGDB::getStateSize() const {
   uint32_t size = (_sdgList.size()) ? _sdgList[0]->getStateSize() : 0;
   for (uint32_t i = 1; i < _sdgList.size(); ++i) {
      assert (size); assert (_sdgList[i]->getStateSize());
      if (!V3SafeMult(size, _sdgList[i]->getStateSize())) return V3NtkUD;
   }
   return size;
}

// Private FSMSDG Computation Functions
const bool
V3FSMSDGDB::startConstructFSMSDG(V3SDGBase* const sdg, uint32_t& stateSize, V3FSMSDGMap& selectMap, V3NetVec& parentNets) {
   assert (isValid()); assert (sdg);
   // Get Dependent MUX Nodes
   V3UI32Set depMuxSet = sdg->getDepMuxSet(); if (!depMuxSet.size()) return true;
   // Omit Nodes that are Independent to Variables in _depVars
   if (isOmitNodesIndepToVarsEnabled() || isOmitNodesAllCombVarsEnabled()) {
      updateDepMuxSet(depMuxSet); if (!isValid()) return false; }
   // Prepare Internal Variables
   V3FSMSDGMap::const_iterator is;
   // Construct FSMSDG for Each Dependent SDGMUX Node
   for (V3UI32Set::const_iterator it = depMuxSet.begin(); it != depMuxSet.end(); ++it) {
      assert (_sdgMain->getSDGNode(V3NetId::makeNetId(*it).id));
      V3FSMSDG* const depSDG = computeFSMSDG(V3NetId::makeNetId(*it), parentNets, selectMap);
      if (!depSDG) { assert (!isValid()); return false; } assert (isValid());
      // Merge Sibling Nodes with the Same Select Signal
      if (!isRenderNonEmptyStatesEnabled()) {
         is = selectMap.find(depSDG->getStateNet().id);
         if (selectMap.end() != is) {
            const uint32_t oldStateSize = is->second->getStateSize(); assert (oldStateSize);
            if (mergeMuxFaninSDG(depSDG, is->second, depSDG->getStateNet() != is->second->getStateNet())) {
               assert (stateSize); assert (stateSize % oldStateSize == 0); stateSize /= oldStateSize;
               if (V3SafeMult(stateSize, is->second->getStateSize()) && (_maxSize >= stateSize)) continue;
               setEarlySuspended(); assert (!isValid()); return false;
            }
            assert (!isValid()); delete depSDG; return false;
         }
         assert (selectMap.end() == selectMap.find(depSDG->getStateNet().id));
      }
      else {  // Currently Asserts Single Instances in the selectMap if isRenderNonEmptyStatesEnabled() is true
         assert (selectMap.size() <= 1); selectMap.clear();
      }
      selectMap.insert(make_pair(depSDG->getStateNet().id, depSDG));
      // Check State Size Overflow
      assert ((selectMap.size() == 1) || stateSize);
      if (1 == selectMap.size()) stateSize = depSDG->getStateSize();
      else if (!V3SafeMult(stateSize, depSDG->getStateSize()) || (_maxSize < stateSize)) {
         setEarlySuspended(); assert (!isValid()); return false;
      }
   }
   assert (isValid()); return true;
}

V3FSMSDG* const
V3FSMSDGDB::computeFSMSDG(const V3NetId& id, V3NetVec& parentNets, const V3FSMSDGMap& siblingMap) {
   // Create New FSMSDG and Set State Net
   V3SDGMUX* const mux = dynamic_cast<V3SDGMUX*>(_sdgMain->getSDGNode(id.id)); assert (mux);
   V3FSMSDG* const sdg = new V3FSMSDG(mux->getNetId()); assert (sdg);
   // Recursively Compute the True FSMSDGs
   parentNets.push_back(mux->getNetId());
   uint32_t trueSize = 0, falseSize = 0;
   if (existState(parentNets)) {
      V3FSMSDGMap selectMap; selectMap.clear();
      if (startConstructFSMSDG(mux->getTBase(), trueSize, selectMap, parentNets)) {
         if (!selectMap.size()) {
            if (isRenderNonEmptyStatesEnabled() && siblingMap.size()) {
               // Currently Asserts Single Instances in the selectMap if isRenderNonEmptyStatesEnabled() is true
               assert (siblingMap.size() == 1);
               V3FSMSDG* const newSDG = computeFSMSDG(siblingMap.begin()->second, parentNets);
               assert (newSDG); sdg->addTrueSDG(newSDG); trueSize = newSDG->getStateSize();
            }
            else { assert (!trueSize); ++trueSize; sdg->setTrueEmptyButExist(); }
         }
         else {
            // Currently Asserts Single Instances in the selectMap if isRenderNonEmptyStatesEnabled() is true
            assert (!isRenderNonEmptyStatesEnabled() || selectMap.size() == 1);
            for (V3FSMSDGMap::const_iterator is = selectMap.begin(); is != selectMap.end(); ++is) 
               sdg->addTrueSDG(is->second);
         }
      }
      else {
         for (V3FSMSDGMap::iterator is = selectMap.begin(); is != selectMap.end(); ++is) delete is->second;
         delete sdg; return 0;
      }
   }
   assert (trueSize == sdg->getTrueStateSize());
   // Recursively Compute the False FSMSDGs
   parentNets.back().cp ^= 1;
   if (existState(parentNets)) {
      V3FSMSDGMap selectMap; selectMap.clear();
      if (startConstructFSMSDG(mux->getFBase(), falseSize, selectMap, parentNets)) {
         if (!selectMap.size()) {
            if (isRenderNonEmptyStatesEnabled() && siblingMap.size()) {
               // Currently Asserts Single Instances in the selectMap if isRenderNonEmptyStatesEnabled() is true
               assert (siblingMap.size() == 1);
               V3FSMSDG* const newSDG = computeFSMSDG(siblingMap.begin()->second, parentNets);
               assert (newSDG); sdg->addFalseSDG(newSDG); falseSize = newSDG->getStateSize();
            }
            else { assert (!falseSize); ++falseSize; sdg->setFalseEmptyButExist(); }
         }
         else {
            // Currently Asserts Single Instances in the selectMap if isRenderNonEmptyStatesEnabled() is true
            assert (!isRenderNonEmptyStatesEnabled() || selectMap.size() == 1);
            for (V3FSMSDGMap::const_iterator is = selectMap.begin(); is != selectMap.end(); ++is) 
               sdg->addFalseSDG(is->second);
         }
      }
      else {
         for (V3FSMSDGMap::iterator is = selectMap.begin(); is != selectMap.end(); ++is) delete is->second;
         delete sdg; return 0;
      }
   }
   assert (falseSize == sdg->getFalseStateSize());
   parentNets.pop_back();
   // Confirm that SDG Should NOT be Empty
   assert (trueSize || falseSize);
   if (trueSize && falseSize && (!V3SafeAdd(trueSize, falseSize) || (_maxSize < trueSize))) {
      setEarlySuspended(); assert (!isValid()); delete sdg; return 0;
   }
   assert (sdg->getStateSize()); assert (isValid()); return sdg;
}

V3FSMSDG* const
V3FSMSDGDB::computeFSMSDG(V3FSMSDG* const mux, V3NetVec& parentNets) {
   assert (isRenderNonEmptyStatesEnabled()); assert (isValid()); assert (mux);
   // Create New FSMSDG and Set State Net
   V3FSMSDG* const sdg = new V3FSMSDG(mux->getStateNet()); assert (sdg);
   // Recursively Compute the True FSMSDGs
   parentNets.push_back(mux->getStateNet());
   uint32_t trueSize = 0, falseSize = 0;
   if (existState(parentNets)) {
      // Currently Asserts Single Instances in the trueSDG if isRenderNonEmptyStatesEnabled() is true
      const V3FSMSDGSet& trueSDG = mux->getTrueSDG(); assert (trueSDG.size() <= 1);
      if (trueSDG.size()) {
         V3FSMSDG* const newSDG = computeFSMSDG(*(trueSDG.begin()), parentNets);
         assert (newSDG); sdg->addTrueSDG(newSDG); trueSize = newSDG->getStateSize();
      }
      else { assert (!trueSize); ++trueSize; sdg->setTrueEmptyButExist(); }
   }
   assert (trueSize == sdg->getTrueStateSize());
   // Recursively Compute the False FSMSDGs
   parentNets.back().cp ^= 1;
   if (existState(parentNets)) {
      // Currently Asserts Single Instances in the falseSDG if isRenderNonEmptyStatesEnabled() is true
      const V3FSMSDGSet& falseSDG = mux->getFalseSDG(); assert (falseSDG.size() <= 1);
      if (falseSDG.size()) {
         V3FSMSDG* const newSDG = computeFSMSDG(*(falseSDG.begin()), parentNets);
         assert (newSDG); sdg->addFalseSDG(newSDG); falseSize = newSDG->getStateSize();
      }
      else { assert (!falseSize); ++falseSize; sdg->setFalseEmptyButExist(); }
   }
   assert (falseSize == sdg->getFalseStateSize());
   parentNets.pop_back();
   // Confirm that SDG Should NOT be Empty
   assert (!(trueSize && falseSize) || (V3SafeAdd(trueSize, falseSize) && (_maxSize >= trueSize)));
   assert (trueSize || falseSize); assert (sdg->getStateSize()); assert (isValid()); return sdg;
}

const bool
V3FSMSDGDB::mergeMuxFaninSDG(V3FSMSDG* const sdgNew, V3FSMSDG* const sdgOld, const bool& invert) {
   assert (sdgNew); assert (sdgOld);
   const V3FSMSDGSet& falseSDG = invert ? sdgNew->getTrueSDG() : sdgNew->getFalseSDG();
   const V3FSMSDGSet& trueSDG = invert ? sdgNew->getFalseSDG() : sdgNew->getTrueSDG();
   uint32_t stateSize = sdgOld->getFalseStateSize(); assert (stateSize);
   for (V3FSMSDGSet::const_iterator it = falseSDG.begin(); it != falseSDG.end(); ++it) {
      if (V3SafeMult(stateSize, (*it)->getStateSize())) sdgOld->addFalseSDG(*it);
      else { setEarlySuspended(); assert (!isValid()); return false; }
   }
   stateSize = sdgOld->getTrueStateSize(); assert (stateSize);
   for (V3FSMSDGSet::const_iterator it = trueSDG.begin(); it != trueSDG.end(); ++it) {
      if (V3SafeMult(stateSize, (*it)->getStateSize())) sdgOld->addTrueSDG(*it);
      else { setEarlySuspended(); assert (!isValid()); return false; }
   }
   return V3SafeAdd(stateSize, sdgOld->getFalseStateSize());
}

void
V3FSMSDGDB::updateDepMuxSet(V3UI32Set& depMuxSet) {
   V3UI32Vec sourceList; sourceList.clear(); sourceList.reserve(depMuxSet.size());
   V3UI32Set::const_iterator it;
   for (it = depMuxSet.begin(); it != depMuxSet.end(); ++it) sourceList.push_back(*it);
   V3UI32Set failSet; failSet.clear(); depMuxSet.clear(); assert (_depVars.size());
   for (uint32_t i = 0; i < sourceList.size(); ++i) {
      const V3NetId id = V3NetId::makeNetId(sourceList[i]);
      assert (dynamic_cast<V3SDGMUX*>(_sdgMain->getSDGNode(id.id)));
      if (failSet.end() != failSet.find(id.id)) continue;
      if (depMuxSet.end() != depMuxSet.find(id.id)) continue;
      // Collect All Dependent FF of the Select Signal
      V3SDGBase* const node = _sdgMain->getSDGNode(id.id)->getBase(); assert (node);
      if (!node->getDepFFCone()) {
         node->newDepFFCone(); assert (node->getDepFFCone());
         _sdgMain->collectFaninFF(node->getDepFFCone(), node);
      }
      const V3UI32Set& depFFSet = node->getDepFFCone()->getDepFFSet();
      // Check if the Select Signal Contains Any Core Variables
      if (depFFSet.size()) {
         if (!isOmitNodesIndepToVarsEnabled()) { depMuxSet.insert(id.id); continue; }
         if (isConstructStandardFSMEnabled()) {
            for (it = depFFSet.begin(); it != depFFSet.end(); ++it) {
               assert (V3_FF == _sdgMain->getNtkHandler()->getNtk()->getGateType(V3NetId::makeNetId(*it)));
               if (_depVars.end() == _depVars.find(*it)) break;
            }
            if (depFFSet.end() == it) { depMuxSet.insert(id.id); continue; }
         }
         else {
            for (it = depFFSet.begin(); it != depFFSet.end(); ++it) {
               assert (V3_FF == _sdgMain->getNtkHandler()->getNtk()->getGateType(V3NetId::makeNetId(*it)));
               if (_depVars.end() != _depVars.find(*it)) break;
            }
            if (depFFSet.end() != it) { depMuxSet.insert(id.id); continue; }
         }
      }
      else if (!isOmitNodesAllCombVarsEnabled()) { depMuxSet.insert(id.id); continue; }
      // Remove the MUX and Replace by its Fanin MUX
      V3SDGMUX* const mux = dynamic_cast<V3SDGMUX*>(_sdgMain->getSDGNode(id.id)); assert (mux);
      assert (mux->getFBase()); assert (mux->getTBase()); failSet.insert(id.id);
      const V3UI32Set& depFalseMuxSet = mux->getFBase()->getDepMuxSet();
      for (it = depFalseMuxSet.begin(); it != depFalseMuxSet.end(); ++it) sourceList.push_back(*it);
      const V3UI32Set& depTrueMuxSet = mux->getTBase()->getDepMuxSet();
      for (it = depTrueMuxSet.begin(); it != depTrueMuxSet.end(); ++it) sourceList.push_back(*it);
   }
   if (isConstructStandardFSMEnabled() && depMuxSet.size()) {
      V3UI32Set::iterator is = depMuxSet.begin();
      while (is != depMuxSet.end()) {
         const V3NetId muxId = _handler->getNtk()->getInputNetId(V3NetId::makeNetId(*is), 2);
         if (BV_EQUALITY != _handler->getNtk()->getGateType(muxId)) { depMuxSet.erase(is++); continue; }
         if (((*(_depVars.begin())) != _handler->getNtk()->getInputNetId(muxId, 0).id) && 
             ((*(_depVars.begin())) != _handler->getNtk()->getInputNetId(muxId, 1).id)) {
            depMuxSet.erase(is++); continue; }
         ++is;
      }
   }
}

const bool
V3FSMSDGDB::existState(const V3NetVec& stateVars) {
   _solver->assumeRelease();
   for (uint32_t i = 0; i < stateVars.size(); ++i) {
      if (!_solver->existVerifyData(stateVars[i], 0)) {
         _solver->addBoundedVerifyData(stateVars[i], 0);
      }
      _solver->assumeProperty(stateVars[i], false, 0);
   }
   return _solver->assump_solve();
}

/* -------------------------------------------------- *\
 * Class V3FSM Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3FSM::V3FSM(V3FSMSDGDB* const sdgDB, V3SvrBase* const solver) 
   : _sdgDB(sdgDB), _solver(solver),
     _simulator(dynamic_cast<V3BvNtk*>(_sdgDB->getNtkHandler()->getNtk()) ? 
                (V3AlgSimulate*)(new V3AlgBvSimulate(_sdgDB->getNtkHandler())) : 
                (V3AlgSimulate*)(new V3AlgAigSimulate(_sdgDB->getNtkHandler()))) {
   assert (_sdgDB); assert (_solver); assert (_simulator);
   _terminal.clear(); _distInit.clear(); _distTerm.clear(); _reachSucc.clear(); _reachFail.clear();
   _configs = 0; _cutSize = 0;
}

V3FSM::~V3FSM() {
   _terminal.clear(); _distInit.clear(); _distTerm.clear(); delete _simulator;
   for (uint32_t i = 0; i < _reachSucc.size(); ++i) _reachSucc[i].clear(); _reachSucc.clear();
   for (uint32_t i = 0; i < _reachFail.size(); ++i) _reachFail[i].clear(); _reachFail.clear();
}

// FSM Information Functions
const uint32_t
V3FSM::getInitStateSize() const {
   uint32_t size = 0;
   for (uint32_t i = 0; i < _distInit.size(); ++i) if (!_distInit[i]) ++size;
   assert (getStateSize() >= size); return size;
}

const uint32_t
V3FSM::getTermStateSize() const {
   uint32_t size = 0;
   for (uint32_t i = 0; i < _distTerm.size(); ++i) if (!_distTerm[i]) ++size;
   assert (getStateSize() >= size); return size;
}

const uint32_t
V3FSM::getReachableSize() const {
   uint32_t size = 0;
   for (uint32_t i = 0; i < _distInit.size(); ++i) 
      if (V3NtkUD == _distInit[i] && V3NtkUD == _distTerm[i]) ++size;
   assert (getStateSize() >= size); return getStateSize() - size;
}

const uint32_t
V3FSM::getMileStoneSize() const {
   uint32_t size = 0;
   for (uint32_t i = 0; i < _distInit.size(); ++i) 
      if (V3NtkUD != _distInit[i] && V3NtkUD != _distTerm[i]) ++size;
   assert (getStateSize() >= size); return size;
}

const uint32_t
V3FSM::getInitFwdReachableSize() {
   uint32_t initReach = 0;
   for (uint32_t i = 0; i < _distInit.size(); ++i) 
      if (V3NtkUD != _distInit[i]) ++initReach;
   assert (getStateSize() >= initReach); return initReach;
}

const uint32_t
V3FSM::getTermBwdReachableSize() {
   uint32_t termReach = 0;
   for (uint32_t i = 0; i < _distTerm.size(); ++i) 
      if (V3NtkUD != _distTerm[i]) ++termReach;
   assert (getStateSize() >= termReach); return termReach;
}

const uint32_t
V3FSM::getStepFwdFromInit(const uint32_t& state) const {
   assert (_distInit.size() > state); assert (isCompleteFSM()); return _distInit[state];
}

const uint32_t
V3FSM::getStepBwdFromTerm(const uint32_t& state) const {
   assert (_distTerm.size() > state); assert (isCompleteFSM()); return _distTerm[state];
}

const bool
V3FSM::isMileStone(const uint32_t& state) const {
   return V3NtkUD != getStepFwdFromInit(state) && V3NtkUD != getStepBwdFromTerm(state);
}

const bool
V3FSM::isAllReachableFromFwd() const {
   uint32_t i, j;
   for (i = 0; i < _reachSucc.size(); ++i) {
      if (V3NtkUD == getStepFwdFromInit(i) || !_reachSucc[i].size()) continue;
      for (j = 0; j < _reachSucc[i].size(); ++j) if (!_reachSucc[i][j]) break;
      if (j == _reachSucc[i].size()) return true;
   }
   return false;
}

const bool
V3FSM::isAllReachableFromBwd() const {
   uint32_t i, j;
   for (i = 0; i < _reachSucc.size(); ++i) {
      if (V3NtkUD == getStepBwdFromTerm(i)) continue;
      for (j = 0; j < _reachSucc.size(); ++i) 
         if (_reachSucc[j].size() && _reachSucc[j][i]) break;
      if (j == _reachSucc.size()) return true;
   }
   return false;
}

void
V3FSM::collectStateNets(V3UI32Set& stateNets) const {
   const V3FSMSDGList& fsmSDGList = _sdgDB->getFSMSDGList();
   for (uint32_t i = 0; i < fsmSDGList.size(); ++i) collectStateNets(fsmSDGList[i], stateNets);
}

void
V3FSM::getStateNetId(const uint32_t& state, V3NetVec& stateData) const {
   V3UI32Set stateNets; stateNets.clear(); decodeStateToNetId(state, stateData);
   for (uint32_t i = 0; i < stateData.size(); ++i) stateNets.insert(V3NetType(stateData[i]).value);
   // Perform Containment Check to Further Reduce stateNets
   V3UI32Set::const_iterator it = stateNets.begin(), is; stateData.clear(); assert (_solver);
   while (it != stateNets.end()) {
      _solver->assumeRelease(); 
      for (is = stateNets.begin(); is != stateNets.end(); ++is) {
         assert (_solver->existVerifyData(V3NetType(*is).id, 0));
         _solver->assumeProperty(V3NetType(*is).id, it == is, 0);
      }
      if (!_solver->assump_solve()) stateNets.erase(it++);
      else { stateData.push_back(V3NetType(*it).id); ++it; }
   }
   assert (stateData.size());
}

void
V3FSM::getInitStates(V3UI32Vec& states, const bool& mileStoneOnly) const {
   states.clear();
   for (uint32_t i = 0; i < _distInit.size(); ++i) 
      if (!_distInit[i] && (!mileStoneOnly || isMileStone(i))) states.push_back(i);
}

void
V3FSM::getTermStates(V3UI32Vec& states, const bool& mileStoneOnly) const {
   states.clear();
   for (uint32_t i = 0; i < _distTerm.size(); ++i) 
      if (!_distTerm[i] && (!mileStoneOnly || isMileStone(i))) states.push_back(i);
}

void
V3FSM::getStateFwdReachable(const uint32_t& state, V3UI32Vec& fwdStates, const bool& mileStoneOnly) const {
   assert (_reachSucc.size() > state); fwdStates.clear();
   for (uint32_t i = 0; i < _reachSucc[state].size(); ++i)
      if (_reachSucc[state][i] && (!mileStoneOnly || isMileStone(i))) fwdStates.push_back(i);
}

void
V3FSM::getStateBwdReachable(const uint32_t& state, V3UI32Vec& bwdStates, const bool& mileStoneOnly) const {
   assert (_reachSucc.size() > state); bwdStates.clear();
   for (uint32_t i = 0; i < _reachSucc.size(); ++i)
      if (_reachSucc[i].size() && _reachSucc[i][state] && (!mileStoneOnly || isMileStone(i))) bwdStates.push_back(i);
}

// Main Functions
void
V3FSM::startExtractFSM(const V3NetVec& terminal) {
   // Initialize Tables
   const uint32_t stateSize = getStateSize(); if (!stateSize || _distInit.size()) return;
   assert (!_distInit.size() && !_distTerm.size() && !_reachSucc.size() && !_reachFail.size());
   _distInit = V3UI32Vec(stateSize, V3NtkUD); _distTerm = V3UI32Vec(stateSize, V3NtkUD);
   _reachSucc = V3BoolTable(stateSize, V3BoolVec()); _reachFail = V3BoolTable(stateSize, V3BoolVec());
   // Compute Total Cut Size
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (!_terminal.size() && !_cutSize);
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) _cutSize += ntk->getNetWidth(ntk->getLatch(i));
   // Put Full Ntk Data to Solver
   V3UI32Set stateNetSet; stateNetSet.clear(); collectStateNets(stateNetSet);
   for (uint32_t j = 0; j < 2; ++j)
      for (V3UI32Set::const_iterator it = stateNetSet.begin(); it != stateNetSet.end(); ++it)
         if (!_solver->existVerifyData(V3NetId::makeNetId(*it), j)) 
            _solver->addBoundedVerifyData(V3NetId::makeNetId(*it), j);
   // Perform Simulation and Formal Confirmation to Investigate Initial and Terminal State Reachable States
   if (!simulateForForwardConstruction(V3NtkUD)) return;
   if (terminal.size()) {
      for (uint32_t j = 0; j < 2; ++j) 
         for (uint32_t i = 0; i < terminal.size(); ++i)
            if (!_solver->existVerifyData(terminal[i], j)) _solver->addBoundedVerifyData(terminal[i], j);
      _terminal = terminal; if (!formalForBackwardConstruction(V3NtkUD)) return;
   }
   // BFS for Shortest Depth (Reachability) Computation
   bfsUpdateForwardReachability(); bfsUpdateBackwardReachability();
}

void
V3FSM::updateExtractedFSM(const V3NetVec& terminal) {
   assert (_distInit.size()); if (!isCompleteFSM() || (terminal == _terminal)) return;
   _distTerm = V3UI32Vec(getStateSize(), V3NtkUD);
   if (terminal.size()) {
      for (uint32_t j = 0; j < 2; ++j) 
         for (uint32_t i = 0; i < terminal.size(); ++i)
            if (!_solver->existVerifyData(terminal[i], j)) _solver->addBoundedVerifyData(terminal[i], j);
      _terminal = terminal; if (!formalForBackwardConstruction(V3NtkUD)) return;
   }
   // BFS for Shortest Depth (Reachability) Computation
   bfsUpdateForwardReachability(); bfsUpdateBackwardReachability();
}

// Debug Functions
void
V3FSM::debugValues(const V3NetId& id, const uint32_t& depth) const {
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk);
   assert (id.id < ntk->getNetSize()); assert (depth < 2);
   const uint32_t width = ntk->getNetWidth(id);
   const V3BitVecX simValue = _simulator->getSimValue(id).bv_slice(width - 1, 0);
   const V3BitVecX svrValue = _solver->getDataValue(id, depth).bv_slice(width - 1, 0);
   if (simValue.regEx() != svrValue.regEx()) {
      const V3GateType type = ntk->getGateType(id); assert (V3_XD > type); assert (V3_MODULE != type);
      if (V3_FF >= type || AIG_FALSE == type || BV_CONST == type) {
         Msg(MSG_DBG) << "NEQ Found : ";
         Msg(MSG_DBG) << "id = " << (id.cp ? "~" : "") << id.id << ", type = " << V3GateTypeStr[type] << endl;
         Msg(MSG_DBG) << "sim = " << simValue << ", svr = " << svrValue << endl;
      }
      else {
         const uint32_t inSize = (type == AIG_NODE || isV3PairType(type)) ? 2 : (BV_MUX == type) ? 3 : 1;
         for (uint32_t i = 0; i < inSize; ++i) debugValues(ntk->getInputNetId(id, i), depth);
         Msg(MSG_DBG) << "NEQ Found : ";
         Msg(MSG_DBG) << "id = " << (id.cp ? "~" : "") << id.id << ", type = " << V3GateTypeStr[type] << ", inputs =";
         for (uint32_t i = 0; i < inSize; ++i)
            Msg(MSG_DBG) << " " << (ntk->getInputNetId(id, i).cp ? "~" : "") << ntk->getInputNetId(id, i).id;
         Msg(MSG_DBG) << endl; Msg(MSG_DBG) << "sim = " << simValue << ", svr = " << svrValue << endl;
      }
   }
   else {
      Msg(MSG_DBG) << "id = " << (id.cp ? "~" : "") << id.id << ", value = " << simValue << endl;
   }
}

const bool
V3FSM::check(V3SvrBase* const solver) const {
   assert (solver); assert (isCompleteFSM());
   // Push Ntk into Solver
   V3UI32Set stateNetSet; stateNetSet.clear(); collectStateNets(stateNetSet);
   for (uint32_t j = 0; j < 2; ++j)
      for (V3UI32Set::const_iterator it = stateNetSet.begin(); it != stateNetSet.end(); ++it)
         if (!solver->existVerifyData(V3NetId::makeNetId(*it), j))
            solver->addBoundedVerifyData(V3NetId::makeNetId(*it), j);
   for (uint32_t j = 0; j < 2; ++j) 
      for (uint32_t i = 0; i < _terminal.size(); ++i)
         if (!solver->existVerifyData(_terminal[i], j)) solver->addBoundedVerifyData(_terminal[i], j);
   // Confirm Forward Transitions
   for (uint32_t i = 0; i < _reachSucc.size(); ++i) {
      solver->assumeRelease(); pushStateIntoSolver(solver, i, 0);
      if (!solver->assump_solve()) {
         if (V3NtkUD != _distInit[i]) return false;
         for (uint32_t j = 0; j < _reachSucc[i].size(); ++j) { if (!_reachSucc[i][j]) continue; return false; }
         continue;
      }
      // Confirm Initial State
      solver->assumeRelease(); pushStateIntoSolver(solver, i, 0); solver->assumeInit();
      const bool init = solver->assump_solve();
      if (_distInit[i] && init) return false;
      if (!_distInit[i] && !init) return false;
      // Confirm state[i] can reach state[j] in one transition
      if (V3NtkUD != _distInit[i]) {
         for (uint32_t j = 0; j < _reachSucc[i].size(); ++j) {
            assert (!_reachSucc[i][j] || !_reachFail[i][j]);
            solver->assumeRelease(); pushStateIntoSolver(solver, i, 0); pushStateIntoSolver(solver, j, 1);
            const bool reach = solver->assump_solve();
            if (!_reachSucc[i][j] && reach) return false;
            if (_reachSucc[i][j] && !reach) return false;
            if (reach && V3NtkUD != _distInit[i] && _distInit[j] > (1 + _distInit[i])) return false;
         }
      }
   }
   // Confirm Backward Transitions
   for (uint32_t i = 0; i < _reachSucc.size(); ++i) {
      solver->assumeRelease(); pushStateIntoSolver(solver, i, 0);
      if (!solver->assump_solve()) {
         if (V3NtkUD != _distTerm[i]) return false;
         for (uint32_t j = 0; j < _reachSucc.size(); ++j) {
            if (!_reachSucc[j].size() || !_reachSucc[j][i]) continue;
            return false;
         }
         continue;
      }
      // Confirm Terminal State
      bool term = false;
      for (uint32_t j = 0; j < _terminal.size(); ++j) {
         solver->assumeRelease(); pushStateIntoSolver(solver, i, 0);
         solver->assumeProperty(solver->getFormula(_terminal[j], 0));
         term = solver->assump_solve();
         if (term) { if (!_distTerm[i]) break; return false; }
      }
      if (!_distTerm[i] && !term) return false;
      // Confirm state[i] is reachable from state[j] in one transition
      if (V3NtkUD != _distTerm[i]) {
         for (uint32_t j = 0; j < _reachSucc.size(); ++j) {
            assert ((!_reachSucc[j].size() || !_reachSucc[j][i]) || (!_reachFail[j].size() || !_reachFail[j][i]));
            if (V3NtkUD == _distInit[j] && V3NtkUD == _distTerm[j]) continue;
            solver->assumeRelease(); pushStateIntoSolver(solver, j, 0); pushStateIntoSolver(solver, i, 1);
            const bool reach = solver->assump_solve();
            if ((!_reachSucc[j].size() || !_reachSucc[j][i]) && reach) return false;
            if ((_reachSucc[j].size() && _reachSucc[j][i]) && !reach) return false;
            if (reach && V3NtkUD != _distTerm[i] && _distTerm[j] > (1 + _distTerm[i])) return false;
         }
      }
   }
   return true;
}

// Output Functions
void
V3FSM::writeFSM(const string& fileName) const {
   assert (fileName.size());
   // Open FSM SDG Output File
   ofstream DOTFile; DOTFile.open(fileName.c_str());
   if (!DOTFile.is_open()) {
      Msg(MSG_ERR) << "FSM Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Header in DOT Output
   writeV3GeneralHeader("FSM Extraction Result", DOTFile, "//");
   DOTFile << "digraph G {" << endl;
   // Write All States
   DOTFile << V3_INDENT << "// States" << endl;
   for (uint32_t i = 0; i < _distInit.size(); ++i) {
      if (V3NtkUD == _distInit[i] && V3NtkUD == _distTerm[i]) continue;
      DOTFile << V3_INDENT << "\"" << i << "\" [label = \"" << getStateName(i) << "\"";
      // Mark Initial States and Terminal States
      if (!_distTerm[i]) DOTFile << ", style = filled, fillcolor = " << (_distInit[i] ? "crimson" : "lightpink");
      else if (!_distInit[i]) DOTFile << ", style = filled, fillcolor = lightgoldenrodyellow";
      // Mark Milestone States
      if (V3NtkUD != _distInit[i] && V3NtkUD != _distTerm[i]) DOTFile << ", shape = doublecircle, color = crimson";
      DOTFile << "];" << endl;
   }
   // Write Transitions
   DOTFile << V3_INDENT << "// State Transitions" << endl;
   for (uint32_t i = 0; i < _distInit.size(); ++i)
      for (uint32_t j = 0; j < _reachSucc[i].size(); ++j)
         if (_reachSucc[i][j]) DOTFile << V3_INDENT << "\"" << i << "\" -> \"" << j << "\";" << endl;

   // Close DOT Output
   DOTFile << "}" << endl; DOTFile.close(); dotToPng(fileName);
}

// Private FSM Information Functions
void
V3FSM::collectStateNets(V3FSMSDG* const sdg, V3UI32Set& stateNets) const {
   assert (sdg); stateNets.insert(sdg->getStateNet().id); V3FSMSDGSet::const_iterator it;
   for (it = sdg->getFalseSDG().begin(); it != sdg->getFalseSDG().end(); ++it) collectStateNets(*it, stateNets);
   for (it = sdg->getTrueSDG().begin(); it != sdg->getTrueSDG().end(); ++it) collectStateNets(*it, stateNets);
}

// Private State Investigation Functions
void
V3FSM::initializeReachTable(const uint32_t& state) {
   assert (state < _distInit.size());
   assert (_reachSucc[state].size() == _reachFail[state].size());
   if (_reachSucc[state].size()) return;
   _reachSucc[state] = V3BoolVec(_distInit.size(), false);
   _reachFail[state] = V3BoolVec(_distInit.size(), false);
}

const bool
V3FSM::simulateForForwardConstruction(const uint32_t& state) {
   uint32_t failCount = 0, curState = V3NtkUD;
   if (V3NtkUD == state) {
      while (log(_distInit.size() / (1 + getInitFwdReachableSize())) > failCount) {
         // Perform Random Simulation to Find New Initial States
         curState = randomGetInitialState(); assert (curState < _distInit.size());
         if (!_distInit[curState]) ++failCount;  // Penalize
         else {
            const bool newState = (V3NtkUD == _distInit[curState]);
            _distInit[curState] = 0;  // Set Initial State
            if (newState) {
               if (isStopOnAllInitialOrAllTerminal() && (getStateSize() == getInitStateSize())) {
                  setEarlySuspended(); return false; }
               initializeReachTable(curState); failCount = 0;  // Reward
               if (!simulateForForwardConstruction(curState)) return false;
            }
         }
      }
      // Perform Formal Solution to Find New Initial States
      if (!formalForForwardConstruction(state)) return false;
   }
   else {
      assert (state < _distInit.size()); assert (V3NtkUD != _distInit[state]);
      V3SimTrace inputVec; inputVec.clear(); recordSimulationValue(inputVec);
      while (log(_distInit.size() / (1 + getInitFwdReachableSize())) > failCount) {
         // Perform Random Simulation to Find New Reachability
         curState = randomSimulateOneCycle(); assert (curState < _distInit.size());
         const bool newState = (V3NtkUD == _distInit[curState]);
         if (newState) _distInit[curState] = _distInit.size();
         assert (!_reachFail[state][curState]);
         if (_reachSucc[state][curState]) ++failCount;  // Penalize
         else {
            _reachSucc[state][curState] = true;
            if (newState) {
               initializeReachTable(curState); failCount = 0;  // Reward
               if (!simulateForForwardConstruction(curState)) return false;
            }
         }
         // Set Current State Back
         setStateForSimulation(inputVec);
      }
      // Perform Formal Solution to Find New Reachable States
      if (!formalForForwardConstruction(state)) return false;
   }
   return true;
}

const bool
V3FSM::formalForForwardConstruction(const uint32_t& state) {
   const V3SvrData svrData = _solver->reserveFormula();
   uint32_t curState = V3NtkUD;
   if (V3NtkUD == state) {
      // Block Existing States
      for (uint32_t i = 0; i < _distInit.size(); ++i)
         if (!_distInit[i]) formalBlockCurrentState(i, svrData);
      while (true) {
         // Perform Formal Solution to Find New Initial States
         curState = formalGetInitialState(svrData); if (V3NtkUD == curState) break;
         assert (curState < _distInit.size()); assert (_distInit[curState]);
         if (!_distInit[curState]) continue; formalBlockCurrentState(curState, svrData);
         const bool newState = (V3NtkUD == _distInit[curState]); _distInit[curState] = 0;
         if (newState) {
            if (isStopOnAllInitialOrAllTerminal() && (getStateSize() == getInitStateSize())) {
               setEarlySuspended(); return false; }
            initializeReachTable(curState);
            if (!simulateForForwardConstruction(curState)) return false;
         }
      }
   }
   else {
      // Block Existing States
      assert (_distInit.size() == _reachSucc[state].size());
      for (uint32_t i = 0; i < _reachSucc[state].size(); ++i) {
         assert (_reachSucc[i].size() == _reachFail[i].size());
         if (_reachSucc[state][i] || _reachFail[state][i]) formalBlockNextState(i, svrData);
      }
      V3NetVec stateData; stateData.clear(); decodeStateToNetId(state, stateData);
      while (true) {
         // Perform Formal Solution to Find New Reachable States
         curState = formalGetNextState(stateData, svrData); if (V3NtkUD == curState) break;
         assert (curState < _distInit.size()); assert (!_reachFail[state][curState]);
         if (_reachSucc[state][curState]) continue; formalBlockNextState(curState, svrData);
         const bool newState = (V3NtkUD == _distInit[curState]); _reachSucc[state][curState] = true;
         if (newState) {
            _distInit[curState] = _distInit.size();
            initializeReachTable(curState);
            if (!simulateForForwardConstruction(curState)) return false;
         }
      }
      // Set Other States Unreachable
      for (uint32_t i = 0; i < _distInit.size(); ++i) {
         assert (!_reachSucc[state][i] || !_reachFail[state][i]);
         if (!_reachSucc[state][i]) _reachFail[state][i] = true;
      }
   }
   // Invalidate Solver Data
   _solver->assertProperty(_solver->getNegFormula(svrData));
   return true;
}

const bool
V3FSM::formalForBackwardConstruction(const uint32_t& state) {
   V3SvrData svrData = _solver->reserveFormula();
   V3UI32Vec curStateVec; curStateVec.clear();
   uint32_t curState = V3NtkUD;
   if (V3NtkUD == state) {
      // Block Existing States
      for (uint32_t i = 0; i < _distTerm.size(); ++i)
         if (!_distTerm[i]) formalBlockCurrentState(i, svrData);
      // Set Terminal States
      V3SvrDataVec termSvrData; termSvrData.clear(); termSvrData.reserve(1 + _terminal.size());
      for (uint32_t i = 0; i < _terminal.size(); ++i) termSvrData.push_back(_solver->getFormula(_terminal[i], 0));
      termSvrData.push_back(_solver->getNegFormula(svrData)); _solver->assertImplyUnion(termSvrData);
      while (true) {
         curStateVec = formalGetTerminalState(svrData); if (!curStateVec.size()) break;
         for (uint32_t i = 0; i < curStateVec.size(); ++i) {
            curState = curStateVec[i]; assert (curState < _distTerm.size());
            if (!_distTerm[curState]) continue; formalBlockCurrentState(curState, svrData);
            const bool newState = (V3NtkUD == _distTerm[curState]); _distTerm[curState] = 0;
            if (newState) {
               if (isStopOnAllInitialOrAllTerminal() && (getStateSize() == getTermStateSize())) {
                  setEarlySuspended(); return false; }
               initializeReachTable(curState);
               if (!formalForBackwardConstruction(curState)) return false;
            }
         }
      }
   }
   else {
      // Block Existing States
      for (uint32_t i = 0; i < _distTerm.size(); ++i) {
         if (!_reachSucc[i].size()) continue;
         assert (_reachSucc[i].size() == _reachFail[i].size());
         assert (_distInit.size() == _reachSucc[i].size());
         if (_reachSucc[i][state] || _reachFail[i][state]) {
            formalBlockCurrentState(i, svrData);
         }
         if (_reachSucc[i][state] && V3NtkUD == _distTerm[i]) {
            _distTerm[i] = _distTerm.size();
            if (!formalForBackwardConstruction(i)) return false;
         }
      }
      V3NetVec stateData; stateData.clear(); decodeStateToNetId(state, stateData);
      while (true) {
         curStateVec = formalGetCurrentState(stateData, svrData); if (!curStateVec.size()) break;
         for (uint32_t i = 0; i < curStateVec.size(); ++i) {
            curState = curStateVec[i]; assert (curState < _distTerm.size());
            assert (!_reachSucc[curState].size() || !_reachFail[curState][state]);
            if (_reachSucc[curState].size() && _reachSucc[curState][state]) continue;
            formalBlockCurrentState(curState, svrData);
            const bool newState = (V3NtkUD == _distTerm[curState]);
            if (newState) initializeReachTable(curState); _reachSucc[curState][state] = true;
            if (newState) {
               _distTerm[curState] = _distTerm.size();
               if (!formalForBackwardConstruction(curState)) return false;
            }
         }
      }
      // Set Other States Unreachable
      for (uint32_t i = 0; i < _distInit.size(); ++i)
         if (_reachSucc[i].size()) {
            assert (!_reachSucc[i][state] || !_reachFail[i][state]);
            if (!_reachSucc[i][state]) _reachFail[i][state] = true;
         }
   }
   // Invalidate Solver Data
   _solver->assertProperty(_solver->getNegFormula(svrData));
   return true;
}

// Private Simulation Functions
const uint32_t
V3FSM::randomGetInitialState() {
   // Reset Simulator  (Set DFF Next State as Initial State)
   _simulator->reset();
   // Simulate One Cycle  (Initial State Achieved)
   return randomSimulateOneCycle();
}

const uint32_t
V3FSM::randomSimulateOneCycle() {
   // Update Next Value
   _simulator->updateNextStateValue();
   // Set Random Input Values
   _simulator->setSourceFree(V3_PI, true);
   _simulator->setSourceFree(V3_PIO, true);
   // Simulate for One Cycle
   _simulator->simulate();
   // Find the State from Simulation Values
   return encodeStateFromSimulator();
}

void
V3FSM::recordSimulationValue(V3SimTrace& trace) const {
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk);
   trace.clear(); trace.reserve(ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) trace.push_back(_simulator->getSimValue(ntk->getInput(i)));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) trace.push_back(_simulator->getSimValue(ntk->getInout(i)));
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) trace.push_back(_simulator->getSimValue(ntk->getLatch(i)));
   assert (trace.size() == (ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize()));
}

void
V3FSM::setStateForSimulation(const V3SimTrace& trace) {
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk);
   assert (trace.size() == (ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize()));
   uint32_t i, j = 0;
   for (i = 0; i < ntk->getInputSize(); ++i, ++j) _simulator->setSource(ntk->getInput(i), trace[j]);
   for (i = 0; i < ntk->getInoutSize(); ++i, ++j) _simulator->setSource(ntk->getInout(i), trace[j]);
   for (i = 0; i < ntk->getLatchSize(); ++i, ++j) _simulator->setSource(ntk->getLatch(i), trace[j]);
   // Simulate for One Cycle
   assert (trace.size() == j); _simulator->simulate();
}

const uint32_t
V3FSM::encodeStateFromSimulator() const {
   const V3FSMSDGList& sdgList = _sdgDB->getFSMSDGList();
   uint32_t state = 0;
   for (uint32_t i = 0; i < sdgList.size(); ++i) {
      if (i) state *= sdgList[i]->getStateSize();
      state += encodeStateFromSimulator(sdgList[i]);
   }
   assert (_distInit.size() > state); return state;
}

const uint32_t
V3FSM::encodeStateFromSimulator(V3FSMSDG* const sdg) const {
   uint32_t state = 0; assert (sdg);
   // Check Simulation Values Against State Nets
   assert (_simulator->getSimValue(sdg->getStateNet()).size());
   assert ('X' != _simulator->getSimValue(sdg->getStateNet())[0]);
   if (_simulator->getSimValue(sdg->getStateNet())[0] == '0') {  // False
      const V3FSMSDGSet& falseState = sdg->getFalseSDG();
      assert (sdg->getFalseStateSize());
      if (falseState.size()) {
         uint32_t faninIndex = 0;
         for (V3FSMSDGSet::const_iterator it = falseState.begin(); it != falseState.end(); ++it) {
            if (falseState.begin() != it) faninIndex *= (*it)->getStateSize();
            faninIndex += encodeStateFromSimulator(*it);
         }
         state += faninIndex;
      }
   }
   else {  // True
      state += sdg->getFalseStateSize();
      const V3FSMSDGSet& trueState = sdg->getTrueSDG();
      assert (sdg->getTrueStateSize());
      if (trueState.size()) {
         uint32_t faninIndex = 0;
         for (V3FSMSDGSet::const_iterator it = trueState.begin(); it != trueState.end(); ++it) {
            if (trueState.begin() != it) faninIndex *= (*it)->getStateSize();
            faninIndex += encodeStateFromSimulator(*it);
         }
         state += faninIndex;
      }
   }
   assert (sdg->getStateSize() > state); return state;
}

const V3UI32Vec
V3FSM::encodeAllStateFromSimulator() const {
   // Find all States Possible from Simulation Values
   const V3FSMSDGList& sdgList = _sdgDB->getFSMSDGList();
   V3UI32Vec state, faninState; state.reserve(_distInit.size());
   for (uint32_t i = 0; i < sdgList.size(); ++i) {
      if (i) {
         for (uint32_t j = 0, k = sdgList[i]->getStateSize(); j < state.size(); ++j) state[j] *= k;
         faninState = encodeAllStateFromSimulator(sdgList[i]); assert (faninState.size());
         for (uint32_t j = 0; j < state.size(); ++j) state[j] += faninState[0];
         for (uint32_t j = 1, x = state.size(); j < faninState.size(); ++j)
            for (uint32_t k = 0; k < x; ++k) state.push_back(faninState[j] + state[k]);
      }
      else { assert (!state.size()); state = encodeAllStateFromSimulator(sdgList[i]); assert (state.size()); }
   }
   return state;
}

const V3UI32Vec
V3FSM::encodeAllStateFromSimulator(V3FSMSDG* const sdg) const {
   V3UI32Vec fState, tState; fState.clear(); tState.clear();
   // Check Simulation Values Against State Nets
   assert (sdg); assert (_simulator->getSimValue(sdg->getStateNet()).size());
   if (_simulator->getSimValue(sdg->getStateNet())[0] != '1') {  // False
      const V3FSMSDGSet& falseState = sdg->getFalseSDG();
      if (falseState.size()) {
         V3UI32Vec faninState;
         for (V3FSMSDGSet::const_iterator it = falseState.begin(); it != falseState.end(); ++it) {
            if (falseState.begin() != it) {
               for (uint32_t i = 0, j = (*it)->getStateSize(); i < fState.size(); ++i) fState[i] *= j;
               faninState = encodeAllStateFromSimulator(*it); assert (faninState.size());
               for (uint32_t i = 0; i < fState.size(); ++i) fState[i] += faninState[0];
               for (uint32_t i = 1, k = fState.size(); i < faninState.size(); ++i)
                  for (uint32_t j = 0; j < k; ++j) fState.push_back(faninState[i] + fState[j]);
            }
            else { assert (!fState.size()); fState = encodeAllStateFromSimulator(*it); assert (fState.size()); }
         }
      }
      else if (sdg->getFalseStateSize()) fState.push_back(0);
   }
   if (_simulator->getSimValue(sdg->getStateNet())[0] != '0') {  // True
      const V3FSMSDGSet& trueState = sdg->getTrueSDG();
      if (trueState.size()) {
         V3UI32Vec faninState;
         for (V3FSMSDGSet::const_iterator it = trueState.begin(); it != trueState.end(); ++it) {
            if (trueState.begin() != it) {
               for (uint32_t i = 0, j = (*it)->getStateSize(); i < tState.size(); ++i) tState[i] *= j;
               faninState = encodeAllStateFromSimulator(*it); assert (faninState.size());
               for (uint32_t i = 0; i < tState.size(); ++i) tState[i] += faninState[0];
               for (uint32_t i = 1, k = tState.size(); i < faninState.size(); ++i)
                  for (uint32_t j = 0; j < k; ++j) tState.push_back(faninState[i] + tState[j]);
            }
            else { assert (!tState.size()); tState = encodeAllStateFromSimulator(*it); assert (tState.size()); }
         }
         for (uint32_t i = 0; i < tState.size(); ++i) tState[i] += sdg->getFalseStateSize();
      }
      else if (sdg->getTrueStateSize()) tState.push_back(sdg->getFalseStateSize());
   }
   for (uint32_t i = 0; i < tState.size(); ++i) fState.push_back(tState[i]);
   return fState;
}

// Private SAT-solving Functions
const uint32_t
V3FSM::formalGetInitialState(const V3SvrData& svrData) {
   _solver->assumeRelease(); _solver->assumeProperty(svrData); _solver->assumeInit();
   if (!_solver->assump_solve()) return V3NtkUD;
   // Record Solver Values for Simulator
   V3SimTrace trace; trace.clear();
   recordSolverValue(trace, 0);
   setStateForSimulation(trace);
   // Find the State from Simulation Values
   return encodeStateFromSimulator();
}

const uint32_t
V3FSM::formalGetNextState(const V3NetVec& stateData, const V3SvrData& svrData) {
   assert (stateData.size()); _solver->assumeRelease(); _solver->assumeProperty(svrData);
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      assert (_solver->existVerifyData(stateData[i], 0));
      _solver->assumeProperty(stateData[i], false, 0);
   }
   if (!_solver->assump_solve()) return V3NtkUD;
   // Record Solver Values for Simulator
   V3SimTrace trace; trace.clear();
   recordSolverValue(trace, 1);
   setStateForSimulation(trace);
   // Find the State from Simulation Values
   return encodeStateFromSimulator();
}

const V3UI32Vec
V3FSM::formalGetTerminalState(const V3SvrData& svrData) {
   _solver->assumeRelease(); _solver->assumeProperty(svrData);
   if (!_solver->assump_solve()) return V3UI32Vec();
   // Record Solver Values for Simulator
   V3SimTrace trace; trace.clear();
   recordSolverValue(trace, 0);
   setStateForSimulation(trace);
   return V3UI32Vec(1, encodeStateFromSimulator());
}

const V3UI32Vec
V3FSM::formalGetCurrentState(const V3NetVec& stateData, const V3SvrData& svrData) {
   assert (stateData.size()); _solver->assumeRelease(); _solver->assumeProperty(svrData);
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      assert (_solver->existVerifyData(stateData[i], 1));
      _solver->assumeProperty(stateData[i], false, 1);
   }
   if (!_solver->assump_solve()) return V3UI32Vec();
   // Record Solver Values for Simulator
   V3SimTrace trace; trace.clear();
   recordSolverValue(trace, 0);
   setStateForSimulation(trace);
   return V3UI32Vec(1, encodeStateFromSimulator());
}

void
V3FSM::formalBlockNextState(const uint32_t& state, const V3SvrData& svrData) {
   V3NetVec stateData; stateData.clear(); decodeStateToNetId(state, stateData);
   V3SvrDataVec blockList; blockList.clear(); blockList.reserve(1 + stateData.size());
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      assert (_solver->existVerifyData(stateData[i], 1));
      blockList.push_back(_solver->getFormula(~stateData[i], 1));
   }
   blockList.push_back(_solver->getNegFormula(svrData)); _solver->assertImplyUnion(blockList);
}

void
V3FSM::formalBlockCurrentState(const uint32_t& state, const V3SvrData& svrData) {
   V3NetVec stateData; stateData.clear(); decodeStateToNetId(state, stateData);
   V3SvrDataVec blockList; blockList.clear(); blockList.reserve(1 + stateData.size());
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      assert (_solver->existVerifyData(stateData[i], 0));
      blockList.push_back(_solver->getFormula(~stateData[i], 0));
   }
   blockList.push_back(_solver->getNegFormula(svrData)); _solver->assertImplyUnion(blockList);
}

void
V3FSM::recordSolverValue(V3SimTrace& trace, const uint32_t& depth) const {
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk); assert (!(depth & ~1ul));
   trace.clear(); trace.reserve(ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
      if (_solver->existVerifyData(ntk->getInput(i), depth)) {
         trace.push_back(_solver->getDataValue(ntk->getInput(i), depth)); trace.back().restrict0(); }
      else { V3BitVec value(ntk->getNetWidth(ntk->getInput(i))); value.random(); trace.push_back(value); }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i)
      if (_solver->existVerifyData(ntk->getInout(i), depth)) {
         trace.push_back(_solver->getDataValue(ntk->getInout(i), depth)); trace.back().restrict0(); }
      else { V3BitVec value(ntk->getNetWidth(ntk->getInout(i))); value.random(); trace.push_back(value); }
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i)
      if (_solver->existVerifyData(ntk->getLatch(i), depth)) {
         trace.push_back(_solver->getDataValue(ntk->getLatch(i), depth)); trace.back().restrict0(); }
      else { V3BitVec value(ntk->getNetWidth(ntk->getLatch(i))); value.random(); trace.push_back(value); }
   assert (trace.size() == (ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize()));
}

void
V3FSM::recordSolverXValue(V3SimTrace& trace, const uint32_t& depth) const {
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk); assert (!(depth & ~1ul));
   trace.clear(); trace.reserve(ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize());
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
      if (_solver->existVerifyData(ntk->getInput(i), depth)) 
         trace.push_back(_solver->getDataValue(ntk->getInput(i), depth));
      else trace.push_back(V3BitVecX(ntk->getNetWidth(ntk->getInput(i))));
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i)
      if (_solver->existVerifyData(ntk->getInout(i), depth)) 
         trace.push_back(_solver->getDataValue(ntk->getInout(i), depth));
      else trace.push_back(V3BitVecX(ntk->getNetWidth(ntk->getInout(i))));
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i)
      if (_solver->existVerifyData(ntk->getLatch(i), depth)) 
         trace.push_back(_solver->getDataValue(ntk->getLatch(i), depth));
      else trace.push_back(V3BitVecX(ntk->getNetWidth(ntk->getLatch(i))));
   assert (trace.size() == (ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize()));
}

void
V3FSM::decodeStateToNetId(const uint32_t& state, V3NetVec& netIdVec) const {
   assert (state < _sdgDB->getStateSize()); netIdVec.clear();
   // Push State Variables into netIdVec
   const V3FSMSDGList& sdgList = _sdgDB->getFSMSDGList();
   uint32_t i = sdgList.size(), j = 0, curState = state;
   while (i--) {
      assert (sdgList[i]->getStateSize()); j = curState % sdgList[i]->getStateSize();
      decodeStateToNetId(sdgList[i], j, netIdVec); curState -= j;
      assert (!(curState % sdgList[i]->getStateSize())); curState /= sdgList[i]->getStateSize();
   }
}

void
V3FSM::decodeStateToNetId(const V3FSMSDG* const sdg, const uint32_t& state, V3NetVec& netIdVec) const {
   assert (sdg); assert (state < sdg->getStateSize());
   if (state < sdg->getFalseStateSize()) {  // False
      netIdVec.push_back(~(sdg->getStateNet()));  // Push State Variable into netIdVec
      const V3FSMSDGSet& falseState = sdg->getFalseSDG();
      if (falseState.size()) {
         uint32_t i = 0, curState = state;
         for (V3FSMSDGSet::const_reverse_iterator it = falseState.rbegin(); it != falseState.rend(); ++it) {
            i = curState % (*it)->getStateSize(); decodeStateToNetId(*it, i, netIdVec);
            curState -= i; assert (!(curState % (*it)->getStateSize())); curState /= (*it)->getStateSize();
         }
      }
      else { assert (sdg->getFalseStateSize()); assert (!state); }
   }
   else {  // True
      netIdVec.push_back(sdg->getStateNet());  // Push State Variable into netIdVec
      const V3FSMSDGSet& trueState = sdg->getTrueSDG();
      if (trueState.size()) {
         uint32_t i = 0, curState = state - sdg->getFalseStateSize();
         for (V3FSMSDGSet::const_reverse_iterator it = trueState.rbegin(); it != trueState.rend(); ++it) {
            i = curState % (*it)->getStateSize(); decodeStateToNetId(*it, i, netIdVec);
            curState -= i; assert (!(curState % (*it)->getStateSize())); curState /= (*it)->getStateSize();
         }
      }
      else { assert (sdg->getTrueStateSize()); assert (state == sdg->getFalseStateSize()); }
   }
}

// Private Generalization Functions
void
V3FSM::generalizeStateFromSimulation(const V3NetVec& targetNets, const uint32_t& depth) {
   assert (targetNets.size()); assert (!(depth & ~1ul));
   V3Ntk* const ntk = _sdgDB->getNtkHandler()->getNtk(); assert (ntk); ntk->clearCutSignals();
   // Set Cut Signals
   V3NetVec cutSignals; cutSignals.reserve(ntk->getLatchSize() + targetNets.size());
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) cutSignals.push_back(ntk->getLatch(i));
   for (uint32_t i = 0; i < targetNets.size(); ++i) 
      cutSignals.push_back(targetNets[i].cp ? ~targetNets[i] : targetNets[i]);
   ntk->setCutSignals(cutSignals);
   // Set Generalization Targets
   cutSignals.clear(); cutSignals.reserve(targetNets.size());
   for (uint32_t i = 0; i < targetNets.size(); ++i) 
      cutSignals.push_back(V3NetId::makeNetId(_cutSize + i, targetNets[i].cp));
}

// Private State Traversal Functions
void
V3FSM::bfsUpdateForwardReachability() {
   V3UI32Vec frontier; frontier.clear(); frontier.reserve(_distInit.size());
   for (uint32_t i = 0; i < _distInit.size(); ++i) if (!_distInit[i]) frontier.push_back(i);
   for (uint32_t i = 0; i < frontier.size(); ++i) {
      assert (frontier[i] < _distInit.size()); assert (_reachSucc[frontier[i]].size());
      for (uint32_t j = 0, k = 1 + _distInit[frontier[i]]; j < _reachSucc[frontier[i]].size(); ++j)
         if (_reachSucc[frontier[i]][j] && (k < _distInit[j])) {
            _distInit[j] = k; frontier.push_back(j);
         }
   }
}

void
V3FSM::bfsUpdateBackwardReachability() {
   V3UI32Vec frontier; frontier.clear(); frontier.reserve(_distTerm.size());
   for (uint32_t i = 0; i < _distTerm.size(); ++i) if (!_distTerm[i]) frontier.push_back(i);
   for (uint32_t i = 0; i < frontier.size(); ++i) {
      assert (frontier[i] < _distTerm.size());
      for (uint32_t j = 0, k = 1 + _distTerm[frontier[i]]; j < _distTerm.size(); ++j)
         if (_reachSucc[j].size() && _reachSucc[j][frontier[i]] && (k < _distTerm[j])) {
            _distTerm[j] = k; frontier.push_back(j);
         }
   }
}

// Private Output Functions
const string
V3FSM::getStateName(const uint32_t& i) const {
   V3NetVec stateData; stateData.clear();
   getStateNetId(i, stateData);
   string stateName = "";
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      if (i) stateName += "\\n";
      //stateName += _sdgDB->getNtkHandler()->getNetNameOrFormedWithId(stateData[i]);
      //stateName += (stateData[i].cp ? "~" : "") + v3Int2Str(stateData[i].id);
      stateName += _sdgDB->getNtkHandler()->getNetExpression(stateData[i]);
   }
   return stateName;
}

// Private Debug Functions
void
V3FSM::pushStateIntoSolver(V3SvrBase* const solver, const uint32_t& state, const uint32_t& depth) const {
   assert (solver); assert (_reachSucc.size() > state); assert (!(depth & ~1ul));
   V3NetVec stateData; stateData.clear(); decodeStateToNetId(state, stateData);
   for (uint32_t i = 0; i < stateData.size(); ++i) {
      assert (solver->existVerifyData(stateData[i], depth));
      solver->assumeProperty(stateData[i], false, depth);
   }
}

#endif

