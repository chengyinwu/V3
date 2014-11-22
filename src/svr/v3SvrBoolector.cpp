/****************************************************************************
  FileName     [ v3SvrBoolector.cpp ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ V3 Solver with Boolector as Engine. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef BV_SVR_BOOLECTOR_C
#define BV_SVR_BOOLECTOR_C

#include "v3Msg.h"
#include "v3SvrBoolector.h"

/* -------------------------------------------------- *\
 * Class V3SvrBoolector Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3SvrBoolector::V3SvrBoolector(const V3Ntk* const ntk, const bool& freeBound) : V3SvrBase(ntk, freeBound) {
   _Solver = boolector_new(); assert (_Solver);
   boolector_enable_model_gen(_Solver);   // Enable SAT model return
   boolector_enable_inc_usage(_Solver);   // Enable incremental SAT
   setRewriteLevel(); initRelease(); assumeRelease();
   _ntkData.clear(); _ntkData.reserve(ntk->getNetSize());
   for (uint32_t i = 0; i < ntk->getNetSize(); ++i) { _ntkData.push_back(V3BtorExpVec()); _ntkData.back().clear(); }
}

V3SvrBoolector::V3SvrBoolector(const V3SvrBoolector& solver) : V3SvrBase(solver._ntk, solver._freeBound) {
   _Solver = boolector_new(); assert (_Solver);
   boolector_enable_model_gen(_Solver);   // Enable SAT model return
   boolector_enable_inc_usage(_Solver);   // Enable incremental SAT
   setRewriteLevel(); initRelease(); assumeRelease();
   _ntkData.clear(); _ntkData.reserve(_ntk->getNetSize());
   for (uint32_t i = 0; i < _ntk->getNetSize(); ++i) { _ntkData.push_back(V3BtorExpVec()); _ntkData.back().clear(); }
}

V3SvrBoolector::~V3SvrBoolector() {
   boolector_delete(_Solver); initRelease(); assumeRelease();
   for (uint32_t i = 0; i < _ntkData.size(); ++i) _ntkData[i].clear(); _ntkData.clear();
}

// Basic Operation Functions
void
V3SvrBoolector::reset() {
   _Solver = boolector_new(); assert (_Solver);
   boolector_enable_model_gen(_Solver);   // Enable SAT model return
   boolector_enable_inc_usage(_Solver);   // Enable incremental SAT
   setRewriteLevel(); initRelease(); assumeRelease();
   for (uint32_t i = 0; i < _ntkData.size(); ++i) _ntkData[i].clear();
   if (_ntkData.size() == _ntk->getNetSize()) return; _ntkData.clear(); _ntkData.reserve(_ntk->getNetSize());
   for (uint32_t i = 0; i < _ntk->getNetSize(); ++i) { _ntkData.push_back(V3BtorExpVec()); _ntkData.back().clear(); }
}

void
V3SvrBoolector::update() {
   assert (_Solver); assert (_ntk->getNetSize() >= _ntkData.size());
   V3BtorExpVec svrData; svrData.clear(); _ntkData.resize(_ntk->getNetSize(), svrData);
}

void
V3SvrBoolector::assumeInit() {
   for (uint32_t i = 0; i < _init.size(); ++i) _assump.push_back(boolector_copy(_Solver, _init[i]));
}

void
V3SvrBoolector::assertInit() {
   for (uint32_t i = 0; i < _init.size(); ++i) boolector_assert(_Solver, _init[i]);
}

void
V3SvrBoolector::initRelease() {
   //for (uint32_t i = 0; i < _init.size(); ++i) boolector_release(_Solver, _init[i]);
   _init.clear();
}

void
V3SvrBoolector::assumeRelease() {
   //for (uint32_t i = 0; i < _assump.size(); ++i) boolector_release(_Solver, _assump[i]);
   _assump.clear();
}

void
V3SvrBoolector::assumeProperty(const size_t& exp, const bool& invert) {
   _assump.push_back(invert ^ isNegFormula(exp) ? boolector_not(_Solver, getOriExp(exp)) : 
                                                  boolector_copy(_Solver, getOriExp(exp)));
}

void
V3SvrBoolector::assertProperty(const size_t& exp, const bool& invert) {
   if (invert ^ isNegFormula(exp)) {
      BtorExp* invExp = boolector_not(_Solver, getOriExp(exp));
      boolector_assert(_Solver, invExp);
      boolector_release(_Solver, invExp);
   }
   else boolector_assert(_Solver, getOriExp(exp));
}

void
V3SvrBoolector::assumeProperty(const V3NetId& id, const bool& invert, const uint32_t& depth) {
   assert (validNetId(id)); assert (1 == _ntk->getNetWidth(id));
   BtorExp* const exp = getVerifyData(id, depth); assert (exp);
   if (!invert) _assump.push_back(boolector_copy(_Solver, exp));
   else _assump.push_back(boolector_not(_Solver, exp));
}

void
V3SvrBoolector::assertProperty(const V3NetId& id, const bool& invert, const uint32_t& depth) {
   assert (validNetId(id)); assert (1 == _ntk->getNetWidth(id));
   BtorExp* const exp = getVerifyData(id, depth); assert (exp);
   if (!invert) boolector_assert(_Solver, exp);
   else boolector_assert(_Solver, boolector_not(_Solver, exp));
}

const bool
V3SvrBoolector::simplify() {
   return true;  // NO Conflict
}

const bool
V3SvrBoolector::solve() {
   double ctime = (double)clock() / CLOCKS_PER_SEC;
   uint32_t result = boolector_sat(_Solver); ++_solves;
   _runTime += (((double)clock() / CLOCKS_PER_SEC) - ctime);
   return (BOOLECTOR_SAT == result);
}

const bool
V3SvrBoolector::assump_solve() {
   for (uint32_t i = 0; i < _assump.size(); ++i) boolector_assume(_Solver, _assump[i]);
   bool result = solve(); return result;
}

// Manipulation Helper Functions
void
V3SvrBoolector::setTargetValue(const V3NetId& id, const V3BitVecX& value, const uint32_t& depth, V3SvrDataVec& formula) {
   // Note : This Function will set formula such that AND(formula) represents (id == value)
   uint32_t size = value.size(); assert (size == _ntk->getNetWidth(id));
   BtorExp* const aExp = getVerifyData(id, depth); assert (aExp);
   char* bv_value = new char[size + 1];
   BtorExp *bExp, *cExp;
   uint32_t i = size, j = 0;
   while (i--) {
      if ('1' == value[i]) bv_value[j++] = '1';
      else if ('0' == value[i]) bv_value[j++] = '0';
      else if (j) {
         bv_value[j] = '\0';
         bExp = boolector_slice(_Solver, aExp, i + j, i + 1);
         cExp = boolector_const(_Solver, bv_value); j = 0;
         formula.push_back(getPosExp(boolector_eq(_Solver, bExp, cExp)));
         boolector_release(_Solver, bExp); boolector_release(_Solver, cExp);
      }
   }
   if (j) {
      bv_value[j] = '\0';
      if (j == size) bExp = boolector_copy(_Solver, aExp);
      else bExp = boolector_slice(_Solver, aExp, j - 1, 0);
      cExp = boolector_const(_Solver, bv_value);
      formula.push_back(getPosExp(boolector_eq(_Solver, bExp, cExp)));
      boolector_release(_Solver, bExp); boolector_release(_Solver, cExp);
   }
   delete[] bv_value;
}

void
V3SvrBoolector::assertImplyUnion(const V3SvrDataVec& Exps) {
   assertProperty(setImplyUnion(Exps));
   /*
   if (Exps.size() == 0) return;
   vector<size_t>::const_iterator it = Exps.begin(); assert (*it);
   BtorExp *aExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
   BtorExp *bExp, *oExp; ++it;
   for (; it != Exps.end(); ++it) {
      assert (*it); assert (aExp);
      bExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
      oExp = boolector_or(_Solver, aExp, bExp); assert (oExp);
      boolector_release(_Solver, aExp);
      boolector_release(_Solver, bExp);
      aExp = oExp;
   }
   boolector_assert(_Solver, aExp);
   boolector_release(_Solver, aExp);
   */
}

