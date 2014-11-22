/****************************************************************************
  FileName     [ v3NtkWriter.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ Generic Writers for V3 Ntk Output. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_WRITER_C
#define V3_NTK_WRITER_C

#include "v3Msg.h"
#include "v3Graph.h"
#include "v3StrUtil.h"
#include "v3NtkWriter.h"

#include <sys/stat.h>
#include <sys/types.h>

// Writer Helper Functions
void writeV3GeneralHeader(const string& formatType, ofstream& output, const string& commentStart) {
   assert (output.is_open());
   // Get Current Time and Date
   time_t cur_time; time (&cur_time);
   string curTimeStr = asctime(localtime(&cur_time));
   // Alter into Other Formats, see http://www.cplusplus.com/reference/clibrary/ctime/strftime/
   output << commentStart << " " << formatType << " generated @ " 
          << curTimeStr.substr(0, curTimeStr.size() - 1) << " by V3 " << endl;
   output << commentStart << " " << string(formatType.size(), ' ') << " compiled @ " 
          << __DATE__ << " " << __TIME__ << endl;
}

// Helper Network Plot Functions
void recurTraverseNtk(V3Ntk* const ntk, const uint32_t& id, const uint32_t& pId, unsigned& depth, V3Graph<uint32_t>& G) {
   // This function traverse ntk to level bound only, and insert edge to graph
   assert (ntk); assert (ntk->getNetSize() > id); assert (V3NtkUD == pId || ntk->getNetSize() > pId);
   const V3GateType type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_XD > type);
   
   if (V3NtkUD != pId) {
      if (V3_FF != type) G.insertEdge(pId, id);
      else { G.insertEdge(pId, ntk->getNetSize() + id); return; }
   }

   if (--depth) {
      if (V3_MODULE == type) {
         assert (ntk->getInputNetSize(V3NetId::makeNetId(id)) == 1); assert (ntk->getModule(V3NetId::makeNetId(id)));
         const V3NetVec& inputs = ntk->getModule(V3NetId::makeNetId(id))->getInputList();
         for (uint32_t i = 0; i < inputs.size(); ++i) recurTraverseNtk(ntk, inputs[i].id, id, depth, G);
      }
      else {
         const uint32_t inSize = (V3_FF == type || AIG_NODE == type || isV3PairType(type)) ? 2 : 
                                 (BV_SLICE == type || isV3ReducedType(type)) ? 1 : (BV_MUX == type) ? 3 : 0;
         for (uint32_t i = 0; i < inSize; ++i) 
            recurTraverseNtk(ntk, ntk->getInputNetId(V3NetId::makeNetId(id), i).id, id, depth, G);
      }
   }
   ++depth;
}

void plotNetFanin(ofstream& os, V3Ntk* const ntk, const uint32_t& id, const unsigned& depth) {
   assert (ntk); assert (id < ntk->getNetSize()); assert (depth);
   const V3GateType type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_XD > type);
   // Plot Module Instance
   if (V3_MODULE == type) {
      assert (ntk->getInputNetSize(V3NetId::makeNetId(id)) == 1); assert (ntk->getModule(V3NetId::makeNetId(id)));
      const V3NetVec& inputs = ntk->getModule(V3NetId::makeNetId(id))->getInputList();
      for (uint32_t i = 0; i < inputs.size(); ++i) {
         const V3NetId netId = inputs[i]; assert (ntk->getNetSize() > netId.id);
         os << V3_INDENT << "\"" << netId.id << "_" << depth << "\" -> \"" << id << "_" 
            << (V3_FF == ntk->getGateType(V3NetId::makeNetId(id)) ? (depth - 1) : depth) 
            << "\" [style = " << ((ntk->getNetWidth(netId) > 1) ? "bold" : "solid");
         const string label = ((ntk->getNetWidth(netId) > 1) ? v3Int2Str(ntk->getNetWidth(netId)) : "") + 
            ((inputs.size() > 1) ? ("\"[" + v3Int2Str(i) + "]\"") : "");
         if (label.size()) os << ", label = " << label << ", labeldistance = 0.5";
         if (netId.cp) os << ", arrowhead = odot"; os << "];" << endl;
      }
   }
   else {
      // Plot Primitive Types
      const uint32_t inSize = (V3_FF == type || AIG_NODE == type || isV3PairType(type)) ? 2 : 
                              (BV_SLICE == type || isV3ReducedType(type)) ? 1 : (BV_MUX == type) ? 3 : 0;
      for (uint32_t i = 0; i < inSize; ++i) {
         const V3NetId netId = ntk->getInputNetId(V3NetId::makeNetId(id), i); assert (ntk->getNetSize() > netId.id);
         os << V3_INDENT << "\"" << netId.id << "_" << depth << "\" -> \"" << id << "_" 
            << (V3_FF == ntk->getGateType(V3NetId::makeNetId(id)) ? (depth - 1) : depth) 
            << "\" [style = " << ((ntk->getNetWidth(netId) > 1) ? "bold" : "solid");
         const string label = ((ntk->getNetWidth(netId) > 1) ? v3Int2Str(ntk->getNetWidth(netId)) : "") + 
            ((inSize > 1) ? ("[" + v3Int2Str(i) + "]") : "");
         if (label.size()) os << ", label = \"" << label << "\", labeldistance = 0.5";
         if (netId.cp) os << ", arrowhead = odot"; os << "];" << endl;
      }
   }
}

// Generic Network Plot Functions
void V3PlotNtkByLevel(const V3NtkHandler* const handler, const char* fileName, const uint32_t& maxLevel, const V3NetVec& rootNets, const bool& monoChrome) {
   assert (handler); assert (handler->getNtk()); assert (fileName); assert (maxLevel);
   V3Ntk* const ntk = handler->getNtk(); assert (rootNets.size());
   // Open Network Plot Output File
   assert (fileName); ofstream DOTFile; DOTFile.open(fileName);
   if (!DOTFile.is_open()) { Msg(MSG_ERR) << "Network Plot File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Plot Network by Level
   V3Queue<uint32_t>::Queue rootList, eqList, tempList;
   V3Stack<V3Queue<uint32_t>::Queue>::Stack sortedList;
   uint32_t id, i = 0, j = 0, k = 0, level = 1 + maxLevel;
   
   for (i = 0; i < rootNets.size(); ++i) rootList.push(rootNets[i].id);
   V3Graph<uint32_t> ntkGraph; V3GateType type; i = 0;
   DOTFile << "strict digraph G {" << endl;

   while (level) {
      // Ntk Traversal From Roots
      ntkGraph.clearGraph();
      while (!rootList.empty()) {
         id = rootList.front(); rootList.pop();
         if (i) {
            if (ntk->getNetSize() > id) continue;
            id -= ntk->getNetSize(); assert (ntk->getNetSize() > id);
            assert (V3_FF == ntk->getGateType(V3NetId::makeNetId(id)));
            recurTraverseNtk(ntk, id, V3NtkUD, level, ntkGraph);
         }
         else {
            assert (ntk->getNetSize() > id);
            recurTraverseNtk(ntk, id, V3NtkUD, level, ntkGraph);
         }
      }
      ntkGraph.getLeaves(rootList); ntkGraph.topologicalSort(sortedList);

      // Write to DOTFile
      j = 0; ++k;
      while (!sortedList.empty()) {
         // Plot Net Connectivity
         while (!tempList.empty()) {
            if (ntk->getNetSize() > tempList.front()) plotNetFanin(DOTFile, ntk, tempList.front(), k);
            else plotNetFanin(DOTFile, ntk, tempList.front() - ntk->getNetSize(), k); tempList.pop();
         }
         eqList = sortedList.top(); tempList = eqList; sortedList.pop(); ++j;
         if (j == 1) { if (i != 0) continue; else ++level; }
         // Plot Nets in Topological Order
         if (i == 0) DOTFile << V3_INDENT << "{ rank = same; \"(root)\"" << " [shape = plaintext]; ";
         else DOTFile << V3_INDENT << "{ rank = same; Level_" << i << " [shape = plaintext]; ";
         while (!eqList.empty()) {
            id = eqList.front(); eqList.pop();
            if (ntk->getNetSize() <= id) {
               id -= ntk->getNetSize(); assert (ntk->getNetSize() > id);
               type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_FF == type);
               DOTFile << "\"" << id << "_" << k << "\" [shape = box";
               if (!monoChrome) DOTFile << ", color = indigo, style = filled, fontcolor = snow";
            }
            else {
               type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_XD > type); DOTFile << "\"" << id << "_";
               if (V3_FF == type) {
                  DOTFile << k - 1 << "\" [shape = box";
                  if (!monoChrome) DOTFile << ", color = indigo, style = filled, fontcolor = snow";
               }
               else if (V3_FF > type) {
                  DOTFile << k << "\" [shape = triangle";
                  if (!monoChrome) DOTFile << ", color = springgreen, style = bold";
               }
               else if (V3_MODULE == type) {
                  DOTFile << k << "\" [shape = box3d";
                  if (!monoChrome) DOTFile << ", color = black, style = filled, fontcolor = white";
               }
               else if (BV_MUX == type) {
                  DOTFile << k << "\" [shape = trapezium";
                  if (!monoChrome) DOTFile << ", color = red, style = filled, fontcolor = snow";
               }
               else if (BV_CONST == type) DOTFile << k << "\" [shape = plaintext";
               else DOTFile << k << "\" [shape = ellipse";
            }
            // Set Label
            DOTFile << ", label = \"" << V3GateTypeStr[type] << "\\n";  // Gate Type
            if ("" == handler->getNetName(V3NetId::makeNetId(id))) DOTFile << "id = " << id << "\\n";  // Net Id
            else DOTFile << handler->getNetName(V3NetId::makeNetId(id)) << "\\n";  // Net Name
            DOTFile << "\"]; ";
         }
         DOTFile << "}" << endl;
         --level; ++i;
      }
      if (rootList.empty()) break;
   }
   if (i) {
      DOTFile << endl; DOTFile << V3_INDENT << "{ " << endl;
      DOTFile << V3_INDENT << V3_INDENT << "node [shape = plaintext]; edge [dir = none];" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "\"(root)\"";
      for (j = 1; j < i; ++j) DOTFile << " -> Level_" << j;
      DOTFile << "; " << endl; DOTFile << V3_INDENT << "} " << endl;
   }
   DOTFile << "}" << endl; DOTFile.close();
}

void V3PlotNtkByDepth(const V3NtkHandler* const handler, const char* fileName, const uint32_t& maxDepth, const V3NetVec& rootNets, const bool& monoChrome) {
   assert (handler); assert (handler->getNtk()); assert (fileName); assert (maxDepth);
   V3Ntk* const ntk = handler->getNtk(); assert (rootNets.size());
   // Open Network Plot Output File
   assert (fileName); ofstream DOTFile; DOTFile.open(fileName);
   if (!DOTFile.is_open()) { Msg(MSG_ERR) << "Network Plot File \"" << fileName << "\" Not Found !!" << endl; return; }
   // Plot Network by Depth
   V3Queue<uint32_t>::Queue rootList, eqList, tempList;
   V3Stack<V3Queue<uint32_t>::Queue>::Stack sortedList;
   uint32_t id, i = 0, j = 0, k = 0, level;
   
   for (i = 0; i < rootNets.size(); ++i) rootList.push(rootNets[i].id);
   V3Graph<uint32_t> ntkGraph; V3GateType type; i = 0;
   DOTFile << "strict digraph G {" << endl;

   for (uint32_t depth = 0; depth < maxDepth; ++depth) {
      // Ntk Traversal From Roots
      ntkGraph.clearGraph(); level = V3NtkUD;
      while (!rootList.empty()) {
         id = rootList.front(); rootList.pop();
         if (i) {
            if (ntk->getNetSize() > id) continue;
            id -= ntk->getNetSize(); assert (ntk->getNetSize() > id);
            assert (V3_FF == ntk->getGateType(V3NetId::makeNetId(id)));
            recurTraverseNtk(ntk, id, V3NtkUD, level, ntkGraph);
         }
         else {
            assert (ntk->getNetSize() > id);
            recurTraverseNtk(ntk, id, V3NtkUD, level, ntkGraph);
         }
      }
      ntkGraph.getLeaves(rootList); ntkGraph.topologicalSort(sortedList);

      // Write to DOTFile
      j = 0; ++k;
      while (!sortedList.empty()) {
         // Plot Net Connectivity
         while (!tempList.empty()) {
            if (ntk->getNetSize() > tempList.front()) plotNetFanin(DOTFile, ntk, tempList.front(), k);
            else plotNetFanin(DOTFile, ntk, tempList.front() - ntk->getNetSize(), k); tempList.pop();
         }
         eqList = sortedList.top(); tempList = eqList; sortedList.pop(); ++j;
         if (j == 1) if (i != 0) continue;
         // Plot Nets in Topological Order
         if (i == 0) DOTFile << V3_INDENT << "{ rank = same; \"(root)\"" << " [shape = plaintext]; ";
         else DOTFile << V3_INDENT << "{ rank = same; Level_" << i << " [shape = plaintext]; ";
         while (!eqList.empty()) {
            id = eqList.front(); eqList.pop();
            if (ntk->getNetSize() <= id) {
               id -= ntk->getNetSize(); assert (ntk->getNetSize() > id);
               type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_FF == type);
               DOTFile << "\"" << id << "_" << k << "\" [shape = box";
               if (!monoChrome) DOTFile << ", color = indigo, style = filled, fontcolor = snow";
            }
            else {
               type = ntk->getGateType(V3NetId::makeNetId(id)); assert (V3_XD > type); DOTFile << "\"" << id << "_";
               if (V3_FF == type) {
                  DOTFile << k - 1 << "\" [shape = box";
                  if (!monoChrome) DOTFile << ", color = indigo, style = filled, fontcolor = snow";
               }
               else if (V3_FF > type) {
                  DOTFile << k << "\" [shape = triangle";
                  if (!monoChrome) DOTFile << ", color = springgreen, style = bold";
               }
               else if (V3_MODULE == type) {
                  DOTFile << k << "\" [shape = box3d";
                  if (!monoChrome) DOTFile << ", color = black, style = filled, fontcolor = white";
               }
               else if (BV_MUX == type) {
                  DOTFile << k << "\" [shape = trapezium";
                  if (!monoChrome) DOTFile << ", color = red, style = filled, fontcolor = snow";
               }
               else if (BV_CONST == type) DOTFile << k << "\" [shape = plaintext";
               else DOTFile << k << "\" [shape = ellipse";
            }
            // Set Label
            DOTFile << ", label = \"" << V3GateTypeStr[type] << "\\n";  // Gate Type
            if ("" == handler->getNetName(V3NetId::makeNetId(id))) DOTFile << "id = " << id << "\\n";  // Net Id
            else DOTFile << handler->getNetName(V3NetId::makeNetId(id)) << "\\n";  // Net Name
            DOTFile << "\"]; ";
         }
         DOTFile << "}" << endl;
         ++i;
      }
      if (rootList.empty()) break;
   }
   if (i) {
      DOTFile << endl; DOTFile << V3_INDENT << "{ " << endl;
      DOTFile << V3_INDENT << V3_INDENT << "node [shape = plaintext]; edge [dir = none];" << endl;
      DOTFile << V3_INDENT << V3_INDENT << "\"(root)\"";
      for (j = 1; j < i; ++j) DOTFile << " -> Level_" << j;
      DOTFile << "; " << endl; DOTFile << V3_INDENT << "} " << endl;
   }
   DOTFile << "}" << endl; DOTFile.close();
}

#endif

