/****************************************************************************
  FileName     [ v3LTLFormula.h ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ V3 LTL Formula. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_LTL_FORMULA_H
#define V3_LTL_FORMULA_H

#include "v3Formula.h"

// Forward Declaration
class V3Formula;

enum V3LTLOpType {
   V3_LTL_P = 0, V3_LTL_T_G, V3_LTL_T_F, V3_LTL_T_X, V3_LTL_T_U, V3_LTL_T_R,
   V3_LTL_L_NEG, V3_LTL_L_AND, V3_LTL_L_IMPLY, V3_LTL_TOTAL
};

// V3LTLFormula : LTL Formula Storage
class V3LTLFormula
{
   public : 
      // Constructor and Destructor
      V3LTLFormula(V3NtkHandler* const, const string&, const string& = "");
      V3LTLFormula(const V3LTLFormula&, const string& = "");
      ~V3LTLFormula();
      // Constructor for Simple Property (AGp and AFp)
      V3LTLFormula(V3NtkHandler* const, const V3NetId&, const bool&, const string& = "");
      // Inline LTL Formula Interface Functions
      inline V3NtkHandler* const getHandler() const { return _handler; }
      inline const string& getName() const { return _name; }
      inline const bool isValid() const { return _rootId < _formula.size(); }
      inline const bool isValid(const uint32_t& i) const { return i < _formula.size(); }
      inline const bool isLeaf(const uint32_t& i) const { assert (isValid(i)); return V3_LTL_P == _formula[i].first; }
      inline const V3LTLOpType& getOpType(const uint32_t& i) const { assert (!isLeaf(i)); return _formula[i].first; }
      inline V3Formula* const getFormula(const uint32_t& i) const {
         assert (isLeaf(i)); assert (1 == _formula[i].second.size()); return (V3Formula*)(_formula[i].second[0]); }
      inline const uint32_t getBranchSize(const uint32_t& i) const {
         assert (!isLeaf(i)); return _formula[i].second.size(); }
      inline const uint32_t getBranchIndex(const uint32_t& i, const uint32_t& j) const {
         assert (!isLeaf(i)); assert (j < getBranchSize(i)); return (uint32_t)(_formula[i].second[j]); }
      inline const uint32_t getRoot() const { assert (isValid()); return _rootId; }
      // Restricted Copy Functions
      V3LTLFormula* const createSuccessor(V3NtkHandler* const) const;
      // Elaboration Functions
      void collectLeafFormula(V3UI32Set&) const;
   private : 
      typedef pair<V3LTLOpType, V3PtrVec>    V3LTLFormulaNode;
      typedef V3Vec<V3LTLFormulaNode>::Vec   V3LTLFormulaList;
      // Formula Expression Lexing Functions
      const uint32_t lexLTLFormula(const string&);
      // Private Data Members
      V3NtkHandler* const  _handler;
      const string         _name;
      V3LTLFormulaList     _formula;
      uint32_t             _rootId;
};

#endif

