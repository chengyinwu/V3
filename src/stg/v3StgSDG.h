/****************************************************************************
  FileName     [ v3StgSDG.h ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ Sequential Dependency Graph for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_SDG_H
#define V3_STG_SDG_H

#include "v3NtkHandler.h"

// Forward Declaration
class V3SDGNode;

// Defines
typedef V3Vec<V3SDGNode*>::Vec   V3SDGNodeList;

// Class V3SDGBase : Base Node Structure for SDG
class V3SDGBase {
   public : 
      // Constructor and Destructor
      V3SDGBase();
      V3SDGBase(const V3SDGBase&);
      ~V3SDGBase();
      // Inline Interface Functions
      inline const V3UI32Set& getDepFFSet() const { return _depFFSet; }
      inline const V3UI32Set& getDepMuxSet() const { return _depMuxSet; }
      inline void insertFF(const uint32_t& i) { _depFFSet.insert(i); }
      inline void insertMux(const uint32_t& i) { _depMuxSet.insert(i); }
      inline void clearDepFF() { _depFFSet.clear(); }
      inline void clearDepMux() { _depMuxSet.clear(); }
      // Inline Fanin Cone Collection Functions
      inline V3SDGBase* const getDepFFCone() { return _depFFCone; }
      inline V3SDGBase* const getDepMuxCone() { return _depMuxCone; }
      inline void newDepFFCone() { assert (!_depFFCone); _depFFCone = new V3SDGBase(); assert (_depFFCone); }
      inline void newDepMuxCone() { assert (!_depMuxCone); _depMuxCone = new V3SDGBase(); assert (_depMuxCone); }
      // Special Handling Functions
      void markDepFF(V3BitVec&) const;
      void markDepMux(V3BitVec&) const;
      // Print and Debug Functions
      void print() const;
   private : 
      // Private Data Members
      V3UI32Set         _depFFSet;     // Dependent FF Nodes
      V3UI32Set         _depMuxSet;    // Dependent MUX Nodes
      V3SDGBase*        _depFFCone;    // V3SDGBase of Dependent FaninFF Nodes
      V3SDGBase*        _depMuxCone;   // V3SDGBase of Dependent FaninMuxFF Nodes
};

// V3SDGNode : Simple Node (Other than MUX) in SDG
class V3SDGNode {
   public : 
      // Constructor and Destructor
      V3SDGNode(const V3NetId&);
      virtual ~V3SDGNode();
      // Inline Interface Functions
      inline const V3NetId& getNetId() const { return _netId; }
      inline V3SDGBase* const getBase() const { return _base; }
   private : 
      const V3NetId     _netId;        // NetId Corresponding to Ntk
      V3SDGBase* const  _base;         // Base Dependency Structure
};

// V3SDGMUX : MUX Node in SDG
class V3SDGMUX : public V3SDGNode {
   public : 
      // Constructor and Destructor
      V3SDGMUX(const V3NetId&);  // NOTE: Set Select Signal, NOT MUX Output Signal
      ~V3SDGMUX();
      // Inline Interface Functions
      inline V3SDGBase* const getTBase() const { return _tBase; }
      inline V3SDGBase* const getFBase() const { return _fBase; }
   private : 
      V3SDGBase* const  _tBase;        // Base Structure for True Part of MUX
      V3SDGBase* const  _fBase;        // Base Structure for False Part of MUX
};

// Class V3SDG : Sequential Dependency Graph (SDG) Representation of BV Networks
class V3SDG {
   public : 
      // Constructor and Destructor
      V3SDG(V3NtkHandler* const);
      virtual ~V3SDG();
      // Inline Interface Functions
      inline V3NtkHandler* const getNtkHandler() const { return _handler; }
      inline V3SDGNode* const getSDGNode(const uint32_t& i) const { 
         assert (i < _nodeList.size()); return _nodeList[i]; }
      // SDG Construction Function
      virtual V3SDGBase* const constructSDG(const V3NetId&);
      void collectFaninFF(V3SDGBase* const, V3SDGBase* const = 0);
      void collectFaninMuxFF(V3SDGBase* const, V3SDGBase* const = 0);
   private : 
      // Private SDG Construction Functions
      const bool constructBvSDG(const V3NetId&, V3SDGBase* const);
   protected : 
      // Private Data Members
      V3NtkHandler* const  _handler;   // Ntk Handler
      V3SDGNodeList        _nodeList;  // List of SDG Nodes for Care Signals
};

// Class V3AigSDG : SDG of AIG Networks
class V3AigSDG : public V3SDG {
   public : 
      // Constructor and Destructor
      V3AigSDG(V3NtkHandler* const);
      // SDG Construction Function
      V3SDGBase* const constructSDG(const V3NetId&);
   private : 
      // Private SDG Construction Functions
      void identifyCandidates();
      const bool constructAigSDG(const V3NetId&, V3SDGBase* const);
      // Private Data Members
      V3UI32Vec         _muxCandidate; // MUX Candidates on AIG Network
      V3BoolVec         _mexCandidate; // Mutex Candidates for MUX Candidates
};

#endif

