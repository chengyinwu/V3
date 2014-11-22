/****************************************************************************
  FileName     [ v3DfxSimplify.cpp ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Trace Simplification. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2013-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_SIMPLIFY_C
#define V3_DFX_SIMPLIFY_C

#include "v3NtkUtil.h"
#include "v3VrfBase.h"
#include "v3DfxSimplify.h"
#include "v3NtkSimplify.h"

/* -------------------------------------------------- *\
 * Class V3TraceSimplify Implementations
\* -------------------------------------------------- */
// Constructor and Destructor
V3TraceSimplify::V3TraceSimplify(V3NtkHandler* const handler, const V3CexTrace& cex) : _handler(handler) {
   // Currently we assume cex to be a witness of p, where p is the only PO of handler
   assert (_handler); _target.clear(); computeSimTrace(cex);
   // Statistics
   _totalStat = _piUnrollStat = _piGenStat = _ffUnrollStat = _ffGenStat = 0;
   if (V3VrfBase::profileON()) {
      _totalStat     = new V3Stat("TOTAL");
      _piUnrollStat  = new V3Stat("UNROLL PI"         , _totalStat);
      _piGenStat     = new V3Stat("GEN PI"            , _totalStat);
      _ffUnrollStat  = new V3Stat("UNROLL BOUNDARY FF", _totalStat);
      _ffGenStat     = new V3Stat("GEN BOUNDARY FF"   , _totalStat);
   }
}

V3TraceSimplify::~V3TraceSimplify() {
   _target.clear();
   for (uint32_t i = 0; i < _piValue.size(); ++i) _piValue[i].clear(); _piValue.clear();
   for (uint32_t i = 0; i < _ffValue.size(); ++i) _ffValue[i].clear(); _ffValue.clear();
}

// Trace Retrieval Functions
V3CexTrace* const
V3TraceSimplify::getTrace() const {
   assert (_piValue.size()); assert (_ffValue.size());
   // Compute the Number of bits for PI and FF
   uint32_t piBits = 0, ffBits = 0;
   for (uint32_t i = 0; i < _piValue[0].size(); ++i) piBits += _piValue[0][i].size();
   for (uint32_t i = 0; i < _ffValue[0].size(); ++i) ffBits += _ffValue[0][i].size();
   // Generate Trace
   V3CexTrace* const cex = new V3CexTrace(_piValue.size()); assert (cex);
   if (piBits) {
      V3BitVecX piValue(piBits); uint32_t i, j, k, x;
      for (i = 0; i < _piValue.size(); ++i) {
         for (j = 0, x = 0; j < _piValue[i].size(); ++j)
            for (k = 0; k < _piValue[i][j].size(); ++k, ++x)
               if ('1' == _piValue[i][j][k]) piValue.set1(x);
               else if ('0' == _piValue[i][j][k]) piValue.set0(x);
         assert (piBits == x); cex->pushData(piValue); piValue.clear();
      }
   }
   if (ffBits) {
      V3BitVecX ffValue(ffBits); uint32_t j, k, x;
      for (j = 0, x = 0; j < _ffValue[0].size(); ++j)
         for (k = 0; k < _ffValue[0][j].size(); ++k, ++x)
            if ('1' == _ffValue[0][j][k]) ffValue.set1(x);
            else if ('0' == _ffValue[0][j][k]) ffValue.set0(x);
      assert (ffBits == x); cex->setInit(ffValue);
   }
   return cex;
}

