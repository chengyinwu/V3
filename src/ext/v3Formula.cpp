/****************************************************************************
  FileName     [ v3Formula.cpp ]
  PackageName  [ v3/src/ext ]
  Synopsis     [ V3 External Formula. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_FORMULA_C
#define V3_FORMULA_C

#include "v3Msg.h"
#include "v3Formula.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"

/* -------------------------------------------------- *\
 * Class V3Formula Implementations
\* -------------------------------------------------- */
// Initialization to Static Members
V3OperatorMap V3Formula::_opTable   = V3Formula::initializeOperatorMap();

// Constructor and Destructor
V3Formula::V3Formula(V3NtkHandler* const handler, const string& formulaExp) : _handler(handler) {
   assert (handler); _formula.clear();
   V3OperatorMap::const_iterator it = _opTable.end(); string exp = formulaExp;
   if (!lexFormula(it, exp, _rootId)) _rootId = V3NtkUD;
}

V3Formula::V3Formula(const V3Formula& formula) : _handler(formula._handler) {
   assert (_handler); _formula = formula._formula; _rootId = formula._rootId;
}

V3Formula::~V3Formula() {
   _formula.clear();
}

// Constructor for Simple Formula (Specify an Existing V3NetId)
V3Formula::V3Formula(V3NtkHandler* const handler, const V3NetId& id) : _handler(handler) {
   assert (handler); _formula.clear(); _rootId = V3NtkUD;
   if (id.id < handler->getNtk()->getNetSize()) {
      _formula.push_back(make_pair(V3_PI, V3InputVec()));
      _formula.back().second.push_back(V3NetType(id));
      _rootId = 0; assert (isValid());
   }
   else assert (!isValid());
}

// Constructor for Random Formula
V3Formula::V3Formula(V3NtkHandler* const handler, const V3SimTraceVec& cexTrace, const double& strength) : _handler(handler) {
   assert (handler); assert (cexTrace.size()); assert (strength > 0 && strength <= 1.00);
   V3UI32Vec constrId; constrId.clear(); constrId.reserve(cexTrace.size()); _formula.clear();
   // Create Constraints
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   for (uint32_t i = 0; i < cexTrace.size(); ++i) {
      const uint32_t startId = _formula.size(), end = 1 + (uint32_t)(strength * ntk->getLatchSize());
      assert (cexTrace[i].size()); assert (ntk->getLatchSize() == cexTrace[i].size());
      for (uint32_t j = 0; j < end; ++j) {
         const uint32_t index = rand() % cexTrace[i].size(); assert (1 == ntk->getNetWidth(ntk->getLatch(index)));
         const V3NetId id = ('0' == cexTrace[i][index][0]) ? ~(ntk->getLatch(index)) : ntk->getLatch(index);
         _formula.push_back(make_pair(V3_PI, V3InputVec())); _formula.back().second.push_back(V3NetType(id));
      }
      assert (startId <= _formula.size());
      // Conjunction of Leaf Nodes
      for (uint32_t j = 0, k = _formula.size() - startId; j < k; ++j) {
         if (!j) constrId.push_back(startId + j);
         else {
            _formula.push_back(make_pair(BV_AND, V3InputVec()));
            _formula.back().second.push_back(constrId.back());
            _formula.back().second.push_back(startId + j);
            constrId.back() = _formula.size() - 1;
         }
      }
   }
   // Disjunction of Constraints
   for (uint32_t i = 0; i < constrId.size(); ++i) {
      if (!i) _rootId = constrId[i];
      else {
         _formula.push_back(make_pair(BV_OR, V3InputVec()));
         _formula.back().second.push_back(_rootId);
         _formula.back().second.push_back(constrId[i]);
         _rootId = _formula.size() - 1;
      }
   }
}

