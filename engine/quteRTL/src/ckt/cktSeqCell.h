/****************************************************************************
  FileName     [ cktSeqCell.h ]
  PackageName  [ ckt ]
  Synopsis     [ Define circuit sequential cell data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2005 LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CKT_SEQ_CELL_H
#define CKT_SEQ_CELL_H

//---------------------------
//  system include
//---------------------------
#include <string>
using namespace std;

//---------------------------
//  user include
//---------------------------
#include "cktCell.h"
#include "cktPin.h"

//---------------------------
//  Forward declaration
//---------------------------


//---------------------------
//  class definitions
//---------------------------
class CktDffCell : public CktCell
{
   //if (getReset() == 0) => SYNChronous
   //else                 => ASYNChronous
public:
   CktDffCell();
   CktDffCell(bool, CktInPin*&, CktOutPin*&);
   ~CktDffCell();

   // Attribute Descriptive Functions
   inline string getOpStr() const { return ""; }
   void writeOutput() const;
   void writeBLIF() const;
   void calLevel(unsigned);
   // DFF Related Operating Functions
   void      connReset(CktInPin*);
   void      connDefault(CktInPin*);
   void      connClk(CktInPin*);
   CktInPin* getReset() const;
   CktInPin* getDefault() const;
   CktInPin* getClk() const;

   // Operating Functions
   void travelDP(CktInPinAry&, const CktInPin*, PinLitMap&, int&, unsigned); 
   CktCell* cellDuplicate(string&, OutPinMap&, InPinMap&) const;
   void connDuplicate(CktCell*&, OutPinMap&, InPinMap&) const;
   void collectFin(queue<CktCell*>&) const;
   void handleDP(CktInPinAry&) const;
   void initDP(CktDpMap&);
   
private:
   CktInPin  *_clk,
             *_rst,
             *_default;
};

class CktDlatCell : public CktCell
{
public:
   CktDlatCell();
   CktDlatCell(bool, CktInPin*&, CktInPin*&, CktOutPin*&);
   ~CktDlatCell();

   // Attribute Descriptive Functions
   inline string getOpStr() const { return ""; }
   void writeOutput() const;
   void writeBTOR(unsigned&);
   void calLevel(unsigned);

   // DLAT Related Operating Functions
   void       connLoad(CktInPin*);
   CktInPin * getLoad() const;

   // Operating Functions
   void collectFin(queue<CktCell*>&) const;

private:
   //CktOutPin *_qBar; // no use
   CktInPin  *_load;
};

#endif // CKT_SEQ_CELL_H