// Trace Simplification Functions
void
V3TraceSimplify::simplifyInputPatterns(const bool& minTrans, const uint32_t& partition) {
   // Sanity Check
   assert (partition); assert (_piValue.size()); assert (_ffValue.size());
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (!_target.size());
   // Initialize Data Members
   uint32_t frameSize = (partition < _piValue.size()) ? partition : _piValue.size();
   V3NtkHandler* unrolledNtk = 0; V3AlgGeneralize* unrolledGen = 0;
   V3NtkHandler* boundaryNtk = 0; V3AlgGeneralize* boundaryGen = 0;
   uint32_t piSize = 0; for (uint32_t i = 0; i < ntk->getInputSize(); ++i) piSize += ntk->getNetWidth(ntk->getInput(i));
   uint32_t ffSize = 0; for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) ffSize += ntk->getNetWidth(ntk->getLatch(i));
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(ffSize + (frameSize * (ntk->getOutputSize() - 1)));
   // Partition the Problem into Several Generalization Runs
   uint32_t frame = _piValue.size() - frameSize, constrStart;
   V3SimTrace piValue, ffValue;
   if (_totalStat) _totalStat->start();
   while (true) {
      // 1. Generalize on PI in Current Partition
      // Construct Pattern Values
      piValue = _ffValue[frame]; ffValue.clear();
      for (uint32_t i = 0; i < frameSize; ++i)
         ffValue.insert(ffValue.end(), _piValue[frame + i].begin(), _piValue[frame + i].end());
      // Create Unrolled Network and Generalization Engine
      if (!unrolledNtk) {
         if (_piUnrollStat) _piUnrollStat->start();
         unrolledNtk = getUnrolledNtk(false, frameSize); assert (unrolledNtk);
         assert (unrolledNtk->getNtk()); if (unrolledGen) delete unrolledGen;
         if (dynamic_cast<V3BvNtk*>(unrolledNtk->getNtk())) unrolledGen = new V3AlgBvGeneralize(unrolledNtk);
         else unrolledGen = new V3AlgAigGeneralize(unrolledNtk); assert (unrolledGen);
         if (_piUnrollStat) _piUnrollStat->end();
      }
      // Compute Constraints
      constrStart = frameSize * piSize; targetNets.clear();
      // 1st Part: PO[1:$] of the Original Network, which are unrolled into PO[1 + ffSize: $]
      for (uint32_t i = 0, j = frameSize * (ntk->getOutputSize() - 1), k = 1 + constrStart + ffSize; i < j; ++i, ++k)
         targetNets.push_back(V3NetId::makeNetId(k));
      // 2nd Part: property or boundary latch values
      if (_piValue.size() == (frameSize + frame)) targetNets.push_back(V3NetId::makeNetId(constrStart + ffSize));
      else {
         const V3SimTrace& boundaryValue = _ffValue[frame + frameSize];
         for (uint32_t i = 0, k = constrStart; i < boundaryValue.size(); ++i)
            for (uint32_t j = 0; j < boundaryValue[i].size(); ++j, ++k)
               if ('X' != boundaryValue[i][j]) targetNets.push_back(V3NetId::makeNetId(k, '0' == boundaryValue[i][j]));
      }
      // Perform Generalization
      if (_piGenStat) _piGenStat->start();
      if (minTrans) minimizeTransitions(unrolledNtk->getNtk(), unrolledGen, piValue, ffValue, targetNets);
      else maximizeDontCares(unrolledNtk->getNtk(), unrolledGen, piValue, ffValue, targetNets);
      if (_piGenStat) _piGenStat->end();
      // Update Generalization Results
      for (uint32_t i = 0, k = 0; i < frameSize; ++i)
         for (uint32_t j = 0; j < _piValue[frame].size(); ++j, ++k) _piValue[frame + i][j] = ffValue[k];

      if (!frame) break;  // NO remaining partitions

      // 2. Generalize on Boundary Latches
      // Construct Pattern Values
      ffValue = _ffValue[frame]; piValue.clear();
      for (uint32_t i = 0; i < frameSize; ++i)
         piValue.insert(piValue.end(), _piValue[frame + i].begin(), _piValue[frame + i].end());
      // Create Unrolled Network and Generalization Engine
      if (!boundaryNtk) {
         if (_ffUnrollStat) _ffUnrollStat->start();
         boundaryNtk = getUnrolledNtk(true, frameSize); assert (boundaryNtk);
         assert (boundaryNtk->getNtk()); if (boundaryGen) delete boundaryGen;
         if (dynamic_cast<V3BvNtk*>(boundaryNtk->getNtk())) boundaryGen = new V3AlgBvGeneralize(boundaryNtk);
         else boundaryGen = new V3AlgAigGeneralize(boundaryNtk); assert (boundaryGen);
         if (_ffUnrollStat) _ffUnrollStat->end();
      }
      // Compute Constraints
      constrStart = ffSize; targetNets.clear();
      // 1st Part: PO[1:$] of the Original Network, which are unrolled into PO[1 + ffSize: $]
      for (uint32_t i = 0, j = frameSize * (ntk->getOutputSize() - 1), k = 1 + constrStart + ffSize; i < j; ++i, ++k)
         targetNets.push_back(V3NetId::makeNetId(k));
      // 2nd Part: property or boundary latch values
      if (_piValue.size() == (frameSize + frame)) targetNets.push_back(V3NetId::makeNetId(constrStart + ffSize));
      else {
         const V3SimTrace& boundaryValue = _ffValue[frame + frameSize];
         for (uint32_t i = 0, k = constrStart; i < boundaryValue.size(); ++i)
            for (uint32_t j = 0; j < boundaryValue[i].size(); ++j, ++k)
               if ('X' != boundaryValue[i][j]) targetNets.push_back(V3NetId::makeNetId(k, '0' == boundaryValue[i][j]));
      }
      // Perform Generalization
      if (_ffGenStat) _ffGenStat->start();
      maximizeDontCares(boundaryNtk->getNtk(), boundaryGen, piValue, ffValue, targetNets);
      if (_ffGenStat) _ffGenStat->end();
      // Update Generalization Results
      _ffValue[frame] = ffValue;

      // 3. Decrement frame for the Next Partition
      if (frame >= frameSize) frame -= frameSize;
      else { frameSize = frame; frame = 0; delete unrolledNtk; unrolledNtk = 0; }
   }
   if (_totalStat) {
      _totalStat->end();
      Msg(MSG_IFO) << *_piUnrollStat << endl;
      Msg(MSG_IFO) << *_piGenStat << endl;
      Msg(MSG_IFO) << *_ffUnrollStat << endl;
      Msg(MSG_IFO) << *_ffGenStat << endl;
      Msg(MSG_IFO) << *_totalStat << endl;
   }
   // Delete Data Members for Generalization
   if (boundaryGen) delete boundaryGen; if (boundaryNtk) delete boundaryNtk;
   if (unrolledGen) delete unrolledGen; if (unrolledNtk) delete unrolledNtk;
   // Resimulate for Accurate Latch Values
   V3CexTrace* const cex = getTrace(); assert (cex);
   computeSimTrace(*cex); delete cex; _target.clear();
}