const size_t
V3SvrBoolector::setTargetValue(const V3NetId& id, const V3BitVecX& value, const uint32_t& depth, const size_t& prev) {
   // Construct formula y = b0 & b1' & b3 & ... & bn', and return expr y
   if (prev) assert (!isNegFormula(prev));  // Constrain input prev expr should NOT be negative!
   uint32_t size = value.size(); assert (size == _ntk->getNetWidth(id));
   BtorExp* const aExp = getVerifyData(id, depth); assert (aExp);
   BtorExp* pExp = (prev) ? boolector_copy(_Solver, getOriExp(prev)) : 0, *bExp, *cExp, *eExp;
   char* bv_value = new char[size + 1];
   uint32_t i = size, j = 0;
   while (i--) {
      if ('1' == value[i]) bv_value[j++] = '1';
      else if ('0' == value[i]) bv_value[j++] = '0';
      else if (j) {
         bv_value[j] = '\0';
         bExp = boolector_slice(_Solver, aExp, i + j, i + 1);
         cExp = boolector_const(_Solver, bv_value);
         eExp = boolector_eq(_Solver, bExp, cExp);
         boolector_release(_Solver, bExp);
         boolector_release(_Solver, cExp);
         if (pExp) {
            bExp = boolector_and(_Solver, pExp, eExp);
            boolector_release(_Solver, pExp);
            boolector_release(_Solver, eExp);
            pExp = bExp;
         }
         else pExp = eExp;
         j = 0;
      }
   }
   if (j) {
      bv_value[j] = '\0';
      if (j == size) bExp = boolector_copy(_Solver, aExp);
      else bExp = boolector_slice(_Solver, aExp, j - 1, 0);
      cExp = boolector_const(_Solver, bv_value);
      eExp = boolector_eq(_Solver, bExp, cExp);
      boolector_release(_Solver, bExp);
      boolector_release(_Solver, cExp);
      if (pExp) {
         bExp = boolector_and(_Solver, pExp, eExp);
         boolector_release(_Solver, pExp);
         boolector_release(_Solver, eExp);
         pExp = bExp;
      }
      else pExp = eExp;
   }
   delete[] bv_value;

   assert (!isNegFormula(getPosExp(pExp)));
   return getPosExp(pExp);
}