V3Formula::V3Formula(V3NtkHandler* const handler, const uint32_t& constrSize, const uint32_t& maxCard, const V3GateType& gateType, const uint32_t& noPIorFF) : _handler(handler) {
   assert (handler); assert (constrSize); assert (maxCard); assert (!(noPIorFF & ~3ul));
   assert (BV_AND == gateType || BV_OR == gateType || BV_XOR == gateType || BV_XNOR == gateType);
   // Collect Leaf Candidates
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   V3NetVec leafId; leafId.clear(); leafId.reserve(ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize());
   if (!(1ul & noPIorFF)) {
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
         if (1 == ntk->getNetWidth(ntk->getInput(i))) leafId.push_back(ntk->getInput(i));
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
         if (1 == ntk->getNetWidth(ntk->getInout(i))) leafId.push_back(ntk->getInout(i));
   }
   if (!(2ul & noPIorFF)) {
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) 
         if (1 == ntk->getNetWidth(ntk->getLatch(i))) leafId.push_back(ntk->getLatch(i));
   }
   V3UI32Vec constrId; constrId.clear(); constrId.reserve(constrSize); _formula.clear();
   // Create Constraints
   for (uint32_t i = 0; i < constrSize; ++i) {
      const uint32_t startId = _formula.size();
      // Create Leaf Nodes
      for (uint32_t j = 0; j < maxCard; ++j) {
         if (j && !(rand() % maxCard)) break;
         const V3NetId id = (rand() % 5) ? leafId[rand() % leafId.size()] : ~leafId[rand() % leafId.size()];
         _formula.push_back(make_pair(V3_PI, V3InputVec())); _formula.back().second.push_back(V3NetType(id));
      }
      assert (startId < _formula.size());
      // Link Leaf Nodes with the Specified GateType
      for (uint32_t j = 0, k = _formula.size() - startId; j < k; ++j) {
         if (!j) constrId.push_back(startId + j);
         else {
            _formula.push_back(make_pair(gateType, V3InputVec()));
            _formula.back().second.push_back(constrId.back());
            _formula.back().second.push_back(startId + j);
            constrId.back() = _formula.size() - 1;
         }
      }
      assert ((1 + i) == constrId.size());
   }
   // Combine Constraints
   for (uint32_t i = 0; i < constrId.size(); ++i) {
      if (!i) _rootId = constrId[i];
      else {
         _formula.push_back(make_pair(BV_AND, V3InputVec()));
         _formula.back().second.push_back(_rootId);
         _formula.back().second.push_back(constrId[i]);
         _rootId = _formula.size() - 1;
      }
   }
}

// Restricted Copy Functions
V3Formula* const
V3Formula::createSuccessor(V3NtkHandler* const handler) const {
   assert (handler);
   if (_handler != handler->getHandler()) {
      Msg(MSG_ERR) << "Require handler to be the Immediate Sucessor of formula's Handler !!" << endl;
      return 0;
   }
   V3Formula* const formula = new V3Formula(handler); assert (formula);
   formula->_formula.clear(); formula->_formula.reserve(_formula.size());
   formula->_rootId = _rootId; V3NetId id; V3InputVec inputs; inputs.clear();
   for (uint32_t i = 0; i < _formula.size(); ++i)
      if (isLeaf(i)) {
         id = getNetId(i); assert (V3NetUD != id);
         id = handler->getCurrentNetId(id); inputs.push_back(id);
         if (V3NetUD == id) {
            Msg(MSG_ERR) << "Unresolvable Formula Found !!" << endl;
            delete formula; return 0;
         }
         else formula->_formula.push_back(make_pair(_formula[i].first, inputs));
         inputs.clear();
      }
      else formula->_formula.push_back(_formula[i]);
   assert (formula); return formula;
}

// Elaboration Functions
void
V3Formula::collectLeafFormula(V3UI32Set& netSet) const {
   for (uint32_t i = 0; i < _formula.size(); ++i)
      if (isLeaf(i)) netSet.insert(getNetId(i).id);
}