// Trace Information
void
V3TraceSimplify::printTraceInfo() const {
   // Sanity Check
   assert (_piValue.size()); assert (_ffValue.size());
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   // Print Pattern Size
   uint32_t piSize = 0; for (uint32_t i = 0; i < ntk->getInputSize(); ++i) piSize += ntk->getNetWidth(ntk->getInput(i));
   uint32_t ffSize = 0; for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) ffSize += ntk->getNetWidth(ntk->getLatch(i));
   Msg(MSG_IFO) << "PI = " << piSize << ", FF = " << ffSize << ", Net = " << ntk->getNetSize() << ", ";
   // Print Trace Size
   Msg(MSG_IFO) << "Length = " << _piValue.size() << ", ";
   // Print Care Size
   piSize = 0;
   for (uint32_t i = 0; i < _piValue.size(); ++i)
      for (uint32_t j = 0; j < _piValue[i].size(); ++j)
         for (uint32_t k = 0; k < _piValue[i][j].size(); ++k) if ('X' != _piValue[i][j][k]) ++piSize;
   Msg(MSG_IFO) << "Care = " << piSize << ", ";
   // Print Transition Size
   V3SimTrace piValue = _piValue[0]; piSize = 0;
   for (uint32_t i = 1; i < _piValue.size(); ++i)
      for (uint32_t j = 0; j < _piValue[i].size(); ++j)
         for (uint32_t k = 0; k < _piValue[i][j].size(); ++k)
            if ('0' == piValue[j][k]) {
               if ('1' == _piValue[i][j][k]) { ++piSize; piValue[j].set1(k); }
               else if ('0' == _piValue[i][j][k]) piValue[j].set0(k);
            }
            else if ('1' == piValue[j][k]) {
               if ('0' == _piValue[i][j][k]) { ++piSize; piValue[j].set0(k); }
               else if ('1' == _piValue[i][j][k]) piValue[j].set1(k);
            }
            else {
               if ('0' == _piValue[i][j][k]) piValue[j].set0(k);
               else if ('1' == _piValue[i][j][k]) piValue[j].set1(k);
            }
   Msg(MSG_IFO) << "Trans = " << piSize << endl;
}

