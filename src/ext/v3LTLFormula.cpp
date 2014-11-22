/****************************************************************************
  FileName     [ v3LTLFormula.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ V3 LTL Formula. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_LTL_FORMULA_C
#define V3_LTL_FORMULA_C

#include "v3StrUtil.h"
#include "v3LTLFormula.h"

/* -------------------------------------------------- *\
 * Class V3Formula Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3LTLFormula::V3LTLFormula(V3NtkHandler* const handler, const string& ltlFormulaExp, const string& name) 
   : _handler(handler), _name(name) {
   assert (handler); _formula.clear();
   _rootId = lexLTLFormula(ltlFormulaExp);
}

V3LTLFormula::V3LTLFormula(const V3LTLFormula& ltlFormula, const string& name) 
   : _handler(ltlFormula._handler), _name(name) {
   assert (_handler); _formula = ltlFormula._formula; _rootId = ltlFormula._rootId;
   for (uint32_t i = 0; i < _formula.size(); ++i)
      if (isLeaf(i)) _formula[i].second[0] = (size_t)(new V3Formula(*(getFormula(i))));
}

V3LTLFormula::~V3LTLFormula() {
   for (uint32_t i = 0; i < _formula.size(); ++i) {
      if (isLeaf(i)) delete (V3Formula*)(_formula[i].second[0]);
      _formula[i].second.clear();
   }
   _formula.clear();
}

// Constructor for Simple LTL Formula (AGp and AFp)
V3LTLFormula::V3LTLFormula(V3NtkHandler* const handler, const V3NetId& id, const bool& safe, const string& name) 
   : _handler(handler), _name(name) {
   assert (handler); _formula.clear(); _rootId = V3NtkUD;
   V3Formula* const formula = new V3Formula(handler, id); assert (formula);
   if (formula->isValid()) {
      _formula.push_back(make_pair(V3_LTL_P, V3PtrVec(1, (size_t)(formula))));
      _formula.push_back(make_pair((safe ? V3_LTL_T_G : V3_LTL_T_F), V3PtrVec(1, 0)));
      _rootId = 1; assert (isValid());
   }
   else { delete formula; assert (!isValid()); }
}

// Restricted Copy Functions
V3LTLFormula* const
V3LTLFormula::createSuccessor(V3NtkHandler* const handler) const {
   V3LTLFormula* const ltlFormula = new V3LTLFormula(handler, "", _name); assert (ltlFormula);
   ltlFormula->_formula.clear(); ltlFormula->_formula.reserve(_formula.size());
   ltlFormula->_rootId = _rootId; V3Formula* formula = 0;
   for (uint32_t i = 0; i < _formula.size(); ++i)
      if (isLeaf(i)) {
         formula = getFormula(i)->createSuccessor(handler);
         if (!formula) { delete ltlFormula; return 0; }
         else ltlFormula->_formula.push_back(make_pair(_formula[i].first, V3PtrVec(1, (size_t)formula)));
      }
      else ltlFormula->_formula.push_back(_formula[i]);
   assert (ltlFormula); return ltlFormula;
}

// Elaboration Functions
void
V3LTLFormula::collectLeafFormula(V3UI32Set& netSet) const {
   for (uint32_t i = 0; i < _formula.size(); ++i)
      if (isLeaf(i)) getFormula(i)->collectLeafFormula(netSet);
}

// Formula Expression Lexing Functions
const uint32_t
V3LTLFormula::lexLTLFormula(const string& exp) {
   // Currently we support only G(formula), F(formula), and formula
   const string expNoSpaces = v3StrRemoveSpaces(exp);
   if ((expNoSpaces.size() > 4) && ('(' == expNoSpaces[1]) && (')' == expNoSpaces[expNoSpaces.size() - 1])) {
      const size_t begin = exp.find_first_of('('); assert (string::npos != begin);
      const size_t end = exp.find_last_of('('); assert (string::npos != end);
      if ('G' == expNoSpaces[0]) {  // safety property
         V3Formula* const formula = new V3Formula(_handler, exp.substr(1 + begin, end - begin - 1));
         if (!formula || !formula->isValid()) { delete formula; return V3NtkUD; }
         _formula.push_back(make_pair(V3_LTL_P, V3PtrVec(1, (size_t)(formula))));
         _formula.push_back(make_pair(V3_LTL_T_G, V3PtrVec(1, 0))); return 1;
      }
      else if ('F' == expNoSpaces[0]) {  // liveness property
         V3Formula* const formula = new V3Formula(_handler, exp.substr(1 + begin, end - begin - 1));
         if (!formula || !formula->isValid()) { delete formula; return V3NtkUD; }
         _formula.push_back(make_pair(V3_LTL_P, V3PtrVec(1, (size_t)(formula))));
         _formula.push_back(make_pair(V3_LTL_T_F, V3PtrVec(1, 0))); return 1;
      }
   }
   V3Formula* const formula = new V3Formula(_handler, exp);
   if (!formula || !formula->isValid()) { delete formula; return V3NtkUD; }
   _formula.push_back(make_pair(V3_LTL_P, V3PtrVec(1, (size_t)(formula)))); return 0;
}

#endif