// Static Member Initialization Functions
const V3OperatorMap
V3Formula::initializeOperatorMap() {
   V3OperatorMap opTable; opTable.clear();
   // Parenthesis
   opTable.insert(make_pair("(",  make_pair(14, BV_BUF)));
   // Concatenate
   opTable.insert(make_pair("{",  make_pair(13, BV_MERGE)));
   // Negation and Reduced Opeartors
   opTable.insert(make_pair("!",  make_pair(12, BV_INV)));
   opTable.insert(make_pair("~",  make_pair(12, BV_INV)));
   // Arithmetic Operators
   opTable.insert(make_pair("*",  make_pair(11, BV_MULT)));
   opTable.insert(make_pair("/",  make_pair(11, BV_DIV)));
   opTable.insert(make_pair("%",  make_pair(11, BV_MODULO)));
   opTable.insert(make_pair("+",  make_pair(10, BV_ADD)));
   opTable.insert(make_pair("-",  make_pair(10, BV_SUB)));
   // Shifters
   opTable.insert(make_pair("<<", make_pair(9, BV_SHL)));
   opTable.insert(make_pair(">>", make_pair(9, BV_SHR)));
   // Comparators
   opTable.insert(make_pair(">",  make_pair(8, BV_GREATER)));
   opTable.insert(make_pair(">=", make_pair(8, BV_GEQ)));
   opTable.insert(make_pair("<",  make_pair(8, BV_LESS)));
   opTable.insert(make_pair("<=", make_pair(8, BV_LEQ)));
   opTable.insert(make_pair("==", make_pair(7, BV_EQUALITY)));
   opTable.insert(make_pair("!=", make_pair(7, BV_NEQ)));
   // Bit-wise Operators
   opTable.insert(make_pair("&",  make_pair(6, BV_AND)));
   opTable.insert(make_pair("^",  make_pair(5, BV_XOR)));
   opTable.insert(make_pair("|",  make_pair(4, BV_OR)));
   // Logical Operators
   opTable.insert(make_pair("&&", make_pair(3, BV_AND)));
   opTable.insert(make_pair("||", make_pair(2, BV_OR)));
   // Multiplexer
   opTable.insert(make_pair("?",  make_pair(1, BV_MUX)));
   return opTable;
}