// Private Helper Functions
void
V3TraceSimplify::computeSimTrace(const V3CexTrace& cex) {
   // Sanity Check
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (ntk->getOutputSize());
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) assert (1 == ntk->getNetWidth(ntk->getOutput(i)));
   // Initialize
   _piValue.clear(); _piValue.reserve(cex.getTraceSize());
   _ffValue.clear(); _ffValue.reserve(cex.getTraceSize());
   // Create Simulator
   V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(ntk)) simulator = new V3AlgBvSimulate(_handler);
   else simulator = new V3AlgAigSimulate(_handler); assert (simulator);
   // Set Patterns from cex and then Simulate
   V3BitVecX pattern, value;
   for (uint32_t i = 0, k, index; i < cex.getTraceSize(); ++i) {
      _piValue.push_back(V3SimTrace(ntk->getInputSize() + ntk->getInoutSize()));
      _ffValue.push_back(V3SimTrace(ntk->getLatchSize()));
      // Update FF Next State Values
      simulator->updateNextStateValue();
      // Set Initial State Values
      if (!i && cex.getInit()) {
         V3BitVecX* const initValue = cex.getInit(); k = 0;
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
            simulator->setSource(ntk->getLatch(j), initValue->bv_slice(k + ntk->getNetWidth(ntk->getLatch(j)) - 1, k));
            k += ntk->getNetWidth(ntk->getLatch(j)); assert (k <= initValue->size());
         }
      }
      // Record State Values into _ffValue
      for (uint32_t j = 0; j < ntk->getLatchSize(); ++j)
         _ffValue.back()[j] = (simulator->getSimValue(ntk->getLatch(j))).bv_slice(ntk->getNetWidth(ntk->getLatch(j)) - 1, 0);
      // Set PI Values and Record them into _piValue
      if (cex.getTraceDataSize()) pattern = cex.getData(i); k = 0; index = 0;
      for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
         value = pattern.bv_slice(k + ntk->getNetWidth(ntk->getInput(j)) - 1, k);
         //value.restrict0();  // Restrict X to be 0 for Full Pattern
         simulator->setSource(ntk->getInput(j), value); _piValue.back()[index++] = value;
         k += ntk->getNetWidth(ntk->getInput(j)); assert (k <= pattern.size());
      }
      for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
         value = pattern.bv_slice(k + ntk->getNetWidth(ntk->getInout(j)) - 1, k);
         //value.restrict0();  // Restrict X to be 0 for Full Pattern
         simulator->setSource(ntk->getInout(j), value); _piValue.back()[index++] = value;
         k += ntk->getNetWidth(ntk->getInout(j)); assert (k <= pattern.size());
      }
      // Simulate Ntk for a Cycle
      simulator->simulate();
      // Check the Validation of cex
      for (uint32_t j = 1; j < ntk->getOutputSize(); ++j) assert ('1' == simulator->getSimValue(ntk->getOutput(j))[0]);
   }
   // Check the Validation of cex
   assert ('1' == simulator->getSimValue(ntk->getOutput(0))[0]);
   // Delete Simulator
   delete simulator; simulator = 0;
}

