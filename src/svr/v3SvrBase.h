/****************************************************************************
  FileName     [ v3SvrBase.h ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ V3 Engine-Encapsulated Base Solver. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_BASE_H
#define V3_SVR_BASE_H

#include "v3Ntk.h"
#include "v3SvrType.h"

// V3SvrBase : Virtual Class for V3 Solver
class V3SvrBase
{
   public : 
      // Constructor and Destructor
      V3SvrBase(const V3Ntk* const, const bool& = false);
      virtual ~V3SvrBase();
      // Basic Operation Functions
      virtual void reset();
      virtual void update();
      virtual void assumeInit();
      virtual void assertInit();
      virtual void initRelease();
      virtual void assumeRelease();
      virtual void assumeProperty(const size_t&, const bool& = false);
      virtual void assertProperty(const size_t&, const bool& = false);
      virtual void assumeProperty(const V3NetId&, const bool&, const uint32_t&);
      virtual void assertProperty(const V3NetId&, const bool&, const uint32_t&);
      virtual const bool simplify();
      virtual const bool solve();
      virtual const bool assump_solve();
      // Manipulation Helper Functions
      virtual void setTargetValue(const V3NetId&, const V3BitVecX&, const uint32_t&, V3SvrDataVec&);
      virtual void assertImplyUnion(const V3SvrDataVec&);
      virtual const size_t setTargetValue(const V3NetId&, const V3BitVecX&, const uint32_t&, const size_t&);
      virtual const size_t setImplyUnion(const V3SvrDataVec&);
      virtual const size_t setImplyIntersection(const V3SvrDataVec&);
      virtual const size_t setImplyInit();
      // Retrieval Functions
      virtual const V3BitVecX getDataValue(const V3NetId&, const uint32_t&) const;
      virtual const bool getDataValue(const size_t&) const;
      virtual void getDataConflict(V3SvrDataVec&) const;
      virtual const size_t getFormula(const V3NetId&, const uint32_t&);
      virtual const size_t getFormula(const V3NetId&, const uint32_t&, const uint32_t&);
      // Variable Interface Functions
      virtual const size_t reserveFormula();
      virtual const bool isNegFormula(const size_t&) const;
      virtual const size_t getNegFormula(const size_t&) const;
      // Formula Output Functions
      virtual const bool setOutputFile(const string);
      virtual const bool isOutputSet() const;
      // Print Data Functions
      virtual void printInfo() const;
      virtual void printVerbose() const;
      // Resource Functions
      virtual const double getTime() const;
      virtual const int getMemory() const;
      // Gate Formula to Solver Functions
      virtual void add_FALSE_Formula(const V3NetId&, const uint32_t&);
      virtual void add_PI_Formula(const V3NetId&, const uint32_t&);
      virtual void add_FF_Formula(const V3NetId&, const uint32_t&);
      virtual void add_AND_Formula(const V3NetId&, const uint32_t&);
      virtual void add_XOR_Formula(const V3NetId&, const uint32_t&);
      virtual void add_MUX_Formula(const V3NetId&, const uint32_t&);
      virtual void add_RED_AND_Formula(const V3NetId&, const uint32_t&);
      virtual void add_RED_OR_Formula(const V3NetId&, const uint32_t&);
      virtual void add_RED_XOR_Formula(const V3NetId&, const uint32_t&);
      virtual void add_ADD_Formula(const V3NetId&, const uint32_t&);
      virtual void add_SUB_Formula(const V3NetId&, const uint32_t&);
      virtual void add_MULT_Formula(const V3NetId&, const uint32_t&);
      virtual void add_DIV_Formula(const V3NetId&, const uint32_t&);
      virtual void add_MODULO_Formula(const V3NetId&, const uint32_t&);
      virtual void add_SHL_Formula(const V3NetId&, const uint32_t&); 
      virtual void add_SHR_Formula(const V3NetId&, const uint32_t&); 
      virtual void add_CONST_Formula(const V3NetId&, const uint32_t&);
      virtual void add_SLICE_Formula(const V3NetId&, const uint32_t&);
      virtual void add_MERGE_Formula(const V3NetId&, const uint32_t&);
      virtual void add_EQUALITY_Formula(const V3NetId&, const uint32_t&);
      virtual void add_GEQ_Formula(const V3NetId&, const uint32_t&);
      // Network to Solver Functions
      void addBoundedVerifyData(const V3NetId&, const uint32_t&);
      virtual const bool existVerifyData(const V3NetId&, const uint32_t&);
      void assertBoundedVerifyData(const V3NetVec&, const uint32_t&);
      // Inline Base Functions
      inline const uint32_t totalSolves() const { return _solves; }
      inline const double totalTime() const { return _runTime; }
      inline const bool validNetId(const V3NetId& id) const { return _ntk->getNetSize() > id.id; }
   protected : 
      // Private Network to Solver Functions
      void addVerifyData(const V3NetId&, const uint32_t&);
      void addSimpleBoundedVerifyData(V3NetId, uint32_t);
      // Data Members
      const V3Ntk* const   _ntk;       // Network Under Verification
      uint32_t             _solves;    // Number of Solve Called
      double               _runTime;   // Total Runtime in Solving
      // Configurations
      const bool           _freeBound; // Set FF Bounds Free
};

#endif

