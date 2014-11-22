/****************************************************************************
  FileName     [ v3SvrMiniSat.cpp ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ V3 Solver with MiniSAT as Engine. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_MSAT_C
#define V3_SVR_MSAT_C

#include "v3Msg.h"
#include "v3SvrMiniSat.h"
#include "v3SvrSatHelper.h"

/* -------------------------------------------------- *\
 * Class V3SvrMiniSat Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3SvrMiniSat::V3SvrMiniSat(const V3Ntk* const ntk, const bool& freeBound) : V3SvrBase(ntk, freeBound) {
   _Solver = new MSolver(); assert (_Solver); assumeRelease(); initRelease();
   _curVar = 0; _Solver->newVar(l_Undef, false); ++_curVar;  // 0 for Recycle Literal, if Needed
   _ntkData.clear(); _ntkData.reserve(ntk->getNetSize());
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) { _ntkData.push_back(V3SvrMVarData()); _ntkData.back().clear(); }
}

V3SvrMiniSat::V3SvrMiniSat(const V3SvrMiniSat& solver) : V3SvrBase(solver._ntk, solver._freeBound) {
   _Solver = new MSolver(); assert (_Solver); assumeRelease(); initRelease();
   _curVar = 0; _Solver->newVar(l_Undef, false); ++_curVar;  // 0 for Recycle Literal, if Needed
   _ntkData.clear(); _ntkData.reserve(_ntk->getNetSize());
   for (uint32_t i = 0; i < _ntk->getNetSize(); ++i) { _ntkData.push_back(V3SvrMVarData()); _ntkData.back().clear(); }
}

V3SvrMiniSat::~V3SvrMiniSat() {
   delete _Solver; assumeRelease(); initRelease();
   for (uint32_t i = 0; i < _ntkData.size(); ++i) _ntkData[i].clear(); _ntkData.clear();
}

// Basic Operation Functions
void
V3SvrMiniSat::reset() {
   delete _Solver; _Solver = new MSolver(); assert (_Solver); assumeRelease(); initRelease();
   _curVar = 0; _Solver->newVar(l_Undef, false); ++_curVar;  // 0 for Recycle Literal, if Needed
   for (uint32_t i = 0; i < _ntkData.size(); ++i) _ntkData[i].clear();
   if (_ntkData.size() == _ntk->getNetSize()) return; _ntkData.clear(); _ntkData.reserve(_ntk->getNetSize());
   for (uint32_t i = 0; i < _ntk->getNetSize(); ++i) { _ntkData.push_back(V3SvrMVarData()); _ntkData.back().clear(); }
}

void
V3SvrMiniSat::update() {
   assert (_Solver); assert (_ntk->getNetSize() >= _ntkData.size());
   V3SvrMVarData svrData; svrData.clear(); _ntkData.resize(_ntk->getNetSize(), svrData);
}

void
V3SvrMiniSat::assumeInit() {
   for (uint32_t i = 0; i < _init.size(); ++i) _assump.push(_init[i]);
}

void
V3SvrMiniSat::assertInit() {
   for (uint32_t i = 0; i < _init.size(); ++i) _Solver->addClause(_init[i]);
}

void
V3SvrMiniSat::initRelease() { _init.clear(); }

void
V3SvrMiniSat::assumeRelease() { _assump.clear(); }

void
V3SvrMiniSat::assumeProperty(const size_t& var, const bool& invert) {
   _assump.push(mkLit(getOriVar(var), invert ^ isNegFormula(var)));
}

void
V3SvrMiniSat::assertProperty(const size_t& var, const bool& invert) {
   _Solver->addClause(mkLit(getOriVar(var), invert ^ isNegFormula(var)));
}

void
V3SvrMiniSat::assumeProperty(const V3NetId& id, const bool& invert, const uint32_t& depth) {
   assert (validNetId(id)); assert (1 == _ntk->getNetWidth(id));
   const Var var = getVerifyData(id, depth); assert (var);
   _assump.push(mkLit(var, invert ^ isV3NetInverted(id)));
}

void
V3SvrMiniSat::assertProperty(const V3NetId& id, const bool& invert, const uint32_t& depth) {
   assert (validNetId(id)); assert (1 == _ntk->getNetWidth(id));
   const Var var = getVerifyData(id, depth); assert (var);
   _Solver->addClause(mkLit(var, invert ^ isV3NetInverted(id)));
}

const bool
V3SvrMiniSat::simplify() { return _Solver->simplify(); }

const bool
V3SvrMiniSat::solve() {
   double ctime = (double)clock() / CLOCKS_PER_SEC;
   _Solver->solve(); ++_solves;
   _runTime += (((double)clock() / CLOCKS_PER_SEC) - ctime);
   return _Solver->okay();
}

const bool
V3SvrMiniSat::assump_solve() {
   double ctime = (double)clock() / CLOCKS_PER_SEC;
   bool result = _Solver->solve(_assump); ++_solves;
   _runTime += (((double)clock() / CLOCKS_PER_SEC) - ctime);
   return result;
}

// Manipulation Helper Functions
void
V3SvrMiniSat::setTargetValue(const V3NetId& id, const V3BitVecX& value, const uint32_t& depth, V3SvrDataVec& formula) {
   // Note : This Function will set formula such that AND(formula) represents (id == value)
   uint32_t i, size = value.size(); assert (size == _ntk->getNetWidth(id));
   const Var var = getVerifyData(id, depth); assert (var);
   if (isV3NetInverted(id)) {
      for (i = 0; i < size; ++i) {
         if ('1' == value[i]) formula.push_back(getNegVar(var + i));
         else if ('0' == value[i]) formula.push_back(getPosVar(var + i));
      }
   }
   else {
      for (i = 0; i < size; ++i) {
         if ('1' == value[i]) formula.push_back(getPosVar(var + i));
         else if ('0' == value[i]) formula.push_back(getNegVar(var + i));
      }
   }
}

void
V3SvrMiniSat::assertImplyUnion(const V3SvrDataVec& vars) {
   // Construct a CNF formula (var1 + var2 + ... + varn) and add to the solver
   if (vars.size() == 0) return; vec<Lit> lits; lits.clear();
   for (V3SvrDataVec::const_iterator it = vars.begin(); it != vars.end(); ++it) {
      assert (*it); lits.push(mkLit(getOriVar(*it), isNegFormula(*it)));
   }
   _Solver->addClause(lits); lits.clear();
}

const size_t
V3SvrMiniSat::setTargetValue(const V3NetId& id, const V3BitVecX& value, const uint32_t& depth, const size_t& prev) {
   // Construct formula y = b0 & b1' & b3 & ... & bn', and return variable y
   assert (!prev || !isNegFormula(prev));  // Constrain input prev variable should NOT be negative!
   uint32_t i, size = value.size(); assert (size == _ntk->getNetWidth(id));
   const Var _var = getVerifyData(id, depth); assert (_var);
   Lit aLit = (prev) ? mkLit(getOriVar(prev)) : lit_Undef, bLit, yLit;
   vec<Lit> lits; lits.clear();
   for (i = 0; i < size; ++i) {
      if ('1' == value[i]) bLit = mkLit(_var + i, isV3NetInverted(id));
      else if ('0' == value[i]) bLit = ~mkLit(_var + i, isV3NetInverted(id));
      else bLit = lit_Undef;
      if (!(bLit == lit_Undef)) {
         if (!(aLit == lit_Undef)) {
            yLit = mkLit(newVar(1));
            lits.push(aLit); lits.push(~yLit); _Solver->addClause(lits); lits.clear();
            lits.push(bLit); lits.push(~yLit); _Solver->addClause(lits); lits.clear();
            lits.push(~aLit); lits.push(~bLit); lits.push(yLit); _Solver->addClause(lits); lits.clear();
            aLit = yLit; assert (!sign(aLit));
         }
         else aLit = bLit;
      }
   }
   if (aLit == lit_Undef) return 0;
   else if (sign(aLit)) {
      yLit = mkLit(newVar(1));
      lits.push(~aLit); lits.push(yLit); _Solver->addClause(lits); lits.clear();
      lits.push(aLit); lits.push(~yLit); _Solver->addClause(lits); lits.clear();
      aLit = yLit;
   }
   assert (!isNegFormula(getPosVar(var(aLit))));
   return getPosVar(var(aLit));
}

const size_t
V3SvrMiniSat::setImplyUnion(const V3SvrDataVec& vars) {
   // Construct a CNF formula (y' + var1 + var2 + ... + varn), and return variable y
   if (vars.size() == 0) return 0; vec<Lit> lits; lits.clear();
   Lit lit = mkLit(newVar(1), true); lits.push(lit);
   for (V3SvrDataVec::const_iterator it = vars.begin(); it != vars.end(); ++it) {
      assert (*it); lits.push(mkLit(getOriVar(*it), isNegFormula(*it)));
   }
   _Solver->addClause(lits); lits.clear();
   assert (!isNegFormula(getPosVar(var(lit))));
   return getPosVar(var(lit));
}

const size_t
V3SvrMiniSat::setImplyIntersection(const V3SvrDataVec& vars) {
   // Goal : y --> (var1 && var2 && ... && varn)
   // Construct CNF formulas (y' + var1) && (y' + var2) &&  ... (y' + varn), and return variable y
   if (vars.size() == 0) return 0;
   Lit lit = mkLit(newVar(1), true);
   vec<Lit> lits; lits.clear();
   for (V3SvrDataVec::const_iterator it = vars.begin(); it != vars.end(); ++it) {
      assert (*it); lits.push(lit);
      lits.push(mkLit(getOriVar(*it), isNegFormula(*it)));
      _Solver->addClause(lits); lits.clear();
   }
   assert (!isNegFormula(getPosVar(var(lit))));
   return getPosVar(var(lit));
}

const size_t
V3SvrMiniSat::setImplyInit() {
   Lit lit = mkLit(newVar(1), true);
   vec<Lit> lits; lits.clear();
   for (uint32_t i = 0; i < _init.size(); ++i) {
      lits.push(lit); lits.push(_init[i]); _Solver->addClause(lits); lits.clear();
   }
   assert (!isNegFormula(getPosVar(var(lit))));
   return getPosVar(var(lit));
}

const V3BitVecX
V3SvrMiniSat::getDataValue(const V3NetId& id, const uint32_t& depth) const {
   Var var = getVerifyData(id, depth); assert (var);
   uint32_t i, width = _ntk->getNetWidth(id);
   V3BitVecX value(width);
   if (isV3NetInverted(id)) {
      for (i = 0; i < width; ++i)
         if (l_True == _Solver->model[var + i]) value.set0(i);
         else value.set1(i);
   }
   else {
      for (i = 0; i < width; ++i)
         if (l_True == _Solver->model[var + i]) value.set1(i);
         else value.set0(i);
   }
   return value;
}

const bool
V3SvrMiniSat::getDataValue(const size_t& var) const {
   return (isNegFormula(var)) ^ (l_True == _Solver->model[getOriVar(var)]);
}

void
V3SvrMiniSat::getDataConflict(V3SvrDataVec& vars) const {
   for (int i = 0; i < _Solver->conflict.size(); ++i)
      vars.push_back(getPosVar(var(_Solver->conflict[i])));
}

const size_t
V3SvrMiniSat::getFormula(const V3NetId& id, const uint32_t& depth) {
   Var var = getVerifyData(id, depth); assert (var);
   assert (!isNegFormula(getPosVar(var)));
   return (isV3NetInverted(id) ? getNegVar(var) : getPosVar(var));
}

const size_t
V3SvrMiniSat::getFormula(const V3NetId& id, const uint32_t& bit, const uint32_t& depth) {
   Var var = getVerifyData(id, depth); assert (var);
   assert (bit < _ntk->getNetWidth(id)); assert (!isNegFormula(getPosVar(var + bit)));
   return (isV3NetInverted(id) ? getNegVar(var + bit) : getPosVar(var + bit));
}

// Print Data Functions
void
V3SvrMiniSat::printInfo() const {
   Msg(MSG_IFO) << "#Vars = " << _Solver->nVars() << ", #Cls = " << _Solver->nClauses() << ", " 
                << "#SV = " << totalSolves() << ", AccT = " << totalTime();
}

void
V3SvrMiniSat::printVerbose() const {
   //_Solver->toDimacs(stdout, vec<Lit>());
}

// Resource Functions
const double
V3SvrMiniSat::getTime() const {
   return totalTime();
}

const int
V3SvrMiniSat::getMemory() const {
   // NOTE: 1G for 16M clauses
   return _Solver->nClauses() >> 4;
}

// Gate Formula to Solver Functions
void
V3SvrMiniSat::add_FALSE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (AIG_FALSE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   _ntkData[index].push_back(newVar(1)); assert (getVerifyData(out, depth));
   _Solver->addClause(mkLit(_ntkData[index].back(), true));
}

void
V3SvrMiniSat::add_PI_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (V3_PI == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   _ntkData[index].push_back(newVar(_ntk->getNetWidth(out))); assert (getVerifyData(out, depth));
}

void
V3SvrMiniSat::add_FF_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (V3_FF == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   if (_freeBound) {
      // Set SATVar
      _ntkData[index].push_back(newVar(width));
   }
   else if (depth) {
      // Build FF I/O Relation
      const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
      const Var var1 = getVerifyData(in1, depth - 1); assert (var1);
      // Set SATVar
      if (isV3NetInverted(in1)) {
         _ntkData[index].push_back(newVar(width));
         for (uint32_t i = 0; i < width; ++i) 
            buf(_Solver, mkLit(_ntkData[index].back() + i), mkLit(var1 + i, true));
      }
      else _ntkData[index].push_back(var1);
   }
   else {
      // Set SATVar
      _ntkData[index].push_back(newVar(width));
      const Var& var = _ntkData[index].back();
      // Build FF Initial State
      const V3NetId in1 = _ntk->getInputNetId(out, 1); assert (validNetId(in1));
      const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk);
      if (ntk) {
         if (BV_CONST == ntk->getGateType(in1)) {
            const V3BitVecX value = ntk->getInputConstValue(in1); assert (width == value.size());
            for (uint32_t i = 0; i < width; ++i)
               if ('X' != value[i]) _init.push_back(mkLit(var + i, '0' == value[i]));
         }
         else if (out.id != in1.id) {  // Build Initial Circuit
            const Var var1 = getVerifyData(in1, 0); assert (var1);
            const Var initVar = newVar(width + 1);
            for (uint32_t i = 0; i < width; ++i) 
               xor_2(_Solver, mkLit(1 + initVar + i, true), mkLit(var + i), mkLit(var1 + i, isV3NetInverted(in1)));
            and_red(_Solver, mkLit(initVar), mkLit(1 + initVar), width); _init.push_back(mkLit(initVar));
         }
      }
      else {
         if (AIG_FALSE == _ntk->getGateType(in1)) _init.push_back(mkLit(var, !isV3NetInverted(in1)));
         else if (out.id != in1.id) {  // Build Initial Circuit
            const Var var1 = getVerifyData(in1, 0); assert (var1);
            const Var initVar = newVar(1);
            xor_2(_Solver, mkLit(initVar, true), mkLit(var), mkLit(var1, isV3NetInverted(in1)));
            _init.push_back(mkLit(initVar));
         }
      }
   }
   assert (getVerifyData(out, depth));
}

void
V3SvrMiniSat::add_AND_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (!getVerifyData(out, depth));
   assert ((AIG_NODE == _ntk->getGateType(out)) || (BV_AND == _ntk->getGateType(out)));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(_ntk->getNetWidth(out))); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build AND I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   for (uint32_t i = 0; i < width; ++i) 
      and_2(_Solver, mkLit(var + i), mkLit(var1 + i, isV3NetInverted(in1)), mkLit(var2 + i, isV3NetInverted(in2)));
}

void
V3SvrMiniSat::add_XOR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_XOR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(_ntk->getNetWidth(out))); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build XOR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   for (uint32_t i = 0; i < width; ++i) 
      xor_2(_Solver, mkLit(var + i), mkLit(var1 + i, isV3NetInverted(in1)), mkLit(var2 + i, isV3NetInverted(in2)));
}

void
V3SvrMiniSat::add_MUX_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MUX == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(_ntk->getNetWidth(out))); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build MUX I/O Relation
   const V3NetId fIn = _ntk->getInputNetId(out, 0); assert (validNetId(fIn));
   const V3NetId tIn = _ntk->getInputNetId(out, 1); assert (validNetId(tIn));
   const V3NetId sIn = _ntk->getInputNetId(out, 2); assert (validNetId(sIn));
   const Var fVar = getVerifyData(fIn, depth); assert (fVar);
   const Var tVar = getVerifyData(tIn, depth); assert (tVar);
   const Var sVar = getVerifyData(sIn, depth); assert (sVar);
   for (uint32_t i = 0; i < width; ++i) {
      mux_a(_Solver, mkLit(var + i), mkLit(sVar, isV3NetInverted(sIn)), mkLit(fVar + i, isV3NetInverted(fIn)));
      mux_b(_Solver, mkLit(var + i), mkLit(sVar, isV3NetInverted(sIn)), mkLit(tVar + i, isV3NetInverted(tIn)));
      mux_abo(_Solver, mkLit(var + i), mkLit(fVar + i, isV3NetInverted(fIn)), mkLit(tVar + i, isV3NetInverted(tIn)));
   }
}

void
V3SvrMiniSat::add_RED_AND_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_AND == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   _ntkData[index].push_back(newVar(1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build RED_AND I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   and_red(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), _ntk->getNetWidth(in1));
}

void
V3SvrMiniSat::add_RED_OR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_OR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   _ntkData[index].push_back(newVar(1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build RED_OR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   or_red(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), _ntk->getNetWidth(in1));
}

void
V3SvrMiniSat::add_RED_XOR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_XOR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const uint32_t width = _ntk->getNetWidth(in1); assert (width);
   _ntkData[index].push_back(newVar((width > 1) ? (width - 1) : 1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build RED_XOR I/O Relation
   xor_red(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), _ntk->getNetWidth(in1));
}

void
V3SvrMiniSat::add_ADD_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_ADD == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar((width * 2) - 1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build ADD I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   xor_2(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), mkLit(var2, isV3NetInverted(in2)));  // xor
   if (width > 1) 
      carry_2_woc(_Solver, mkLit(var + width), mkLit(var1, isV3NetInverted(in1)), 
                           mkLit(var2, isV3NetInverted(in2)));  // carry
   for (uint32_t i = 1; i < width; ++i) {
      // FA_sum
      xor_3(_Solver, mkLit(var + i), mkLit(var1 + i, isV3NetInverted(in1)), 
                     mkLit(var2 + i, isV3NetInverted(in2)), mkLit(var + width - 1 + i));
      // FA_carry
      if (i < width - 1) 
         carry_2(_Solver, mkLit(var + width + i), mkLit(var1 + i, isV3NetInverted(in1)), 
                          mkLit(var2 + i, isV3NetInverted(in2)), mkLit(var + width - 1 + i));
   }
}

void
V3SvrMiniSat::add_SUB_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SUB == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar((width * 2) - 1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build SUB I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   xor_2(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), mkLit(var2, isV3NetInverted(in2)));  // xor
   if (width > 1) 
      borrow_2_woB(_Solver, mkLit(var + width), mkLit(var1, isV3NetInverted(in1)), 
                            mkLit(var2, isV3NetInverted(in2)));  // borrow
   for (uint32_t i = 1; i < width; ++i) {
      // Sub_Diff
      xor_3(_Solver, mkLit(var + i), mkLit(var1 + i, isV3NetInverted(in1)), 
                     mkLit(var2 + i, isV3NetInverted(in2)), mkLit(var + width - 1 + i));
      // FA_carry
      if (i < width - 1) 
         borrow_2(_Solver, mkLit(var + width + i), mkLit(var1 + i, isV3NetInverted(in1)), 
                           mkLit(var2 + i, isV3NetInverted(in2)), mkLit(var + width - 1 + i));
   }
}

void
V3SvrMiniSat::add_MULT_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MULT == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   // Note : #variables = C(n, 2) for corresponding Ai * Bj = and(Ai, Bj)
   //                   + 2 * C(n-1, 2) for sum_out and carry of full adders
   //                   - (n-1) carries are not necessary for bounded multiplication
   _ntkData[index].push_back(newVar((width * (width + 1) >> 1) + ((width - 1) * (width - 1))));
   assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build MULT I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   Var curVar = var + width;  // Internal variables start from var + width
   Var prev_vars[width];  // For fast look-up up vars in previous loop
   for (uint32_t i = 1; i < width; ++i) prev_vars[i] = var + width + i - 1;

   // 1. AND results for corresponding ai * bj
   and_2(_Solver, mkLit(var), mkLit(var1, isV3NetInverted(in1)), 
                  mkLit(var2, isV3NetInverted(in2)));  // C[0] = a[0] * b[0]
   for (uint32_t i = 0; i < width; ++i) {
      for (uint32_t j = (i == 0) ? i + 1 : i; j < width; ++j) {
         and_2(_Solver, mkLit(curVar), mkLit(var1 + j - i, isV3NetInverted(in1)), 
                        mkLit(var2 + i, isV3NetInverted(in2))); ++curVar;
      }
   }
   assert (curVar == (var + (int)(width * (width + 1) >> 1) + (int)width - 1));  // var + width + C(width, 2) - 1
   
   // 2. Adders for sum_out and carry
   Var baseVar = var + (width << 1) - 1;  // Also for efficient var lookup built in 1.
   for (uint32_t i = 1; i < width; ++i) {
      xor_2(_Solver, mkLit(var + i), mkLit(prev_vars[i]), mkLit(baseVar));  // C[i] = sum_out(prev_vars[i], baseVar);
      if (i != (width - 1)) {
         carry_2_woc(_Solver, mkLit(curVar), mkLit(prev_vars[i]), mkLit(baseVar)); ++curVar;
      }
      ++baseVar;
      for (uint32_t j = i + 1; j < width; ++j) {
         xor_3(_Solver, mkLit(curVar), mkLit(prev_vars[j]), mkLit(baseVar), mkLit(curVar - 1)); ++curVar;
         if (j != (width - 1)) {
            carry_2(_Solver, mkLit(curVar), mkLit(prev_vars[j]), mkLit(baseVar), mkLit(curVar - 2)); ++curVar;
            prev_vars[j] = curVar - 2;
         }
         else prev_vars[j] = curVar - 1;
         ++baseVar;
      }
   }
   assert (baseVar == (var + (int)(width * (width + 1) >> 1) + (int)width - 1));  // var + width + C(width, 2) - 1
   assert (curVar == _curVar);
}

void
V3SvrMiniSat::add_DIV_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_DIV == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(width)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build DIV I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);

   V3SvrMLitData sVec(width), qVec(width), rVec(width); uint32_t i, j;
   for (i = 0; i < width; ++i) rVec[i] = mkLit(0); i = width;
   while (i--) {
      // Set current dividen r = {(r << 1), a[i]}
      j = width; while (--j) rVec[j] = rVec[j - 1]; rVec[0] = mkLit(var1 + i, isV3NetInverted(in1));
      // Compute s = r - id2 and the final borrow
      for (j = 0; j < width; ++j) sVec[j] = mkLit(newVar(1));
      for (j = 0; j < width - 1; ++j) qVec[j] = mkLit(newVar(1)); qVec[j] = mkLit(var + i, 1);
      xor_2(_Solver, sVec[0], rVec[0], mkLit(var2, isV3NetInverted(in2)));  // xor
      borrow_2_woB(_Solver, qVec[0], rVec[0], mkLit(var2, isV3NetInverted(in2)));  // borrow
      for (j = 1; j < width; ++j) {
         // Sub_Diff
         xor_3(_Solver, sVec[j], rVec[j], mkLit(var2 + j, isV3NetInverted(in2)), qVec[j - 1]);
         // FA_carry
         borrow_2(_Solver, qVec[j], rVec[j], mkLit(var2 + j, isV3NetInverted(in2)), qVec[j - 1]);
      }
      // Set remainder = borrow ? rVec : sVec
      if (i) {  // Omit the computation of final remainder
         qVec = rVec;
         for (j = 0; j < width; ++j) {
            rVec[j] = mkLit(newVar(1));
            mux_a(_Solver, rVec[j], mkLit(var + i), qVec[j]);
            mux_b(_Solver, rVec[j], mkLit(var + i), sVec[j]);
            mux_abo(_Solver, rVec[j], qVec[j], sVec[j]);
         }
      }
   }
}

void
V3SvrMiniSat::add_MODULO_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MODULO == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(width)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build DIV I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);

   V3SvrMLitData sVec(width), qVec(width), rVec(width); uint32_t i, j;
   for (i = 0; i < width; ++i) rVec[i] = mkLit(0); i = width;
   while (i--) {
      // Set current dividen r = {(r << 1), a[i]}
      j = width; while (--j) rVec[j] = rVec[j - 1]; rVec[0] = mkLit(var1 + i, isV3NetInverted(in1));
      // Compute s = r - id2 and the final borrow
      for (j = 0; j < width; ++j) sVec[j] = mkLit(newVar(1));
      for (j = 0; j < width; ++j) qVec[j] = mkLit(newVar(1));
      xor_2(_Solver, sVec[0], rVec[0], mkLit(var2, isV3NetInverted(in2)));  // xor
      borrow_2_woB(_Solver, qVec[0], rVec[0], mkLit(var2, isV3NetInverted(in2)));  // borrow
      for (j = 1; j < width; ++j) {
         // Sub_Diff
         xor_3(_Solver, sVec[j], rVec[j], mkLit(var2 + j, isV3NetInverted(in2)), qVec[j - 1]);
         // FA_carry
         borrow_2(_Solver, qVec[j], rVec[j], mkLit(var2 + j, isV3NetInverted(in2)), qVec[j - 1]);
      }
      // Set remainder = borrow ? rVec : sVec
      const Lit borrow = qVec[j - 1]; qVec = rVec;
      for (j = 0; j < width; ++j) {
         rVec[j] = i ? mkLit(newVar(1)) : mkLit(var + j);
         mux_a(_Solver, rVec[j], borrow, sVec[j]);
         mux_b(_Solver, rVec[j], borrow, qVec[j]);
         mux_abo(_Solver, rVec[j], sVec[j], qVec[j]);
      }
   }
}

void
V3SvrMiniSat::add_SHL_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SHL == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   // Build SHL I/O Relation
   if (BV_CONST == _ntk->getGateType(in2) && (sizeof(int) << 3) > _ntk->getNetWidth(in2)) {
      _ntkData[index].push_back(newVar(width));
      const Var& var = _ntkData[index].back();
      const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
      const V3BitVecX value = ntk->getInputConstValue(in2);
      if (value.size() == value.firstX()) {
         const int shift = value.value();
         for (int i = width; i > 0; --i) {
            if (i > shift) buf(_Solver, mkLit(var + i - 1), mkLit(var1 + ((i - 1) - shift), isV3NetInverted(in1)));
            else _Solver->addClause(mkLit(var + i - 1, true));
         }
         assert (getVerifyData(out, depth)); return;
      }
   }
   const int aWidth = _ntk->getNetWidth(in1), bWidth = _ntk->getNetWidth(in2);
   int shift_width = (int)ceil(log2(aWidth + 1));
   shift_width = (shift_width < bWidth) ? shift_width : bWidth;
   _ntkData[index].push_back(newVar(aWidth * shift_width));
   const Var& var = _ntkData[index].back();
   int start_index, prev_index, pow_shift;
   bool is_prev_inverted;
   // Compute OR msb of var2 starting from index = shift_width
   V3SvrMLitData shift(shift_width); assert (shift_width);
   for (int i = 0; i < (shift_width - 1); ++i) shift[i] = mkLit(var2 + i, isV3NetInverted(in2));
   vec<Lit> ORlits; ORlits.clear(); Var ORVar = newVar(1); shift[shift_width - 1] = mkLit(ORVar);
   for (int i = shift_width - 1; i < bWidth; ++i) {
      ORlits.push(mkLit(var2 + i, !isV3NetInverted(in2)));
      ORlits.push(mkLit(ORVar)); _Solver->addClause(ORlits); ORlits.clear();
   }
   for (int i = shift_width - 1; i < bWidth; ++i) ORlits.push(mkLit(var2 + i, isV3NetInverted(in2)));
   ORlits.push(mkLit(ORVar, 1)); _Solver->addClause(ORlits); ORlits.clear();
   for (int i = 0; i < shift_width; ++i) {
      start_index = var + ((shift_width - 1 - i) * aWidth);
      prev_index = (i == 0) ? var1 : (start_index + aWidth);
      is_prev_inverted = (i == 0) ? isV3NetInverted(in1) : false;
      pow_shift = (int)pow(2.00, i);  // Sub-Shift Process : a << pow_shift
      for (int j = 0; j < pow_shift; ++j)
         and_2(_Solver, mkLit(start_index + j), mkLit(prev_index + j, is_prev_inverted), ~shift[i]);  // c[j] = a[j] & !shift
      for (int j = pow_shift; j < aWidth; ++j)
         sh_select(_Solver, mkLit(start_index + j), mkLit(prev_index + j - pow_shift, is_prev_inverted), 
                            mkLit(prev_index + j, is_prev_inverted), shift[i]);
   }
   assert (getVerifyData(out, depth));
}

void
V3SvrMiniSat::add_SHR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SHR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   // Build SHR I/O Relation
   if (BV_CONST == _ntk->getGateType(in2) && (sizeof(int) << 3) > _ntk->getNetWidth(in2)) {
      _ntkData[index].push_back(newVar(width));
      const Var& var = _ntkData[index].back();
      const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
      const V3BitVecX value = ntk->getInputConstValue(in2);
      if (value.size() == value.firstX()) {
         const int shift = value.value();
         for (int i = width; i > 0; --i) {	  
            if (i > shift) buf(_Solver, mkLit(var + ((i - 1) - shift)), mkLit(var1 + i - 1, isV3NetInverted(in1)));
            else _Solver->addClause(mkLit(var + width + i - shift - 1, true));
         }
         assert (getVerifyData(out, depth)); return;
      }
   }
   const int aWidth = _ntk->getNetWidth(in1), bWidth = _ntk->getNetWidth(in2);
   int shift_width = (int)ceil(log2(aWidth + 1));
   shift_width = (shift_width < bWidth) ? shift_width : bWidth;
   _ntkData[index].push_back(newVar(aWidth * shift_width));
   const Var& var = _ntkData[index].back();
   int start_index, prev_index, pow_shift;
   bool is_prev_inverted;
   // Compute OR msb of var2 starting from index = shift_width
   V3SvrMLitData shift(shift_width); assert (shift_width);
   for (int i = 0; i < (shift_width - 1); ++i) shift[i] = mkLit(var2 + i, isV3NetInverted(in2));
   vec<Lit> ORlits; ORlits.clear(); Var ORVar = newVar(1); shift[shift_width - 1] = mkLit(ORVar);
   for (int i = shift_width - 1; i < bWidth; ++i) {
      ORlits.push(mkLit(var2 + i, !isV3NetInverted(in2)));
      ORlits.push(mkLit(ORVar)); _Solver->addClause(ORlits); ORlits.clear();
   }
   for (int i = shift_width - 1; i < bWidth; ++i) ORlits.push(mkLit(var2 + i, isV3NetInverted(in2)));
   ORlits.push(mkLit(ORVar, 1)); _Solver->addClause(ORlits); ORlits.clear();
   for (int i = 0; i < shift_width; ++i) {
      start_index = var + ((shift_width - 1 - i) * aWidth);
      prev_index = (i == 0) ? var1 : (start_index + aWidth);
      is_prev_inverted = (i == 0) ? isV3NetInverted(in1) : false;
      pow_shift = (int)pow(2.00, i);  // Sub-Shift Process : a >> pow_shift
      for (int j = 0; j < aWidth - pow_shift; ++j)
         sh_select(_Solver, mkLit(start_index + j), mkLit(prev_index + pow_shift + j, is_prev_inverted), 
                            mkLit(prev_index + j, is_prev_inverted), shift[i]);
      for (int j = aWidth - pow_shift; j < aWidth; ++j)
         and_2(_Solver, mkLit(start_index + j), mkLit(prev_index + j, is_prev_inverted), ~shift[i]);  // c[j] = a[j] & !shift
   }
   assert (getVerifyData(out, depth));
}

void
V3SvrMiniSat::add_CONST_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_CONST == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(width)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build CONST I/O Relation
   const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
   const V3BitVecX value = ntk->getInputConstValue(V3NetId::makeNetId(out.id)); assert (width == value.size());
   for (uint32_t i = 0; i < width; ++i)
      if ('X' != value[i]) _Solver->addClause(mkLit(var + i, '0' == value[i]));
}

void
V3SvrMiniSat::add_SLICE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SLICE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build SLICE I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
   const uint32_t msb = ntk->getInputSliceBit(out, true);
   const uint32_t lsb = ntk->getInputSliceBit(out, false);
   // Set SATVar
   if (msb >= lsb) {
      if (isV3NetInverted(in1)) {
         const uint32_t width = _ntk->getNetWidth(out); assert (width);
         _ntkData[index].push_back(newVar(width));
         const Var& var = _ntkData[index].back();
         for (uint32_t i = 0; i < width; ++i) 
            buf(_Solver, mkLit(var + i), mkLit(var1 + lsb + i, isV3NetInverted(in1)));
      }
      else _ntkData[index].push_back(var1 + lsb);
   }
   else {
      const uint32_t width = _ntk->getNetWidth(out); assert (width);
      _ntkData[index].push_back(newVar(width));
      const Var& var = _ntkData[index].back();
      for (uint32_t i = 0; i < width; ++i) 
         buf(_Solver, mkLit(var + i), mkLit(var1 + lsb - i, isV3NetInverted(in1)));
   }
   assert (getVerifyData(out, depth));
}

void
V3SvrMiniSat::add_MERGE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MERGE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Set SATVar
   _ntkData[index].push_back(newVar(width)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build MERGE I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   const uint32_t width_in1 = _ntk->getNetWidth(in1), width_in2 = _ntk->getNetWidth(in2);
   for (int i = 0, j = (int)width_in2; i < j; ++i) 
      buf(_Solver, mkLit(var + i), mkLit(var2 + i, isV3NetInverted(in2)));
   for (int i = 0, j = (int)width_in1; i < j; ++i) 
      buf(_Solver, mkLit(var + width_in2 + i), mkLit(var1 + i, isV3NetInverted(in1)));
}

void
V3SvrMiniSat::add_EQUALITY_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_EQUALITY == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const uint32_t width = _ntk->getNetWidth(in1); assert (width);
   _ntkData[index].push_back(newVar(width + 1)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build EQUALITY I/O Relation
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   for (uint32_t i = 0; i < width; ++i) 
      xor_2(_Solver, mkLit(1 + var + i, true), 
                     mkLit(var1 + i, isV3NetInverted(in1)), mkLit(var2 + i, isV3NetInverted(in2)));
   and_red(_Solver, mkLit(var), mkLit(1 + var), width);
}

void
V3SvrMiniSat::add_GEQ_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_GEQ == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set SATVar
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const uint32_t width = _ntk->getNetWidth(in1); assert (width);
   _ntkData[index].push_back(newVar(width * 3)); assert (getVerifyData(out, depth));
   const Var& var = _ntkData[index].back();
   // Build GEQ I/O Relation
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   const Var var1 = getVerifyData(in1, depth); assert (var1);
   const Var var2 = getVerifyData(in2, depth); assert (var2);
   geq(_Solver, mkLit(var + width), mkLit(var1, isV3NetInverted(in1)), mkLit(var2, isV3NetInverted(in2)));  // geq
   for (uint32_t i = 1; i < width; ++i) {
      xor_2(_Solver, mkLit(var + i, true), 
                     mkLit(var1 + i, isV3NetInverted(in1)), mkLit(var2 + i, isV3NetInverted(in2)));  // equal
      greater_2(_Solver, mkLit(var + width + i), mkLit(var1 + i, isV3NetInverted(in1)), 
                         mkLit(var2 + i, isV3NetInverted(in2)));  // geq
   }
   geqInt(_Solver, var + (width * 2), var + width, var + 1, width);
   or_red(_Solver, mkLit(var), mkLit(var + (width * 2)), width);  // OR part
}

// Network to Solver Functions
const bool
V3SvrMiniSat::existVerifyData(const V3NetId& id, const uint32_t& depth) {
   return getVerifyData(id, depth);
}

// MiniSat Functions
const Var
V3SvrMiniSat::newVar(const uint32_t& width) {
   Var cur_var = _curVar;
   for (uint32_t i = 0; i < width; ++i) _Solver->newVar();
   _curVar += width; return cur_var;
}

const Var
V3SvrMiniSat::getVerifyData(const V3NetId& id, const uint32_t& depth) const {
   assert (validNetId(id));
   if (depth >= _ntkData[getV3NetIndex(id)].size()) return 0;
   else return _ntkData[getV3NetIndex(id)][depth];
}

#endif