// Formula Expression Lexing Functions
const bool
V3Formula::lexFormula(V3OperatorMap::const_iterator& it, string& exp, uint32_t& lhs) {
   //cout << "lexFormula: exp = " << exp << endl;
   const uint32_t minPriority = (_opTable.end() == it) ? 0 : it->second.first;
   uint32_t pos, rhs = V3NtkUD; string formula;
   V3OperatorMap::const_iterator is;
   // Find the next operator and lhs if input not given
   if (_opTable.end() == it) {
      it = findNextOperator(exp, pos);
      if (_opTable.end() == it) return lexOperand(exp, lhs); ++pos;
      if (!lexOperand(exp.substr(0, pos - it->first.size()), lhs)) return false;
      exp = (exp.size() > pos) ? exp.substr(pos) : "";
   }
   // Try Associate lhs with rhs by the operator it->first
   while (_opTable.end() != it && minPriority <= it->second.first) {
      //cout << "While loop with op = " << it->first << ", lhs = " << lhs << ", rhs = " << rhs << ", exp = " << exp << endl;
      if ("(" == it->first) {  // Parenthesis
         if (V3NtkUD != lhs) { Msg(MSG_ERR) << "Unexpected Operand on LHS of \"(\" Found !!" << endl; return false; }
         // Find the corresponding ")"
         pos = findCorresOperator(exp, '(', ')');
         if (exp.size() == pos) { Msg(MSG_ERR) << "Missing Corresponding \")\" !!" << endl; return false; }
         // Compute the formula in the parenthesis
         formula = exp.substr(0, pos); exp = exp.substr(++pos); is = _opTable.end();
         //cout << "In () = " << formula << ", after = " << exp << endl;
         if (!lexFormula(is, formula, lhs)) return false;
         if (V3NtkUD == lhs) { Msg(MSG_ERR) << "Missing Formula Inside Parentheses !!" << endl; return false; }
         // Find the next operator
         it = findNextOperator(exp, pos); ++pos;
         // Check if () followed by a concatenation
         formula = (_opTable.end() == it) ? exp : exp.substr(0, pos - it->first.size());
         exp = (exp.size() > pos) ? exp.substr(pos) : "";
         if (formula.size() && '[' == formula[0]) {
            pos = findCorresOperator(formula.substr(1), '[', ']'); uint32_t msb, lsb;
            if (formula.size() == (1 + pos)) { Msg(MSG_ERR) << "Missing Corresponding \"]\" !!" << endl; return false; }
            if (!lexOperand(formula.substr(2 + pos), rhs)) return false;
            if (V3NtkUD != rhs) { Msg(MSG_ERR) << "Unexpected Operand on RHS of \"]\" Found !!" << endl; return false; }
            if (!findSliceOperand(formula.substr(1, pos), msb, lsb)) {
               Msg(MSG_ERR) << "Unexpected Operand \"" << formula.substr(0, 2 + pos) << "\" !!" << endl; return false; }
            _formula.push_back(make_pair(BV_SLICE, V3InputVec()));
            _formula.back().second.push_back(V3NetType(lhs));
            _formula.back().second.push_back(V3NetType(V3BvNtk::hashV3BusId(msb, lsb)));
            lhs = _formula.size() - 1;
         }
         else {
            if (!lexOperand(formula, rhs)) return false;
            if (V3NtkUD != rhs) { Msg(MSG_ERR) << "Unexpected Operand on RHS of \")\" Found !!" << endl; return false; }
         }
      }
      else if ("{" == it->first) {  // Concatenation
         if (V3NtkUD != lhs) { Msg(MSG_ERR) << "Unexpected Operand on LHS of \"{\" Found !!" << endl; return false; }
         // Find the corresponding "}"
         pos = findCorresOperator(exp, '{', '}');
         if (exp.size() == pos) { Msg(MSG_ERR) << "Missing Corresponding \"}\" !!" << endl; return false; }
         // Compute the formula in the parenthesis
         formula = exp.substr(0, pos); exp = exp.substr(++pos);
         if (!lexConcatenateOperand(formula, lhs)) return false; assert (V3NtkUD != lhs);
         // Find the next operator
         it = findNextOperator(exp, pos); ++pos;
         // Check if () followed by a concatenation
         formula = (_opTable.end() == it) ? exp : exp.substr(0, pos - it->first.size());
         exp = (exp.size() > pos) ? exp.substr(pos) : "";
         if (formula.size() && '[' == formula[0]) {
            pos = findCorresOperator(formula.substr(1), '[', ']'); uint32_t msb, lsb;
            if (formula.size() == (1 + pos)) { Msg(MSG_ERR) << "Missing Corresponding \"]\" !!" << endl; return false; }
            if (!lexOperand(formula.substr(2 + pos), rhs)) return false;
            if (V3NtkUD != rhs) { Msg(MSG_ERR) << "Unexpected Operand on RHS of \"]\" Found !!" << endl; return false; }
            if (!findSliceOperand(formula.substr(1, pos), msb, lsb)) {
               Msg(MSG_ERR) << "Unexpected Operand \"" << formula.substr(0, 2 + pos) << "\" !!" << endl; return false; }
            _formula.push_back(make_pair(BV_SLICE, V3InputVec()));
            _formula.back().second.push_back(V3NetType(lhs));
            _formula.back().second.push_back(V3NetType(V3BvNtk::hashV3BusId(msb, lsb)));
            lhs = _formula.size() - 1;
         }
         else {
            if (!lexOperand(formula, rhs)) return false;
            if (V3NtkUD != rhs) { Msg(MSG_ERR) << "Unexpected Operand on RHS of \"}\" Found !!" << endl; return false; }
         }
      }
      else {  // Operators
         if ("!" == it->first || "~" == it->first) {  // Unary Operators
            if (V3NtkUD != lhs) {
               Msg(MSG_ERR) << "Unexpected Operand on LHS of \"" << it->first << "\" Found !!" << endl; return false; }
            if (exp.size() && isspace(exp[0])) {
               Msg(MSG_ERR) << "Unexpected Space after Operator \"" << it->first << "\" Found !!" << endl; return false; }
            // Find the next operator and rhs
            is = findNextOperator(exp, pos); ++pos;
            if (_opTable.end() == is) { if (!lexOperand(exp, rhs)) return false; }
            else if (!lexOperand(exp.substr(0, pos - is->first.size()), rhs)) return false;
            exp = (exp.size() > pos) ? exp.substr(pos) : "";
            // Check Operator Precedence
            if (it->second.first <= is->second.first) {
               if (!lexFormula(is, exp, rhs)) return false;
               if (V3NtkUD == rhs) {
                  Msg(MSG_ERR) << "Missing Operand on RHS of \"" << it->first << "\" !!" << endl;
                  return false;
               }
            }
            // Associate with RHS
            if (V3NtkUD == rhs) {
               Msg(MSG_ERR) << "Missing Operand on RHS of \"" << it->first << "\" !!" << endl; return false; }
            _formula.push_back(make_pair(BV_INV, V3InputVec()));
            _formula.back().second.push_back(rhs);
            if ("!" == it->first) {
               _formula.push_back(make_pair(BV_RED_AND, V3InputVec()));
               _formula.back().second.push_back(_formula.size() - 2);
            }
            lhs = _formula.size() - 1; it = is;
         }
         else if (("&" == it->first || "|" == it->first || "^" == it->first) && (V3NtkUD == lhs)) {  // Reduced Operators
            if (V3NtkUD != lhs) {
               Msg(MSG_ERR) << "Unexpected Operand on LHS of \"" << it->first << "\" Found !!" << endl; return false; }
            if (exp.size() && isspace(exp[0])) {
               Msg(MSG_ERR) << "Unexpected Space after Operator \"" << it->first << "\" Found !!" << endl; return false; }
            // Find the next operator and rhs
            is = findNextOperator(exp, pos); ++pos;
            if (_opTable.end() == is) { if (!lexOperand(exp, rhs)) return false; }
            else if (!lexOperand(exp.substr(0, pos - is->first.size()), rhs)) return false;
            exp = (exp.size() > pos) ? exp.substr(pos) : "";
            // Check Operator Precedence
            while (_opTable.end() != is && 13 < is->second.first) {  // 13 is the priority of REDUCED Gates
               if (!lexFormula(is, exp, rhs)) return false;
               if (V3NtkUD == rhs) break;
            }
            // Associate with RHS
            if (V3NtkUD == rhs) {
               Msg(MSG_ERR) << "Missing Operand on RHS of \"" << it->first << "\" !!" << endl; return false; }
            const V3GateType type = "&" == it->first ? BV_RED_AND : "|" == it->first ? BV_RED_OR : BV_RED_XOR;
            _formula.push_back(make_pair(type, V3InputVec()));
            _formula.back().second.push_back(rhs);
            lhs = _formula.size() - 1; it = is;
         }
         else if ("?" == it->first) {  // Multiplexers
            if (V3NtkUD == lhs) { Msg(MSG_ERR) << "Missing Operand on LHS of \"?\" !!" << endl; return false; }
            // Find the corresponding ":"
            pos = findCorresOperator(exp, '?', ':');
            if (exp.size() == pos) { Msg(MSG_ERR) << "Missing Corresponding \":\" !!" << endl; return false; }
            // Compute the formula between ? and :
            formula = exp.substr(0, pos); exp = exp.substr(++pos); is = _opTable.end();
            if (!lexFormula(is, formula, rhs)) return false;
            if (V3NtkUD == rhs) { Msg(MSG_ERR) << "Missing Formula Between \"?\" and \":\" !!" << endl; return false; }
            // Compute the formula after :
            uint32_t falseId = V3NtkUD; is = _opTable.end();
            if (!lexFormula(is, exp, falseId)) return false;
            if (V3NtkUD == falseId) { Msg(MSG_ERR) << "Missing Formula After \":\" !!" << endl; return false; }
            _formula.push_back(make_pair(it->second.second, V3InputVec()));
            _formula.back().second.push_back(falseId);
            _formula.back().second.push_back(rhs);
            _formula.back().second.push_back(lhs);
            lhs = _formula.size() - 1; it = is;
         }
         else {  // Binary Operators
            if (V3NtkUD == lhs) {
               Msg(MSG_ERR) << "Missing Operand on LHS of \"" << it->first << "\" !!" << endl; return false; }
            // Find the next operator and rhs
            is = findNextOperator(exp, pos); ++pos;
            if (_opTable.end() == is) { if (!lexOperand(exp, rhs)) return false; }
            else if (!lexOperand(exp.substr(0, pos - is->first.size()), rhs)) return false;
            exp = (exp.size() > pos) ? exp.substr(pos) : "";
            // Check Operator Precedence
            while (_opTable.end() != is && ((it->second.first < is->second.first) ||
                   (V3NtkUD == rhs && ("&" == is->first || "|" == is->first || "^" == is->first)))) {
               if (!lexFormula(is, exp, rhs)) return false;
               if (V3NtkUD == rhs) break;
            }
            if (V3NtkUD == rhs) {
               Msg(MSG_ERR) << "Missing Operand on RHS of \"" << it->first << "\" !!" << endl; return false; }
            _formula.push_back(make_pair(it->second.second, V3InputVec()));
            _formula.back().second.push_back(lhs);
            _formula.back().second.push_back(rhs);
            lhs = _formula.size() - 1; it = is;
         }
      }
   }
   return true;
}