const size_t
V3SvrBoolector::setImplyUnion(const V3SvrDataVec& Exps) {
   if (Exps.size() == 0) return 0;
   vector<size_t>::const_iterator it = Exps.begin(); assert (*it);
   BtorExp *aExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
   BtorExp *bExp, *oExp; ++it;
   for (; it != Exps.end(); ++it) {
      assert (*it); assert (aExp);
      bExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
      oExp = boolector_or(_Solver, aExp, bExp); assert (oExp);
      boolector_release(_Solver, aExp);
      boolector_release(_Solver, bExp);
      aExp = oExp;
   }
   bExp = boolector_var(_Solver, 1, NULL);
   oExp = boolector_implies(_Solver, bExp, aExp);
   boolector_assert(_Solver, oExp);
   boolector_release(_Solver, oExp);
   boolector_release(_Solver, aExp);
   
   assert (!isNegFormula(getPosExp(bExp)));
   assert (bExp); return getPosExp(bExp);
}

const size_t
V3SvrBoolector::setImplyIntersection(const V3SvrDataVec& Exps) {
   if (Exps.size() == 0) return 0;
   vector<size_t>::const_iterator it = Exps.begin(); assert (*it);
   BtorExp *aExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
   BtorExp *bExp, *oExp; ++it;
   for (; it != Exps.end(); ++it) {
      assert (*it); assert (aExp);
      bExp = (isNegFormula(*it) ? boolector_not(_Solver, getOriExp(*it)) : boolector_copy(_Solver, getOriExp(*it)));
      oExp = boolector_and(_Solver, aExp, bExp); assert (oExp);
      boolector_release(_Solver, aExp);
      boolector_release(_Solver, bExp);
      aExp = oExp;
   }
   bExp = boolector_var(_Solver, 1, NULL);
   oExp = boolector_implies(_Solver, bExp, aExp);
   boolector_assert(_Solver, oExp);
   boolector_release(_Solver, oExp);
   boolector_release(_Solver, aExp);
   
   assert (!isNegFormula(getPosExp(bExp)));
   assert (bExp); return getPosExp(bExp);
}

const size_t
V3SvrBoolector::setImplyInit() {
   V3SvrDataVec init_states; init_states.clear();
   for (uint32_t i = 0; i < _init.size(); ++i) init_states.push_back(getPosExp(_init[i]));
   return setImplyIntersection(init_states);
}

// Retrieval Functions
const V3BitVecX
V3SvrBoolector::getDataValue(const V3NetId& id, const uint32_t& depth) const {
   BtorExp* const exp = getVerifyData(id, depth); if (!exp) return 0;
   const uint32_t width = _ntk->getNetWidth(id); assert (width);
   char* const btor_value = boolector_bv_assignment(_Solver, exp);
   assert (btor_value); assert (width == strlen(btor_value));
   V3BitVecX value(width);
   for (uint32_t i = 0, j = width - 1; i < width; ++i, --j) {
      if ('1' == btor_value[j]) value.set1(i);
      else if ('0' == btor_value[j]) value.set0(i);
   }
   boolector_free_bv_assignment(_Solver, btor_value); return value;
}

