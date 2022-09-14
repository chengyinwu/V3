/****************************************************************************
  FileName     [ v3NtkMiter.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Miter of V3 Networks. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_MITER_H
#define V3_NTK_MITER_H

#include "v3NtkHandler.h"

// class V3NtkMiter : Miter of V3 Networks
class V3NtkMiter : public V3NtkHandler
{
   public : 
      // Constructor and Destructor
      V3NtkMiter(V3NtkHandler* const, V3NtkHandler* const, const bool&, const string& = "");
      V3NtkMiter(V3NtkHandler* const, V3NtkHandler* const, const V3UI32Vec&, const bool&, const string& = "");
      ~V3NtkMiter();
      // Net Ancestry Functions
      void getNetName(V3NetId&, string&) const;
   private : 
      // Transformation Functions
      void performNtkTransformation(V3NtkHandler* const, V3NtkHandler* const, const bool&);
      // Private Members
      const string   _ntkName;
};

#endif

