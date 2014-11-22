/****************************************************************************
  FileName     [ v3DfxTrace.h ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Counterexample Trace Manipulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_TRACE_H
#define V3_DFX_TRACE_H

#include "v3VrfCITP.h"
#include "v3NtkHandler.h"

// class V3DfxCube : Cube Data Structue for Counterexample Optimization
// NOTE: State Variables are Intrinsically Sorted by their Cut Signal Indices (less)
class V3DfxCube
{
   public : 
      V3DfxCube(const uint32_t& f) : _frameId(f) { _stateId.clear(); _signature = 0; }
      ~V3DfxCube() { _stateId.clear(); }
      // Cube Setting Functions
      inline void setState(const V3NetVec&);
      inline const V3NetVec& getState() const { return _stateId; }
      inline const uint64_t getSignature() const { return _signature; }
      inline const uint32_t getFrameId() const { return _frameId; }
   private : 
      // Private Cube Data
      const uint32_t          _frameId;      // Counterexample Frame of the Cube
      V3NetVec                _stateId;      // Cut Signal Index (id) with Value (cp)
      uint64_t                _signature;    // Subsumption Marker
};

// Inline Function Implementations of Cube Setting Functions
inline void V3DfxCube::setState(const V3NetVec& v) {
   _stateId = v; _signature = 0;
   for (uint32_t i = 0; i < _stateId.size(); ++i) _signature |= (1ul << (_stateId[i].id % 64)); }

// Defines for Sorted Dfx Cube List
struct V3DfxCubeCompare {
   const bool operator() (const V3DfxCube* const s1, const V3DfxCube* const s2) const {
      const V3NetVec& state1 = s1->getState();
      const V3NetVec& state2 = s2->getState();
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

typedef V3Set<V3DfxCube*, V3DfxCubeCompare>::Set      V3DfxCubeList;

// Counterexample Trace Utility Functions
void computeTraceData(V3NtkHandler* const, const V3CexTrace&, V3SimTraceVec&, V3SimTraceVec&);

// Counterexample Trace Optimization Functions
void performTraceReduction(V3NtkHandler* const, V3Property* const);
void performTraceGeneralization(V3NtkHandler* const, V3Property* const);

#endif

