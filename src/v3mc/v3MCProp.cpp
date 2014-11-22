/****************************************************************************
  FileName     [ v3MCProp.cpp ]
  PackageName  [ v3/src/v3mc ]
  Synopsis     [ Property Specification Handling for V3 Model Checking. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_MC_PROP_C
#define V3_MC_PROP_C

#include "v3MCMain.h"
#include "v3NtkWriter.h"

// V3 Property Specification Handling Helper Functions
const bool lexPropSpec(ifstream& input, uint32_t& lineNumber, V3StrVec& tokens) {
   assert (input.is_open()); tokens.clear();
   string buffer = ""; size_t i, j;
   while (!input.eof()) {
      getline(input, buffer); ++lineNumber; if (input.eof()) break;
      for (i = j = 0; i < buffer.size(); ++i) {
         if (buffer[i] == ' ' || buffer[i] == '\t') {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1;
         }
         else if ((buffer[i] == ':') && ((i + 1) < buffer.size()) && (buffer[1 + i] == '=')) {
            // Separator for Predicate and Property Name
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            tokens.push_back(":="); j = i + 2;
         }
         else if (buffer[i] == '\n' || ((buffer[i] == '/') && ((1 + i) < buffer.size()) && (buffer[1 + i] == '/'))) {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; break;
         }
      }
      if (i > j) tokens.push_back(buffer.substr(j));
      if (tokens.size()) return true;
   }
   return false;
}

// V3 Property Specification Handling Functions
V3NtkHandler* const readProperty(V3NtkHandler* const handler, const string& fileName) {
   assert (handler); assert (fileName.size());
   V3Ntk* const ntk = handler->getNtk(); assert (ntk);
   // Open Input Property Specification
   ifstream input; input.open(fileName.c_str());
   if (!input.is_open()) {
      Msg(MSG_ERR) << "Property Specification File \"" << fileName << "\" Not Found !!" << endl;
      return 0;
   }
   // Parse Header: Expecting <prop> <#predicate> <#property>
   uint32_t lineNumber = 0, predSize = 0, pSize = 0; V3StrVec tokens;
   if (!lexPropSpec(input, lineNumber, tokens) || tokens.size() != 3 || "prop" != tokens[0]) {
      Msg(MSG_ERR) << (lineNumber ? ("Line " + v3Int2Str((int)lineNumber) + " : ") : "") << "Expecting Header "
                   << "\"<prop> <#predicate> <#property>\" in the Input File !!" << endl;
      input.close(); return 0;
   }
   if (!v3Str2UInt(tokens[1], predSize) || predSize == 0) {
      Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected <#predicate> = " << tokens[1] << " !!" << endl;
      input.close(); return 0;
   }
   if (!v3Str2UInt(tokens[2], pSize) || pSize == 0) {
      Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected <#property> = " << tokens[2] << " !!" << endl;
      input.close(); return 0;
   }
   // Set Every Net as a Target for Elaboration
   V3NetVec targetNets; targetNets.clear(); targetNets.reserve(ntk->getNetSize());
   for (V3NetId id = V3NetId::makeNetId(0); id.id < ntk->getNetSize(); ++id.id) targetNets.push_back(id);
   V3NtkElaborate* const pNtk = new V3NtkElaborate(handler, targetNets); assert (pNtk);
   // Parse Predicate Expressions
   string name = "", expr = "";
   for (uint32_t i = 0; i < predSize; ++i) {
      // Get Predicate Name and Expression
      if (!lexPropSpec(input, lineNumber, tokens)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Expecting Expression of Predicate "
                      << (1 + i) << "/" << predSize << " !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      name = (tokens.size() > 1 && ":=" == tokens[1]) ? tokens[0] : ""; expr = "";
      if (handler->existNetName(name)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Variable Name \"" << name << "\" Already Exists !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      // Elaborate the Predicate
      for (uint32_t j = name.size() ? 2 : 0; j < tokens.size(); ++j) expr += (tokens[j] + " ");
      V3LTLFormula* const formula = new V3LTLFormula(handler, expr, name); assert (formula);
      if (!formula->isValid() || !formula->isLeaf(formula->getRoot())) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Unexpected Expression Found !!" << endl;
         delete formula; delete pNtk; input.close(); return 0;
      }
      const uint32_t index = pNtk->elaborateLTLFormula(formula);
      if (V3NtkUD == index) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Failed to Elaborate the Expression !!" << endl;
         delete formula; delete pNtk; input.close(); return 0;
      }
      delete formula; assert (i == index); assert (pNtk->getNtk()->getOutputSize() == (1 + index));
      if (1 != pNtk->getNtk()->getNetWidth(pNtk->getNtk()->getOutput(index))) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : The Expression is NOT a Boolean Predicate !!" << endl;
         input.close(); delete pNtk; return 0;
      }
      if (name.size()) pNtk->resetOutName(index, name);
   }
   // Parse Property Specifications
   V3UI32Vec prop, invc, fair;
   uint32_t safeSize = 0, liveSize = 0, invcSize = 0, fairSize = 0;
   for (uint32_t i = 0; i < pSize; ++i) {
      // Get Property Name and Expression
      if (!lexPropSpec(input, lineNumber, tokens)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Expecting Expression of Property"
                      << (1 + i) << "/" << predSize << " !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      name = (tokens.size() > 1 && ":=" == tokens[1]) ? tokens[0] : ""; expr = "";
      if (pNtk->existProperty(name)) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Property Name \"" << name << "\" Already Exists !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      uint32_t t = V3NtkUD, idx; prop.clear(); invc.clear(); fair.clear();
      for (uint32_t j = name.size() ? 2 : 0; j < tokens.size(); ++j) {
         if ("F" == tokens[j]) t = 0; else if ("G" == tokens[j]) t = 1; else if ("GF" == tokens[j]) t = 2;
         else {
            if (V3NtkUD == t) {
               Msg(MSG_ERR) << "Line " << lineNumber << " : Missing LTL Operator \"F\", \"G\", or \"GF\" !!" << endl;
               delete pNtk; input.close(); return 0;
            }
            if (!v3Str2UInt(tokens[j], idx) || predSize <= idx) {
               Msg(MSG_ERR) << "Line " << lineNumber << " : Predicate with Index = \"" << tokens[j]
                            << "\" Not Found !!" << endl;
               delete pNtk; input.close(); return 0;
            }
            if (0 == t) prop.push_back((uint32_t)idx);
            else if (1 == t) invc.push_back((uint32_t)idx);
            else if (2 == t) fair.push_back((uint32_t)idx);
         }
      }
      if (prop.size() > 1) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Too Many Predicates Under LTL Operator \"F\" !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      if (prop.size() && fair.size()) {
         Msg(MSG_ERR) << "Line " << lineNumber << " : Specifying \"GF\" Part in a Safety Property !!" << endl;
         delete pNtk; input.close(); return 0;
      }
      // Record Property
      if (prop.size()) ++safeSize; else ++liveSize; invcSize += invc.size(); fairSize += fair.size();
      const V3NetId pId = prop.size() ? ~(pNtk->getNtk()->getOutput(prop[0])) : V3NetId::makeNetId(0);
      V3LTLFormula* f = new V3LTLFormula(pNtk, pId, prop.size(), name); assert (f); assert (f->isValid());
      V3Property* const p = new V3Property(f); assert (p); pNtk->setProperty(p);
      if (invc.size()) for (uint32_t j = 0; j < invc.size(); ++j) p->setInvConstr(pNtk, invc[j], invc[j]);
      if (fair.size()) for (uint32_t j = 0; j < fair.size(); ++j) p->setFairConstr(pNtk, fair[j], fair[j]);
   }
   if (lexPropSpec(input, lineNumber, tokens))
      Msg(MSG_WAR) << "Line " << lineNumber << " : Omit Extra Specifications !!" << endl;
   Msg(MSG_IFO) << "Totally " << safeSize << " Safety, " << liveSize << " Liveness Properties are Added." << endl;
   input.close(); return pNtk;
}

void writeProperty(V3NtkHandler* const handler, const string& fileName, const bool& l2s, const bool& invc2Prop) {
   // This Function Write Property Specifications into PROP Format
   assert (handler); assert (handler->getNtk()); assert (fileName.size());
   // Open Property Specification Output File
   ofstream output; output.open(fileName.c_str());
   if (!output.is_open()) {
      Msg(MSG_ERR) << "Property Specification Output File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Elaborate Properties
   V3StrVec name; V3UI32Vec prop; V3UI32Table invc, fair;
   V3NtkElaborate* const pNtk = elaborateProperties(handler, name, prop, invc, fair, l2s, invc2Prop, 0, 0); assert (pNtk);
   assert (handler->getPropertyList().size() == prop.size()); assert (prop.size() == name.size());
   assert (prop.size() == invc.size()); assert (prop.size() == fair.size());
   // Output Property Specification Header
   output << "prop " << pNtk->getNtk()->getOutputSize() << " " << prop.size() << endl;
   // Output the List of Predicates
   output << "\n// List of Predicates" << endl;
   for (uint32_t i = 0; i < pNtk->getNtk()->getOutputSize(); ++i)
      output << pNtk->getNetExpression(pNtk->getNtk()->getOutput(i)) << endl;
   // Output the List of Properties
   output << "\n// List of Properties" << endl;
   for (uint32_t p = 0; p < prop.size(); ++p) {
      if (name[p].size()) output << name[p] << " := ";
      if (1ul & prop[p]) output << "G " << (prop[p] >> 1) << " ";  // Liveness
      else output << "F " << (prop[p] >> 1) << " ";  // Safety
      if (invc[p].size()) output << "G ";
      for (uint32_t i = 0; i < invc[p].size(); ++i) output << invc[p][i] << " ";
      if (fair[p].size()) output << "GF ";
      for (uint32_t i = 0; i < fair[p].size(); ++i) output << fair[p][i] << " ";
      output << endl;
   }
   // Output Footer
   output << endl;
   writeV3GeneralHeader("Property Specification for Network " + handler->getNtkName(), output, "//");
   output.close();
}

void startVerificationFromProp(V3NtkHandler* const handler, const uint32_t& numOfCores, const double& maxTime, const double& maxMemory) {
   // Start Timer
   struct timeval initTime; gettimeofday(&initTime, NULL); assert (handler);
   // Elaborate Property Network
   V3StrVec propName; V3UI32Vec prop; V3UI32Table invConstr, fairConstr;
   V3NtkElaborate* const elabNtk = elaborateProperties(handler, propName, prop, invConstr, fairConstr, 0, 0, 0, 0);
   assert (elabNtk); assert (prop.size() == propName.size());
   assert (prop.size() == invConstr.size()); assert (prop.size() == fairConstr.size());
   // Reorder Liveness Properties: Put Properties w/o Fairness Constraints Back
   V3StrVec propName2; V3UI32Vec prop2; V3UI32Table invConstr2, fairConstr2;
   propName2.clear(); prop2.clear(); invConstr2.clear(); fairConstr2.clear();
   uint32_t liveSize = 0; V3Ntk* const ntk = elabNtk->getNtk(); assert (ntk);
   for (uint32_t p = 0; p < prop.size(); ++p) {
      if (!(1ul & prop[p])) continue; ++liveSize;  // Safety
      if (fairConstr[p].size()) continue;  // Exist Fairness Constraints
      // Remove it from the List
      propName2.push_back(propName[p]); propName[p] = propName.back(); propName.pop_back();
      invConstr2.push_back(invConstr[p]); invConstr[p] = invConstr.back(); invConstr.pop_back();
      fairConstr2.push_back(fairConstr[p]); fairConstr[p] = fairConstr.back(); fairConstr.pop_back();
      prop2.push_back(prop[p]); prop[p] = prop.back(); prop.pop_back(); --p;
   }
   propName.insert(propName.end(), propName2.begin(), propName2.end());
   invConstr.insert(invConstr.end(), invConstr2.begin(), invConstr2.end());
   fairConstr.insert(fairConstr.end(), fairConstr2.begin(), fairConstr2.end());
   prop.insert(prop.end(), prop2.begin(), prop2.end());
   // Default Settings
   //Msg.setAllOutFile("/dev/null"); Msg.startAllOutFile(false); Msg.stopAllDefault();
   V3VrfBase::setReport(false); V3VrfBase::setDefaultSolver(V3_SVR_MINISAT);
   V3NtkHandler::setReduce(true); V3NtkHandler::setStrash(true); V3NtkHandler::setRewrite(true);
   // Start Elaboration and then Verification
   if (v3MCInteractive) {
      V3VrfShared::printLock();
      Msg(MSG_DBG) << "Network = " << handler->getNtkName() << ", Net Size = " << ntk->getNetSize() << endl;
      Msg(MSG_DBG) << "User Settings: NumOfCores = " << numOfCores << ", MaxTime = " << maxTime << " sec, "
                   << "MaxMemory = " << maxMemory << " MB" << endl;
      V3VrfShared::printUnlock();
   }
   // Initialize Shared Data
   V3VrfSharedMem* const sharedMem = new V3VrfSharedMem(0.5 * maxMemory); assert (sharedMem);
   V3UI32Vec cDistrib(V3MC_TYPE_TOTAL, 0), pDistrib(V3MC_TYPE_TOTAL, 0);
   pDistrib[V3MC_SAFE] = prop.size() - liveSize;
   cDistrib[V3MC_SAFE] = floor((double)(0.8 * numOfCores * pDistrib[V3MC_SAFE]) / (double)(prop.size()));
   pDistrib[V3MC_LIVE] = liveSize - prop2.size();
   cDistrib[V3MC_LIVE] = floor((double)(numOfCores - cDistrib[V3MC_SAFE])) / 2.4;
   pDistrib[V3MC_L2S] = liveSize;
   cDistrib[V3MC_L2S] = numOfCores - cDistrib[V3MC_SAFE] - cDistrib[V3MC_LIVE];
   V3MCResource res(cDistrib, pDistrib); pthread_t pSthread, pKthread, pLthread;
   V3MCMainParam pS, pK, pL; pS._constr.clear(); pK._constr.clear(); pL._constr.clear();
   // Keep the List of Primary Output Signals
   V3NetVec poNets; poNets.clear(); poNets.reserve(ntk->getOutputSize());
   for (uint32_t i = 0, j = ntk->getOutputSize(); i < j; ++i) poNets.push_back(ntk->getOutput(i));
   // Start Safety Checking
   if (pDistrib[V3MC_SAFE]) {
      V3VrfSharedBound* const sharedBound = new V3VrfSharedBound(pDistrib[V3MC_SAFE]);
      V3MCResult* const result = new V3MCResult(pDistrib[V3MC_SAFE]); assert (result); assert (sharedBound);
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) elabNtk->resetOutName(i, "");
      while (ntk->getOutputSize()) ntk->removeLastOutput(); assert (!ntk->getOutputSize());
      // Elaborate Network
      V3UI32Vec property; property.clear(); property.reserve(pDistrib[V3MC_SAFE]);
      V3NetTable invList; invList.clear(); invList.reserve(pDistrib[V3MC_SAFE]);
      for (uint32_t p = 0; p < prop.size(); ++p) {
         if (1ul & prop[p]) continue; assert ((prop[p] >> 1) < poNets.size());
         property.push_back(property.size()); ntk->createOutput(poNets[prop[p] >> 1]);
         elabNtk->resetOutName(ntk->getOutputSize() - 1, propName[p]);
         invList.push_back(V3NetVec()); invList.back().clear();
         for (uint32_t i = 0; i < invConstr[p].size(); ++i) invList.back().push_back(poNets[invConstr[p][i]]);
      }
      assert (pDistrib[V3MC_SAFE] == property.size()); assert (pDistrib[V3MC_SAFE] == invList.size());
      assert (pDistrib[V3MC_SAFE] == ntk->getOutputSize());
      V3NtkHandler* const simpHandler = elaborateSafetyNetwork(elabNtk, property, invList);
      V3Ntk* const simpNtk = simpHandler->getNtk(); assert (simpNtk);
      assert (simpNtk->getOutputSize() == pDistrib[V3MC_SAFE]);
      // Combine Constraints to Property Logic
      for (uint32_t i = 0; i < pDistrib[V3MC_SAFE]; ++i)
         if (invList[i].size()) combineConstraintsToOutputs(simpNtk, V3UI32Vec(1, i), invList[i]);
      // Create Safety Checking Main Parameters
      //V3VrfSharedNtk* const sharedNtk = new V3VrfSharedNtk(); assert (sharedNtk);
      pS._handler = simpHandler;
      pS._baseNtk = handler;
      //pS._constr = invList;
      pS._type = V3MC_SAFE;
      pS._sharedBound = sharedBound;
      pS._mirrorBound = 0;
      pS._sharedInv = new V3VrfSharedInv();
      pS._sharedNtk = 0;
      //pS._sharedNtk = sharedNtk;
      pS._sharedMem = sharedMem;
      pS._res = &res;
      pS._initTime = initTime;
      pS._maxTime = maxTime;
      pS._maxMemory = maxMemory;
      pS._inputSize = elabNtk->getNtk()->getInputSize();
      pS._latchSize = elabNtk->getNtk()->getLatchSize();
      pS._result = result;
      if (!liveSize) { delete elabNtk; startVerificationMain((void*)&pS); goto join; }
      else pthread_create(&pSthread, NULL, &startVerificationMain, (void*)&pS);
   }
   // Start Liveness Checking
   if (liveSize) {
      V3VrfSharedBound* const sharedKBound = new V3VrfSharedBound(pDistrib[V3MC_LIVE]); assert (sharedKBound);
      V3VrfSharedBound* const sharedLBound = new V3VrfSharedBound(pDistrib[V3MC_L2S]); assert (sharedLBound);
      V3MCResult* const result = new V3MCResult(liveSize); assert (result); assert (pDistrib[V3MC_L2S]);
      // Start Liveness Checking
      if (pDistrib[V3MC_LIVE]) {
         for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) elabNtk->resetOutName(i, "");
         while (ntk->getOutputSize()) ntk->removeLastOutput(); assert (!ntk->getOutputSize());
         // Elaborate Network
         V3UI32Vec property; property.clear(); property.reserve(pDistrib[V3MC_LIVE]);
         V3NetTable invList; invList.clear(); invList.reserve(pDistrib[V3MC_LIVE]);
         V3NetTable fairList; fairList.clear(); fairList.reserve(pDistrib[V3MC_LIVE]);
         for (uint32_t p = 0; p < prop.size(); ++p) {
            if (!(1ul & prop[p])) continue; assert ((prop[p] >> 1) < poNets.size());
            property.push_back(property.size()); ntk->createOutput(poNets[prop[p] >> 1]);
            elabNtk->resetOutName(ntk->getOutputSize() - 1, propName[p]);
            invList.push_back(V3NetVec()); invList.back().clear();
            for (uint32_t i = 0; i < invConstr[p].size(); ++i) invList.back().push_back(poNets[invConstr[p][i]]);
            fairList.push_back(V3NetVec()); fairList.back().clear();
            for (uint32_t i = 0; i < fairConstr[p].size(); ++i) fairList.back().push_back(poNets[fairConstr[p][i]]);
            assert (fairList.back().size());
         }
         assert (pDistrib[V3MC_LIVE] == property.size()); assert (pDistrib[V3MC_LIVE] == invList.size());
         assert (pDistrib[V3MC_LIVE] == fairList.size()); assert (pDistrib[V3MC_LIVE] == ntk->getOutputSize());
         V3NtkHandler* const simpHandler = elaborateLivenessNetwork(elabNtk, property, invList, fairList);
         V3Ntk* const simpNtk = simpHandler->getNtk(); assert (simpNtk);
         assert (simpNtk->getOutputSize() == pDistrib[V3MC_LIVE]);

         // IF ALL LIVENESS PROPERTIES HAS FAIRNESS CONSTRAINTS, START L2S FROM HERE
         if (pDistrib[V3MC_LIVE] == pDistrib[V3MC_L2S]) {
            // Create Live2Safe Network
            V3StrVec pName; pName.clear(); pName.reserve(pDistrib[V3MC_L2S]);
            for (uint32_t i = 0; i < pDistrib[V3MC_L2S]; ++i) pName.push_back(simpHandler->getOutputName(i));
            for (uint32_t i = 0; i < simpNtk->getOutputSize(); ++i) simpHandler->resetOutName(i, "");
            while (simpNtk->getOutputSize()) simpNtk->removeLastOutput(); assert (!simpNtk->getOutputSize());
            V3Vec<V3Property*>::Vec pList; pList.clear(); pList.reserve(property.size());
            for (uint32_t p = 0; p < property.size(); ++p) {
               const uint32_t constrStart = simpNtk->getOutputSize();
               for (uint32_t i = 0; i < invList[p].size(); ++i) simpNtk->createOutput(invList[p][i]);
               for (uint32_t i = 0; i < fairList[p].size(); ++i) simpNtk->createOutput(fairList[p][i]);
               V3LTLFormula* const f = new V3LTLFormula(simpHandler, V3NetId::makeNetId(0), false, pName[p]);
               pList.push_back(new V3Property(f)); assert (pList.back());
               if (invList[p].size())
                  pList.back()->setInvConstr(simpHandler, constrStart, constrStart + invList[p].size() - 1);
               if (fairList[p].size())
                  pList.back()->setFairConstr(simpHandler, constrStart + invList[p].size(), simpNtk->getOutputSize() - 1);
            }
            V3NetTable invListL2S; invListL2S.clear(); invListL2S.reserve(pDistrib[V3MC_L2S]);
            V3NtkElaborate* const pNtk = new V3NtkElaborate(simpHandler); assert (pNtk);
            for (uint32_t p = 0; p < property.size(); ++p) {
               invListL2S.push_back(V3NetVec()); invListL2S.back().clear();
               const uint32_t pIndex = pNtk->elaborateProperty(pList[p], invListL2S.back());
               assert (p == pIndex); pNtk->resetOutName(p, pName[p]);
            }
            // Recover Input Network
            for (uint32_t i = 0; i < simpNtk->getOutputSize(); ++i) simpHandler->resetOutName(i, "");
            while (simpNtk->getOutputSize()) simpNtk->removeLastOutput(); assert (!simpNtk->getOutputSize());
            for (uint32_t i = 0; i < pName.size(); ++i) {
               simpNtk->createOutput(V3NetId::makeNetId(0, 1)); simpHandler->resetOutName(i, pName[i]); }
            // Combine Constraints to Property Logic
            for (uint32_t i = 0; i < pDistrib[V3MC_L2S]; ++i)
               if (invListL2S[i].size()) combineConstraintsToOutputs(pNtk->getNtk(), V3UI32Vec(1, i), invListL2S[i]);
            // Create Live2Safe Main Parameters
            pL._handler = pNtk;
            pL._baseNtk = handler;
            //pL._constr = invListL2S;
            pL._type = V3MC_L2S;
            pL._sharedBound = sharedLBound;
            pL._mirrorBound = sharedKBound;
            pL._sharedInv = new V3VrfSharedInv();
            pL._sharedNtk = 0;
            //pL._sharedNtk = new V3VrfSharedNtk();
            pL._sharedMem = sharedMem;
            pL._res = &res;
            pL._initTime = initTime;
            pL._maxTime = maxTime;
            pL._maxMemory = maxMemory;
            pL._inputSize = elabNtk->getNtk()->getInputSize();
            pL._latchSize = elabNtk->getNtk()->getLatchSize();
            pL._result = result;
            pthread_create(&pLthread, NULL, &startVerificationMain, (void*)&pL);
         }
         else {
            for (uint32_t i = 0; i < simpNtk->getOutputSize(); ++i)
               simpNtk->replaceOutput(i, V3NetId::makeNetId(0, 1));
         }
         // Combine Constraints to Property Logic
         for (uint32_t i = 0; i < pDistrib[V3MC_LIVE]; ++i)
            if (invList[i].size()) combineConstraintsToOutputs(simpNtk, V3UI32Vec(1, i), invList[i]);
         // Create K-Liveness Main Parameters
         pK._handler = simpHandler;
         pK._baseNtk = handler;
         pK._constr = fairList;
         pK._type = V3MC_LIVE;
         pK._sharedBound = sharedKBound;
         pK._mirrorBound = 0;
         pK._sharedInv = 0;
         pK._sharedNtk = 0;
         //pK._sharedNtk = new V3VrfSharedNtk();
         pK._sharedMem = sharedMem;
         pK._res = &res;
         pK._initTime = initTime;
         pK._maxTime = maxTime;
         pK._maxMemory = maxMemory;
         pK._inputSize = elabNtk->getNtk()->getInputSize();
         pK._latchSize = elabNtk->getNtk()->getLatchSize();
         pK._result = result;
         if (pDistrib[V3MC_LIVE] == pDistrib[V3MC_L2S]) { delete elabNtk; startVerificationMain((void*)&pK); goto join; }
         else pthread_create(&pKthread, NULL, &startVerificationMain, (void*)&pK);
      }
      // Start Live2Safe Checking
      assert (pDistrib[V3MC_L2S]); assert (pDistrib[V3MC_L2S] > pDistrib[V3MC_LIVE]);
      for (uint32_t i = 0; i < ntk->getOutputSize(); ++i) elabNtk->resetOutName(i, "");
      while (ntk->getOutputSize()) ntk->removeLastOutput(); assert (!ntk->getOutputSize());
      // Elaborate Network
      V3UI32Vec property; property.clear(); property.reserve(pDistrib[V3MC_L2S]);
      V3NetTable invList; invList.clear(); invList.reserve(pDistrib[V3MC_L2S]);
      V3NetTable fairList; fairList.clear(); fairList.reserve(pDistrib[V3MC_L2S]);
      for (uint32_t p = 0; p < prop.size(); ++p) {
         if (!(1ul & prop[p])) continue; assert ((prop[p] >> 1) < poNets.size());
         property.push_back(property.size()); ntk->createOutput(poNets[prop[p] >> 1]);
         elabNtk->resetOutName(ntk->getOutputSize() - 1, propName[p]);
         invList.push_back(V3NetVec()); invList.back().clear();
         for (uint32_t i = 0; i < invConstr[p].size(); ++i) invList.back().push_back(poNets[invConstr[p][i]]);
         fairList.push_back(V3NetVec()); fairList.back().clear();
         for (uint32_t i = 0; i < fairConstr[p].size(); ++i) fairList.back().push_back(poNets[fairConstr[p][i]]);
      }
      assert (pDistrib[V3MC_L2S] == property.size()); assert (pDistrib[V3MC_L2S] == invList.size());
      assert (pDistrib[V3MC_L2S] == fairList.size()); assert (pDistrib[V3MC_L2S] == ntk->getOutputSize());
      V3NtkHandler* const simpHandler = elaborateLivenessNetwork(elabNtk, property, invList, fairList);
      V3Ntk* const simpNtk = simpHandler->getNtk(); assert (simpNtk);
      assert (simpNtk->getOutputSize() == pDistrib[V3MC_L2S]);
      // Create Live2Safe Network
      V3StrVec pName; pName.clear(); pName.reserve(pDistrib[V3MC_L2S]);
      for (uint32_t i = 0; i < pDistrib[V3MC_L2S]; ++i) pName.push_back(simpHandler->getOutputName(i));
      for (uint32_t i = 0; i < simpNtk->getOutputSize(); ++i) simpHandler->resetOutName(i, "");
      while (simpNtk->getOutputSize()) simpNtk->removeLastOutput(); assert (!simpNtk->getOutputSize());
      V3Vec<V3Property*>::Vec pList; pList.clear(); pList.reserve(property.size());
      for (uint32_t p = 0; p < property.size(); ++p) {
         const uint32_t constrStart = simpNtk->getOutputSize();
         for (uint32_t i = 0; i < invList[p].size(); ++i) simpNtk->createOutput(invList[p][i]);
         for (uint32_t i = 0; i < fairList[p].size(); ++i) simpNtk->createOutput(fairList[p][i]);
         V3LTLFormula* const f = new V3LTLFormula(simpHandler, V3NetId::makeNetId(0), false, pName[p]);
         pList.push_back(new V3Property(f)); assert (pList.back());
         if (invList[p].size())
            pList.back()->setInvConstr(simpHandler, constrStart, constrStart + invList[p].size() - 1);
         if (fairList[p].size())
            pList.back()->setFairConstr(simpHandler, constrStart + invList[p].size(), simpNtk->getOutputSize() - 1);
      }
      V3NtkElaborate* const pNtk = new V3NtkElaborate(simpHandler); assert (pNtk);
      for (uint32_t p = 0; p < property.size(); ++p) {
         invList[p].clear(); pNtk->elaborateProperty(pList[p], invList[p]);
         pNtk->resetOutName(p, pName[p]);
      }
      // Combine Constraints to Property Logic
      for (uint32_t i = 0; i < pDistrib[V3MC_L2S]; ++i)
         if (invList[i].size()) combineConstraintsToOutputs(pNtk->getNtk(), V3UI32Vec(1, i), invList[i]);
      // Create Live2Safe Main Parameters
      pL._handler = pNtk;
      pL._baseNtk = handler;
      //pL._constr = invList;
      pL._type = V3MC_L2S;
      pL._sharedBound = sharedLBound;
      pL._mirrorBound = sharedKBound;
      pL._sharedInv = new V3VrfSharedInv();
      pL._sharedNtk = 0;
      //pL._sharedNtk = new V3VrfSharedNtk();
      pL._sharedMem = sharedMem;
      pL._res = &res;
      pL._initTime = initTime;
      pL._maxTime = maxTime;
      pL._maxMemory = maxMemory;
      pL._inputSize = elabNtk->getNtk()->getInputSize();
      pL._latchSize = elabNtk->getNtk()->getLatchSize();
      pL._result = result;
      delete simpHandler; delete elabNtk;
      startVerificationMain((void*)&pL);
   }

join:
   // Waiting Running Threads
   if (pDistrib[V3MC_SAFE]) { if (liveSize) pthread_join(pSthread, 0); }
   else {
      if (pDistrib[V3MC_LIVE] == pDistrib[V3MC_L2S]) pthread_join(pLthread, 0);
      else if (pDistrib[V3MC_LIVE]) pthread_join(pKthread, 0);
   }

   // Report Verification Results
   Msg(MSG_IFO) << "Totally ";
   if (pDistrib[V3MC_SAFE]) Msg(MSG_IFO) << (pDistrib[V3MC_SAFE] - pS._sharedBound->getUnsolvedSize()) << "/" << pDistrib[V3MC_SAFE] << " Safety ";
   if (pDistrib[V3MC_L2S ]) Msg(MSG_IFO) << (pDistrib[V3MC_L2S ] - pL._sharedBound->getUnsolvedSize()) << "/" << pDistrib[V3MC_L2S ] << " Liveness ";
   Msg(MSG_IFO) << "Solved." << endl;
}

#endif

