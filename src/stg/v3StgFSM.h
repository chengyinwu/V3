/****************************************************************************
  FileName     [ v3StgFSM.h ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ FSM Data Structure for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_FSM_H
#define V3_STG_FSM_H

#include "v3StgSDG.h"
#include "v3NtkHandler.h"
#include "v3SvrHandler.h"
#include "v3AlgSimulate.h"

// Forward Declaration
class V3FSMSDG;
class V3FSM;

// Defines
typedef V3Map<uint32_t, V3FSMSDG*>::Map   V3FSMSDGMap;
typedef V3Set<V3FSMSDG*>::Set             V3FSMSDGSet;
typedef V3Vec<V3FSMSDG*>::Vec             V3FSMSDGList;

// class V3FSMSDG : FSM Specific SDG  (Usually Derived from V3SDG)
class V3FSMSDG {
   public : 
      // Constructor and Destructor
      V3FSMSDG(const V3NetId&);
      ~V3FSMSDG();
      // Inline Member Functions
      inline void setFalseEmptyButExist() { assert (!_falseSize); _falseSize = 1; }
      inline void setTrueEmptyButExist() { assert (!_trueSize); _trueSize = 1; }
      inline const V3NetId getStateNet() const { return _netId; }
      inline const V3FSMSDGSet& getFalseSDG() const { return _falseSDG; }
      inline const V3FSMSDGSet& getTrueSDG() const { return _trueSDG; }
      // State Computation Functions
      void addFalseSDG(V3FSMSDG* const&);
      void addTrueSDG(V3FSMSDG* const&);
      const uint32_t getStateSize() const;
      inline const uint32_t getFalseStateSize() const { return _falseSize; }
      inline const uint32_t getTrueStateSize() const { return _trueSize; }
      void getSDGExpr(const V3UI32Vec&, string&) const;
   private : 
      // Private Data Members
      const V3NetId           _netId;     // Embraced NetId in V3SDGBase
      V3FSMSDGSet             _falseSDG;  // Set of SDG for the FALSE Part
      V3FSMSDGSet             _trueSDG;   // Set of SDG for the TRUE Part
      uint32_t                _falseSize; // Size of the False States
      uint32_t                _trueSize;  // Size of the True States
};

// class V3FSMSDGDB : FSM SDG Database
class V3FSMSDGDB {
   public : 
      // Constructor and Destructor
      V3FSMSDGDB(V3SDG* const, const V3NetVec&, const V3SolverType&);
      V3FSMSDGDB(V3SDG* const, const V3NetVec&, V3SvrBase* const);
      V3FSMSDGDB(V3NtkHandler* const, const V3StrTable&);
      ~V3FSMSDGDB();
      // Main Functions
      void startConstructFSMSDG();
      const uint32_t getStateSize() const;
      // Inline Member Functions
      inline V3NtkHandler* const getNtkHandler() const { return _handler; }
      inline const V3FSMSDGList& getFSMSDGList() const { return _sdgList; }
      inline const V3UI32Set& getDepVars() const { return _depVars; }
      inline V3SvrBase* const getSolver() const { return _solver; }
      // Inline Configuration and Status Functions
      inline void setMaxAllowedStateSize(const uint32_t& s) { _maxSize = s; }
      inline void setOmitNodesAllCombVars(const bool& e) { if (e) _configs |= 1ul; else _configs &= ~1ul; }
      inline void setOmitNodesIndepToVars(const bool& e) { if (e) _configs |= 2ul; else _configs &= ~2ul; }
      inline void setRenderNonEmptyStates(const bool& e) { if (e) _configs |= 4ul; else _configs &= ~4ul; }
      inline void setConstructStandardFSM(const bool& e) { if (e) _configs |= 8ul; else _configs &= ~8ul; }
      inline const bool isValid() const { return !isEarlySuspended() && (getStateSize() <= _maxSize); }
   private : 
      // Private FSMSDG Computation Functions
      const bool startConstructFSMSDG(V3SDGBase* const, uint32_t&, V3FSMSDGMap&, V3NetVec&);
      V3FSMSDG* const computeFSMSDG(const V3NetId&, V3NetVec&, const V3FSMSDGMap&);
      V3FSMSDG* const computeFSMSDG(V3FSMSDG* const, V3NetVec&);
      const bool mergeMuxFaninSDG(V3FSMSDG* const, V3FSMSDG* const, const bool&);
      void updateDepMuxSet(V3UI32Set&);
      const bool existState(const V3NetVec&);
      // Private Configuration Functions
      inline const uint32_t getMaxAllowedStateSize() const { return _maxSize; }
      inline const bool isOmitNodesAllCombVarsEnabled() const { return 1ul & _configs; }
      inline const bool isOmitNodesIndepToVarsEnabled() const { return 2ul & _configs; }
      inline const bool isRenderNonEmptyStatesEnabled() const { return 4ul & _configs; }
      inline const bool isConstructStandardFSMEnabled() const { return 8ul & _configs; }
      inline void setEarlySuspended() { _configs |= 128ul; }
      // Private Internal Configurations
      inline const bool isExternalSolver() const { return  64ul & _configs; }
      inline const bool isEarlySuspended() const { return 128ul & _configs; }
      // Private Data Members
      V3NtkHandler* const     _handler;   // Network that FSMSDG Extracted From
      V3SvrBase* const        _solver;    // SAT Solver for Formal Confirmation
      V3SDG* const            _sdgMain;   // Main SDG Extracted from the Network
      V3FSMSDGList            _sdgList;   // List of Computed FSMSDGs for V3FSM
      V3UI32Set               _depVars;   // Dependent Variables of the FSM
      uint32_t                _maxSize;   // Maximum Allowed Number of States
      unsigned char           _configs;   // FSMSDG Construction Configurations  (2 Leading MSBs are reserved)
};

class V3FSM {
   public : 
      // Constructor and Destructor
      V3FSM(V3FSMSDGDB* const, V3SvrBase* const);
      ~V3FSM();
      // Main Functions
      void startExtractFSM(const V3NetVec&);
      void updateExtractedFSM(const V3NetVec&);
      // Inline Interface Functions
      inline const uint32_t getStateSize() const { return _sdgDB->getStateSize(); }
      inline const V3FSMSDGList& getFSMSDGList() const { return _sdgDB->getFSMSDGList(); }
      inline V3NtkHandler* const getNtkHandler() const { return _sdgDB->getNtkHandler(); }
      // Inline Configuration and Status Functions
      inline void stopOnAllInitialOrAllTerminal(const bool& e) { if (e) _configs |= 1ul; else _configs &= ~1ul; }
      inline const bool isCompleteFSM() const { return !(128ul & _configs) && _distInit.size(); }
      // FSM Information Functions
      const uint32_t getInitStateSize() const;
      const uint32_t getTermStateSize() const;
      const uint32_t getReachableSize() const;
      const uint32_t getMileStoneSize() const;
      const uint32_t getInitFwdReachableSize();
      const uint32_t getTermBwdReachableSize();
      const uint32_t getStepFwdFromInit(const uint32_t&) const;
      const uint32_t getStepBwdFromTerm(const uint32_t&) const;
      const bool isMileStone(const uint32_t&) const;
      const bool isAllReachableFromFwd() const;
      const bool isAllReachableFromBwd() const;
      void collectStateNets(V3UI32Set&) const;
      void getStateNetId(const uint32_t&, V3NetVec&) const;
      void getInitStates(V3UI32Vec&, const bool& = false) const;
      void getTermStates(V3UI32Vec&, const bool& = false) const;
      void getStateFwdReachable(const uint32_t&, V3UI32Vec&, const bool& = false) const;
      void getStateBwdReachable(const uint32_t&, V3UI32Vec&, const bool& = false) const;
      // Debug Functions
      void debugValues(const V3NetId&, const uint32_t&) const;
      const bool check(V3SvrBase* const) const;
      // Output Functions
      void writeFSM(const string&) const;
   private : 
      // Private Configuration Functions
      inline void setEarlySuspended() { _configs |= 128ul; }
      inline const bool isStopOnAllInitialOrAllTerminal() const { return 1ul & _configs; }
      // Private FSM Information Functions
      void collectStateNets(V3FSMSDG* const, V3UI32Set&) const;
      // Private State Investigation Functions
      void initializeReachTable(const uint32_t&);
      const bool simulateForForwardConstruction(const uint32_t&);
      const bool formalForForwardConstruction(const uint32_t&);
      const bool formalForBackwardConstruction(const uint32_t&);
      // Private Simulation Functions
      const uint32_t randomGetInitialState();
      const uint32_t randomSimulateOneCycle();
      void recordSimulationValue(V3SimTrace&) const;
      void setStateForSimulation(const V3SimTrace&);
      const uint32_t encodeStateFromSimulator() const;
      const uint32_t encodeStateFromSimulator(V3FSMSDG* const) const;
      const V3UI32Vec encodeAllStateFromSimulator() const;
      const V3UI32Vec encodeAllStateFromSimulator(V3FSMSDG* const) const;
      // Private SAT-solving Functions
      const uint32_t formalGetInitialState(const V3SvrData&);
      const uint32_t formalGetNextState(const V3NetVec&, const V3SvrData&);
      const V3UI32Vec formalGetTerminalState(const V3SvrData&);
      const V3UI32Vec formalGetCurrentState(const V3NetVec&, const V3SvrData&);
      void formalBlockNextState(const uint32_t&, const V3SvrData&);
      void formalBlockCurrentState(const uint32_t&, const V3SvrData&);
      void recordSolverValue(V3SimTrace&, const uint32_t&) const;
      void recordSolverXValue(V3SimTrace&, const uint32_t&) const;
      void decodeStateToNetId(const uint32_t&, V3NetVec&) const;
      void decodeStateToNetId(const V3FSMSDG* const, const uint32_t&, V3NetVec&) const;
      // Private Generalization Functions
      void generalizeStateFromSimulation(const V3NetVec&, const uint32_t&);
      // Private State Traversal Functions
      void bfsUpdateForwardReachability();
      void bfsUpdateBackwardReachability();
      // Private Output Functions
      const string getStateName(const uint32_t&) const;
      // Private Debug Functions
      void pushStateIntoSolver(V3SvrBase* const, const uint32_t&, const uint32_t&) const;
      // Private Data Members
      V3FSMSDGDB* const       _sdgDB;     // Main FSMSDGDB for FSM Extraction
      V3NetVec                _terminal;  // Set of Terminal (or Bug) States for the FSM
      V3UI32Vec               _distInit;  // The Shortest Step that the Node Id is Reachable from Initial States
      V3UI32Vec               _distTerm;  // The Shortest Step that the Node Id is Reachable to Terminal States
      V3BoolTable             _reachSucc; // Transition Relation Table from Node Id to Node Id that are Reachable
      V3BoolTable             _reachFail; // Transition Relation Table from Node Id to Node Id that are Unreachable
      // Private Engines
      V3SvrBase* const        _solver;    // SAT Solver for Formal Confirmation
      V3AlgSimulate* const    _simulator; // Simulator for (Semi-) Random Reachability Analysis
      // Statistics
      unsigned char           _configs;   // Configurations
      uint32_t                _cutSize;   // Number of Cut Signals in terms of Total Bit Width
};

#endif