const bool
V3Formula::lexOperand(const string& exp, uint32_t& index) {
   //cout << "lexOperand: exp = " << exp << endl;
   uint32_t i = 0; string name = ""; bool ok = false;
   for (; i < exp.size(); ++i) if (!isspace(exp[i])) name += exp[i]; else if (name.size()) break;
   if (name.size()) {
      const V3NetId id = _handler->getNetFromName(name);
      if (V3NetUD == id) {
         if (lexConstOperand(name)) {
            _formula.push_back(make_pair(BV_CONST, V3InputVec()));
            _formula.back().second.push_back(V3NetType(V3BvNtk::hashV3ConstBitVec(name))); ok = true;
         }
      }
      else {
         _formula.push_back(make_pair(V3_PI, V3InputVec()));
         _formula.back().second.push_back(V3NetType(id)); ok = true;
      }
   }
   if (ok) {
      for (++i; i < exp.size(); ++i)
         if (!isspace(exp[i])) {
            Msg(MSG_ERR) << "Missing Operator on RHS of Operand \"" << name << "\" !!" << endl; return false; }
   }
   else if (name.size()) {
      uint32_t msb, lsb;
      if (lexSliceOperand(v3StrRemoveSpaces(exp), msb, lsb, index)) {
         _formula.push_back(make_pair(BV_SLICE, V3InputVec()));
         _formula.back().second.push_back(V3NetType(index));
         _formula.back().second.push_back(V3NetType(V3BvNtk::hashV3BusId(msb, lsb))); ok = true;
      }
      else { Msg(MSG_ERR) << "Fail to Find Net with Name = \"" << name << "\" !!" << endl; return false; }
   }
   index = ok ? (_formula.size() - 1) : V3NtkUD; return true;
}

