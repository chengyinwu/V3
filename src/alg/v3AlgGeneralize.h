/****************************************************************************
  FileName     [ v3AlgGeneralize.h ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ V3 Generalization Utility. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_GENERALIZE_H
#define V3_ALG_GENERALIZE_H

#include "v3SvrType.h"
#include "v3AlgSimulate.h"

// Defines
typedef V3List<V3NetId>::List    V3NetList;

// class V3GenBucket : Specialized Bucket List for Generalization
class V3GenBucket {
   public : 
      // Constructor and Destructor
      V3GenBucket(const uint32_t& = 0, const uint32_t& = 0);
      ~V3GenBucket();
      // Inline Member Functions
      inline const bool isEmpty() const { return _buckets.size() <= _bestIndex; }
      // Basic Manipulation Functions
      void noticePushComplete();
      void push(const uint32_t&, const uint32_t&);
      const uint32_t pop(uint32_t&);
      void inc(const uint32_t&, const uint32_t&);
   private : 
      // Private Data Member
      V3UI32Vec         _prevData;  // The Prev (fwd pointer) Element
      V3UI32Vec         _nextData;  // The Next (bwd pointer) Element
      V3UI32Vec         _buckets;   // The First Element in the Bucket
      V3UI32Vec         _bucketIdx; // The Index for Element Storage
      uint32_t          _bestIndex; // The Lowest Non-empty Bucket
};

// class V3GenStruct : Generalization Data Structure
class V3GenStruct {
   public : 
      // Constructor and Destructor
      V3GenStruct(const uint32_t&, const V3NetVec& = V3NetVec());
      ~V3GenStruct();
      // Inline Member Functions
      inline const uint32_t getVarSize() const { return _var2Cube.size(); }
      inline const uint32_t getCubeSize() const { return _cube2Var.size(); }
      // Maintanence Functions
      const bool pushCube(const V3NetVec&, const bool& = false);
      void startVarOrder(const V3NetVec&);
      const V3NetId getNextVar();
      const V3NetId getNextVar(uint32_t&);
      void updateVarOrder(const bool&);
      void clear(const V3NetVec&);
   private : 
      struct V3GenComp;
      typedef V3Vec<V3UI32Set>::Vec                V3GenCubeList;
      typedef pair<V3NetId, uint32_t>              V3GenVarPair;
      typedef V3Set<V3GenVarPair, V3GenComp>::Set  V3GenVarOrder;
      struct V3GenComp {
         inline const bool operator () (const V3GenVarPair& p1, const V3GenVarPair& p2) const {
            return (p1.second == p2.second) ? (p1.first.id < p2.first.id) : (p1.second < p2.second); } };
      // Private Member Functions
      const bool isValidCube(const V3NetVec&) const;
      void printCube(const V3NetVec&) const;
      void printConfl(const V3UI32Set&) const;
      void printVar2Cube() const;
      void printCube2Var() const;
      // Private Data Members
      V3NetVec          _baseCube;
      V3UI32Vec         _varValue;
      V3UI32Table       _var2Cube;
      V3GenCubeList     _cube2Var;
      V3GenVarOrder     _varOrder;
};

// class V3AlgGeneralize : Generalization Configurations
class V3AlgGeneralize {
   public : 
      // Constructor and Destructor
      V3AlgGeneralize();
      virtual ~V3AlgGeneralize();
      // Generalization Main Functions
      virtual void setTargetNets(const V3NetVec&, const V3NetVec& = V3NetVec());
      const V3NetVec getUndecided() const;
      const V3NetVec getGeneralizationResult() const;
      // Preprocessing Techniques
      virtual void performSetXForNotCOIVars();
      virtual void performFixForControlVars(const bool& = true);
      // Generalization Heuristics
      virtual void performXPropForExtensibleVars(const V3UI32Vec&);
      virtual void performXPropForMinimizeTransitions(const uint32_t&, const V3NetVec& = V3NetVec());
   protected : 
      // Private Data Members
      V3NetList            _genResult; // Generalization Result
      V3NetList            _undecided; // Extensibility Records
};

// class V3AlgAigGeneralize : Generalization for V3 AIG Ntk
class V3AlgAigGeneralize : public V3AlgAigSimulate, public V3AlgGeneralize {
   public : 
      // Constructor and Destructor
      V3AlgAigGeneralize(const V3NtkHandler* const);
      ~V3AlgAigGeneralize();
      // Generalization Main Functions
      void setTargetNets(const V3NetVec&, const V3NetVec& = V3NetVec());
      // Preprocessing Techniques
      void performSetXForNotCOIVars();
      void performFixForControlVars(const bool& = true);
      // Generalization Heuristics
      void performXPropForExtensibleVars(const V3UI32Vec&);
      void performXPropForMinimizeTransitions(const uint32_t&, const V3NetVec& = V3NetVec());
   private : 
      // Private Generalization Functions
      void simulateForGeneralization(const V3NetId&, const V3BitVecS&);
      const bool simulateForGeneralization(const V3NetId&, const V3BitVecS&, const V3BoolVec&);
      const bool generalizationValid();
      void recoverForGeneralization();
      void performSimulationForFrozenPropagation(const V3NetVec&);
      const bool performImplicationForTargetUpdate(V3NetVec&);
      const bool performImplicationForTargetUpdate(const V3NetId&, V3NetVec&, V3BoolVec&);
      // Private Data Members
      V3NetVec             _targetId;  // Resolved Target NetId
      V3BoolVec            _isFrozen;  // Frozen Net Table
      V3BoolVec            _traverse;  // Traversed Net Table
};

// class V3AlgBvGeneralize : Generalization for V3 BV Ntk
class V3AlgBvGeneralize : public V3AlgBvSimulate, public V3AlgGeneralize {
   public : 
      // Constructor and Destructor
      V3AlgBvGeneralize(const V3NtkHandler* const);
      ~V3AlgBvGeneralize();
      // Generalization Main Functions
      void setTargetNets(const V3NetVec&, const V3NetVec& = V3NetVec());
      // Preprocessing Techniques
      void performSetXForNotCOIVars();
      void performFixForControlVars(const bool& = true);
      // Generalization Heuristics
      void performXPropForExtensibleVars(const V3UI32Vec&);
      void performXPropForMinimizeTransitions(const uint32_t&, const V3NetVec& = V3NetVec());
   private : 
      // Private Generalization Functions
      void simulateForGeneralization(const V3NetId&, const V3BitVecX&);
      const bool simulateForGeneralization(const V3NetId&, const V3BitVecX&, const V3BoolVec&);
      const bool generalizationValid();
      void recoverForGeneralization();
      // Private Data Members
      V3NetVec             _targetId;  // Resolved Target NetId
      V3BoolVec            _traverse;  // Traversed Net Table
};

#endif

