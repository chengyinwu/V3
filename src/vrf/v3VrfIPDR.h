/****************************************************************************
  FileName     [ v3VrfIPDR.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Incremental Property Directed Reachability on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_IPDR_H
#define V3_VRF_IPDR_H

#include "v3Usage.h"
#include "v3SvrType.h"
#include "v3VrfBase.h"
#include "v3AlgGeneralize.h"

// Forward Declaration
class V3IPDRCube;
class V3IPDRFrame;

// Defines for Incremental PDR Global Data
typedef pair<uint32_t, V3IPDRCube*>          V3IPDRTimedCube;
typedef V3Vec<V3IPDRFrame*>::Vec             V3IPDRFrameVec;
typedef V3Vec<V3SvrBase*>::Vec               V3IPDRSvrList;

// class V3IPDRCube : Cube Data Structue for Incremental PDR
// NOTE: State Variables are Intrinsically Sorted by their indices (small to large)
class V3IPDRCube
{
   public : 
      V3IPDRCube(const V3IPDRCube* const c) : _nextCube(c) { _stateId.clear(); _signature = 0; }
      ~V3IPDRCube() { _stateId.clear(); }
      // Cube Setting Functions
      inline void setState(const V3NetVec&);
      inline const V3NetVec& getState() const { return _stateId; }
      inline const uint64_t getSignature() const { return _signature; }
      // Trace Logging Functions
      inline void setInputData(const V3BitVecX& v) { _inputData = v; }
      inline const V3IPDRCube* const getNextCube() const { return _nextCube; }
      inline const V3BitVecX& getInputData() const { return _inputData; }
   private : 
      // Private Cube Data
      V3NetVec                _stateId;      // State Variable Index (id) with Value (cp)
      uint64_t                _signature;    // Subsumption Marker
      // Trace Logging Members
      V3BitVecX               _inputData;    // Primary Input / Inout Values
      const V3IPDRCube* const _nextCube;     // Successor Proof Obligation
};

// Defines for Sorted Incremental PDR Cube List
struct V3IPDRCubeCompare {
   const bool operator() (const V3IPDRCube* const c1, const V3IPDRCube* const c2) const {
      assert (c1); const V3NetVec& state1 = c1->getState();
      assert (c2); const V3NetVec& state2 = c2->getState();
      uint32_t i = 0;
      // Render the One with Larger Leading Id the Latter
      while (i < state1.size() && i < state2.size()) {
         assert (!i || (state1[i].id > state1[i-1].id));
         assert (!i || (state2[i].id > state2[i-1].id));
         if (state1[i].id == state2[i].id) {
            if (state1[i].cp == state2[i].cp) ++i;
            else return state1[i].cp < state2[i].cp;
         }
         else return state1[i].id < state2[i].id;
      }
      // Render the One with Less Variables the Latter
      return (state2.size() > i);  //return (state1.size() <= i);
   }
};

typedef V3Set<V3IPDRCube*, V3IPDRCubeCompare>::Set   V3IPDRCubeList;

// class V3IPDRFrame : Frame in Incremental PDR
// NOTE: Cubes are Intentionally Sorted for Fast Containment Check
class V3IPDRFrame
{
   public : 
      // Constructor and Destructor
      V3IPDRFrame();
      ~V3IPDRFrame();
      // Retrieval Functions
      inline const V3IPDRCubeList& getCubeList() const { return _cubeList; }
      inline const bool pushCube(V3IPDRCube* const);
      inline void clearCubeList() { _cubeList.clear(); }
      // Cube Containment Functions
      const bool subsumes(const V3IPDRCube* const) const;
      void removeSubsumed(const V3IPDRCube* const);
      void removeSubsumed(const V3IPDRCube* const, const V3IPDRCubeList::const_iterator&);
      void removeSelfSubsumed();
   private : 
      // Private Data Members
      V3IPDRCubeList _cubeList;     // List of Cubes Blocked in this Frame
};

// class V3VrfIPDR : Verification Handler for Incremental Property Directed Reachability
class V3VrfIPDR : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfIPDR(const V3NtkHandler* const);
      ~V3VrfIPDR();
      // PDR Specific Settings
      inline void setRecycle(const uint32_t& r) { _pdrRecycle = r; }
      inline void setForwardSATGen  (const bool& s) { if (s) _pdrAttr |= 1ul; else _pdrAttr &= ~1ul; }
      inline void setForwardUNSATGen(const bool& u) { if (u) _pdrAttr |= 2ul; else _pdrAttr &= ~2ul; }
   private : 
      // Private Attribute Setting Functions
      inline const bool isForwardSATGen()   const { return _pdrAttr & 1ul; }
      inline const bool isForwardUNSATGen() const { return _pdrAttr & 2ul; }
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      // PDR Initialization Functions
      void initializeSolver(const uint32_t&, const bool& = false);
      void addCubeToSolver(const uint32_t&, const V3NetVec&, const uint32_t&);
      void addLastFrameInfoToSolvers();
      void recycleSolver(const uint32_t&);
      // PDR Main Functions
      V3IPDRCube* const getInitialObligation();
      V3IPDRCube* const recursiveBlockCube(V3IPDRCube* const);
      const bool propagateCubes();
      // PDR Auxiliary Functions
      const bool checkReachability(const uint32_t&, const V3NetVec&, const bool& = true);
      const bool isBlocked(const V3IPDRTimedCube&);
      const bool existInitial(const V3NetVec&);
      V3IPDRCube* const extractModel(const uint32_t&, const V3IPDRCube* const);
      V3IPDRCube* const forwardModel(const V3IPDRCube* const);
      void resolveInitial(const uint32_t&, V3IPDRCube* const, const V3SvrDataSet&);
      void generalization(V3IPDRTimedCube&);
      void addBlockedCube(const V3IPDRTimedCube&);
      void recordCubeInputForTraceLog(V3IPDRCube* const&);
      // PDR Generalization Functions
      void generalizeSimulation(const uint32_t&, V3IPDRCube* const, const V3IPDRCube* const);
      const bool removeFromProof(V3IPDRTimedCube&);
      void generalizeProof(V3IPDRTimedCube&);
      void forwardProof(V3IPDRTimedCube&);
      const bool generalizeCex(V3IPDRTimedCube&);
      // PDR Helper Functions
      inline const uint32_t getPDRDepth() const { return _pdrFrame.size() - 1; }
      inline const uint32_t getPDRFrame() const { return _pdrFrame.size(); }
      const bool reportUnsupportedInitialState();
      // PDR Debug Functions
      void printState(const V3NetVec&) const;
      // Private Data Members
      V3IPDRFrameVec    _pdrFrame;        // List of Frames (Ri) in Incremental PDR
      V3IPDRCube*       _pdrBad;          // Cube for the Bad State (!p)
      uint32_t          _pdrSize;         // Input Size for the Instance
      unsigned char     _pdrAttr;         // Specific Attributes for MPDR
      // Recycle Members
      V3IPDRFrameVec    _pdrBackup;       // Incremental Backup of _pdrFrame
      V3UI32Vec         _pdrActBackup;    // Incremental Backup of _pdrActCount
      V3IPDRSvrList     _pdrSvrBackup;    // Incremental Backup of _pdrSvr
      uint32_t          _pdrRecycle;      // Upper Bound to Recycle
      V3UI32Vec         _pdrActCount;     // List of Activation Variable Counts
      // Private Engines
      V3IPDRSvrList     _pdrSvr;          // List of Incremental SAT Solvers
      V3AlgSimulate*    _pdrSim;          // Simulation Handler
      V3AlgGeneralize*  _pdrGen;          // Generalization Handler
      // Private Tables
      V3BoolVec         _pdrInitConst;    // Initial State of a State Variable (whether it is a const)
      V3BoolVec         _pdrInitValue;    // Initial State of a State Variable (value of the const)
      // Extended Data Members
      V3SvrData         _pdrSvrData;      // Solver Data of the Latest Activation Variable
      V3UI32Vec         _pdrPriority;     // Priority List for State Variables
      // Statistics
      V3Stat*           _totalStat;       // Total Statistic (Should be Called Only Once)
      V3Stat*           _initSvrStat;     // CNF Computation
      V3Stat*           _solveStat;       // SAT Solving
      V3Stat*           _generalStat;     // UNSAT Generalization
      V3Stat*           _propagateStat;   // Propagation
      V3Stat*           _ternaryStat;     // SAT Generalization
};

// Inline Function Implementations of Cube Setting Functions
inline void V3IPDRCube::setState(const V3NetVec& v) {
   _stateId = v; _signature = 0;
   for (uint32_t i = 0; i < _stateId.size(); ++i) _signature |= (1ul << (_stateId[i].id % 64)); }

#endif