const bool
V3Formula::lexConstOperand(const string& exp) {
   uint32_t i = 0;
   for (; i < exp.size(); ++i) if ('\'' == exp[i]) break; else if (!isdigit(exp[i])) return false;
   if (i < exp.size()) {
      if (++i == exp.size()) return false;
      if ('b' == exp[i] || 'B' == exp[i]) {
         for (++i; i < exp.size(); ++i) if ('_' != exp[i] && '0' != exp[i] && '1' != exp[i]) return false;
      }
      else if ('d' == exp[i] || 'D' == exp[i]) {
         for (++i; i < exp.size(); ++i) if ('_' != exp[i] && !isdigit(exp[i])) return false;
      }
      else if ('h' == exp[i]) {
         for (++i; i < exp.size(); ++i) if ('_' != exp[i] && !isdigit(exp[i]) && 'a' > exp[i] && 'f' < exp[i]) return false;
      }
      else if ('H' == exp[i]) {
         for (++i; i < exp.size(); ++i) if ('_' != exp[i] && !isdigit(exp[i]) && 'A' > exp[i] && 'F' < exp[i]) return false;
      }
      else return false;
   }
   return true;
}

const bool
V3Formula::lexSliceOperand(const string& exp, uint32_t& msb, uint32_t& lsb, uint32_t& index) {
   string name = "", bitStr = ""; uint32_t i = 0;
   for (; i < exp.size(); ++i)
      if ('[' == exp[i] || (isspace(exp[i]) && name.size())) break;
      else if (!isspace(exp[i]) || name.size()) name += exp[i];
   if (i == exp.size() || isspace(exp[i]) || !name.size()) return false;
   for (++i; i < exp.size(); ++i)
      if (':' == exp[i] || ']' == exp[i]) break; else if (!isspace(exp[i]) || bitStr.size()) bitStr += exp[i];
   if (i == exp.size() || !bitStr.size() || !v3Str2UInt(bitStr, msb)) return false;
   if (':' == exp[i]) {
      bitStr = "";
      for (++i; i < exp.size(); ++i)
         if (']' == exp[i]) break; else if (!isspace(exp[i]) || bitStr.size()) bitStr += exp[i];
      if (i == exp.size() || !bitStr.size() || !v3Str2UInt(bitStr, lsb)) return false;
   }
   else lsb = msb;
   for (++i; i < exp.size(); ++i) if (!isspace(exp[i])) return false;
   const V3NetId id = _handler->getNetFromName(name); if (V3NetUD == id) return false; index = _formula.size();
   if (msb >= _handler->getNtk()->getNetWidth(id) || lsb >= _handler->getNtk()->getNetWidth(id)) return false;
   _formula.push_back(make_pair(V3_PI, V3InputVec())); _formula.back().second.push_back(V3NetType(id)); return true;
}

