/****************************************************************************
  FileName     [ v3StgExtract.h ]
  PackageName  [ v3/src/stg ]
  Synopsis     [ STG Extraction Interface for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_STG_EXTRACT_H
#define V3_STG_EXTRACT_H

#include "v3Graph.h"
#include "v3StgFSM.h"

// Defines
typedef V3Vec<V3FSM*>::Vec          V3FSMVec;
typedef V3Vec<V3FSMSDGDB*>::Vec     V3FSMSDGDBList;

// class V3FSMExtract : FSM Extraction Handler
class V3FSMExtract {
   public : 
      // Constructor and Destructor
      V3FSMExtract(V3NtkHandler* const, const V3NetVec&);
      V3FSMExtract(V3NtkHandler* const, const string&, const V3NetVec&);
      V3FSMExtract(V3NtkHandler* const, const V3NetVec&, const V3NetVec&);
      ~V3FSMExtract();
      // Inline Interface Functions
      inline V3NtkHandler* const getNtkHandler() const { return _handler; }
      inline const uint32_t getVariableSize() const { return _depGraph.getNodeSize(); }
      inline const uint32_t getClusterSize() const { return _clusters.size(); }
      inline const uint32_t getFSMListSize() const { return _fsmList.size(); }
      inline const uint32_t getFSMSDGDBSize() const { return _sdgDBList.size(); }
      inline const V3UI32Set& getCluster(const uint32_t& i) const { assert (i < _clusters.size()); return _clusters[i]; }
      inline V3FSM* const getFSM(const uint32_t& i) const { assert (i < _fsmList.size()); return _fsmList[i]; }
      // Inline Variable Clustering Configuration Functions
      inline void setVariableClusterBySCC()     { assert (!(_configs & 3ul  )); _configs |= 1ul;   }
      inline void setInvariantDirectedCluster() { assert (!(_configs & 3ul  )); _configs |= 2ul;   }
      inline void filterOutNoMuxVariable()      { assert (!(_configs & 4ul  )); _configs |= 4ul;   }
      inline void filterOutNonSeqVariable()     { assert (!(_configs & 8ul  )); _configs |= 8ul;   }
      // Inline FSM Extraction Configuration Functions
      inline void enableOmitNodesAllCombVars()  { assert (!(_configs & 16ul )); _configs |= 16ul;  }
      inline void enableOmitNodesIndepToVars()  { assert (!(_configs & 32ul )); _configs |= 32ul;  }
      inline void enableRenderNonEmptyStates()  { assert (!(_configs & 64ul )); _configs |= 64ul;  }
      inline void enableStopOnAllInitOrTerms()  { assert (!(_configs & 128ul)); _configs |= 128ul; }
      inline void enableConstructStandardFSM()  { assert (!(_configs & 256ul)); _configs |= 256ul; }
      inline void setMaxStateSize(const uint32_t& s) { assert (!_depGraph.getNodeSize()); _maxSize = s; }
      inline void setMaxTimeUsage(const double& t) { assert (t >= 0); _maxTime = t; }
      inline void pushTerminalState(const V3NetId& id) { _terminals.push_back(id); }
      // Main Functions
      void startExtractFSMs();
      // Debug Functions
      const bool checkCompleteFSMs() const;
      // Output Functions
      void writeClusterResult(const string&) const;
      void writeFSM(const string&, V3FSM* const) const;
   private : 
      typedef V3Vec<V3UI32Set>::Vec    V3SDGClusters;
      // Inline Member Functions
      inline const bool isVariableClusterBySCC()       const { return _configs & 1ul;   }
      inline const bool isInvariantDirectedCluster()   const { return _configs & 2ul;   }
      inline const bool isFilterOutNoMuxVariable()     const { return _configs & 4ul;   }
      inline const bool isFilterOutNonSeqVariable()    const { return _configs & 8ul;   }
      inline const bool isEnableOmitNodesAllCombVars() const { return _configs & 16ul;  }
      inline const bool isEnableOmitNodesIndepToVars() const { return _configs & 32ul;  }
      inline const bool isEnableRenderNonEmptyStates() const { return _configs & 64ul;  }
      inline const bool isEnableStopOnAllInitOrTerms() const { return _configs & 128ul; }
      inline const bool isEnableConstructStandardFSM() const { return _configs & 256ul; }
      // Private Member Functions
      void constructInputFSMSDG(V3NtkHandler* const, const string&);
      void constructStandardFSMSDG();
      void initializeDepGraph();
      void startClusterVariables();
      void startExtractClusteredFSMs();
      void startExtractInvariantFSMs();
      // Private Data Members
      V3NtkHandler* const  _handler;   // Network that Extraction Works on
      V3SDG* const         _sdgMain;   // Main SDG for Variable Clustering
      unsigned short       _configs;   // Configurations
      uint32_t             _maxSize;   // Max Allowed State Size for Each FSM
      V3NetVec             _terminals; // List of Terminal (Bug) States
      V3Graph<uint32_t>    _depGraph;  // Variable Dependency Graph
      V3SDGClusters        _clusters;  // Variable Cluster Results
      V3BoolVec            _depRetain; // Retained Depdendent Variables
      V3FSMVec             _fsmList;   // List Extracted FSMs from Each Cluster
      V3FSMSDGDBList       _sdgDBList; // List of FSMSDGDB from Each Cluster
      double               _maxTime;   // Maximum Time Usage on FSM Extraction
      double               _curTime;   // Time Used Currently on FSM Extraction
};

#endif

