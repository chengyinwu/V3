/****************************************************************************
  FileName     [ v3MCReorder.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ V3 Model Checker Verification Property Reordering. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_REORDER_C
#define V3_MC_REORDER_C

#include "v3MCMain.h"
#include "v3NtkUtil.h"

void reorderPropertyByNetId(V3NtkHandler* const handler, V3NetTable& constr, const bool& live) {
   assert (handler); V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Table of Property NetId and Names
   V3NetVec propId; propId.clear(); propId.reserve(ntk->getOutputSize());
   V3StrVec propName; propName.clear(); propName.reserve(ntk->getOutputSize());
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      propId.push_back(ntk->getOutput(i)); propName.push_back(handler->getOutputName(i));
   }
   // Sort Property by Ids
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map propIdMap; propIdMap.clear();
   uint32_t i = ntk->getOutputSize(), j;
   while (i--) {
      if (live && (constr.size() > i)) {
         j = 0; for (uint32_t f = 0; f < constr[i].size(); ++f) j += constr[i][f].id;
         propIdMap.insert(make_pair((double)(ntk->getNetSize() - j) / (double)constr[i].size(), i));
      }
      else propIdMap.insert(make_pair((double)(ntk->getNetSize() - ntk->getOutput(i).id), i));
   }
   assert (ntk->getOutputSize() == propIdMap.size());
   // Remove Outputs
   while (ntk->getOutputSize()) { handler->resetOutName(ntk->getOutputSize() - 1, ""); ntk->removeLastOutput(); }
   // Reorder Primary Outputs and Constraints
   const V3NetTable oriConstr = constr;
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map::const_iterator it;
   for (it = propIdMap.begin(); it != propIdMap.end(); ++it) {
      assert (it->second < propId.size()); ntk->createOutput(propId[it->second]);
      handler->resetOutName(ntk->getOutputSize() - 1, propName[it->second]);
      if (it->second < oriConstr.size()) constr[ntk->getOutputSize() - 1] = oriConstr[it->second];
   }
}

void reorderPropertyByNetLevel(V3NtkHandler* const handler, V3NetTable& constr, const bool& live) {
   assert (handler); V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Table of Property NetId and Names
   V3NetVec propId; propId.clear(); propId.reserve(ntk->getOutputSize());
   V3StrVec propName; propName.clear(); propName.reserve(ntk->getOutputSize());
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      propId.push_back(ntk->getOutput(i)); propName.push_back(handler->getOutputName(i));
   }
   // Sort Property by Levels
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(ntk->getNetSize());
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) targetNets.push_back(id);
   V3UI32Vec levelData; levelData.clear(); computeLevel(ntk, levelData, targetNets);
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map propIdMap; propIdMap.clear();
   uint32_t i = ntk->getOutputSize(), j;
   while (i--) {
      if (live && (constr.size() > i)) {
         j = 0; for (uint32_t f = 0; f < constr[i].size(); ++f) j += levelData[constr[i][f].id];
         propIdMap.insert(make_pair((double)j / (double)constr[i].size(), i));
      }
      else propIdMap.insert(make_pair((double)levelData[ntk->getOutput(i).id], i));
   }
   assert (ntk->getOutputSize() == propIdMap.size());
   // Remove Outputs
   while (ntk->getOutputSize()) { handler->resetOutName(ntk->getOutputSize() - 1, ""); ntk->removeLastOutput(); }
   // Reorder Primary Outputs and Constraints
   const V3NetTable oriConstr = constr;
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map::const_iterator it;
   for (it = propIdMap.begin(); it != propIdMap.end(); ++it) {
      assert (it->second < propId.size()); ntk->createOutput(propId[it->second]);
      handler->resetOutName(ntk->getOutputSize() - 1, propName[it->second]);
      if (it->second < oriConstr.size()) constr[ntk->getOutputSize() - 1] = oriConstr[it->second];
   }
}

void reorderPropertyByCOI(V3NtkHandler* const handler, V3NetTable& constr, const bool& live) {
   assert (handler); V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   if (live) return;  // NO Good idea for Liveness
   // Table of Property NetId and Names
   V3NetVec propId; propId.clear(); propId.reserve(ntk->getOutputSize());
   V3StrVec propName; propName.clear(); propName.reserve(ntk->getOutputSize());
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      propId.push_back(ntk->getOutput(i)); propName.push_back(handler->getOutputName(i));
   }
   // Compute COI of Property Signals
   V3NetTable idGroup; idGroup.clear(); idGroup.reserve(ntk->getOutputSize());
   for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) {
      if (live && (constr.size() > i)) idGroup.push_back(constr[i]);
      else idGroup.push_back(V3NetVec(1, ntk->getOutput(i)));
   }
   V3BoolTable marker; dfsMarkFaninCone(ntk, idGroup, marker);
   assert (ntk->getNetSize() == marker.size());
   // Record COI of Each Property
   const uint32_t inSize = ntk->getInputSize() + ntk->getInoutSize() + ntk->getLatchSize();
   V3BoolTable idMarker(idGroup.size(), V3BoolVec(inSize)); uint32_t k = 0;
   for (uint32_t i = 0; i < ntk->getInputSize(); ++i, ++k) {
      assert (idGroup.size() == marker[ntk->getInput(i).id].size());
      for (uint32_t j = 0; j < idGroup.size(); ++j)
         if (marker[ntk->getInput(i).id][j]) idMarker[j][k] = true;
   }
   for (uint32_t i = 0; i < ntk->getInoutSize(); ++i, ++k) {
      assert (idGroup.size() == marker[ntk->getInout(i).id].size());
      for (uint32_t j = 0; j < idGroup.size(); ++j)
         if (marker[ntk->getInout(i).id][j]) idMarker[j][k] = true;
   }
   for (uint32_t i = 0; i < ntk->getLatchSize(); ++i, ++k) {
      assert (idGroup.size() == marker[ntk->getLatch(i).id].size());
      for (uint32_t j = 0; j < idGroup.size(); ++j)
         if (marker[ntk->getLatch(i).id][j]) idMarker[j][k] = true;
   }
   // Compute An Adjacent Matrix for COI Similarity
   V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map sizeMap; sizeMap.clear();
   for (uint32_t i = 0; i < idMarker.size(); ++i) {
      k = 0; for (uint32_t j = 0; j < idMarker[i].size(); ++j) if (idMarker[i][j]) ++k;
      sizeMap.insert(make_pair(k, i));
   }
   V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map simVecEmpty; simVecEmpty.clear();
   V3Vec<V3Map<uint32_t, uint32_t, V3UI32LessOrEq<uint32_t> >::Map>::Vec simVec(idMarker.size(), simVecEmpty);
   for (uint32_t i = 0; i < idMarker.size(); ++i) {
      for (uint32_t j = 1; j < idMarker.size(); ++j) {
         uint32_t d = 0; k = 0; assert (idMarker[i].size() == idMarker[j].size());
         for (uint32_t x = 0; x < idMarker[i].size(); ++x) {
            if (idMarker[i][x] & idMarker[j][x]) ++k;
            if (idMarker[i][x] ^ idMarker[j][x]) ++d;
         }
         if (!k && d) k = V3NtkUD; simVec[i].insert(make_pair(k, j)); simVec[j].insert(make_pair(k, i));
      }
   }
   // Compute Levels
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(ntk->getNetSize());
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) targetNets.push_back(id);
   V3UI32Vec levelData; levelData.clear(); computeLevel(ntk, levelData, targetNets);
   // Categorize Properties into Groups According to the Similarity
   V3Vec<V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map>::Vec propGroup; propGroup.clear();
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map propGroupEmpty; propGroupEmpty.clear();
   V3BoolVec grouped(idGroup.size(), false); uint32_t pivot = V3NtkUD;
   for (V3Map<uint32_t, uint32_t>::Map::const_iterator it = sizeMap.begin(); it != sizeMap.end(); ++it) {
      if (grouped[it->second]) continue; propGroup.push_back(propGroupEmpty); pivot = it->second;
      while (true) {
         grouped[pivot] = true;
         if (live && (constr.size() > pivot)) {
            k = 0; for (uint32_t f = 0; f < constr[pivot].size(); ++f) k += levelData[constr[pivot][f].id];
            propGroup.back().insert(make_pair((double)k / (double)constr[pivot].size(), pivot));
         }
         else propGroup.back().insert(make_pair((double)levelData[ntk->getOutput(pivot).id], pivot));
         // Find the Most Similar Neighbor
         V3Map<uint32_t, uint32_t>::Map::iterator is = simVec[pivot].begin();
         while (is != simVec[pivot].end()) { if (!grouped[is->second]) break; ++is; }
         if (is == simVec[pivot].end() || V3NtkUD == is->first) break; pivot = is->second;
      }
   }
   // Sort Property Group By their Average Level
   V3Map<double, uint32_t, V3UI32LessOrEq<double> >::Map propOrder; propOrder.clear();
   for (uint32_t i = 0; i < propGroup.size(); ++i) {
      double level = 0;
      for (V3Map<double, uint32_t>::Map::const_iterator it = propGroup[i].begin(); it != propGroup[i].end(); ++it)
         level += it->first;
      level /= (double)propGroup[i].size();
      propOrder.insert(make_pair(level, i));
   }
   // Compute the Final Property Order
   V3UI32Vec propertyOrder; propertyOrder.clear(); propertyOrder.reserve(idGroup.size());
   for (V3Map<double, uint32_t>::Map::const_iterator it = propOrder.begin(); it != propOrder.end(); ++it) {
      for (V3Map<double, uint32_t>::Map::const_iterator is = propGroup[it->second].begin(); is != propGroup[it->second].end(); ++is)
         propertyOrder.push_back(is->second);
   }
   assert (ntk->getOutputSize() == propertyOrder.size());
   // Remove Outputs
   while (ntk->getOutputSize()) { handler->resetOutName(ntk->getOutputSize() - 1, ""); ntk->removeLastOutput(); }
   // Reorder Primary Outputs and Constraints
   const V3NetTable oriConstr = constr;
   for (uint32_t i = 0; i < propertyOrder.size(); ++i) {
      assert (propertyOrder[i] < propId.size()); ntk->createOutput(propId[propertyOrder[i]]);
      handler->resetOutName(ntk->getOutputSize() - 1, propName[propertyOrder[i]]);
      if (propertyOrder[i] < oriConstr.size()) constr[ntk->getOutputSize() - 1] = oriConstr[propertyOrder[i]];
   }
}

#endif
