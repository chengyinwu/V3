/****************************************************************************
  FileName     [ v3DfxSimplify.h ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Trace Simplification. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_SIMPLIFY_H
#define V3_DFX_SIMPLIFY_H

#include "v3Property.h"
#include "v3NtkHandler.h"
#include "v3AlgGeneralize.h"

// class V3TraceSimplify: Trace Simplification Based-on Generalization
class V3TraceSimplify
{
   public : 
      // Constructor and Destructor
      V3TraceSimplify(V3NtkHandler* const, const V3CexTrace&);
      ~V3TraceSimplify();
      // Trace Retrieval Functions
      V3CexTrace* const getTrace() const;
      // Trace Simplification Functions
      void simplifyInputPatterns(const bool&, const uint32_t& = V3NtkUD);
      // Trace Information
      void printTraceInfo() const;
   private: 
      // Private Helper Functions
      void computeSimTrace(const V3CexTrace&);
      V3NtkHandler* const getUnrolledNtk(const bool&, const uint32_t&);
      void maximizeDontCares(V3Ntk* const, V3AlgGeneralize* const, const V3SimTrace&, V3SimTrace&, const V3NetVec&);
      void minimizeTransitions(V3Ntk* const, V3AlgGeneralize* const, const V3SimTrace&, V3SimTrace&, const V3NetVec&);
      // Private Data Members
      V3NtkHandler* const  _handler;
      V3NetVec             _target;
      V3SimTraceVec        _piValue;
      V3SimTraceVec        _ffValue;
      // Statistics
      V3Stat*              _totalStat;
      V3Stat*              _piUnrollStat;
      V3Stat*              _piGenStat;
      V3Stat*              _ffUnrollStat;
      V3Stat*              _ffGenStat;
};

#endif