const bool
V3Formula::lexConcatenateOperand(const string& exp, uint32_t& lhs) {
   //cout << "lexConcatenateOperand, exp = " << exp << endl;
   V3StrVec expVec; expVec.clear(); uint32_t rhs = lhs = V3NtkUD;
   V3OperatorMap::const_iterator it; uint32_t i = 0, j = 0; int k;
   for (k = 0; i < exp.size(); ++i) {
      if (!k && (',' == exp[i])) {
         if (j < i) { expVec.push_back(exp.substr(j, i - j)); j = i + 1; }
         else { Msg(MSG_ERR) << "Missing Operand for Concatenation !!" << endl; return false; }
      }
      else if ('{' == exp[i]) ++k; else if ('}' == exp[i]) --k;
   }
   if (!k && (j < i)) expVec.push_back(exp.substr(j, i - j));
   for (i = 0; i < expVec.size(); ++i) {
      it = _opTable.end(); if (!lexFormula(it, expVec[i], rhs)) return false;
      if (V3NtkUD == rhs) { Msg(MSG_ERR) << "Missing Operand for Concatenation !!" << endl; return false; }
      if (V3NtkUD != lhs) {
         _formula.push_back(make_pair(BV_MERGE, V3InputVec()));
         _formula.back().second.push_back(V3NetType(lhs));
         _formula.back().second.push_back(V3NetType(rhs));
         lhs = _formula.size() - 1;
      }
      else lhs = rhs;
   }
   if (expVec.size() < 2) { Msg(MSG_ERR) << "Too Few Operands for Concatenation !!" << endl; return false; }
   return true;
}

const V3OperatorMap::const_iterator
V3Formula::findNextOperator(const string& exp, uint32_t& pos) {
   V3OperatorMap::const_iterator it = _opTable.end();
   for (pos = 0; pos < exp.size(); ++pos) {
      if ((1 + pos) < exp.size()) {
         it = _opTable.find(exp.substr(pos, 2));
         if (_opTable.end() != it) { ++pos; break; }
      }
      it = _opTable.find(exp.substr(pos, 1));
      if (_opTable.end() != it) break;
   }
   //cout << "findNextOperator: exp = " << exp;
   //if (_opTable.end() != it) cout << ", op = " << it->first << ", pos = " << pos;
   //cout << endl;
   return it;
}

const bool
V3Formula::findSliceOperand(const string& exp, uint32_t& msb, uint32_t& lsb) {
   uint32_t i; string bitStr = "";
   for (i = 0; i < exp.size(); ++i)
      if (':' == exp[i]) break; else if (!isspace(exp[i]) || bitStr.size()) bitStr += exp[i];
   if (!bitStr.size() || !v3Str2UInt(bitStr, msb)) return false;
   if (':' == exp[i]) {
      bitStr = "";
      for (++i; i < exp.size(); ++i)
         if (!isspace(exp[i])) bitStr += exp[i]; else if (bitStr.size()) break;
      if (!bitStr.size() || !v3Str2UInt(bitStr, lsb)) return false;
   }
   else lsb = msb;
   for (++i; i < exp.size(); ++i) if (!isspace(exp[i])) return false; return true;
}

const uint32_t
V3Formula::findCorresOperator(const string& exp, const char& prefix, const char& suffix) {
   //cout << "findCorresOperator: exp = " << exp << " target = " << prefix << suffix << endl;
   assert (exp.size()); assert (prefix != suffix);
   uint32_t i = 0, prefixSize = 1;
   for (; i < exp.size(); ++i)
      if (('?' == prefix) && ('(' == exp[i])) {
         const string subExp = exp.substr(1 + i);
         const uint32_t subPos = findCorresOperator(subExp, '(', ')');
         if (subPos == subExp.size()) return exp.size();
         i += (1 + subPos); assert (i < exp.size());
      }
      else if (('?' == prefix) && ('[' == exp[i])) {
         const string subExp = exp.substr(1 + i);
         const uint32_t subPos = findCorresOperator(subExp, '[', ']');
         if (subPos == subExp.size()) return exp.size();
         i += (1 + subPos); assert (i < exp.size());
      }
      else if (suffix == exp[i]) { if (!(--prefixSize)) break; }
      else if (prefix == exp[i]) ++prefixSize;
   //cout << "Found suffix at " << i << endl;
   return i;
}

#endif

