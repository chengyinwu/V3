/****************************************************************************
  FileName     [ v3VrfMPDR.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Monolithic Property Directed Reachability on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_MPDR_H
#define V3_VRF_MPDR_H

#include "v3SvrType.h"
#include "v3VrfBase.h"
#include "v3AlgGeneralize.h"

// Forward Declaration
class V3MPDRCube;
class V3MPDRFrame;

// Defines for Monolithic PDR Global Data
typedef pair<uint32_t, V3MPDRCube*>       V3MPDRTimedCube;
typedef V3Vec<V3MPDRFrame*>::Vec          V3MPDRFrameVec;

// class V3MPDRCube : Cube Data Structue for Monolithic PDR
// NOTE: State Variables are Intrinsically Sorted by their indices (small to large)
class V3MPDRCube
{
   public : 
      V3MPDRCube(const V3MPDRCube* const c) : _nextCube(c) { _stateId.clear(); _signature = 0; }
      ~V3MPDRCube() { _stateId.clear(); }
      // Cube Setting Functions
      inline void setState(const V3NetVec&);
      inline const V3NetVec& getState() const { return _stateId; }
      inline const uint64_t getSignature() const { return _signature; }
      // Trace Logging Functions
      inline void setInputData(const V3BitVecX& v) { _inputData = v; }
      inline const V3MPDRCube* const getNextCube() const { return _nextCube; }
      inline const V3BitVecX& getInputData() const { return _inputData; }
   private : 
      // Private Cube Data
      V3NetVec                _stateId;      // State Variable Index (id) with Value (cp)
      uint64_t                _signature;    // Subsumption Marker
      // Trace Logging Members
      V3BitVecX               _inputData;    // Primary Input / Inout Values
      const V3MPDRCube* const _nextCube;     // Successor Proof Obligation
};

// Defines for Sorted Monolithic PDR Cube List
struct V3MPDRCubeCompare {
   const bool operator() (const V3MPDRCube* const c1, const V3MPDRCube* const c2) const {
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

typedef V3Set<V3MPDRCube*, V3MPDRCubeCompare>::Set   V3MPDRCubeList;

// class V3MPDRFrame : Frame in Monolithic PDR
// NOTE: Cubes are Intentionally Sorted for Fast Containment Check
class V3MPDRFrame
{
   public : 
      // Constructor and Destructor
      V3MPDRFrame(const V3SvrData&);
      ~V3MPDRFrame();
      // Retrieval Functions
      inline const V3SvrData& getActivator() const { return _activator; }
      inline const V3MPDRCubeList& getCubeList() const { return _cubeList; }
      inline const bool pushCube(V3MPDRCube* const);
      inline void clearCubeList() { _cubeList.clear(); }
      // Cube Containment Functions
      const bool subsumes(const V3MPDRCube* const) const;
      void removeSubsumed(const V3MPDRCube* const);
      void removeSubsumed(const V3MPDRCube* const, const V3MPDRCubeList::const_iterator&);
      void removeSelfSubsumed();
      // Recycle Auxiliary Functions
      inline void setActivator(const V3SvrData& act) { _activator = act; }
   private : 
      // Private Data Members
      V3SvrData      _activator;    // Activation Variable for Clause Indication
      V3MPDRCubeList _cubeList;     // List of Cubes Blocked in this Frame
};

// class V3VrfMPDR : Verification Handler for Monolithic Property Directed Reachability
class V3VrfMPDR : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfMPDR(const V3NtkHandler* const);
      ~V3VrfMPDR();
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
      void initializeSolver();
      void addCubeToSolver(const V3NetVec&, const uint32_t&);
      void addFrameInfoToSolver(const uint32_t&);
      void recycleSolver();
      // PDR Main Functions
      V3MPDRCube* const getInitialObligation();
      V3MPDRCube* const recursiveBlockCube(V3MPDRCube* const);
      const bool propagateCubes();
      // PDR Auxiliary Functions
      const bool checkReachability(const uint32_t&, const V3NetVec&, const bool& = true);
      const bool isBlocked(const V3MPDRTimedCube&);
      const bool existInitial(const V3NetVec&);
      void assumeReachability(const uint32_t&);
      V3MPDRCube* const extractModel(const V3MPDRCube* const, const uint32_t&);
      V3MPDRCube* const forwardModel(const V3MPDRCube* const);
      void resolveInitial(V3MPDRCube* const, const V3SvrDataSet&);
      void generalization(V3MPDRTimedCube&);
      void addBlockedCube(const V3MPDRTimedCube&);
      void recordCubeInputForTraceLog(V3MPDRCube* const&);
      // PDR Generalization Functions
      void generalizeSimulation(V3MPDRCube* const, const V3MPDRCube* const, const uint32_t&);
      const bool removeFromProof(V3MPDRTimedCube&, const bool& = true);
      void generalizeProof(V3MPDRTimedCube&);
      void forwardProof(V3MPDRTimedCube&);
      const bool generalizeCex(V3MPDRTimedCube&);
      // PDR Helper Functions
      inline const uint32_t getPDRDepth() const { return _pdrFrame.size() - 2; }
      inline const uint32_t getPDRFrame() const { return _pdrFrame.size() - 1; }
      const bool reportUnsupportedInitialState();
      // PDR Debug Functions
      void printState(const V3NetVec&) const;
      // Private Data Members
      V3MPDRFrameVec    _pdrFrame;        // List of Frames (Ri) in Monolithic PDR
      V3MPDRCube*       _pdrBad;          // Cube for the Bad State (!p)
      uint32_t          _pdrSize;         // Input Size for the Instance
      unsigned char     _pdrAttr;         // Specific Attributes for MPDR
      // Recycle Members
      V3MPDRFrameVec    _pdrBackup;       // Backup Frames for Reachability Reuse
      uint32_t          _pdrRecycle;      // Upper Bound to Recycle
      uint32_t          _pdrActCount;     // Activation Variable Count
      // Private Engines
      V3SvrBase*        _pdrSvr;          // The Monolithic SAT Solver
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
inline void V3MPDRCube::setState(const V3NetVec& v) {
   _stateId = v; _signature = 0;
   for (uint32_t i = 0; i < _stateId.size(); ++i) _signature |= (1ul << (_stateId[i].id % 64)); }

#endif