const bool
V3SvrBoolector::getDataValue(const size_t& exp) const {
   assert (!isNegFormula(exp));
   char* const btor_value = boolector_bv_assignment(_Solver, getOriExp(exp));
   assert (btor_value); assert (1 == strlen(btor_value));
   const bool ret_value = ('0' != btor_value[0]);
   boolector_free_bv_assignment(_Solver, btor_value); return ret_value;
}

void
V3SvrBoolector::getDataConflict(V3SvrDataVec& Exps) const {
   // Return ... Will be performed then...
   return;
}

const size_t
V3SvrBoolector::getFormula(const V3NetId& id, const uint32_t& depth) {
   BtorExp* const exp = getVerifyData(id, depth); if (!exp) return 0;
   assert (!isNegFormula(getPosExp(exp))); return getPosExp(exp);
}

const size_t
V3SvrBoolector::getFormula(const V3NetId& id, const uint32_t& bit, const uint32_t& depth) {
   BtorExp* const exp = getVerifyData(id, depth); if (!exp) return 0;
   assert (bit < _ntk->getNetWidth(id));
   BtorExp* const bExp = (_ntk->getNetWidth(id) == 1) ? exp : boolector_slice(_Solver, exp, bit, bit);
   assert (!isNegFormula(getPosExp(bExp))); return getPosExp(bExp);
}

// Print Data Functions
void
V3SvrBoolector::printInfo() const {
   Msg(MSG_IFO) << "#Ref = " << boolector_get_refs(_Solver) << ", "
                << "#SV = " << totalSolves() << ", AccT = " << totalTime();
}