V3NtkHandler* const
V3TraceSimplify::getUnrolledNtk(const bool& genOnFF, const uint32_t& f){
   // Sanity Check
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk); assert (f);
   // Unroll Network for f frames
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(ntk);
   V3Ntk* unrolledNtk = createV3Ntk(isBvNtk); assert (unrolledNtk);
   V3NetId unrolledId; V3GateType type; uint32_t inSize, shift = 0;
   V3InputVec inputs; inputs.clear(); inputs.reserve(3);
   for (uint32_t i = 0; i < f; ++i) {
      unrolledId = V3NetId::makeNetId(unrolledNtk->getNetSize() - 1);
      shift = i * (ntk->getNetSize() - 1);
      // Create Nets
      for (V3NetId id = V3NetId::makeNetId(1); id.id < ntk->getNetSize(); ++id.id)
         createNet(unrolledNtk, ntk->getNetWidth(id));
      // Create Gates
      for (V3NetId id = V3NetId::makeNetId(1); id.id < ntk->getNetSize(); ++id.id) {
         type = ntk->getGateType(id); assert (V3_XD > type); ++unrolledId.id;
         assert (unrolledId.id && unrolledId.id < unrolledNtk->getNetSize());
         assert (ntk->getNetWidth(id) == unrolledNtk->getNetWidth(unrolledId));
         if (V3_FF >= ntk->getGateType(id)) continue; inputs.clear();
         if (isBvNtk) {
            if (BV_SLICE == type) {
               inputs.push_back(ntk->getInputNetId(id, 0)); if (inputs.back().id.id) inputs.back().id.id += shift;
               inputs.push_back(ntk->getInputNetId(id, 1));
            }
            else if (BV_CONST == type) inputs.push_back(ntk->getInputNetId(id, 0));
            else {
               inSize = (isV3PairType(type)) ? 2 : (BV_MUX == type) ? 3 : 1;
               for (uint32_t j = 0; j < inSize; ++j) {
                  inputs.push_back(ntk->getInputNetId(id, j)); if (inputs.back().id.id) inputs.back().id.id += shift;
               }
            }
         }
         else if (AIG_NODE == type) {
            inputs.push_back(ntk->getInputNetId(id, 0)); if (inputs.back().id.id) inputs.back().id.id += shift;
            inputs.push_back(ntk->getInputNetId(id, 1)); if (inputs.back().id.id) inputs.back().id.id += shift;
         }
         unrolledNtk->setInput(unrolledId, inputs); unrolledNtk->createGate(type, unrolledId);
      }
      // Create Buffers at Latch Boundaries
      if (i) {
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
            unrolledId = ntk->getLatch(j); unrolledId.id += shift; inputs.clear();
            inputs.push_back(ntk->getInputNetId(ntk->getLatch(j), 0));
            if (inputs.back().id.id) inputs.back().id.id += (shift - (ntk->getNetSize() - 1));
            assert (inputs.back().id.id < unrolledNtk->getNetSize());
            createV3BufGate(unrolledNtk, unrolledId, inputs.back().id);
         }
      }
      if (genOnFF) {
         // Create Primary Inputs
         for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
            unrolledId = ntk->getInput(j); unrolledId.id += shift;
            assert (V3_PI == unrolledNtk->getGateType(unrolledId));
            unrolledNtk->createInput(unrolledId);
         }
         for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
            unrolledId = ntk->getInout(j); unrolledId.id += shift;
            assert (V3_PI == unrolledNtk->getGateType(unrolledId));
            unrolledNtk->createInput(unrolledId);
         }
      }
      else {
         // Create Latches
         inputs.clear(); inputs.push_back(0); inputs.push_back(0);
         for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
            unrolledId = ntk->getInput(j); unrolledId.id += shift;
            assert (V3_PI == unrolledNtk->getGateType(unrolledId));
            unrolledNtk->setInput(unrolledId, inputs); unrolledNtk->createLatch(unrolledId);
         }
         for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
            unrolledId = ntk->getInout(j); unrolledId.id += shift;
            assert (V3_PI == unrolledNtk->getGateType(unrolledId));
            unrolledNtk->setInput(unrolledId, inputs); unrolledNtk->createLatch(unrolledId);
         }
      }
   }
   // Consider Latches in the First Frame
   if (genOnFF) {
      // Create Latches
      inputs.clear(); inputs.push_back(0); inputs.push_back(0);
      for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
         unrolledId = ntk->getLatch(j);
         assert (V3_PI == unrolledNtk->getGateType(unrolledId));
         unrolledNtk->setInput(unrolledId, inputs); unrolledNtk->createLatch(unrolledId);
      }
      assert (unrolledNtk->getLatchSize() == ntk->getLatchSize());
      assert (unrolledNtk->getInputSize() == f * (ntk->getInputSize() + ntk->getInoutSize()));
   }
   else {
      // Create Primary Inputs
      for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
         unrolledId = ntk->getLatch(j);
         assert (V3_PI == unrolledNtk->getGateType(unrolledId));
         unrolledNtk->createInput(unrolledId);
      }
      assert (unrolledNtk->getInputSize() == ntk->getLatchSize());
      assert (unrolledNtk->getLatchSize() == f * (ntk->getInputSize() + ntk->getInoutSize()));
   }
   // Set Primary Outputs
   // 1. Inputs of Latches of the Original Network  (as boundary constraints)
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) {
      unrolledId = ntk->getInputNetId(ntk->getLatch(i), 0); if (unrolledId.id) unrolledId.id += shift;
      assert (unrolledId.id < unrolledNtk->getNetSize()); unrolledNtk->createOutput(unrolledId);
   }
   // 2. Property Output (i.e. the first PO of the Original Network)
   unrolledId = ntk->getOutput(0); if (unrolledId.id) unrolledId.id += shift;
   assert (unrolledId.id < unrolledNtk->getNetSize()); unrolledNtk->createOutput(unrolledId);
   // 3. f Copies of Constraints (i.e. remaining POs of the Original Network)
   for (uint32_t i = 1; i < ntk->getOutputSize(); ++i) {
      unrolledId = ntk->getOutput(i);
      if (unrolledId.id) {
         for (uint32_t j = 0; j < f; ++j) {
            assert (unrolledId.id < unrolledNtk->getNetSize());
            unrolledNtk->createOutput(unrolledId);
            unrolledId.id += (ntk->getNetSize() - 1);
         }
      }
      else {  // Literally these constraints should be redundant
         for (uint32_t j = 0; j < f; ++j) {
            assert (unrolledId.id < unrolledNtk->getNetSize());
            unrolledNtk->createOutput(unrolledId);
         }
      }
   }
   // Set Handler for the Unrolled Network
   V3NtkHandler* unrolledHandler = new V3NtkHandler(0, unrolledNtk); assert (unrolledHandler);
   // Simplify the Network
   unrolledHandler = new V3NtkSimplify(unrolledHandler); assert (unrolledHandler);
   unrolledNtk = unrolledHandler->getNtk(); assert (unrolledNtk);
   // Set Cut Signals
   V3NetVec targetNets; targetNets.clear();
   targetNets.reserve(unrolledNtk->getLatchSize() + unrolledNtk->getOutputSize());
   for (uint32_t i = 0; i < unrolledNtk->getLatchSize(); ++i) targetNets.push_back(unrolledNtk->getLatch(i));
   for (uint32_t i = 0; i < unrolledNtk->getOutputSize(); ++i) targetNets.push_back(unrolledNtk->getOutput(i));
   unrolledNtk->clearCutSignals(); unrolledNtk->setCutSignals(targetNets);
   // Return the Unrolled Network
   return unrolledHandler;
}

