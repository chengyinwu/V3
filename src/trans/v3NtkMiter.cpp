/****************************************************************************
  FileName     [ v3NtkMiter.cpp ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Bit Blasting of V3 BV Network to AIG Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_MITER_C
#define V3_NTK_MITER_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkMiter.h"
#include "v3NtkSimplify.h"

/* -------------------------------------------------- *\
 * Class V3NtkMiter Implementations
\* -------------------------------------------------- */
// Constuctor and Destructor
V3NtkMiter::V3NtkMiter(V3NtkHandler* const p1, V3NtkHandler* const p2, const bool& merge, const string& n) 
   : V3NtkHandler(p1, createV3Ntk(dynamic_cast<V3BvNtk*>(p1->getNtk()))), _ntkName(n) {
   assert (p1); assert (p1->getNtk()); assert (p2); assert (p2->getNtk());
   // Perform Network Transformation
   performNtkTransformation(p1, p2, merge);  // Construct SEC Miter
}

V3NtkMiter::V3NtkMiter(V3NtkHandler* const p1, V3NtkHandler* const p2, const V3UI32Vec& latchMap, const bool& merge, const string& n) 
   : V3NtkHandler(p1, createV3Ntk(dynamic_cast<V3BvNtk*>(p1->getNtk()))), _ntkName(n) {
   assert (p1); assert (p1->getNtk()); assert (p2); assert (p2->getNtk());
   assert (latchMap.size() == p1->getNtk()->getLatchSize());
   // Duplicate Networks
   V3NtkSimplify* const newP1 = new V3NtkSimplify(p1); assert (newP1); assert (newP1->getNtk());
   V3NtkSimplify* const newP2 = new V3NtkSimplify(p2); assert (newP2); assert (newP2->getNtk());
   for (uint32_t i = 0; i < latchMap.size(); ++i) {
      newP1->getNtk()->createOutput(newP1->getNtk()->getLatch(i));
      newP2->getNtk()->createOutput(newP2->getNtk()->getLatch(latchMap[i]));
   }
   // Perform Network Transformation
   performNtkTransformation(newP1, newP2, merge);  // Construct CEC Miter
}

V3NtkMiter::~V3NtkMiter() {
}

// Net Ancestry Functions
void
V3NtkMiter::getNetName(V3NetId& id, string& name) const {
   name = "";  // For V3NtkMiter is mainly for Internal Usage, NO Name is Given for all Nets
}

// Transformation Functions
void
V3NtkMiter::performNtkTransformation(V3NtkHandler* const handler1, V3NtkHandler* const handler2, const bool& merge) {
   // Construct a BV Network by Mitering handler1 and handler2
   // NOTE : I/O Size and Namings follow that in handler1
   assert (handler1 == handler2 || 
           !reportIncompatibleModule(const_cast<V3NtkHandler*>(handler1), const_cast<V3NtkHandler*>(handler2)));
   V3Ntk* const ntk = handler1->getNtk(); assert (ntk); assert (!ntk->getInoutSize());
   assert (handler2->getNtk()); assert (!handler2->getNtk()->getInoutSize());
   V3NetVec outputs; outputs.clear(); outputs.reserve(ntk->getOutputSize());
   const bool isBvNtk = dynamic_cast<V3BvNtk*>(_ntk);
   // 1. Create Inputs of ntk for _ntk
   V3NetVec inputs; inputs.clear();
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i) {
      inputs.push_back(_ntk->createNet(ntk->getNetWidth(ntk->getInput(i))));
      _ntk->createInput(inputs.back());
   }
   // 2. Create V3NetId for Outputs of ntk for _ntk  (2 copies)
   V3NetVec outputs1, outputs2; outputs1.clear(); outputs2.clear();
   V3NetId id; V3InputVec input(2);
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      outputs1.push_back(_ntk->createNet(ntk->getNetWidth(ntk->getOutput(i)))); input[0] = outputs1.back();
      outputs2.push_back(_ntk->createNet(ntk->getNetWidth(ntk->getOutput(i)))); input[1] = outputs2.back();
      // Create Boolean Outputs Indicates the Difference between Corresponding Output Pairs
      if (isBvNtk) {
         id = _ntk->createNet(); _ntk->setInput(id, input);
         _ntk->createGate(BV_EQUALITY, id); outputs.push_back(id);
      }
      else {
         id = _ntk->createNet(); _ntk->setInput(id, input); _ntk->createGate(AIG_NODE, id);
         const V3NetId cId = _ntk->createNet(); input[0] = ~(input[0].id); input[1] = ~(input[1].id);
         _ntk->setInput(cId, input); _ntk->createGate(AIG_NODE, cId); input[0] = ~id; input[1] = ~cId;
         id = _ntk->createNet(); _ntk->setInput(id, input); _ntk->createGate(AIG_NODE, id); outputs.push_back(~id);
      }
   }
   // 3. Create Module Instances for 2 Networks
   createModule(_ntk, inputs, outputs1, const_cast<V3NtkHandler*>(handler1));
   createModule(_ntk, inputs, outputs2, const_cast<V3NtkHandler*>(handler2));
   // 4. Create Outputs
   if (!merge) for (uint32_t i = 0; i < outputs.size(); ++i) _ntk->createOutput(~outputs[i]);
   else if (outputs.size()) {  // Combine All Outputs
      for (uint32_t i = 0; i < outputs.size(); ++i) {
         assert (1 == _ntk->getNetWidth(outputs[i]));
         if (!i) input[0] = outputs[i];
         else {
            input[1] = outputs[i]; id = _ntk->createNet(); _ntk->setInput(id, input);
            _ntk->createGate((isBvNtk ? BV_AND : AIG_NODE), id); input[0] = id;
         }
      }
      _ntk->createOutput(~(input[0].id));
   }
}

#endif

