/****************************************************************************
  FileName     [ v3SvrMiniSat.h ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ V3 Solver with MiniSAT as Engine. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_MSAT_H
#define V3_SVR_MSAT_H

#include "v3SvrBase.h"

// V3SvrMiniSat : V3 Solver with MiniSAT as Engine
class V3SvrMiniSat : public V3SvrBase
{
   public : 
      // Constructor and Destructor
      V3SvrMiniSat(const V3Ntk* const, const bool& = false);
      V3SvrMiniSat(const V3SvrMiniSat&);
      ~V3SvrMiniSat();
      // Basic Operation Functions
      void reset();
      void update();
      void assumeInit();
      void assertInit();
      void initRelease();
      void assumeRelease();
      void assumeProperty(const size_t&, const bool& = false);
      void assertProperty(const size_t&, const bool& = false);
      void assumeProperty(const V3NetId&, const bool&, const uint32_t&);
      void assertProperty(const V3NetId&, const bool&, const uint32_t&);
      const bool simplify();
      const bool solve();
      const bool assump_solve();
      // Manipulation Helper Functions
      void setTargetValue(const V3NetId&, const V3BitVecX&, const uint32_t&, V3SvrDataVec&);
      void assertImplyUnion(const V3SvrDataVec&);
      const size_t setTargetValue(const V3NetId&, const V3BitVecX&, const uint32_t&, const size_t&);
      const size_t setImplyUnion(const V3SvrDataVec&);
      const size_t setImplyIntersection(const V3SvrDataVec&);
      const size_t setImplyInit();
      // Retrieval Functions
      const V3BitVecX getDataValue(const V3NetId&, const uint32_t&) const;
      const bool getDataValue(const size_t&) const;
      void getDataConflict(V3SvrDataVec&) const;
      const size_t getFormula(const V3NetId&, const uint32_t&);
      const size_t getFormula(const V3NetId&, const uint32_t&, const uint32_t&);
      // Variable Interface Functions
      inline const size_t reserveFormula() { return getPosVar(newVar(1)); }
      inline const bool isNegFormula(const size_t& v) const { return (v & 1ul); }
      inline const size_t getNegFormula(const size_t& v) const { return (v ^ 1ul); }
      // Print Data Functions
      void printInfo() const;
      void printVerbose() const;
      // Resource Functions
      const double getTime() const;
      const int getMemory() const;
      // Gate Formula to Solver Functions
      void add_FALSE_Formula(const V3NetId&, const uint32_t&);
      void add_PI_Formula(const V3NetId&, const uint32_t&);
      void add_FF_Formula(const V3NetId&, const uint32_t&);
      void add_AND_Formula(const V3NetId&, const uint32_t&);
      void add_XOR_Formula(const V3NetId&, const uint32_t&);
      void add_MUX_Formula(const V3NetId&, const uint32_t&);
      void add_RED_AND_Formula(const V3NetId&, const uint32_t&);
      void add_RED_OR_Formula(const V3NetId&, const uint32_t&);
      void add_RED_XOR_Formula(const V3NetId&, const uint32_t&);
      void add_ADD_Formula(const V3NetId&, const uint32_t&);
      void add_SUB_Formula(const V3NetId&, const uint32_t&);
      void add_MULT_Formula(const V3NetId&, const uint32_t&);
      void add_DIV_Formula(const V3NetId&, const uint32_t&);
      void add_MODULO_Formula(const V3NetId&, const uint32_t&);
      void add_SHL_Formula(const V3NetId&, const uint32_t&); 
      void add_SHR_Formula(const V3NetId&, const uint32_t&); 
      void add_CONST_Formula(const V3NetId&, const uint32_t&);
      void add_SLICE_Formula(const V3NetId&, const uint32_t&);
      void add_MERGE_Formula(const V3NetId&, const uint32_t&);
      void add_EQUALITY_Formula(const V3NetId&, const uint32_t&);
      void add_GEQ_Formula(const V3NetId&, const uint32_t&);
      // Network to Solver Functions
      const bool existVerifyData(const V3NetId&, const uint32_t&);
   private : 
      // MiniSat Functions
      const Var newVar(const uint32_t&);
      const Var getVerifyData(const V3NetId&, const uint32_t&) const;
      // Helper Functions : Transformation Between Internal and External Representations
      inline const Var getOriVar(const size_t& v) const { return (Var)(v >> 1ul); }
      inline const size_t getPosVar(const Var& v) const { return (((size_t)v) << 1ul); }
      inline const size_t getNegVar(const Var& v) const { return ((getPosVar(v)) | 1ul); }
      // Data Members
      MSolver*       _Solver;    // Pointer to a Minisat solver
      Var            _curVar;    // Latest Fresh Variable
      vec<Lit>       _assump;    // Assumption List for assumption solve
      V3SvrMLitData  _init;      // Initial state Var storage
      V3SvrMVarTable _ntkData;   // Mapping between V3NetId and Solver Data
};

#endif

