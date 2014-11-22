/****************************************************************************
  FileName     [ v3AlgSimulate.h ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ Algorithm for V3 Network Simulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_SIMULATE_H
#define V3_ALG_SIMULATE_H

#include "v3Usage.h"
#include "v3Bucket.h"
#include "v3AlgType.h"
#include "v3NtkHandler.h"

// class V3AlgSimulate : Base Class for V3 Ntk Simulation
class V3AlgSimulate
{
   public : 
      // Constructor and Destructor
      V3AlgSimulate(const V3NtkHandler* const);
      virtual ~V3AlgSimulate();
      // Simulation Main Functions
      virtual void simulate();
      virtual void reset(const V3NetVec& = V3NetVec());
      // Simulation Data Functions
      virtual const V3BitVecX getSimValue(const V3NetId&) const;
      // Simulation Setting Functions
      virtual void updateNextStateValue();
      virtual void setSource(const V3NetId&, const V3BitVecX&);
      virtual void setSourceFree(const V3GateType&, const bool&);
      virtual void clearSource(const V3NetId&, const bool&);
      inline const uint32_t getCycleCount() const { return _cycle; }
      // Simulation with Event-Driven Functions
      virtual void updateNextStateEvent();
      virtual void setSourceEvent(const V3NetId&, const V3BitVecX&);
      virtual void setSourceFreeEvent(const V3GateType&, const bool&);
      // Simulation Record Functions
      virtual void recordSimValue();
      virtual void getSimRecordData(uint32_t, V3SimTrace&) const;
      // Simulation Output Functions
      virtual void printResult() const;
   protected : 
      // Simulation Private Helper Function
      void resetSimulator();
      void initializeEventList();
      // Private Data Members
      const V3NtkHandler* const  _handler;
      uint32_t                   _cycle;
      uint32_t                   _level;
      uint32_t                   _init[2];
      V3NetVec                   _targets;
      V3NetVec                   _orderMap;
      V3BoolVec                  _hasEvent;
      V3UI32Vec                  _levelData;
      V3NetTable                 _fanoutVec;
      V3IncBucketList<V3NetId>   _eventList;
};

// class V3AlgAigSimulate : Base Class for V3 AIG Ntk Simulation
class V3AlgAigSimulate : public V3AlgSimulate
{
   public : 
      // Constructor and Destructor
      V3AlgAigSimulate(const V3NtkHandler* const);
      virtual ~V3AlgAigSimulate();
      // Simulation Main Functions
      void simulate();
      void reset(const V3NetVec& = V3NetVec());
      // Simulation Data Functions
      const V3BitVecX getSimValue(const V3NetId&) const;
      // Simulation Setting Functions
      void updateNextStateValue();
      void setSource(const V3NetId&, const V3BitVecX&);
      void setSourceFree(const V3GateType&, const bool&);
      void clearSource(const V3NetId&, const bool&);
      // Simulation with Event-Driven Functions
      void updateNextStateEvent();
      void setSourceEvent(const V3NetId&, const V3BitVecX&);
      void setSourceFreeEvent(const V3GateType&, const bool&);
      // Simulation Record Functions
      void recordSimValue();
      void getSimRecordData(uint32_t, V3SimTrace&) const;
      // Simulation Output Functions
      void printResult() const;
   protected : 
      // Private Event-Driven Simulation Functions
      void simulateEventList();
      // Private Data Members
      V3AigSimDataVec   _dffValue;     // Next State Simulation Value
      V3AigSimTraceVec  _traceData;    // Simulation Input Sequence
      V3AigSimDataVec   _simValue;     // Simulation Value for Ntk
      // Generalization Members
      V3AigSimRecordVec _simRecord;    // Simulation Value Record
};

// class V3AlgBvSimulate : Base Class for V3 BV Ntk Simulation
class V3AlgBvSimulate : public V3AlgSimulate
{
   public : 
      // Constructor and Destructor
      V3AlgBvSimulate(const V3NtkHandler* const);
      virtual ~V3AlgBvSimulate();
      // Simulation Main Functions
      void simulate();
      void reset(const V3NetVec& = V3NetVec());
      // Simulation Data Functions
      const V3BitVecX getSimValue(const V3NetId&) const;
      // Simulation Setting Functions
      void updateNextStateValue();
      void setSource(const V3NetId&, const V3BitVecX&);
      void setSourceFree(const V3GateType&, const bool&);
      void clearSource(const V3NetId&, const bool&);
      // Simulation with Event-Driven Functions
      void updateNextStateEvent();
      void setSourceEvent(const V3NetId&, const V3BitVecX&);
      void setSourceFreeEvent(const V3GateType&, const bool&);
      // Simulation Record Functions
      void recordSimValue();
      void getSimRecordData(uint32_t, V3SimTrace&) const;
      // Simulation Output Functions
      void printResult() const;
   protected : 
      // Private Event-Driven Simulation Functions
      void simulateEventList();
      // Private Simulation Helper Functions
      void simulateGate(const V3NetId&);
      // Private Data Members
      V3BvSimDataVec    _dffValue;     // Next State Simulation Value
      V3BvSimTraceVec   _traceData;    // Simulation Input Sequence
      V3BvSimDataVec    _simValue;     // Simulation Value for Ntk
      // Generalization Members
      V3BvSimRecordVec  _simRecord;    // Simulation Value Record
};

#endif

