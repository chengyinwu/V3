/****************************************************************************
  FileName     [ v3VrfCITP.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ A Counterexample-Guided Interpolant Generation Algorithm
                 for SAT-based Model Checking (NewITP) on V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_CITP_H
#define V3_VRF_CITP_H

#include "v3SvrType.h"
#include "v3VrfBase.h"
#include "v3AlgGeneralize.h"

// Forward Declaration
class V3CITPCube;
class V3CITPFrame;

// Defines for CITP Global Data
typedef V3Vec<V3CITPFrame*>::Vec          V3CITPFrameVec;

// class V3CITPCube : Cube Data Structue for CITP
// NOTE: State Variables are Intrinsically Sorted by their indices (small to large)
class V3CITPCube
{
   public : 
      V3CITPCube(const V3CITPCube* const c) : _nextCube(c) { _stateId.clear(); _signature = 0; }
      V3CITPCube(const V3CITPCube& c) : _nextCube(0) { _stateId = c._stateId; _signature = c._signature; }
      ~V3CITPCube() { _stateId.clear(); }
      // Cube Setting Functions
      inline void setState(const V3NetVec&);
      inline void clearState() { _signature = 0; }
      inline const bool isEmpty() const { return !_signature; }
      inline const V3NetVec& getState() const { return _stateId; }
      inline const uint64_t getSignature() const { return _signature; }
      // Sucessor Logging Functions
      inline const V3CITPCube* const getNextCube() const { return _nextCube; }
   private : 
      // Private Cube Data
      V3NetVec                _stateId;      // Cut Signal Index (id) with Value (cp)
      uint64_t                _signature;    // Subsumption Marker
      // Sucessor Logging Functions
      const V3CITPCube* const _nextCube;     // Successor State in the Successor Interpolant
};

// Defines for Sorted CITP Cube List
struct V3CITPCubeCompare {
   const bool operator() (const V3CITPCube* const c1, const V3CITPCube* const c2) const {
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

typedef V3Set<V3CITPCube*, V3CITPCubeCompare>::Set   V3CITPCubeList;

// class V3CITPFrame : Frame in CITP
// NOTE: Cubes are Intentionally Sorted for Fast Containment Check
class V3CITPFrame
{
   public : 
      // Constructor and Destructor
      V3CITPFrame();
      ~V3CITPFrame();
      // Retrieval Functions
      inline V3CITPCubeList& getCubeList() { return _cubeList; }
      inline const bool pushCube(V3CITPCube* const);
      inline void clearCubeList() { _cubeList.clear(); }
      // Cube Containment Functions
      void removeSelfSubsumed();
   private : 
      // Private Cube Containment Functions
      void removeSubsumed(const V3CITPCube* const, const V3CITPCubeList::const_iterator&);
      // Private Data Members
      V3CITPCubeList _cubeList;     // List of Cubes Blocked in this Frame
};

// class V3VrfCITP : Verification Handler for CITP
class V3VrfCITP : public V3VrfBase
{
   public : 
      // Constructor and Destructor
      V3VrfCITP(const V3NtkHandler* const);
      ~V3VrfCITP();
      // CITP Specific Settings
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
      // CITP Initialization Functions
      void initializeBMCSolver(const uint32_t&, const uint32_t& = 1);
      void initializeITPSolver();
      // CITP Main Functions
      const int blockNewCube();
      const bool checkCounterExample();
      // CITP Auxiliary Functions
      const bool checkReachability(V3CITPCube* const);
      const bool checkBMCReachability(const V3NetVec&);
      V3CITPCube* const extractNewCube(V3CITPCube* const);
      const bool resolveInitial(V3CITPCube* const, const V3SvrDataSet&);
      void addBlockedCube(V3CITPCube* const);
      void blockBadCubes(const uint32_t&);
      // CITP Generalization Functions
      const bool setBlockingStruct(const V3NetVec&);
      const bool setBlockingStruct(const V3NetVec&, const V3UI32Vec&);
      void generalizeSimulation(V3CITPCube* const, const V3CITPCube* const);
      const bool removeFromProof(V3CITPCube* const);
      const bool generalizeProof(V3CITPCube* const);
      // CITP Debug Functions
      void printState(const V3NetVec&) const;
      const string getStateStr(const V3NetVec&) const;
      // Private Data Members
      V3CITPFrameVec    _itpFrame;        // List of Frames (Ri') in CITP
      V3CITPFrameVec    _itpReuse;        // List of Frames Computed Before
      V3CITPCube*       _itpBad;          // Cube for the Bad State (!p)
      unsigned char     _itpAttr;         // Specific Attributes for CITP
      uint32_t          _itpBMCDepth;     // The Number of Unrolled Frames in BMC Solver
      uint32_t          _itpBadCount;     // Max Bad Cubes to be Blocked for a Given Cube
      // Private Engines
      V3SvrBase*        _itpBMCSvr;       // The BMC SAT Solver (Counterexample Confirm)
      V3SvrBase*        _itpITPSvr;       // The ITP SAT Solver (Reachability Containment)
      V3GenStruct*      _itpGSt;          // Generalization Struct
      V3AlgSimulate*    _itpSim;          // Simulation Handler
      V3AlgGeneralize*  _itpGen;          // Generalization Handler
      // Extended Data Members
      V3UI32Vec         _itpPriority;     // Priority List for State Variables
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
inline void V3CITPCube::setState(const V3NetVec& v) {
   _stateId = v; _signature = 0;
   for (uint32_t i = 0; i < _stateId.size(); ++i) _signature |= (1ul << (_stateId[i].id % 64)); }

#endif