void
V3TraceSimplify::maximizeDontCares(V3Ntk* const unrolledNtk, V3AlgGeneralize* const unrolledGen, const V3SimTrace& piValue, V3SimTrace& ffValue, const V3NetVec& targetNets) {
   // Sanity Check
   assert (unrolledNtk); assert (unrolledGen); assert (targetNets.size());
   assert (piValue.size() == unrolledNtk->getInputSize());
   assert (ffValue.size() == unrolledNtk->getLatchSize());
   // Set Patterns for Generalization
   V3AlgSimulate* unrolledSim = 0;
   if (dynamic_cast<V3BvNtk*>(unrolledNtk)) unrolledSim = dynamic_cast<V3AlgBvSimulate*>(unrolledGen);
   else unrolledSim = dynamic_cast<V3AlgAigSimulate*>(unrolledGen); assert (unrolledSim);
   for (uint32_t i = 0; i < piValue.size(); ++i) unrolledSim->setSource(unrolledNtk->getInput(i), piValue[i]);
   for (uint32_t i = 0; i < ffValue.size(); ++i) unrolledSim->setSource(unrolledNtk->getLatch(i), ffValue[i]);
   // Start Generalization on Latches
   unrolledSim->simulate(); unrolledGen->setTargetNets(targetNets);
   unrolledGen->performSetXForNotCOIVars();  // covered by function performFixForControlVars
   unrolledGen->performFixForControlVars(true);  // false for disabling parallel sim
   // Set Priority over Latches
   V3UI32Vec prioNets; prioNets.clear(); uint32_t ffSize = 0;
   for (uint32_t i = 0; i < unrolledNtk->getLatchSize(); ++i)
      ffSize += unrolledNtk->getNetWidth(unrolledNtk->getLatch(i));
   prioNets.reserve(ffSize); for (uint32_t i = 0; i < ffSize; ++i) prioNets.push_back(i);
   unrolledGen->performXPropForExtensibleVars(prioNets);
   // Update ffValue
   const V3NetVec genResult = unrolledGen->getGeneralizationResult();
   uint32_t ffIndex = 0; ffSize = ffValue.front().size();
   for (uint32_t i = 0; i < ffValue.size(); ++i) ffValue[i].clear();
   for (uint32_t i = 0; i < genResult.size(); ++i) {
      while (genResult[i].id >= ffSize) {
         ++ffIndex; assert (ffIndex < ffValue.size());
         ffSize += ffValue[ffIndex].size();
      }
      if (genResult[i].cp) ffValue[ffIndex].set0(genResult[i].id - (ffSize - ffValue[ffIndex].size()));
      else ffValue[ffIndex].set1(genResult[i].id - (ffSize - ffValue[ffIndex].size()));
   }
}

