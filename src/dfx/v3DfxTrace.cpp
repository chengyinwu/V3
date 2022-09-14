/****************************************************************************
  FileName     [ v3DfxTrace.cpp ]
  PackageName  [ v3/src/dfx ]
  Synopsis     [ Counterexample Trace Manipulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_DFX_TRACE_C
#define V3_DFX_TRACE_C

#include "v3Graph.h"
#include "v3NtkUtil.h"
#include "v3NumUtil.h"
#include "v3DfxTrace.h"
#include "v3Property.h"
#include "v3AlgSimulate.h"

#define V3_DFX_TRACE_RESTRICT_X

// Counterexample Trace Utility Functions
void computeTraceData(V3NtkHandler* const handler, const V3CexTrace& cex, V3SimTraceVec& inVec, V3SimTraceVec& flopVec) {
   // Check Counterexample Directly on Ntk
   assert (handler); assert (handler->getNtk()); assert (cex.getTraceSize());
   V3Ntk* const ntk = handler->getNtk(); assert (1 == ntk->getOutputSize());
   // Initialize inVec and flopVec
   inVec.clear(); inVec.reserve(cex.getTraceSize());
   flopVec.clear(); flopVec.reserve(cex.getTraceSize());
   // Create Simulator
   V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(ntk)) simulator = new V3AlgBvSimulate(handler);
   else simulator = new V3AlgAigSimulate(handler); assert (simulator);
   // For each time-frame, set pattern from counterexample
   V3SimTrace inValue(ntk->getInputSize() + ntk->getInoutSize());
   V3BitVecX pattern, value;
   for (uint32_t i = 0, k, index; i < cex.getTraceSize(); ++i) {
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
      // Record State Value
      if (ntk->getLatchSize()) {
         V3SimTrace flopValue(ntk->getLatchSize());
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j)
            flopValue[j] = (simulator->getSimValue(ntk->getLatch(j))).bv_slice(ntk->getNetWidth(ntk->getLatch(j)) - 1, 0);
         flopVec.push_back(flopValue);
      }
      // Set PI Values and Record Pattern Values
      if (cex.getTraceDataSize()) pattern = cex.getData(i); k = 0; index = 0;
      for (uint32_t j = 0; j < ntk->getInputSize(); ++j) {
         value = pattern.bv_slice(k + ntk->getNetWidth(ntk->getInput(j)) - 1, k);
#ifdef V3_DFX_TRACE_RESTRICT_X
         value.restrict0();  // Restrict X to be 0 for Full Pattern
#endif
         simulator->setSource(ntk->getInput(j), value); inValue[index++] = value;
         k += ntk->getNetWidth(ntk->getInput(j)); assert (k <= pattern.size());
      }
      for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) {
         value = pattern.bv_slice(k + ntk->getNetWidth(ntk->getInout(j)) - 1, k);
#ifdef V3_DFX_TRACE_RESTRICT_X
         value.restrict0();  // Restrict X to be 0 for Full Pattern
#endif
         simulator->setSource(ntk->getInout(j), value); inValue[index++] = value;
         k += ntk->getNetWidth(ntk->getInout(j)); assert (k <= pattern.size());
      }
      inVec.push_back(inValue);
      // Simulate Ntk for a Cycle
      simulator->simulate();
   }
   // Check Property Assertion
   assert ('1' == simulator->getSimValue(ntk->getOutput(0))[0]); delete simulator;
}

// Trace Optimization Subsumption Functions
const V3Vec<V3DfxCube*>::Vec getSubsumingCubes(V3DfxCube* const cube, const V3DfxCubeList& stateList) {
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = ~(cube->getSignature());
   V3DfxCubeList::const_reverse_iterator it = stateList.rbegin();
   V3Vec<V3DfxCube*>::Vec subsumeCubes; subsumeCubes.clear();
   for (; it != stateList.rend(); ++it) {
      const V3NetVec& state = (*it)->getState();
      // Early Return
      if (!cubeState.size() || (state.size() && (cubeState[0].id > state[0].id))) return subsumeCubes;
      if (cubeState.size() < state.size()) continue;
      if (cubeSignature & (*it)->getSignature()) continue;
      // General Check
      uint32_t j = 0, k = 0;
      while (j < cubeState.size() && k < state.size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (state[k].id > state[k - 1].id));
         if (cubeState[j].id > state[k].id) { assert (j >= k); if (j == k) return subsumeCubes; break; }
         else if (cubeState[j].id < state[k].id) ++j;
         else { if (cubeState[j].cp ^ state[k].cp) break; ++j; ++k; }
      }
      if (k == state.size()) subsumeCubes.push_back(*it);
   }
   return subsumeCubes;
}

const V3Vec<V3DfxCube*>::Vec getSubsumedCubes(V3DfxCube* const cube, const V3DfxCubeList& stateList) {
   assert (cube); const V3NetVec& cubeState = cube->getState();
   const uint64_t cubeSignature = cube->getSignature();
   V3DfxCubeList::const_iterator it = stateList.begin();
   V3Vec<V3DfxCube*>::Vec subsumeCubes; subsumeCubes.clear();
   while (it != stateList.end()) {
      const V3NetVec& state = (*it)->getState();
      // Early Return
      if (!state.size() || (cubeState.size() && (cubeState[0].id < state[0].id))) return subsumeCubes;
      if (state.size() < cubeState.size()) { ++it; continue; }
      if (cubeSignature & ~((*it)->getSignature())) { ++it; continue; }
      // General Check
      uint32_t j = 0, k = 0;
      while (j < cubeState.size() && k < state.size()) {
         assert (!j || (cubeState[j].id > cubeState[j - 1].id));
         assert (!k || (state[k].id > state[k - 1].id));
         if (cubeState[j].id < state[k].id) { assert (j <= k); if (j == k) return subsumeCubes; break; }
         else if (cubeState[j].id > state[k].id) ++k;
         else { if (cubeState[j].cp ^ state[k].cp) break; ++j; ++k; }
      }
      if (j == cubeState.size()) subsumeCubes.push_back(*it); ++it;
   }
   return subsumeCubes;
}

// Counterexample Trace Optimization Functions
void performTraceReduction(V3NtkHandler* const handler, V3Property* const p) {
   assert (handler); assert (handler->getNtk()); assert (p); assert (p->getCexTrace());
   V3Ntk* const ntk = handler->getNtk(); assert (1 == ntk->getOutputSize());
   // Statistics
   V3Stat *simGenStat = 0, *solveStat = 0, *totalStat = 0;
   if (V3VrfBase::profileON()) {
      totalStat     = new V3Stat("TOTAL"); totalStat->start();
      simGenStat    = new V3Stat("GENERALIZE",  totalStat);
      solveStat     = new V3Stat("REDUCTION",   totalStat);
   }
   // Compute Full Trace Model
   double runtime = clock();
   V3SimTraceVec inputData, stateData; inputData.clear(); stateData.clear();
   computeTraceData(handler, *(p->getCexTrace()), inputData, stateData);
   assert (inputData.size()); assert (inputData.size() == stateData.size());
   if (V3VrfBase::reportON()) {
      Msg(MSG_IFO) << "Re-simulate for Full Model"; runtime = (clock()- runtime) / CLOCKS_PER_SEC;
      if (V3VrfBase::usageON()) Msg(MSG_IFO) << "  (time = " << setprecision(5) << runtime << "  sec)" << endl;
   }
   runtime = clock();
   // Create Generalization-Enabled Simulator
   V3AlgGeneralize* generalize = 0; V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(ntk)) {
      generalize = new V3AlgBvGeneralize(handler); assert (generalize);
      simulator = dynamic_cast<V3AlgBvSimulate*>(generalize); assert (simulator);
   }
   else {
      generalize = new V3AlgAigGeneralize(handler); assert (generalize);
      simulator = dynamic_cast<V3AlgAigSimulate*>(generalize); assert (simulator);
   }
   // Set Cut Signals On Input Ntk
   V3NetVec cutSignals; cutSignals.clear(); cutSignals.reserve(1 + ntk->getLatchSize()); ntk->clearCutSignals();
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) cutSignals.push_back(ntk->getLatch(i));
   cutSignals.push_back(ntk->getOutput(0)); ntk->setCutSignals(cutSignals);
   // Set Priority for Generalization
   V3UI32Vec prioNets; prioNets.clear(); prioNets.reserve(ntk->getCutSize());
   for (uint32_t i = 0; i < ntk->getCutSize(); ++i) prioNets.push_back(i);
   // Compute Target Size
   uint32_t targetSize = 0;
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i) targetSize += ntk->getNetWidth(ntk->getLatch(i));
   // Start Trace Reduction
   V3DfxCubeList stateList; stateList.clear();
   V3DfxCubeList::const_iterator it;
   V3NetVec targets; targets.clear();
   V3Graph<V3DfxCube*> reachGraph;
   if (simGenStat) simGenStat->start();
   // 1. Compute Initial Cube for the Last Frame
   // Simulate For One Frame
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
      simulator->setSource(ntk->getInput(i), inputData.back()[i]);
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
      simulator->setSource(ntk->getInout(i), inputData.back()[ntk->getInputSize() + i]);
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i)
      simulator->setSource(ntk->getLatch(i), stateData.back()[i]);
   simulator->simulate(); targets.push_back(V3NetId::makeNetId(targetSize, 0));
   // Perform SAT Generalization
   generalize->setTargetNets(targets);
   generalize->performFixForControlVars();
   generalize->performXPropForExtensibleVars(prioNets);
   targets = generalize->getGeneralizationResult();
   // Insert Cube into the List
   V3DfxCube* const badCube = new V3DfxCube(inputData.size() - 1); assert (badCube);
   badCube->setState(targets); stateList.insert(badCube);
   // 2. Compute Cubes for Remaining Frames
   V3Vec<V3DfxCube*>::Vec targetDfxCubes, sourceDfxCubes;
   const string flushSpace = string(100, ' ');
   V3DfxCubeList newList; newList.clear();
   uint32_t frame = inputData.size() - 1;
   while (frame--) {
      // Simulate For One Frame
      for (uint32_t i = 0; i < ntk->getInputSize(); ++i) 
         simulator->setSource(ntk->getInput(i), inputData[frame][i]);
      for (uint32_t i = 0; i < ntk->getInoutSize(); ++i) 
         simulator->setSource(ntk->getInout(i), inputData[frame][ntk->getInputSize() + i]);
      for (uint32_t i = 0; i < ntk->getLatchSize(); ++i)
         simulator->setSource(ntk->getLatch(i), stateData[frame][i]);
      simulator->simulate();
      // Compute Target Cube
      targets.clear(); targets.reserve(targetSize);
      const V3SimTrace& stateTrace = stateData[1 + frame];
      for (uint32_t i = 0, k = 0; i < stateTrace.size(); ++i)
         for (uint32_t j = 0; j < stateTrace[i].size(); ++j, ++k)
            if ('0' == stateTrace[i][j]) targets.push_back(V3NetId::makeNetId(k, 1));
            else if ('1' == stateTrace[i][j]) targets.push_back(V3NetId::makeNetId(k));
      // Get Subsuming Cubes for the Target Cube
      V3DfxCube* cube = new V3DfxCube(V3NtkUD); assert (cube); cube->setState(targets);
      targetDfxCubes = getSubsumingCubes(cube, stateList);
      assert (targetDfxCubes.size()); delete cube; newList.clear();
      if (V3VrfBase::intactON()) {
         if (!V3VrfBase::endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << "Frame = " << frame << " : " << targetDfxCubes.size() << " / " << stateList.size();
         if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
      // For Each Cube in targetDfxCubes, Generalize stateData[frame] and Record Relation
      for (uint32_t i = 0; i < targetDfxCubes.size(); ++i) {
         // Simulate For One Frame
         for (uint32_t j = 0; j < ntk->getInputSize(); ++j) 
            simulator->setSource(ntk->getInput(j), inputData[frame][j]);
         for (uint32_t j = 0; j < ntk->getInoutSize(); ++j) 
            simulator->setSource(ntk->getInout(j), inputData[frame][ntk->getInputSize() + j]);
         for (uint32_t j = 0; j < ntk->getLatchSize(); ++j)
            simulator->setSource(ntk->getLatch(j), stateData[frame][j]);
         simulator->simulate();
         // Perform SAT Generalization
         generalize->setTargetNets(V3NetVec(), targets);
         generalize->performFixForControlVars();
         generalize->performXPropForExtensibleVars(prioNets);
         targets = generalize->getGeneralizationResult();
         // Insert Cube into the List
         cube = new V3DfxCube(frame); assert (cube); cube->setState(targets); it = newList.find(cube);
         if (newList.end() != it) { delete cube; cube = *it; assert (cube); }
         else { newList.insert(cube); reachGraph.insertEdge(cube, targetDfxCubes[i]); }
         // Get Subsumed Cubes for the Generalized Cube
         sourceDfxCubes = getSubsumedCubes(cube, stateList);
         for (uint32_t j = 0; j < sourceDfxCubes.size(); ++j) 
            reachGraph.insertEdge(sourceDfxCubes[j], targetDfxCubes[i]);
         // Get Subsuming Cubes for the Generalized Cube
         if (newList.end() != it) continue;  // Cube Already Exist in this Frame
         sourceDfxCubes = getSubsumingCubes(cube, stateList);
         for (uint32_t j = 0; j < sourceDfxCubes.size(); ++j) {
            const uint32_t index = reachGraph.getNodeIndex(sourceDfxCubes[j]);
            assert (index < reachGraph.getNodeSize());
            for (uint32_t k = 0; k < reachGraph.getOutNodeSize(index); ++k) 
               reachGraph.insertEdge(cube, reachGraph.getNode(reachGraph.getOutNode(index, k)));
         }
         stateList.insert(cube);
      }
   }
   // Set Initial State
   targets.clear(); targets.reserve(targetSize);
   for (uint32_t i = 0, k = 0; i < stateData[0].size(); ++i)
      for (uint32_t j = 0; j < stateData[0][i].size(); ++j, ++k)
         if ('0' == stateData[0][i][j]) targets.push_back(V3NetId::makeNetId(k, 1));
         else if ('1' == stateData[0][i][j]) targets.push_back(V3NetId::makeNetId(k));
   // Get Subsuming Cubes for the Target Cube
   V3DfxCube* const initCube = new V3DfxCube(0); assert (initCube); initCube->setState(targets);
   targetDfxCubes = getSubsumingCubes(initCube, stateList); assert (targetDfxCubes.size()); delete initCube;
   if (simGenStat) simGenStat->end();
   // Run Single Source Shortest Path on reachGraph
   for (uint32_t i = 0; i < targetDfxCubes.size(); ++i) reachGraph.insertEdge(0, targetDfxCubes[i]);
   // Compute Reduced Counterexample Trace
   if (solveStat) solveStat->start();
   V3Stack<V3DfxCube*>::Stack trace; reachGraph.shortestPath(0, badCube, trace);
   if (solveStat) solveStat->end();
   assert (1 < trace.size()); assert (!trace.top()); trace.pop();
   V3CexTrace* const cex = new V3CexTrace(trace.size()); assert (cex);
   while (!trace.empty()) { cex->pushData(p->getCexTrace()->getData(trace.top()->getFrameId())); trace.pop(); }
   if (p->getCexTrace()->getInit()) cex->setInit(*(p->getCexTrace()->getInit()));
   assert (cex->getTraceSize() <= p->getCexTrace()->getTraceSize());
   // Report Counterexample Reduction Result
   if (V3VrfBase::reportON()) {
      runtime = (clock()- runtime) / CLOCKS_PER_SEC;
      if (V3VrfBase::intactON()) {
         if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      Msg(MSG_IFO) << "Counterexample is Reduced from " << p->getCexTrace()->getTraceSize() 
         << " to " << cex->getTraceSize();
      if (V3VrfBase::usageON()) Msg(MSG_IFO) << "  (time = " << setprecision(5) << runtime << "  sec)" << endl;
      if (V3VrfBase::profileON()) {
         totalStat->end();
         Msg(MSG_IFO) << *simGenStat << endl;
         Msg(MSG_IFO) << *solveStat << endl;
         Msg(MSG_IFO) << *totalStat << endl;
      }
   }
   // Update the Counterexample Trace
   V3VrfResult cexResult; assert (cex);
   cexResult.setCexTrace(cex); p->setResult(cexResult); delete generalize;
}

void performTraceGeneralization(V3NtkHandler* const handler, V3Property* const p) {
   assert (handler); assert (handler->getNtk()); assert (p); assert (p->getCexTrace());
   V3Ntk* const ntk = handler->getNtk(); assert (1 == ntk->getOutputSize());
   // Compute Target Size in Advance
   uint32_t targetSize = 0, maxSize;
   if (!V3SafeMult(p->getCexTrace()->getTraceSize(), ntk->getNetSize(), maxSize)) {
      Msg(MSG_ERR) << "Direct Generalization Failed due to Network Size !!" << endl; return; }
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) targetSize += ntk->getNetWidth(ntk->getInput(i));
   if (!V3SafeMult(targetSize, p->getCexTrace()->getTraceSize(), maxSize)) {
      Msg(MSG_ERR) << "Direct Generalization Failed due to Primary Input / Inout Bits !!" << endl; return; }
   // Statistics
   V3Stat *modelStat = 0, *simplifyStat = 0, *solveStat = 0, *totalStat = 0;
   if (V3VrfBase::profileON()) {
      totalStat     = new V3Stat("TOTAL"); totalStat->start();
      modelStat     = new V3Stat("UNROLL MODEL",   totalStat);
      simplifyStat  = new V3Stat("SIMPLIFY NTK",   totalStat);
      solveStat     = new V3Stat("GENERALIZE",     totalStat);
   }
   // Unroll Ntk with Flops in the first frame as PI and all PIs as Flops
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(ntk);
   V3Ntk* unrolledNtk = createV3Ntk(isBvNtk); assert (unrolledNtk);
   V3NetId unrolledId; V3GateType type; uint32_t inSize;
   V3InputVec inputs; inputs.clear(); inputs.reserve(3);
   const string flushSpace = string(100, ' ');
   double runtime = clock();
   if (modelStat) modelStat->start();
   for (uint32_t i = 0; i < p->getCexTrace()->getTraceSize(); ++i) {
      if (V3VrfBase::intactON()) {
         if (!V3VrfBase::endLineON()) Msg(MSG_IFO) << "\r" + flushSpace + "\r";
         Msg(MSG_IFO) << "Unrolling Ntk to Frame = " << (1 + i);
         if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl; else Msg(MSG_IFO) << flush;
      }
      unrolledId = V3NetId::makeNetId(unrolledNtk->getNetSize() - 1);
      const uint32_t shift = i * (ntk->getNetSize() - 1);
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
      // Set Primary Inputs as Flops for Generalization
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
      // Set Flops to Initial Values or Buffers for Generalization
      const uint32_t prev_shift = shift - (ntk->getNetSize() - 1);
      for (uint32_t j = 0; j < ntk->getLatchSize(); ++j) {
         unrolledId = ntk->getLatch(j); unrolledId.id += shift; inputs.clear();
         if (i) {
            inputs.push_back(ntk->getInputNetId(ntk->getLatch(j), 0));
            if (inputs.back().id.id) inputs.back().id.id += prev_shift;
         }
         else {
            inputs.push_back(ntk->getInputNetId(ntk->getLatch(j), 1));
            if (inputs.back().id.id) inputs.back().id.id += shift;
         }
         assert (inputs.back().id.id < unrolledNtk->getNetSize());
         createV3BufGate(unrolledNtk, unrolledId, inputs.back().id);
      }
   }
   assert (unrolledNtk->getLatchSize() == 
           p->getCexTrace()->getTraceSize() * (ntk->getInputSize() + ntk->getInoutSize()));
   // Set Property as the Only Output
   assert (!unrolledNtk->getOutputSize()); unrolledId = ntk->getOutput(0);
   if (unrolledId.id) unrolledId.id += ((p->getCexTrace()->getTraceSize() - 1) * (ntk->getNetSize() - 1));
   assert (unrolledId.id < unrolledNtk->getNetSize()); unrolledNtk->createOutput(unrolledId);
   if (modelStat) modelStat->end();
   // Simplify Unrolled Ntk if Needed
   if (simplifyStat) simplifyStat->start();
   V3NtkHandler* unrolledHandler = new V3NtkHandler(0, unrolledNtk); assert (unrolledHandler);
   if (simplifyStat) simplifyStat->end();
   // Create Generalization-Enabled Simulator
   V3AlgGeneralize* generalize = 0; V3AlgSimulate* simulator = 0;
   if (dynamic_cast<V3BvNtk*>(unrolledNtk)) {
      generalize = new V3AlgBvGeneralize(unrolledHandler); assert (generalize);
      simulator = dynamic_cast<V3AlgBvSimulate*>(generalize); assert (simulator);
   }
   else {
      generalize = new V3AlgAigGeneralize(unrolledHandler); assert (generalize);
      simulator = dynamic_cast<V3AlgAigSimulate*>(generalize); assert (simulator);
   }
   // Simulate the Unrolled Ntk for One frame
   V3BitVecX pattern, value;
   for (uint32_t i = 0, k, flop = 0; i < p->getCexTrace()->getTraceSize(); ++i) {
      if (p->getCexTrace()->getTraceDataSize()) pattern = p->getCexTrace()->getData(i);
      assert (targetSize == pattern.size()); k = 0;
      // Set PI Values in ntk to Flops in unrolledNtk
      for (uint32_t j = 0; j < ntk->getInputSize(); ++j, ++flop) {
         value = pattern.bv_slice(k + ntk->getNetWidth(ntk->getInput(j)) - 1, k);
#ifdef V3_DFX_TRACE_RESTRICT_X
         value.restrict0();  // Restrict X to be 0 for Full Pattern
#endif
         assert (value.size() == unrolledNtk->getNetWidth(unrolledNtk->getLatch(flop)));
         simulator->setSource(unrolledNtk->getLatch(flop), value);
         k += ntk->getNetWidth(ntk->getInput(j)); assert (k <= pattern.size());
      }
      // Simulate Ntk for a Cycle
      simulator->simulate();
   }
   // Check Property Assertion
   assert ('1' == simulator->getSimValue(unrolledNtk->getOutput(0))[0]);
   // Set Cut Signals On Unrolled Ntk
   V3NetVec cutSignals; cutSignals.clear();
   cutSignals.reserve(1 + unrolledNtk->getLatchSize()); unrolledNtk->clearCutSignals();
   for (uint32_t i = 0; i < unrolledNtk->getLatchSize(); ++i) cutSignals.push_back(unrolledNtk->getLatch(i));
   cutSignals.push_back(unrolledNtk->getOutput(0)); unrolledNtk->setCutSignals(cutSignals);
   // Set Priority for Generalization
   V3UI32Vec prioNets; prioNets.clear(); prioNets.reserve(unrolledNtk->getCutSize());
   for (uint32_t i = 0; i < unrolledNtk->getCutSize(); ++i) prioNets.push_back(i);
   // Start Trace Generalization
   if (solveStat) solveStat->start();
   V3NetVec targets; targets.clear();
   targets.push_back(V3NetId::makeNetId(targetSize * p->getCexTrace()->getTraceSize(), 0));
   // Perform SAT Generalization
   //double runtime = clock();
   generalize->setTargetNets(targets);
   generalize->performFixForControlVars();
   generalize->performXPropForExtensibleVars(prioNets);
   targets = generalize->getGeneralizationResult();
   //runtime = (clock()- runtime) / CLOCKS_PER_SEC;
   if (solveStat) solveStat->end();
   // Record Generalization Results
   V3CexTrace* const cex = new V3CexTrace(p->getCexTrace()->getTraceSize()); assert (cex);
   uint32_t oriBits = 0, newBits = 0;
   for (uint32_t i = 0, k = 0; i < p->getCexTrace()->getTraceSize(); ++i) {
      if (p->getCexTrace()->getTraceDataSize()) pattern = p->getCexTrace()->getData(i);
      value.resize(pattern.size()); value.clear(); inSize = (1 + i) * targetSize;
      while (k < targets.size() && targets[k].id < inSize) {
         assert (targets[k].id >= (inSize - targetSize));
         assert ('X' != pattern[targets[k].id - (inSize - targetSize)]);
         if (targets[k].cp) value.set0(targets[k].id - (inSize - targetSize));
         else value.set1(targets[k].id - (inSize - targetSize)); ++k;
      }
      assert (value.bv_cover(pattern)); cex->pushData(value);
      if (V3VrfBase::reportON()) {
         oriBits += (pattern.size() - pattern.dcBits());
         newBits += (value.size() - value.dcBits());
      }
   }
   if (p->getCexTrace()->getInit()) cex->setInit(*(p->getCexTrace()->getInit()));
   // Report Counterexample Generalization Result
   if (V3VrfBase::reportON()) {
      runtime = (clock()- runtime) / CLOCKS_PER_SEC;
      if (V3VrfBase::intactON()) {
         if (V3VrfBase::endLineON()) Msg(MSG_IFO) << endl;
         else Msg(MSG_IFO) << "\r" << flushSpace << "\r";
      }
      Msg(MSG_IFO) << "Counterexample is Generalized from " << oriBits << " to " << newBits 
                   << " Bits in the Input Pattern";
      if (V3VrfBase::usageON()) Msg(MSG_IFO) << "  (time = " << setprecision(5) << runtime << "  sec)" << endl;
      if (V3VrfBase::profileON()) {
         totalStat->end();
         Msg(MSG_IFO) << *modelStat << endl;
         Msg(MSG_IFO) << *simplifyStat << endl;
         Msg(MSG_IFO) << *solveStat << endl;
         Msg(MSG_IFO) << *totalStat << endl;
      }
   }
   // Update the Counterexample Trace
   V3VrfResult cexResult; assert (cex);
   cexResult.setCexTrace(cex); p->setResult(cexResult); delete generalize;
}

#endif

