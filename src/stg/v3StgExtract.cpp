/****************************************************************************
  FileName     [ v3StgExtract.cpp ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ STG Extraction Interface for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_EXTRACT_C
#define V3_STG_EXTRACT_C

#include "v3IntType.h"
#include "v3StrUtil.h"
#include "v3VrfBase.h"
#include "v3FileUtil.h"
#include "v3NtkWriter.h"
#include "v3StgExtract.h"
#include "v3NtkElaborate.h"

/* -------------------------------------------------- *\
 * Class V3FSMExtract Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3FSMExtract::V3FSMExtract(V3NtkHandler* const handler, const V3NetVec& targets) : _handler(handler),
   _sdgMain((dynamic_cast<V3BvNtk*>(handler->getNtk())) ? new V3SDG(handler) : (V3SDG*)(new V3AigSDG(handler))) {
   assert (_handler); assert (_sdgMain); _terminals.clear();
   _configs = 0; _maxSize = 300; _maxTime = V3DblMAX; _curTime = 0;
   _clusters.clear(); _sdgDBList.clear(); _fsmList.clear();
   _depRetain = V3BoolVec(_handler->getNtk()->getNetSize(), false);
   for (uint32_t i = 0; i < targets.size(); ++i) _sdgMain->constructSDG(targets[i]);
}

V3FSMExtract::V3FSMExtract(V3NtkHandler* const handler, const string& fileName, const V3NetVec& targets) 
   : _handler(new V3NtkElaborate(handler, targets)), _sdgMain(0) {
   assert (_handler); assert (fileName.size()); _terminals.clear();
   for (uint32_t i = 0; i < targets.size(); ++i)
      _handler->getNtk()->createOutput(_handler->getCurrentNetId(targets[i]));
   _clusters.clear(); _sdgDBList.clear(); _fsmList.clear(); _depRetain.clear();
   _configs = 0; _maxSize = 300; _maxTime = V3DblMAX; _curTime = 0;
   constructInputFSMSDG(handler, fileName); _terminals.reserve(targets.size());
   for (uint32_t i = 0; i < targets.size(); ++i) _terminals.push_back(_handler->getCurrentNetId(targets[i]));
}

V3FSMExtract::V3FSMExtract(V3NtkHandler* const handler, const V3NetVec& outputs, const V3NetVec& targets) 
   : _handler(new V3NtkElaborate(handler, outputs)), _sdgMain(0) {
   assert (_handler); _terminals.clear(); _depRetain.clear();
   for (uint32_t i = 0; i < targets.size(); ++i)
      _handler->getNtk()->createOutput(_handler->getCurrentNetId(targets[i]));
   _clusters.clear(); _sdgDBList.clear(); _fsmList.clear();
   _configs = 0; _maxSize = 300; _maxTime = V3DblMAX; _curTime = 0;
   constructStandardFSMSDG(); _terminals.reserve(targets.size());
   for (uint32_t i = 0; i < targets.size(); ++i) _terminals.push_back(_handler->getCurrentNetId(targets[i]));
}

V3FSMExtract::~V3FSMExtract() {
   if (_sdgMain) delete _sdgMain; _terminals.clear(); _depRetain.clear();
   for (uint32_t i = 0; i < _clusters.size(); ++i) _clusters[i].clear(); _clusters.clear();
   for (uint32_t i = 0; i < _fsmList.size(); ++i) delete _fsmList[i]; _fsmList.clear();
   for (uint32_t i = 0; i < _sdgDBList.size(); ++i) if (_sdgDBList[i]) delete _sdgDBList[i]; _sdgDBList.clear();
}

// Main Functions
void
V3FSMExtract::startExtractFSMs() {
   if (_fsmList.size()) return;
   if (isEnableConstructStandardFSM()) {
      assert (!isVariableClusterBySCC());
      enableOmitNodesIndepToVars();
   }
   if (isInvariantDirectedCluster()) startExtractInvariantFSMs();
   else {
      if (_sdgMain) startClusterVariables();
      if (_sdgDBList.size()) startExtractClusteredFSMs();
   }
}

// Debug Functions
const bool
V3FSMExtract::checkCompleteFSMs() const {
   bool isComplete = true;
   for (uint32_t i = 0; i < _fsmList.size(); ++i) {
      if (!_fsmList[i] || !_fsmList[i]->isCompleteFSM()) continue;
      V3SvrBase* const solver = allocSolver(V3VrfBase::getDefaultSolver(), _handler->getNtk());
      if (!_fsmList[i]->check(solver)) {
         if (V3VrfBase::intactON()) Msg(MSG_DBG) << "FSM " << i << " confirm Failed !!" << endl;
         if (isComplete) isComplete = false;
      }
   }
   return isComplete;
}

// Output Functions
void
V3FSMExtract::writeClusterResult(const string& fileName) const {
   assert (fileName.size());
   // Open FSM SDG Output File
   ofstream DOTFile; DOTFile.open(fileName.c_str());
   if (!DOTFile.is_open()) {
      Msg(MSG_ERR) << "Cluster Result Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Header in DOT Output
   writeV3GeneralHeader("FSM Cluster Result", DOTFile, "//"); if (!_sdgMain) { DOTFile.close(); return; }
   DOTFile << "digraph G {" << endl;
   DOTFile << V3_INDENT << "compound = true; splines = false; clusterrank = local;" << endl;
   DOTFile << V3_INDENT << "node [shape = box]; edge [headport = n, tailport = s];" << endl;
   DOTFile << V3_INDENT << "graph [center rankdir = TB, ranksep = \"8 equally\"];" << endl;
   // Write All Latches
   V3BoolVec markList(_handler->getNtk()->getNetSize(), false);
   const string colorName[] = { "cadetblue1", "palegreen", "rosybrown1", "ivory3", "antiquewhite" };
   const string curColorName = colorName[rand() % 5];
   for (uint32_t i = 0; i < _clusters.size(); ++i) {
      DOTFile << V3_INDENT << "subgraph clusterC" << i << " {" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "style = filled; color = " << curColorName << ";" << endl;
      for (V3UI32Set::const_iterator it = _clusters[i].begin(); it != _clusters[i].end(); ++it) {
         assert (markList.size() > *it); assert (!markList[*it]); markList[*it] = true;
         DOTFile << V3_INDENT << V3_INDENT << "subgraph cluster" << *it << " {" << endl;
         DOTFile << V3_INDENT << V3_INDENT << V3_INDENT << "style = invis;" << endl;
         DOTFile << V3_INDENT << V3_INDENT << V3_INDENT << "N" << *it << " [style = dashed, label = \""
                 << _handler->getNetNameOrFormedWithId(V3NetId::makeNetId(*it)) << "\"];" << endl;
         DOTFile << V3_INDENT << V3_INDENT << V3_INDENT << "C" << *it << " [style = solid, label = \""
                 << _handler->getNetNameOrFormedWithId(V3NetId::makeNetId(*it)) << "\"];" << endl;
         DOTFile << V3_INDENT << V3_INDENT << V3_INDENT << "N" << *it << " -> " << "C"
                 << *it << " [style = invis];" << endl;
         DOTFile << V3_INDENT << V3_INDENT << V3_INDENT << "C" << *it << " -> " << "N"
                 << *it << " [style = invis];" << endl;
         DOTFile << V3_INDENT << V3_INDENT << "}" << endl;
      }
      DOTFile << V3_INDENT << "}" << endl;
   }
   DOTFile << V3_INDENT << "// All Latches with Existing SDG" << endl;
   for (uint32_t i = 0; i < _handler->getNtk()->getLatchSize(); ++i) {
      const V3NetId id = _handler->getNtk()->getLatch(i);
      if (!_sdgMain->getSDGNode(id.id) || markList[id.id]) continue;
      DOTFile << V3_INDENT << "subgraph cluster" << id.id << " {" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "style = invis;" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "N" << id.id << " [style = dashed, label = \""
              << _handler->getNetNameOrFormedWithId(id) << "\"];" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "C" << id.id << " [style = solid, label = \""
              << _handler->getNetNameOrFormedWithId(id) << "\"];" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "N" << id.id << " -> "
              << "C" << id.id << " [style = invis];" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "C" << id.id << " -> "
              << "N" << id.id << " [style = invis];" << endl;
      DOTFile << V3_INDENT << "}" << endl;
   }
   // Write Latch Dependencies
   V3BitVec allMarker(markList.size()), muxMarker(markList.size());
   for (uint32_t i = 0; i < _handler->getNtk()->getLatchSize(); ++i) {
      const V3NetId id = _handler->getNtk()->getLatch(i);
      if (!_sdgMain->getSDGNode(id.id)) continue;
      V3SDGBase* const node = new V3SDGBase(*(_sdgMain->getSDGNode(id.id)->getBase()));
      // Collect FF in the Fanin Cone
      assert (node); _sdgMain->collectFaninFF(node); allMarker.clear(); node->markDepFF(allMarker);
      // Collect FF only in the Fanin Cone of Dependent MUXes' Select Node
      node->clearDepFF(); _sdgMain->collectFaninMuxFF(node); muxMarker.clear(); node->markDepFF(muxMarker);
      DOTFile << V3_INDENT << "// Dependent Latches of " << id.id << endl;
      for (uint32_t j = 0; j < allMarker.size(); ++j) {
         if (!allMarker[j]) { assert (!muxMarker[j]); continue; }
         assert (V3_FF == _handler->getNtk()->getGateType(V3NetId::makeNetId(j)));
         DOTFile << V3_INDENT << "N" << id.id << " -> C" << j << " [arrowhead = none";
         DOTFile << ", style = " << (_depRetain[j] ? "bold" : "dashed");
         if (muxMarker[j]) DOTFile << ", color = red"; DOTFile << "];" << endl;
      }
   }
   // Close DOT Output
   DOTFile << "}" << endl; DOTFile.close(); dotToPng(fileName);
}

void
V3FSMExtract::writeFSM(const string& fileName, V3FSM* const fsm) const {
   assert (fileName.size()); assert (fsm);
   fsm->writeFSM(fileName);
}

// Private Member Functions
const bool lexFSMSpec(ifstream& input, uint32_t& lineNumber, V3StrVec& tokens) {
   assert (input.is_open()); tokens.clear();
   string buffer = ""; size_t i, j;
   while (!input.eof()) {
      getline(input, buffer); ++lineNumber; if (input.eof()) break;
      for (i = j = 0; i < buffer.size(); ++i) {
         if (buffer[i] == ' ' || buffer[i] == '\t') {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1;
         }
         else if ((buffer[i] == ':') && ((i + 1) < buffer.size()) && (buffer[1 + i] == '=')) {
            // Separator for Predicate Name
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            tokens.push_back(":="); j = i + 2;
         }
         else if (buffer[i] == '\n' || ((buffer[i] == '/') && ((1 + i) < buffer.size()) && (buffer[1 + i] == '/'))) {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; break;
         }
      }
      if (i > j) tokens.push_back(buffer.substr(j));
      if (tokens.size()) return true;
   }
   return false;
}

void
V3FSMExtract::constructInputFSMSDG(V3NtkHandler* const handler, const string& fileName) {
   assert (handler); assert (fileName.size()); assert (!_sdgMain);
   assert (dynamic_cast<V3NtkElaborate*>(_handler)); assert (!_sdgDBList.size());
   // Open Input FSM Specification
   ifstream input; input.open(fileName.c_str());
   if (!input.is_open()) { Msg(MSG_ERR) << "FSM Specification File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Parse Header: Expecting <fsm> <#state_var> <#fsm_sdg> <#fsm>
   uint32_t lineNumber = 0, varSize = 0, sdgSize = 0, fsmSize = 0; V3StrVec tokens;
   if (!lexFSMSpec(input, lineNumber, tokens) || tokens.size() != 4 || "fsm" != tokens[0]) {
      Msg(MSG_ERR) << (lineNumber ? ("Line " + v3Int2Str((int)lineNumber) + " : ") : "") << "Expecting Header "
                   << "\"<fsm> <#state_var> <#fsm_sdg> <#fsm>\" in the Input File !!" << endl; input.close(); return; }
   if (!v3Str2UInt(tokens[1], varSize) || varSize == 0) {
      Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected <#state_var> = " << tokens[1] << " !!" << endl;
      input.close(); return; }
   if (!v3Str2UInt(tokens[2], sdgSize) || sdgSize == 0) {
      Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected <#fsm_sdg> = " << tokens[2] << " !!" << endl;
      input.close(); return; }
   if (!v3Str2UInt(tokens[3], fsmSize) || fsmSize == 0) {
      Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected <#fsm> = " << tokens[3] << " !!" << endl;
      input.close(); return; }
   // Parse State Variable Expressions
   string varName = "", varExpr = "";
   for (uint32_t i = 0; i < varSize; ++i) {
      // Get Variable Name and Expression
      if (!lexFSMSpec(input, lineNumber, tokens)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Expecting Expression of State Variable "
                      << (1 + i) << "/" << varSize << " !!" << endl; input.close(); return; }
      varName = (tokens.size() > 1 && ":=" == tokens[1]) ? tokens[0] : ""; varExpr = "";
      if (handler->existNetName(varName)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Variable Name \"" << varName << "\" Already Exists !!" << endl;
         input.close(); return; }
      // Elaborate the Variable
      for (uint32_t j = varName.size() ? 2 : 0; j < tokens.size(); ++j) varExpr += (tokens[j] + " ");
      V3LTLFormula* const formula = new V3LTLFormula(handler, varExpr, varName); assert (formula);
      if (!formula->isValid() || !formula->isLeaf(formula->getRoot())) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected Expression Found !!" << endl;
         delete formula; input.close(); return; }
      const uint32_t index = dynamic_cast<V3NtkElaborate*>(_handler)->elaborateLTLFormula(formula);
      if (V3NtkUD == index) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Failed to Elaborate the Expression !!" << endl;
         delete formula; input.close(); return; }
      delete formula; assert (_handler->getNtk());
      assert (_handler->getNtk()->getOutputSize() == (1 + index));
      if (1 != _handler->getNtk()->getNetWidth(_handler->getNtk()->getOutput(index))) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : The Expression is NOT a Boolean Predicate !!" << endl;
         input.close(); return; }
      if (varName.size()) _handler->resetOutName(index, varName);
   }
   // Parse FSMSDG (State-variable Dependency Graph)
   V3StrTable sdgSpec; sdgSpec.clear(); sdgSpec.reserve(sdgSize);
   for (uint32_t i = 0; i < sdgSize; ++i) {
      // Get FSMSDG Representation
      if (!lexFSMSpec(input, lineNumber, tokens)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Expecting Expression of State-variable Dependency Graph "
                      << (1 + i) << "/" << sdgSize << " !!" << endl; input.close(); return; }
      tokens.push_back(v3Int2Str(lineNumber)); sdgSpec.push_back(tokens);
   }
   // Parse FSM Specification and Create FSMSDG
   V3StrTable sdgList; sdgList.clear();
   for (uint32_t i = 0; i < fsmSize; ++i) {
      // Get FSM Components
      if (!lexFSMSpec(input, lineNumber, tokens)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Expecting Components of FSM "
                      << (1 + i) << "/" << fsmSize << " !!" << endl; input.close(); return; }
      // Create FSMSDGDB
      uint32_t sdgIndex; sdgList.clear(); sdgList.reserve(tokens.size());
      for (uint32_t j = 0; j < tokens.size(); ++j) {
         if (!v3Str2UInt(tokens[j], sdgIndex) || sdgSize <= sdgIndex) {
            Msg(MSG_ERR) << "Line " << lineNumber << " : SDG with Index = \"" << tokens[j]
               << "\" Not Found !!" << endl; return; }
         sdgList.push_back(sdgSpec[sdgIndex]);
      }
      // NOTE: Configurations to the Construction of FSMSDG are Disabled
      _sdgDBList.push_back(new V3FSMSDGDB(_handler, sdgList));
      if (!_sdgDBList.back()->isValid()) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Failed to Construct FSMSDG for FSM "
                      << (1 + i) << " / " << fsmSize << " !!" << endl; input.close();
         for (uint32_t i = 0; i < _sdgDBList.size(); ++i) delete _sdgDBList[i]; return; }
   }
   if (lexFSMSpec(input, lineNumber, tokens))
      Msg(MSG_WAR) << "Line " << lineNumber << " : Omit Extra Specifications !!" << endl;
   input.close();
}

const bool dfsConstructConstAssignments(V3Ntk* const ntk, const V3NetId& ffId, const V3NetId& id, bool ok, V3NetVec& stateNets) {
   assert (ntk); assert (ntk->getNetSize() > id.id);
   const V3GateType type = ntk->getGateType(id);
   if (BV_MUX == type) {
      if (!ok) {
         V3NetId muxId = ntk->getInputNetId(id, 2); V3GateType muxType = ntk->getGateType(muxId);
         while (BV_AND == muxType) {
            if (ntk->getInputNetId(muxId, 0) == ~(ntk->getInputNetId(muxId, 1))) break;
            if (ntk->getInputNetId(muxId, 0) != ntk->getInputNetId(muxId, 1)) break;
            muxId = ntk->getInputNetId(muxId, 0); muxType = ntk->getGateType(muxId);
         }
         if (V3_FF == muxType && ffId == muxId) ok = true;
         if (BV_EQUALITY == muxType &&
             ((ffId == ntk->getInputNetId(muxId, 0) && (BV_CONST == ntk->getGateType(ntk->getInputNetId(muxId, 1)))) ||
              (ffId == ntk->getInputNetId(muxId, 1) && (BV_CONST == ntk->getGateType(ntk->getInputNetId(muxId, 0)))))) {
            ok = true;
         }
      }
      if (!dfsConstructConstAssignments(ntk, ffId, ntk->getInputNetId(id, 0), ok, stateNets)) return false;
      if (!dfsConstructConstAssignments(ntk, ffId, ntk->getInputNetId(id, 1), ok, stateNets)) return false;
      return true;
   }
   else if (BV_CONST == type || AIG_FALSE == type) {
      uint32_t i = 0; for (; i < stateNets.size(); ++i) if (id == stateNets[i]) break;
      if (ok && (stateNets.size() == i)) stateNets.push_back(id); return true;
   }
   else if (BV_AND == type || AIG_NODE == type) {
      if (ntk->getInputNetId(id, 0) != ntk->getInputNetId(id, 1)) return false;
      return dfsConstructConstAssignments(ntk, ffId, ntk->getInputNetId(id, 0), ok, stateNets);
   }
   else if (V3_FF == type) return (ffId.id == id.id); return false;
}

void
V3FSMExtract::constructStandardFSMSDG() {
   assert (dynamic_cast<V3NtkElaborate*>(_handler));
   assert (!_sdgMain); assert (!_sdgDBList.size());
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3NetVec stateNets; V3StrTable sdgExpr;
   sdgExpr.clear(); sdgExpr.push_back(V3StrVec());
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      // Collect Constant Assignments
      stateNets.clear(); assert (ntk->getNetSize() > ntk->getInputNetId(ntk->getLatch(i), 0).id);
      if (!dfsConstructConstAssignments(ntk, ntk->getLatch(i), ntk->getInputNetId(ntk->getLatch(i), 0), false, stateNets) ||
          !stateNets.size()) continue;
      // Elaborate Predicates for latch[i] == assignment[j]
      V3NetId id = V3NetUD; V3InputVec inputNets(2);
      if (dynamic_cast<V3BvNtk*>(ntk)) {
         inputNets[0] = V3NetType(ntk->getLatch(i));
         for (uint32_t j = 0; j < stateNets.size(); ++j) {
            id = ntk->createNet(1); inputNets[1] = V3NetType(stateNets[j]);
            ntk->setInput(id, inputNets); ntk->createGate(BV_EQUALITY, id); stateNets[j] = id;
         }
      }
      else {
         V3NetId id1 = V3NetUD;
         for (uint32_t j = 0; j < stateNets.size(); ++j) {
            inputNets[0] = V3NetType(ntk->getLatch(i)); inputNets[1] = V3NetType(stateNets[j]);
            id = ntk->createNet(1); ntk->setInput(id, inputNets); ntk->createGate(AIG_NODE, id);
            inputNets[0] = V3NetType(~(ntk->getLatch(i))); inputNets[1] = V3NetType(~(stateNets[j]));
            id1 = ntk->createNet(1); ntk->setInput(id1, inputNets); ntk->createGate(AIG_NODE, id1);
            inputNets[0] = V3NetType(~id); inputNets[1] = V3NetType(~id1);
            id = ntk->createNet(1); ntk->setInput(id, inputNets); ntk->createGate(AIG_NODE, id);
            stateNets[j] = ~id;
         }
      }
      // Create FSMSDGDB
      sdgExpr.back().clear(); sdgExpr.back().reserve(1 + (3 * stateNets.size()));
      for (uint32_t j = 0; j < stateNets.size(); ++j) {
         ntk->createOutput(stateNets[j]); sdgExpr.back().push_back(v3Int2Str(ntk->getOutputSize() - 1));
         sdgExpr.back().push_back("0"); sdgExpr.back().push_back(((1 + j) == (stateNets.size())) ? "0" : "1");
      }
      sdgExpr.back().push_back("0");  // dummy element for "line number"
      _sdgDBList.push_back(new V3FSMSDGDB(_handler, sdgExpr)); assert (_sdgDBList.back());
   }
}

void
V3FSMExtract::initializeDepGraph() {
   assert (_sdgMain); assert (!_depGraph.getNodeSize()); assert (!_clusters.size());
   assert (!_fsmList.size()); assert (!_sdgDBList.size());
   // Add Dependent FF of Dependent MUX to Dependency Graph
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   V3Vec<V3SDGBase*>::Vec depLatchList(ntk->getLatchSize(), 0);
   V3UI32Vec latchMap(ntk->getNetSize(), V3NtkUD);
   V3SDGBase* sdg = 0;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      latchMap[ntk->getLatch(i).id] = i;
      if (!_sdgMain->getSDGNode(ntk->getLatch(i).id)) continue;
      sdg = _sdgMain->getSDGNode(ntk->getLatch(i).id)->getBase(); assert (sdg);
      // Check if SDG has NO Dependent MUX
      if (isFilterOutNoMuxVariable() && !sdg->getDepMuxSet().size()) continue;
      sdg = new V3SDGBase(*sdg); assert (sdg);
      // Add Dependent FF of Dependent MUX to Dependency Graph
      sdg->clearDepFF(); _sdgMain->collectFaninMuxFF(sdg);
      // Check if SDG has NO Sequential Dependent MUX
      if (!isFilterOutNonSeqVariable() || sdg->getDepFFSet().size()) depLatchList[i] = sdg;
      else { delete sdg; continue; }
   }
   // Record Dependency to the Graph
   for (uint32_t i = 0; i < depLatchList.size(); ++i) {
      if (!depLatchList[i]) continue; _depRetain[ntk->getLatch(i).id] = true;
      const V3UI32Set& depFF = depLatchList[i]->getDepFFSet();
      for (V3UI32Set::const_iterator it = depFF.begin(); it != depFF.end(); ++it) {
         assert (V3_FF == ntk->getGateType(V3NetId::makeNetId(*it)));
         assert (V3NtkUD != latchMap[*it]);
         if (!depLatchList[latchMap[*it]]) continue;
         _depGraph.insertEdge(ntk->getLatch(i).id, *it);
      }
   }
   for (uint32_t i = 0; i < depLatchList.size(); ++i) if (depLatchList[i]) delete depLatchList[i];
   // Recursively Remove Nodes from the Graph if it has NO Outgoing Edges
   V3UI32Vec inNodes; V3UI32Queue leafVars; while (!leafVars.empty()) leafVars.pop();
   for (uint32_t i = 0; i < _depGraph.getNodeSize(); ++i) if (!_depGraph.getOutNodeSize(i)) leafVars.push(i);
   while (!leafVars.empty()) {
      uint32_t leaf = leafVars.front(); leafVars.pop();
      inNodes.clear(); inNodes.reserve(_depGraph.getInNodeSize(leaf));
      for (uint32_t i = 0; i < _depGraph.getInNodeSize(leaf); ++i) inNodes.push_back(_depGraph.getInNode(leaf, i));
      for (uint32_t i = 0; i < inNodes.size(); ++i) {
         assert (_depGraph.getOutNodeSize(inNodes[i]));
         _depGraph.deleteEdge(_depGraph.getNode(inNodes[i]), _depGraph.getNode(leaf));
         if (!_depGraph.getOutNodeSize(inNodes[i])) leafVars.push(inNodes[i]);
      }
      assert (!_depGraph.getInNodeSize(leaf));
   }
}

void
V3FSMExtract::startClusterVariables() {
   assert (_sdgMain); assert (!_depGraph.getNodeSize()); assert (!_clusters.size());
   assert (!isInvariantDirectedCluster()); assert (!_fsmList.size()); assert (!_sdgDBList.size());
   double runtime = clock(); if (_maxTime < _curTime) return;
   // Record Dependencies into Graph
   initializeDepGraph();
   // Cluster Variables
   if (isVariableClusterBySCC()) _depGraph.getStronglyConnectedComponents(_clusters);
   else {  // Put Every Variable Independently
      for (uint32_t i = 0; i < _depGraph.getNodeSize(); ++i) 
         if (!_depGraph.isIsolatedNode(i)) {
            _clusters.push_back(V3UI32Set()); _clusters.back().clear();
            _clusters.back().insert(_depGraph.getNode(i));
         }
   }
   V3SvrBase* const solver = allocSolver(V3VrfBase::getDefaultSolver(), _handler->getNtk());
   V3NetVec sdgList; sdgList.clear(); assert (solver);
   // Check if the State Size of the Individual FSM Exceeds the Upper Bound
   for (uint32_t i = 0; i < _clusters.size(); ++i) {
      _curTime += ((clock() - runtime) / CLOCKS_PER_SEC);
      runtime = clock(); if (_maxTime < _curTime) break;
      V3NetVec sdgList; sdgList.reserve(_clusters[i].size()); V3UI32Set::const_iterator it;
      for (it = _clusters[i].begin(); it != _clusters[i].end(); ++it) sdgList.push_back(V3NetId::makeNetId(*it));
      _sdgDBList.push_back(new V3FSMSDGDB(_sdgMain, sdgList, solver)); assert (_sdgDBList.back());
      _sdgDBList.back()->setMaxAllowedStateSize(_maxSize);
      _sdgDBList.back()->setOmitNodesAllCombVars(isEnableOmitNodesAllCombVars());
      _sdgDBList.back()->setOmitNodesIndepToVars(isEnableOmitNodesIndepToVars());
      _sdgDBList.back()->setRenderNonEmptyStates(isEnableRenderNonEmptyStates());
      _sdgDBList.back()->setConstructStandardFSM(isEnableConstructStandardFSM());
      _sdgDBList.back()->startConstructFSMSDG();
      if (!_sdgDBList.back()->isValid()) { delete _sdgDBList.back(); _sdgDBList.pop_back(); }
   }
}

void
V3FSMExtract::startExtractClusteredFSMs() {
   double runtime = clock(); if (_maxTime < _curTime) return;
   uint32_t fsmSize = 0; assert (_sdgDBList.size());
   _fsmList.reserve(_sdgDBList.size());
   // Start Extract FSM from Each Cluster
   V3SvrBase* const solver = allocSolver(V3VrfBase::getDefaultSolver(), _handler->getNtk());
   for (uint32_t i = 0; i < _sdgDBList.size(); ++i) {
      _curTime += ((clock() - runtime) / CLOCKS_PER_SEC);
      runtime = clock(); if (_maxTime < _curTime) break;
      assert (_sdgDBList[i] && _sdgDBList[i]->isValid());
      if (V3VrfBase::intactON()) Msg(MSG_IFO) << "Extracting FSM[" << i << "] : ";
      _fsmList.push_back(new V3FSM(_sdgDBList[i], (_sdgDBList[i]->getSolver()) ? _sdgDBList[i]->getSolver() : solver));
      _fsmList.back()->stopOnAllInitialOrAllTerminal(isEnableStopOnAllInitOrTerms());
      _fsmList.back()->startExtractFSM(_terminals);
      if (V3VrfBase::intactON()) Msg(MSG_IFO) << "#States = " << _fsmList.back()->getStateSize();
      // Remove Empty FSM or Incomplete FSM
      if (!_fsmList.back()->getStateSize()) {
         if (V3VrfBase::intactON()) Msg(MSG_IFO) << endl;
         delete _sdgDBList[i]; _sdgDBList[i] = 0;
         delete _fsmList.back(); _fsmList.pop_back(); continue;
      }
      ++fsmSize; if (!V3VrfBase::intactON()) continue;
      Msg(MSG_IFO) << ", #Init = "   << _fsmList.back()->getInitStateSize() 
                   << ", #InitR = "  << _fsmList.back()->getInitFwdReachableSize()
                   << ", #Term = "   << _fsmList.back()->getTermStateSize() 
                   << ", #TermR = "  << _fsmList.back()->getTermBwdReachableSize()
                   << ", #MStone = " << _fsmList.back()->getMileStoneSize()
                   << endl;
   }
   if (V3VrfBase::reportON()) {
      if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl;
      Msg(MSG_IFO) << "Totally " << fsmSize << (fsmSize > 1 ? " FSMs " : " FSM ") << "found !!";
      if (V3VrfBase::usageON()) Msg(MSG_IFO) << "  (time = " << setprecision(5) << _curTime << "  sec)" << endl;
   }
}

void
V3FSMExtract::startExtractInvariantFSMs() {
   assert (_sdgMain); assert (!_fsmList.size()); assert (!_clusters.size());
   assert (isInvariantDirectedCluster()); assert (!_sdgDBList.size());
   double runtime = clock(); if (_maxTime < _curTime) return;
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Index Latches from the Property Signal
   V3NetVec ffList; ffList.clear(); ffList.reserve(ntk->getLatchSize());
   if (_terminals.size()) bfsIndexFaninConeFF(ntk, ffList, _terminals);
   else for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) ffList.push_back(ntk->getLatch(i));
   V3NetVec pivotNets = _terminals; pivotNets.reserve(_terminals.size() + ffList.size());
   for (uint32_t i = 0; i < ffList.size(); ++i) pivotNets.push_back(ffList[i]);
   // Compute Variable Dependency for Clusters
   V3SvrBase* const solver = allocSolver(V3VrfBase::getDefaultSolver(), _handler->getNtk());
   V3Vec<V3SDGBase*>::Vec pivotDep(ntk->getNetSize(), 0);
   for (uint32_t i = 0; i < pivotNets.size(); ++i) {
      // Add Dependent FF of Dependent MUX to Dependency Graph
      if (pivotDep[pivotNets[i].id]) continue;
      V3SDGBase* const sdg = new V3SDGBase(*(_sdgMain->getSDGNode(pivotNets[i].id)->getBase())); assert (sdg);
      if (isFilterOutNoMuxVariable() && !sdg->getDepMuxSet().size()) { delete sdg; continue; }
      sdg->clearDepFF(); _sdgMain->collectFaninMuxFF(sdg);
      if (isFilterOutNonSeqVariable() && !sdg->getDepFFSet().size()) { delete sdg; continue; }
      pivotDep[pivotNets[i].id] = sdg;
   }
   _curTime += ((clock() - runtime) / CLOCKS_PER_SEC);
   runtime = clock(); if (_maxTime < _curTime) return;
   // Start Compute FSM Pivoted at Each Variable
   V3UI32Vec state2Id(ntk->getNetSize()); V3Set<string>::Set sdgExpr;
   for (uint32_t i = 0; i < state2Id.size(); ++i) state2Id[i] = i;
   V3Vec<V3Set<string>::Set>::Vec computedExprVec; computedExprVec.clear();
   V3Vec<V3BitVec>::Vec failClusters; failClusters.clear();
   V3Vec<V3BitVec>::Vec succClusters; succClusters.clear();
   for (uint32_t i = 0; i < pivotNets.size(); ++i) {
      const V3NetId id = pivotNets[i]; if (!pivotDep[id.id]) continue;
      V3UI32Set cluster; cluster.insert(id.id);
      // Compute FSM Pivoted at this Variable
      V3BitVec clusterVars(ntk->getNetSize()); clusterVars.set1(id.id);
      V3UI32Vec newVariables(1, id.id); V3FSM* latestFSM = 0;
      while (newVariables.size()) {
         // Collect Cluster Variables
         V3NetVec sdgList; sdgList.reserve(cluster.size()); assert (cluster.size());
         for (V3UI32Set::const_iterator it = cluster.begin(); it != cluster.end(); ++it) 
            sdgList.push_back(V3NetId::makeNetId(*it));
         // Check if Current Cluster is Computed Failed
         for (uint32_t j = 0; j < failClusters.size(); ++j) {
            V3BitVec clusterValue = clusterVars; clusterValue &= failClusters[j];
            if (failClusters[j] == clusterValue) { newVariables.clear(); break; }
         }
         if (!newVariables.size()) break;
         // Check if Current Cluster is Computed Succeed
         for (uint32_t j = 0; j < succClusters.size(); ++j) {
            V3BitVec clusterValue = clusterVars; clusterValue &= succClusters[j];
            if (clusterVars == clusterValue) { newVariables.clear(); break; }
         }
         if (!newVariables.size()) {
            // Push More Variables into the Cluster
            V3UI32Set newCluster; newCluster.clear();
            for (uint32_t j = 0; j < newVariables.size(); ++j) {
               V3SDGBase* const sdg = pivotDep[sdgList[j].id]; assert (sdg);
               for (V3UI32Set::const_iterator it = sdg->getDepFFSet().begin(); it != sdg->getDepFFSet().end(); ++it) 
                  newCluster.insert(*it);
            }
            // Record New Variables to the Cluster
            newVariables.clear();
            for (V3UI32Set::const_iterator it = newCluster.begin(); it != newCluster.end(); ++it) {
               if (!pivotDep[*it] || (cluster.end() != cluster.find(*it))) continue;
               assert (!clusterVars[*it]); clusterVars.set1(*it);
               cluster.insert(*it); newVariables.push_back(*it);
            }
            continue;
         }
         // Compute FSMSDGDB
         V3FSMSDGDB* const sdgDB = new V3FSMSDGDB(_sdgMain, sdgList, solver); assert (sdgDB);
         sdgDB->setMaxAllowedStateSize(_maxSize);
         sdgDB->setOmitNodesAllCombVars(isEnableOmitNodesAllCombVars());
         sdgDB->setOmitNodesIndepToVars(isEnableOmitNodesIndepToVars());
         sdgDB->setRenderNonEmptyStates(isEnableRenderNonEmptyStates());
         sdgDB->setConstructStandardFSM(isEnableConstructStandardFSM());
         sdgDB->startConstructFSMSDG();
         if (!sdgDB->isValid()) {
            failClusters.push_back(clusterVars);
            delete sdgDB; break;
         }
         // Check if the FSM of the Current Cluster is Computed
         const V3FSMSDGList& fsmSdgList = sdgDB->getFSMSDGList();
         string exp = ""; bool exists = false; sdgExpr.clear();
         for (uint32_t j = 0; j < fsmSdgList.size(); ++j) {
            fsmSdgList[j]->getSDGExpr(state2Id, exp); assert (exp.size());
            sdgExpr.insert(exp); exp = "";
         }
         for (uint32_t j = 0; j < computedExprVec.size(); ++j) {
            if (sdgExpr.size() != computedExprVec[j].size()) continue;
            V3Set<string>::Set::const_iterator it = sdgExpr.begin();
            for (; it != sdgExpr.end(); ++it) if (computedExprVec[j].end() == computedExprVec[j].find(*it)) break;
            if (sdgExpr.end() == it) { exists = true; break; }
         }
         if (exists) { delete latestFSM; latestFSM = 0; break; }
         // Extract FSM Under Current Cluster
         V3FSM* const fsm = new V3FSM(sdgDB, sdgDB->getSolver()); assert (fsm);
         fsm->stopOnAllInitialOrAllTerminal(isEnableStopOnAllInitOrTerms());
         fsm->startExtractFSM(_terminals); //delete sdgDB;
         if (!fsm->isCompleteFSM()) { delete fsm; break; }
         if (latestFSM) delete latestFSM; latestFSM = fsm;
         if (fsm->getMileStoneSize() == fsm->getStateSize()) {
            // Push More Variables into the Cluster
            V3UI32Set newCluster; newCluster.clear();
            for (uint32_t j = 0; j < newVariables.size(); ++j) {
               V3SDGBase* const sdg = pivotDep[sdgList[j].id]; assert (sdg);
               for (V3UI32Set::const_iterator it = sdg->getDepFFSet().begin(); it != sdg->getDepFFSet().end(); ++it)
                  newCluster.insert(*it);
            }
            // Record New Variables to the Cluster
            newVariables.clear(); succClusters.push_back(clusterVars);  // Set Succeed
            for (V3UI32Set::const_iterator it = newCluster.begin(); it != newCluster.end(); ++it) {
               if (!pivotDep[*it] || (cluster.end() != cluster.find(*it))) continue;
               assert (!clusterVars[*it]); clusterVars.set1(*it);
               cluster.insert(*it); newVariables.push_back(*it);
            }
         }
         else break;
      }
      if (latestFSM) {
         if (!latestFSM->getStateSize()) continue;
         // Report Extraction Result
         if (V3VrfBase::intactON()) {
            Msg(MSG_IFO) << "Cluster " << _clusters.size() << " : #Vars = " << cluster.size();
            Msg(MSG_IFO) << ", #States = " << latestFSM->getStateSize() 
                         << ", #Init = "   << latestFSM->getInitStateSize() 
                         << ", #InitR = "  << latestFSM->getInitFwdReachableSize() 
                         << ", #Term = "   << latestFSM->getTermStateSize() 
                         << ", #TermR = "  << latestFSM->getTermBwdReachableSize() 
                         << ", #MStone = " << latestFSM->getMileStoneSize() 
                         << endl;
         }
         // Record Current FSM
         _clusters.push_back(cluster); _fsmList.push_back(latestFSM);
         // Set Failed FSM to Refute Computing FSM that Covers it
         failClusters.push_back(clusterVars); computedExprVec.push_back(sdgExpr);
         _curTime += ((clock() - runtime) / CLOCKS_PER_SEC);
         runtime = clock(); if (_maxTime < _curTime) break;
      }
   }
   assert (_clusters.size() == _fsmList.size());
   // Report Extraction Result
   if (V3VrfBase::reportON()) {
      if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl;
      Msg(MSG_IFO) << "Totally " << _fsmList.size() << (_fsmList.size() > 1 ? " FSMs " : " FSM ") << "found !!";
      if (V3VrfBase::usageON()) Msg(MSG_IFO) << "  (time = " << setprecision(5) << _curTime << "  sec)" << endl;
   }
}

#endif

