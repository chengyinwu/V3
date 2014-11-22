/****************************************************************************
  FileName     [ v3SvrBoolector.h ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ V3 Solver with Boolector as Engine. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_BOOLECTOR_H
#define V3_SVR_BOOLECTOR_H

#include "v3SvrBase.h"

// V3SvrBoolector : V3 Solver with Boolector as Engine
class V3SvrBoolector : public V3SvrBase
{
   public : 
      // Constructor and Destructor
      V3SvrBoolector(const V3Ntk* const, const bool& = false);
      V3SvrBoolector(const V3SvrBoolector&);
      ~V3SvrBoolector();
      // Basic Operation Functions
      void reset();
      void update();
      void assumeInit();
      void assertInit();
      void initRelease();
      void assumeRelease();
      void assumeProperty(const size_t&, const bool& = false);
      void assertProperty(const size_t&, const bool& = false);
      void assumeProperty(const V3NetId&, const bool&, const unsigned&);
      void assertProperty(const V3NetId&, const bool&, const unsigned&);
      const bool simplify();
      const bool solve();
      const bool assump_solve();
      // Manipulation Helper Functions
      void setTargetValue(const V3NetId&, const V3BitVecX&, const unsigned&, V3SvrDataVec&);
      void assertImplyUnion(const V3SvrDataVec&);
      const size_t setTargetValue(const V3NetId&, const V3BitVecX&, const unsigned&, const size_t&);
      const size_t setImplyUnion(const V3SvrDataVec&);
      const size_t setImplyIntersection(const V3SvrDataVec&);
      const size_t setImplyInit();
      // Retrieval Functions
      const V3BitVecX getDataValue(const V3NetId&, const unsigned&) const;
      const bool getDataValue(const size_t&) const;
      void getDataConflict(V3SvrDataVec&) const;
      const size_t getFormula(const V3NetId&, const uint32_t&);
      const size_t getFormula(const V3NetId&, const uint32_t&, const uint32_t&);
      // Variable Interface Functions
      inline const size_t reserveFormula() { return getPosExp(boolector_var(_Solver, 1, NULL)); }
      inline const bool isNegFormula(const size_t& e) const { return (e & 2ul); }
      inline const size_t getNegFormula(const size_t& e) const { return (e ^ 2ul); }
      // Print Data Functions
      void printInfo() const;
      // Gate Formula to Solver Functions
      void add_FALSE_Formula(const V3NetId&, const uint32_t&);
      void add_PI_Formula(const V3NetId&, const unsigned&);
      void add_FF_Formula(const V3NetId&, const unsigned&);
      void add_AND_Formula(const V3NetId&, const unsigned&);
      void add_XOR_Formula(const V3NetId&, const unsigned&);
      void add_MUX_Formula(const V3NetId&, const unsigned&);
      void add_RED_AND_Formula(const V3NetId&, const unsigned&);
      void add_RED_OR_Formula(const V3NetId&, const unsigned&);
      void add_RED_XOR_Formula(const V3NetId&, const unsigned&);
      void add_ADD_Formula(const V3NetId&, const unsigned&);
      void add_SUB_Formula(const V3NetId&, const unsigned&);
      void add_MULT_Formula(const V3NetId&, const unsigned&);
      void add_DIV_Formula(const V3NetId&, const unsigned&);
      void add_MODULO_Formula(const V3NetId&, const unsigned&);
      void add_SHL_Formula(const V3NetId&, const unsigned&); 
      void add_SHR_Formula(const V3NetId&, const unsigned&); 
      void add_CONST_Formula(const V3NetId&, const unsigned&);
      void add_SLICE_Formula(const V3NetId&, const unsigned&);
      void add_MERGE_Formula(const V3NetId&, const unsigned&);
      void add_EQUALITY_Formula(const V3NetId&, const unsigned&);
      void add_GEQ_Formula(const V3NetId&, const unsigned&);
      // Network to Solver Function
      const bool existVerifyData(const V3NetId&, const uint32_t&);
   private : 
      // Boolector Functions
      BtorExp* const getVerifyData(const V3NetId&, const unsigned&) const;
      void setRewriteLevel(const int& = 3);
      // Helper Functions : Transformation Between Internal and External Representations
      inline BtorExp* const getOriExp(const size_t& e) const { return (BtorExp*)(e & ~2ul); }
      inline const size_t getPosExp(const BtorExp* const e) const { return ((size_t)e); }
      inline const size_t getNegExp(const BtorExp* const e) const { return ((getPosExp(e)) | 2ul); }
      // Data Members
      Btor*          _Solver;    // Pointer to a Boolector solver
      V3BtorExpVec   _assump;    // Assumption List for assumption solve
      V3BtorExpVec   _init;      // Initial state Var storage
      V3BtorExpTable _ntkData;   // Mapping between V3NetId and Solver Data
};

#endif

