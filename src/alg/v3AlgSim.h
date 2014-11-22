/****************************************************************************
  FileName     [ v3AlgSim.h ]
  PackageName  [ v3/src/alg ]
  Synopsis     [ V3 Ntk Simulation. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_ALG_SIM_H
#define V3_ALG_SIM_H

#include "v3AlgSimulate.h"

// General Simulation Functions
const bool performInputFileSimulation(const V3NtkHandler* const, const string&, const bool&, const string&);
const bool performRandomSimulation(const V3NtkHandler* const, const uint32_t&, const bool&, const string&);

#endif