void
V3TraceSimplify::minimizeTransitions(V3Ntk* const unrolledNtk, V3AlgGeneralize* const unrolledGen, const V3SimTrace& piValue, V3SimTrace& ffValue, const V3NetVec& targetNets) {
   // Sanity Check
   assert (unrolledNtk); assert (unrolledGen); assert (targetNets.size());
   assert (piValue.size() == unrolledNtk->getInputSize());
   assert (ffValue.size() == unrolledNtk->getLatchSize());
   // Set Patterns for Generalization
   V3AlgSimulate* unrolledSim = 0;
   if (dynamic_cast<V3BvNtk*>(unrolledNtk)) unrolledSim = dynamic_cast<V3AlgBvSimulate*>(unrolledGen);
   else unrolledSim = dynamic_cast<V3AlgAigSimulate*>(unrolledGen); assert (unrolledSim);
   for (uint32_t i = 0; i < piValue.size(); ++i) unrolledSim->setSource(unrolledNtk->getInput(i), piValue[i]);
   for (uint32_t i = 0; i < ffValue.size(); ++i) unrolledSim->setSource(unrolledNtk->getLatch(i), ffValue[i]);
   // Compute the Number of Frames Unrolled
   V3Ntk* const ntk = _handler->getNtk(); assert (ntk);
   const uint32_t frameSize = (ffValue.size()) ? (ffValue.size() / ntk->getInputSize()) : 1;
   assert (ffValue.size() == (ntk->getInputSize() * frameSize));
   // Start Generalization on Latches
   unrolledSim->simulate(); unrolledGen->setTargetNets(targetNets);
   unrolledGen->performSetXForNotCOIVars();  // covered by function performFixForControlVars
   unrolledGen->performFixForControlVars(true);  // false for disabling parallel sim
   V3NetVec undecided = unrolledGen->getUndecided();
   uint32_t undecidedSize = undecided.size();
   while (true) {
      unrolledGen->performXPropForMinimizeTransitions(frameSize, _target);
      undecided = unrolledGen->getUndecided();
      if (undecided.size() == undecidedSize) break;
      else undecidedSize = undecided.size();
   }
   // Update ffValue
   const V3NetVec genResult = unrolledGen->getGeneralizationResult();
   uint32_t ffIndex = 0, ffSize = ffValue.front().size();
   for (uint32_t i = 0; i < ffValue.size(); ++i) ffValue[i].clear();
   for (uint32_t i = 0; i < genResult.size(); ++i) {
      while (genResult[i].id >= ffSize) {
         ++ffIndex; assert (ffIndex < ffValue.size());
         ffSize += ffValue[ffIndex].size();
      }
      if (genResult[i].cp) ffValue[ffIndex].set0(genResult[i].id - (ffSize - ffValue[ffIndex].size()));
      else ffValue[ffIndex].set1(genResult[i].id - (ffSize - ffValue[ffIndex].size()));
   }
   // Update Target Value
   ffSize = 0; for (uint32_t i = 0; i < ntk->getInputSize(); ++i) ffSize += ntk->getNetWidth(ntk->getInput(i));
   V3BoolVec target0(ffSize, false), target1(ffSize, false);
   for (uint32_t i = 0; i < _target.size(); ++i) {
      assert (ffSize > _target[i].id); assert (!target0[_target[i].id] && !target1[_target[i].id]);
      if (_target[i].cp) target0[_target[i].id] = true; else target1[_target[i].id] = true;
   }
   ffIndex = genResult.size();
   while (ffIndex--) {
      assert ((frameSize * ffSize) > genResult[ffIndex].id);
      if (genResult[ffIndex].cp) {
         target0[genResult[ffIndex].id % ffSize] = true;
         target1[genResult[ffIndex].id % ffSize] = false;
      }
      else {
         target0[genResult[ffIndex].id % ffSize] = false;
         target1[genResult[ffIndex].id % ffSize] = true;
      }
   }
   _target.clear();
   for (uint32_t i = 0; i < ffSize; ++i) {
      assert (!target0[i] || !target1[i]);
      if (target0[i]) _target.push_back(V3NetId::makeNetId(i, 1));
      else if (target1[i]) _target.push_back(V3NetId::makeNetId(i, 0));
   }
}

#endif

