/****************************************************************************
  FileName     [ v3NtkInput.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ Base Input Handler for V3 Ntk. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_INPUT_H
#define V3_NTK_INPUT_H

#include "v3NtkHandler.h"

// Defines for Arrays
typedef V3Vec<string     >::Vec  V3StringVec;

// Define for Functional Pointers
typedef const bool(*V3Str2BoolFuncPtr) (const string&);

// V3NtkInput : Base V3 Ntk Parser
class V3NtkInput : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkInput(const bool&, const string& = "");
      ~V3NtkInput();
      // Ntk Ancestry Functions
      inline const string getNtkName() const { return _ntkName; }
      // Net Ancestry Functions
      const V3NetId createNet(const string& = "", uint32_t = 1);
      const V3NetId getNetId(const string&) const;
      // Ntk Extended Helper Functions
      void removePrefixNetName(V3Str2BoolFuncPtr);
      void renderFreeNetAsInput();
};

#endif