// Gate Formula to Solver Functions
void
V3SvrBoolector::add_FALSE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (AIG_FALSE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set BtorExp*
   _ntkData[index].push_back(boolector_const(_Solver, "0")); assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_PI_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (V3_PI == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Set BtorExp*
   _ntkData[index].push_back(boolector_var(_Solver, _ntk->getNetWidth(out), NULL));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_FF_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (V3_FF == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   if (_freeBound) {
      // Set BtorExp*
      _ntkData[index].push_back(boolector_var(_Solver, width, NULL));
   }
   else if (depth) {
      // Build FF I/O Relation
      const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
      BtorExp* const exp1 = getVerifyData(in1, depth - 1); assert (exp1);
      // Set BtorExp*
      _ntkData[index].push_back(boolector_copy(_Solver, exp1));
   }
   else {
      // Set BtorExp*
      _ntkData[index].push_back(boolector_var(_Solver, width, NULL));
      BtorExp* const exp = _ntkData[index].back(); assert (exp);
      // Build FF Initial State
      const V3NetId in1 = _ntk->getInputNetId(out, 1); assert (validNetId(in1));
      const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk);
      if (ntk) {
         if (BV_CONST == ntk->getGateType(in1)) {
            const V3BitVecX value = ntk->getInputConstValue(in1); assert (width == value.size());
            char* bv_value = new char[width + 1]; bv_value[width] = '\0';
            for (uint32_t i = 0, j = width - 1; i < width; ++i, --j) bv_value[j] = value[i];
            BtorExp* const init_exp = boolector_const(_Solver, bv_value); assert (init_exp);
            _init.push_back(boolector_eq(_Solver, exp, init_exp));
            delete[] bv_value; boolector_release(_Solver, init_exp);
         }
         else if (out.id != in1.id) {  // Build Initial Circuit
            BtorExp* const exp1 = getVerifyData(in1, 0); assert (exp1);
            _init.push_back(boolector_eq(_Solver, exp, exp1));
         }
      }
      else {
         if (AIG_FALSE == _ntk->getGateType(in1)) 
            _init.push_back(!isV3NetInverted(in1) ? boolector_not(_Solver, exp) : boolector_copy(_Solver, exp));
         else if (out.id != in1.id) {  // Build Initial Circuit
            BtorExp* const exp1 = getVerifyData(in1, 0); assert (exp1);
            _init.push_back(boolector_eq(_Solver, exp, exp1));
         }
      }
   }
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_AND_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (!getVerifyData(out, depth));
   assert ((AIG_NODE == _ntk->getGateType(out)) || (BV_AND == _ntk->getGateType(out)));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build AND I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_and(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_XOR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_XOR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build XOR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_xor(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_MUX_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MUX == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build MUX I/O Relation
   const V3NetId fIn = _ntk->getInputNetId(out, 0); assert (validNetId(fIn));
   const V3NetId tIn = _ntk->getInputNetId(out, 1); assert (validNetId(tIn));
   const V3NetId sIn = _ntk->getInputNetId(out, 2); assert (validNetId(sIn));
   BtorExp* const fExp = getVerifyData(fIn, depth); assert (fExp);
   BtorExp* const tExp = getVerifyData(tIn, depth); assert (tExp);
   BtorExp* const sExp = getVerifyData(sIn, depth); assert (sExp);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_cond(_Solver, sExp, tExp, fExp));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_RED_AND_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_AND == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build RED_AND I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_redand(_Solver, exp1));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_RED_OR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_OR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build RED_OR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_redor(_Solver, exp1));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_RED_XOR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_RED_XOR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build RED_XOR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_redxor(_Solver, exp1));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_ADD_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_ADD == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build ADD I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_add(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_SUB_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SUB == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build SUB I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_sub(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_MULT_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MULT == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build MULT I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_mul(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_DIV_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_DIV == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build DIV I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_udiv(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_MODULO_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MODULO == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build MODULO I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_urem(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_SHL_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SHL == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build SHL I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_sll(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_SHR_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SHR == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build SHR I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_srl(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_CONST_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_CONST == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   const uint32_t width = _ntk->getNetWidth(out); assert (width);
   // Build CONST I/O Relation
   const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
   const V3BitVecX value = ntk->getInputConstValue(V3NetId::makeNetId(out.id)); assert (width == value.size());
   char* bv_value = new char[width + 1]; bv_value[width] = '\0';
   for (uint32_t i = 0, j = width - 1; i < width; ++i, --j) bv_value[j] = value[i];
   // Set BtorExp*
   _ntkData[index].push_back(boolector_const(_Solver, bv_value));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_SLICE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_SLICE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build SLICE I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   const V3BvNtk* const ntk = dynamic_cast<const V3BvNtk*>(_ntk); assert (ntk);
   const uint32_t msb = ntk->getInputSliceBit(out, true);
   const uint32_t lsb = ntk->getInputSliceBit(out, false);
   // Set BtorExp*
   if (msb >= lsb) _ntkData[index].push_back(boolector_slice(_Solver, exp1, msb, lsb));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_MERGE_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_MERGE == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build MERGE I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_concat(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_EQUALITY_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_EQUALITY == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build EQUALITY I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_eq(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

void
V3SvrBoolector::add_GEQ_Formula(const V3NetId& out, const uint32_t& depth) {
   // Check Output Validation
   assert (validNetId(out)); assert (BV_GEQ == _ntk->getGateType(out)); assert (!getVerifyData(out, depth));
   const uint32_t index = getV3NetIndex(out); assert (depth == _ntkData[index].size());
   // Build GEQ I/O Relation
   const V3NetId in1 = _ntk->getInputNetId(out, 0); assert (validNetId(in1));
   const V3NetId in2 = _ntk->getInputNetId(out, 1); assert (validNetId(in2));
   BtorExp* const exp1 = getVerifyData(in1, depth); assert (exp1);
   BtorExp* const exp2 = getVerifyData(in2, depth); assert (exp2);
   // Set BtorExp*
   _ntkData[index].push_back(boolector_ugte(_Solver, exp1, exp2));
   assert (getVerifyData(out, depth));
}

// Network to Solver Function
const bool
V3SvrBoolector::existVerifyData(const V3NetId& id, const uint32_t& depth) {
   return getVerifyData(id, depth);
}

// Boolector Functions
BtorExp* const
V3SvrBoolector::getVerifyData(const V3NetId& id, const uint32_t& depth) const {
   assert (validNetId(id)); if (depth >= _ntkData[id.id].size()) return 0;
   else return (id.cp ? boolector_not(_Solver, _ntkData[id.id][depth]) : _ntkData[id.id][depth]);
}

void
V3SvrBoolector::setRewriteLevel(const int& rwl) {
   boolector_set_rewrite_level(_Solver, ((rwl > 3) ? 3 : (rwl < 0) ? 0 : rwl));
}

#endif

