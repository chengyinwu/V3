/****************************************************************************
  FileName     [ v3BvBlastAigHelper.h ]
  PackageName  [ v3/src/trans ]
  Synopsis     [ Helper Functions for Bit Blasting of V3 BV Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BV_BLAST_AIG_HELPER_H
#define V3_BV_BLAST_AIG_HELPER_H

#include "v3Ntk.h"

inline const V3NetId and_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   static V3InputVec inputs(2, V3NetUD); inputs[0] = a; inputs[1] = b;
   const V3NetId id = ntk->createNet(1); ntk->setInput(id, inputs);
   ntk->createGate(AIG_NODE, id); return id;
}

inline const V3NetId or_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   return ~and_2(ntk, ~a, ~b);
}

inline const V3NetId nand_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   return ~and_2(ntk, a, b);
}

inline const V3NetId nor_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   return and_2(ntk, ~a, ~b);
}

inline const V3NetId xor_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   return or_2(ntk, and_2(ntk, ~a, b), and_2(ntk, a, ~b));
}

inline const V3NetId xnor_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   return nor_2(ntk, and_2(ntk, ~a, b), and_2(ntk, a, ~b));
}

inline const V3NetId buf(V3Ntk* const ntk, const V3NetId& a)
{
   return and_2(ntk, a, a);
}

inline const V3NetId inv(V3Ntk* const ntk, const V3NetId& a)
{
   return nand_2(ntk, a, a);
}

inline const V3NetId and_red(V3Ntk* const ntk, const V3NetVec& a)
{
   V3NetId out_aig = a[0];
   for (uint32_t i = 1; i < a.size(); ++i) out_aig = and_2(ntk, out_aig, a[i]);
   return out_aig;
}

inline const V3NetId or_red(V3Ntk* const ntk, const V3NetVec& a)
{
   V3NetId out_aig = a[0];
   for (uint32_t i = 1; i < a.size(); ++i) out_aig = or_2(ntk, out_aig, a[i]);
   return out_aig;
}

inline const V3NetId xor_red(V3Ntk* const ntk, const V3NetVec& a)
{
   V3NetId out_aig = a[0];
   for (uint32_t i = 1; i < a.size(); ++i) out_aig = xor_2(ntk, out_aig, a[i]);
   return out_aig;
}

inline const V3NetId nand_red(V3Ntk* const ntk, const V3NetVec& a)
{
   return inv(ntk, and_red(ntk, a));
}

inline const V3NetId nor_red(V3Ntk* const ntk, const V3NetVec& a)
{
   return inv(ntk, or_red(ntk, a));
}

inline const V3NetId xnor_red(V3Ntk* const ntk, const V3NetVec& a)
{
   return inv(ntk, xor_red(ntk, a));
}

inline const V3NetId carry_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b, const V3NetId& c)
{
   // carry = ab + ac + bc
   return or_2(ntk, and_2(ntk, a, b), or_2(ntk, and_2(ntk, a, c), and_2(ntk, b, c)));
}

inline const V3NetId carry_2_woc(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // carry = a && b
   return and_2(ntk, a, b);
}

inline const V3NetId xor_3(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b, const V3NetId& c)
{
   // xor_3 = ab'c' + a'bc' + a'b'c + abc
   return xor_2(ntk, a, xor_2(ntk, b, c));
}

inline const V3NetId borrow_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b, const V3NetId& B)
{
   // borrow = a'b + a'B + bB
   return or_2(ntk, and_2(ntk, ~a, b), or_2(ntk, and_2(ntk, ~a, B), and_2(ntk, b, B)));
}

inline const V3NetId borrow_2_woB(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // borrow = a'b
   return and_2(ntk, ~a, b);
}

inline const V3NetId mux(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b, const V3NetId& s)
{
   // Mux Output = bs + as'
   return or_2(ntk, and_2(ntk, b, s), and_2(ntk, a, ~s));
}

inline const V3NetId select(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b, const V3NetId& s)
{
   // y = as + bs'
   return or_2(ntk, and_2(ntk, a, s), and_2(ntk, b, ~s));
}

inline const V3NetId geq_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // geq = a + b'
   return or_2(ntk, a, ~b);
}

inline const V3NetId greater_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // gt = ab'
   return and_2(ntk, a, ~b);
}

inline const V3NetId leq_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // leq = a' + b
   return or_2(ntk, ~a, b);
}

inline const V3NetId less_2(V3Ntk* const ntk, const V3NetId& a, const V3NetId& b)
{
   // lt = a'b
   return and_2(ntk, ~a, b);
}

inline const V3NetId compare_generate(V3Ntk* const ntk, const V3NetVec& e, const V3NetVec& c)
{
   V3NetVec out_comp(c.size());
   uint32_t i = c.size();
   while (i--) {
      out_comp[i] = c[i];
      for (uint32_t j = i; j < c.size() - 1; ++j) out_comp[i] = and_2(ntk, out_comp[i], e[j]);
   }
   return or_red(ntk, out_comp);
}

#endif

