/****************************************************************************
  FileName     [ v3VrfFITP.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ A Counterexample-Guided Interpolant Generation Algorithm
                 for SAT-based Model Checking (Reversed NewITP) on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_FITP_H
#define V3_VRF_FITP_H

#include "v3SvrType.h"
#include "v3VrfBase.h"
#include "v3AlgGeneralize.h"

// Forward Declaration
class V3FITPCube;
class V3FITPFrame;

// Defines for FITP Global Data
typedef V3Vec<V3FITPFrame*>::Vec          V3FITPFrameVec;
typedef V3Vec<size_t>::Vec                V3FITPSvrData;

// class V3FITPCube : Cube Data Structue for FITP
// NOTE: State Variables are Intrinsically Sorted by their indices (small to large)
class V3FITPCube
{
   public : 
      V3FITPCube(const V3FITPCube* const c) : _nextCube(c) { _stateId.clear(); _signature = 0; }
      V3FITPCube(const V3FITPCube& c) : _nextCube(0) { _stateId = c._stateId; _signature = c._signature; }
      ~V3FITPCube() { _stateId.clear(); }
      // Cube Setting Functions
      inline void setState(const V3NetVec&);
      inline void clearState() { _signature = 0; }
      inline const bool isEmpty() const { return !_signature; }
      inline const V3NetVec& getState() const { return _stateId; }
      inline const uint64_t getSignature() const { return _signature; }
      // Sucessor Logging Functions
      inline const V3FITPCube* const getNextCube() const { return _nextCube; }
   private : 
      // Private Cube Data
      V3NetVec                _stateId;      // Cut Signal Index (id) with Value (cp)
      uint64_t                _signature;    // Subsumption Marker
      // Sucessor Logging Functions
      const V3FITPCube* const _nextCube;     // Successor State in the Successor Interpolant
};

// Defines for Sorted FITP Cube List
struct V3FITPCubeCompare {
   const bool operator() (const V3FITPCube* const c1, const V3FITPCube* const c2) const {
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

typedef V3Set<V3FITPCube*, V3FITPCubeCompare>::Set   V3FITPCubeList;

// class V3FITPFrame : Frame in FITP
// NOTE: Cubes are Intentionally Sorted for Fast Containment Check
class V3FITPFrame
{
   public : 
      // Constructor and Destructor
      V3FITPFrame();
      ~V3FITPFrame();
      // Retrieval Functions
      inline V3FITPCubeList& getCubeList() { return _cubeList; }
      inline const bool pushCube(V3FITPCube* const);
      // Cube Containment Functions
      void removeSelfSubsumed();
   private : 
      // Private Cube Containment Functions
      void removeSubsumed(const V3FITPCube* const, const V3FITPCubeList::const_iterator&);
      // Private Data Members
      V3FITPCubeList _cubeList;     // List of Cubes Blocked in this Frame
};

// class V3VrfFITP : Verification Handler for FITP
class V3VrfFITP : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfFITP(const V3NtkHandler* const);
      ~V3VrfFITP();
      // FITP Specific Settings
      inline void setBlockBadCount(const uint32_t& m = V3NtkUD) { _itpBadCount = m; }
      inline void setIncrementDepth     (const bool& f) { if (f) _itpAttr |= 1ul;  else _itpAttr &= ~1ul;  }
      inline void setForceUnreachable   (const bool& f) { if (f) _itpAttr |= 2ul;  else _itpAttr &= ~2ul;  }
      inline void setBlockBadCountIndep (const bool& f) { if (f) _itpAttr |= 4ul;  else _itpAttr &= ~4ul;  }
      inline void setNoIncBySharedBound (const bool& d) { if (d) _itpAttr |= 8ul;  else _itpAttr &= ~8ul;  }
      inline void setRecycleInterpolants(const bool& f) { if (f) _itpAttr |= 16ul; else _itpAttr &= ~16ul; }
   private : 
      // Private Attribute Setting Functions
      inline const bool isIncrementDepth()      const { return _itpAttr & 1ul;  }
      inline const bool isForceUnreachable()    const { return _itpAttr & 2ul;  }
      inline const bool isBlockBadCountIndep()  const { return _itpAttr & 4ul;  }
      inline const bool isNoIncBySharedBound()  const { return _itpAttr & 8ul;  }
      inline const bool isRecycleInterpolants() const { return _itpAttr & 16ul; }
      // Private Verification Main Functions
      void startVerify(const uint32_t&);
      // FITP Initialization Functions
      void initializeBMCSolver(const uint32_t&, const uint32_t& = 1);
      void initializeITPSolver();
      // FITP Main Functions
      const int blockNewCube();
      const bool checkCounterExample();
      // FITP Auxiliary Functions
      const bool checkReachability(const V3NetVec&);
      const bool checkBMCReachability(const V3NetVec&);
      V3FITPCube* const extractNewCube(V3FITPCube* const);
      const bool resolveInitial(V3FITPCube* const, const V3SvrDataSet&);
      void addBlockedCube(V3FITPCube* const);
      void blockBadCubes(const uint32_t&);
      // FITP Generalization Functions
      const bool setBlockingStruct(const V3NetVec&);
      const bool setBlockingStruct(const V3NetVec&, const V3UI32Vec&);
      const V3NetVec generalizeSimulation(const V3NetVec&);
      const bool removeFromProof(V3FITPCube* const);
      const bool generalizeProof(V3FITPCube* const);
      // FITP Debug Functions
      void printState(const V3NetVec&) const;
      // Private Data Members
      V3FITPFrameVec    _itpFrame;        // List of Frames (Ri') in FITP
      V3FITPFrameVec    _itpReuse;        // List of Frames Computed Before
      V3FITPCube*       _itpBad;          // Cube for the Bad State (I0)
      unsigned char     _itpAttr;         // Specific Attributes for FITP
      uint32_t          _itpBMCDepth;     // The Number of Unrolled Frames in BMC Solver
      uint32_t          _itpBadCount;     // Max Bad Cubes to be Blocked for a Given Cube
      // Private Engines
      V3SvrBase*        _itpBMCSvr;       // The BMC SAT Solver (Counterexample Confirm)
      V3SvrBase*        _itpITPSvr;       // The ITP SAT Solver (Reachability Containment)
      V3GenStruct*      _itpGSt;          // Generalization Struct
      V3AlgSimulate*    _itpSim;          // Simulation Handler
      V3AlgGeneralize*  _itpGen;          // Generalization Handler
      // Extended Data Members
      V3UI32Vec         _itpPriority;     // Priority List for Cut Signals
      V3SvrData         _itpForceUAct;    // Activation Variable to Force Unreachability
      V3NetTable        _itpBlockCubes;   // Blocking Cubes for Interpolants
      V3StrSet          _itpBadCubeStr;   // Expressions for Pending Bad Cubes
      // Statistics
      V3Stat*           _totalStat;       // Total Statistic (Should be Called Only Once)
      V3Stat*           _initSvrStat;     // CNF Computation
      V3Stat*           _solveStat;       // SAT Solving
      V3Stat*           _generalStat;     // UNSAT Generalization
      V3Stat*           _ternaryStat;     // SAT Generalization
};

// Inline Function Implementations of Cube Setting Functions
inline void V3FITPCube::setState(const V3NetVec& v) {
   _stateId = v; _signature = 0;
   for (uint32_t i = 0; i < _stateId.size(); ++i) _signature |= (1ul << (_stateId[i].id % 64)); }

#endif

