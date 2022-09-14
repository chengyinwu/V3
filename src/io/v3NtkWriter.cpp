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

#endif

