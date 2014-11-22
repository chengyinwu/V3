/****************************************************************************
  FileName     [ v3MCMain.h ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Model Checker. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_MAIN_H
#define V3_MC_MAIN_H

#include "v3VrfBase.h"
#include "v3MCEngine.h"
#include "v3NtkInput.h"
#include "v3NtkElaborate.h"

// Toggle Interactive Status Report
static const bool v3MCInteractive = false;

// Toggle Verification Result Check
static const bool v3MCResultCheck = false;

// Define
typedef V3Vec<V3MCEngineType>::Vec        V3MCEngineList;

// Enumeration of Main Thread Type
enum V3MCMainType { V3MC_SAFE = 0, V3MC_LIVE, V3MC_L2S, V3MC_TYPE_TOTAL };
const string V3MCMainTypeStr[] = { "V3MC_SAFE", "V3MC_LIVE", "V3MC_L2S", "V3MC_TYPE_TOTAL" };

// class V3MCResource : Resource Management for Main Threads
class V3MCResource : public V3VrfShared
{
   public :
      // Constructor and Destructors
      V3MCResource(const V3UI32Vec&, const V3UI32Vec&);
      ~V3MCResource();
      // Member Functions
      const bool isCompleted();
      void reallocNumOfCores();
      const uint32_t getActiveMainSize();
      const uint32_t getTotalOccupiedCores();
      const bool updateRemaining(const V3MCMainType&, const uint32_t&);
      const uint32_t getAvailableCores(const V3MCMainType&);
      const uint32_t getOccupiedCores(const V3MCMainType&);
      const bool incOccupiedCores(const V3MCMainType&);
      void decOccupiedCores(const V3MCMainType&);
   private :
      // Private Data Members
      V3UI32Vec               _remaining;    // Remaining Properties
      V3UI32Vec               _usedCores;    // Occupied Core Numbers
      V3UI32Vec               _numOfCores;   // Total Available Cores
      uint32_t                _emptyCores;   // Number of Empty Cores
      V3DblVec                _cpRatio;      // Ratio of Cores to Property Size
};

// class V3MCResult : Verification Results
class V3MCResult : public V3VrfShared
{
   public :
      // Constructor and Destructor
      V3MCResult(const uint32_t&);
      ~V3MCResult();
      // Public Member Functions
      const bool updateResult(const uint32_t&, const V3VrfResult&);
      inline void getResult(V3VrfResultVec& result) { lock(); result = _result; unlock(); }
   private :
      // Private Data Members
      V3VrfResultVec    _result;    // Verification results (i.e. cex or inv, will be deleted after output)
};

// class V3MCMainParam : Parameters for Model Checking Main Threads
struct V3MCMainParam
{
   // Data Members of Verification Instances
   V3NtkHandler*        _handler;      // Base Elaborated Network Handler
   V3NtkHandler*        _baseNtk;      // The Very Base Network Handler
   V3NetTable           _constr;       // Table of Constraints
   V3MCMainType         _type;         // Type of the Main Thread
   // Data Member of Resources
   V3VrfSharedBound*    _sharedBound;  // Shared Bound of Checkers
   V3VrfSharedBound*    _mirrorBound;  // Mirror Bound of Checkers
   V3VrfSharedInv*      _sharedInv;    // Shared Cube Invariants
   V3VrfSharedNtk*      _sharedNtk;    // Shared Network Data
   V3VrfSharedMem*      _sharedMem;    // Shared Memory Usage
   V3MCResource*        _res;          // Resource Data
   struct timeval       _initTime;     // Initial Time
   double               _maxTime;      // Runtime Limit
   double               _maxMemory;    // Total Available Memory
   // Data Members for Reports
   uint32_t             _inputSize;    // Number of Inputs in the Original Network
   uint32_t             _latchSize;    // Number of Latches in the Original Network
   // Data Members for Results
   V3MCResult*          _result;       // Verification Results
};

// class V3MCParam : Parameters for Model Checkers
struct V3MCParam
{
   V3MCMainParam*       _mainParam;    // Parameter of the Main Thread
   V3MCEngineType       _engineType;   // Type of Verification Engine
   uint32_t             _threadIdx;    // Index of the Checker Thread
};

// V3 Verification Main Functions  (v3MCMain.cpp)
void* startVerificationMain(void*);

// V3 Verification Engine Functions  (v3MCEngine.cpp)
V3VrfBase* getChecker(const V3MCEngineType&, V3NtkHandler* const, const uint32_t&);
void getDefaultCheckerList(const V3MCMainType&, const uint32_t&, V3MCEngineList&);

// V3 Verification Property Reordering Functions  (v3MCReorder.cpp)
void reorderPropertyByNetId(V3NtkHandler* const, V3NetTable&, const bool&);
void reorderPropertyByNetLevel(V3NtkHandler* const, V3NetTable&, const bool&);
void reorderPropertyByCOI(V3NtkHandler* const, V3NetTable&, const bool&);

// V3 Property Specification Handling Functions  (v3MCProp.cpp)
V3NtkHandler* const readProperty(V3NtkHandler* const, const string&);
void writeProperty(V3NtkHandler* const, const string&, const bool&, const bool&);
void startVerificationFromProp(V3NtkHandler* const, const uint32_t&, const double&, const double&);

// V3 Verification Instance Input Functions  (v3MCAiger.cpp)
V3NtkInput* readAiger(const string&, V3NetVec&, V3NetVec&, V3NetTable&, const bool&);
void writeAiger(V3NtkHandler* const, const string&, const bool&, const bool&);

// V3 Verification Result Report Functions  (v3MCReport.cpp)
void reportAigerCex(const uint32_t&, const uint32_t&, const V3CexTrace&, const string&);
void reportAigerInv(const uint32_t&, const uint32_t&, const string&);

// V3 Vernfication Instance Elaboration Functions  (v3MCElaborate.cpp)
void mergeFairnessConstraints(V3Ntk* const, V3NetTable&);
void combineConstraintsToOutputs(V3Ntk* const, const V3UI32Vec&, const V3NetVec&);
V3NtkHandler* const elaborateSafetyNetwork(V3NtkHandler* const, const V3UI32Vec&, V3NetTable&);
V3NtkHandler* const elaborateLivenessNetwork(V3NtkHandler* const, const V3UI32Vec&, V3NetTable&, V3NetTable&);
V3NtkElaborate* const elaborateProperties(V3NtkHandler* const, V3StrVec&, V3UI32Vec&, V3UI32Table&, V3UI32Table&,
                                          const bool&, const bool&, const bool&, const bool&);

#endif

