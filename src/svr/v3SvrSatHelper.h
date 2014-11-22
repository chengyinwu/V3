/****************************************************************************
  FileName     [ v3SvrSatHelper.h ]
  PackageName  [ v3/src/svr ]
  Synopsis     [ Helper Functions for Word-level Formula to SAT Solver
                 Data Types conversion. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_SVR_SAT_HELPER_C
#define V3_SVR_SAT_HELPER_C

#include "SolverTypes.h"

template<class Solver>
inline void carry_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b, const Lit& c)
{
   vec<Lit> lits; lits.clear();
   // Implications : b & c -> y     b' & c' -> y'     a & c -> y     a' & c' -> y'     a & b -> y     a' & b' -> y'
   lits.push(~a); lits.push(~c); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~b); lits.push(~c); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push( c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push( c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void carry_2_woc(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   // It's carry_2 with c == 0
   // Implications : b' -> y'    a' -> y'    a & b -> y     a' & b' -> y'
   lits.push( a); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void xor_3(Solver* SS, const Lit& y, const Lit& a, const Lit& b, const Lit& c)
{
   vec<Lit> lits; lits.clear();
   // Implications : 
   // a' & b' & c' -> y'   a  & b  & c  -> y    a' & b  & c  -> y'   a  & b' & c' -> y
   // a  & b' & c  -> y'   a' & b  & c' -> y    a  & b  & c' -> y'   a' & b' & c  -> y
   lits.push( a); lits.push( b); lits.push( c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push(~c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( b); lits.push(~c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push( c); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push( c); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push( b); lits.push(~c); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push(~c); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( b); lits.push( c); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void borrow_2_woB(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   // It's borrow_2 with B = 0
   // Implications : a' & b -> x    a  & b' -> x'     a  -> y'    b' -> x'
   lits.push(~a); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void borrow_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b, const Lit& B)
{
   vec<Lit> lits; lits.clear();
   // Implications : a' & b -> x    a  & b' -> x'     a' & B -> x    a  & B' -> x'     b  & B -> x    b' & B' -> x'
   lits.push( a); lits.push(~B); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~b); lits.push(~B); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( B); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push( B); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();   
}

template<class Solver>
inline void buf(Solver* SS, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push(~a); lits.push( b); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void and_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push( a); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void or_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push(~a); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void xor_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push( a); lits.push( b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push(~b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( b); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void and_red(Solver* SS, const Lit& y, const Lit& a, const uint32_t& Width)
{
   vec<Lit> lits; lits.clear();
   if (Width == 1) return buf(SS, y, a);
   assert (!sign(y)); assert (Width > 1);
   const Var aVar = var(a); const bool aSign = sign(a);
   for (Var i = 0; i < (Var)Width; ++i) {
      lits.push(mkLit(aVar + i, aSign)); lits.push(~y); SS->addClause(lits); lits.clear();
   }
   for (Var i = 0; i < (Var)Width; ++i) {
      lits.push(mkLit(aVar + i, !aSign));
   }
   lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void or_red(Solver* SS, const Lit& y, const Lit& a, const uint32_t& Width)
{
   vec<Lit> lits; lits.clear();
   if (Width == 1) return buf(SS, y, a);
   assert (!sign(y)); assert (Width > 1);
   const Var aVar = var(a); const bool aSign = sign(a);
   for (Var i = 0; i < (Var)Width; ++i) {
      lits.push( mkLit(aVar + i, !aSign)); lits.push( y); SS->addClause(lits); lits.clear();
   }
   for (Var i = 0; i < (Var)Width; ++i) {
      lits.push( mkLit(aVar + i, aSign));
   }
   lits.push(~y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void xor_red(Solver* SS, const Lit& y, const Lit& a, const uint32_t& Width)
{
   vec<Lit> lits; lits.clear();
   if (Width == 1) return buf(SS, y, a);
   assert (!sign(y)); assert (Width > 1);
   const Var yVar = var(y), aVar = var(a); const bool aSign = sign(a);
   for (Var i = 1; i < (Var)Width - 1; ++i)
      xor_2(SS, mkLit(yVar + i), ((i == 1) ? a : mkLit(yVar + i - 1)), mkLit(aVar + i, aSign));
   xor_2(SS, y, ((Width > 2) ? mkLit(yVar + Width - 2) : a), mkLit(aVar + Width - 1, aSign));
}

template<class Solver>
inline void mux_a(Solver* SS, const Lit& o, const Lit& s, const Lit& a)
{
   vec<Lit> lits; lits.clear();
   // Implication : s' -> ( a == o) 
   lits.push( s); lits.push( a); lits.push(~o); SS->addClause(lits); lits.clear();
   lits.push( s); lits.push(~a); lits.push( o); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void mux_b(Solver* SS, const Lit& o, const Lit& s, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   // Implication : s -> ( b == o)
   lits.push(~s); lits.push( b); lits.push(~o); SS->addClause(lits); lits.clear();
   lits.push(~s); lits.push(~b); lits.push( o); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void mux_abo(Solver* SS, const Lit& o, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   // Implications : a & b -> o  a' & b' -> o'
   lits.push(~a); lits.push(~b); lits.push( o); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push( b); lits.push(~o); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void sh_select(Solver* SS, const Lit& y, const Lit& a, const Lit& b, const Lit& s)
{
   vec<Lit> lits; lits.clear();
   // y = as + bs'
   // Implications : as --> y, bs' --> y, a's --> y', b's' --> y'
   lits.push(~a); lits.push(~s); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push(~b); lits.push( s); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~s); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push( s); lits.push(~y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void geq(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push(~a); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( b); lits.push( y); SS->addClause(lits); lits.clear();
   lits.push( a); lits.push(~b); lits.push(~y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void greater_2(Solver* SS, const Lit& y, const Lit& a, const Lit& b)
{
   vec<Lit> lits; lits.clear();
   lits.push( a); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~b); lits.push(~y); SS->addClause(lits); lits.clear();
   lits.push(~a); lits.push( b); lits.push( y); SS->addClause(lits); lits.clear();
}

template<class Solver>
inline void geqInt(Solver* SS, const Var& y, const Var& g, const Var& e, const uint32_t& Width)
{
   vec<Lit> lits; lits.clear();
   for (Var i = 0; i < (Var)Width; ++i) {
      lits.push( mkLit(g + i)); lits.push(~mkLit(y + i)); SS->addClause(lits); lits.clear();
      for (Var j = i; j < (Var)Width - 1; ++j) {
         lits.push( mkLit(e + j)); lits.push(~mkLit(y + i)); SS->addClause(lits); lits.clear();
      }
      for (Var j = i; j < (Var)Width - 1; ++j) lits.push(~mkLit(e + j)); 
      lits.push(~mkLit(g + i)); lits.push( mkLit(y + i)); SS->addClause(lits); lits.clear();
   }
}

#endif

