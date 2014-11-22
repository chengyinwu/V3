/****************************************************************************
  FileName     [ v3Ntk.h ]
  PackageName  [ v3/src/ntk ]
  Synopsis     [ V3 Network. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_NTK_H
#define V3_NTK_H

#include "v3Misc.h"
#include "v3Type.h"
#include "v3BitVec.h"

// V3 Ntk Defines
#define getV3InvertNet(netId)     (~netId)
#define getV3NetIndex(netId)      (netId.id)
#define isV3NetInverted(netId)    (netId.cp)
#define isV3BlackBoxed(module)    (!module->getRef())
#define isV3ConstType(type)       (AIG_FALSE == type || BV_CONST == type)
#define isV3ReducedType(type)     (BV_RED_AND <= type && BV_RED_XOR >= type)
#define isV3PairType(type)        (BV_AND <= type && BV_GEQ >= type)
#define isV3ExtendType(type)      (BV_OR <= type && BV_LESS >= type)
#define isV3ExtendSwapIn(type)    (BV_GREATER == type || BV_LEQ == type)
#define isV3ExtendInvIn(type)     (BV_OR == type || BV_NOR == type)
#define isV3ExtendInvOut(type)    \
   (BV_OR == type || BV_NAND == type || BV_XNOR == type || BV_NEQ == type || BV_GREATER == type || BV_LESS == type)
#define isV3ExchangableType(type) \
   (AIG_NODE == type || BV_AND == type || BV_XOR == type || BV_ADD == type || BV_MULT == type || BV_EQUALITY == type)
#define getV3ExtendNormal(type)   \
   ((BV_OR <= type && BV_NOR >= type) ? BV_AND : (BV_GREATER <= type && BV_LESS >= type) ? BV_GEQ : \
    (BV_XNOR == type) ? BV_XOR : (BV_NEQ == type) ? BV_EQUALITY : V3_GATE_TOTAL)

// Constants
const V3NetId        V3NetUD = V3NetId::makeNetId();

// Forward Declarations
class V3NtkHandler;
class V3NtkModule;

// Defines for Hash Types
typedef uint32_t     V3BusId;
typedef uint32_t     V3BVXId;
typedef V3PairType   V3BusPair;

// Defines for Arrays
typedef V3Vec<V3NetId      >::Vec   V3NetVec;
typedef V3Vec<V3MiscType   >::Vec   V3TypeVec;
typedef V3Vec<V3NetType    >::Vec   V3InputVec;
typedef V3Vec<V3BitVecX*   >::Vec   V3BitVecXVec;
typedef V3Vec<V3BusPair    >::Vec   V3BusPairVec;
typedef V3Vec<V3NtkModule* >::Vec   V3NtkModuleVec;

// Defines for Tables
typedef V3Vec<V3InputVec   >::Vec   V3InputTable;
typedef V3Vec<V3NetVec     >::Vec   V3NetTable;

// Defines for Hash Tables
typedef V3HashMap<string,     V3BVXId>::Hash V3ConstHash;
typedef V3HashMap<uint64_t,   V3BusId>::Hash V3BusIdHash;
typedef V3HashMap<uint32_t,   V3NetId>::Hash V3RepIdHash;

// V3NtkModule : V3 Ntk Reference (Ntk Instance)
class V3NtkModule
{
   public : 
      // Constructors for V3 Module Instance
      V3NtkModule(const V3NetVec& i, const V3NetVec& o) { _ref = 0; _inputs = i; _outputs = o; }
      ~V3NtkModule();
      // Ntk Reference Functions
      inline V3NtkHandler* const getNtkRef() const { return (V3NtkHandler*)(_ref & ~1ul); }
      inline const bool isNtkRefBlackBoxed() const { return (_ref & 1ul); }
      inline void updateNtkRef(const V3NtkHandler* const h, const bool& b = false) {
         assert (h); _ref = (b) ? ((size_t)h | 1ul) : (size_t)(h); }
      // Ntk I/O Functions
      inline const V3NetVec& getInputList() const { return _inputs; }
      inline const V3NetVec& getOutputList() const { return _outputs; }
      inline void updateInput(const uint32_t& i, const V3NetId& id) { assert (i < _inputs.size()); _inputs[i] = id; }
      inline void updateOutput(const uint32_t& i, const V3NetId& id) { assert (i < _outputs.size()); _outputs[i] = id; }
   private : 
      size_t      _ref;
      V3NetVec    _inputs;
      V3NetVec    _outputs;
};

// V3Ntk : V3 Base Network
class V3Ntk
{
   public : 
      // Constructors for V3 Network
      V3Ntk();
      virtual ~V3Ntk();
      // Ntk Construction Functions
      virtual void initialize();
      virtual const V3NetId createNet(const uint32_t& = 1);
      void createModule(V3NtkModule* const);
      void createInput(const V3NetId&);
      void createOutput(const V3NetId&);
      void createInout(const V3NetId&);
      void createLatch(const V3NetId&);
      void createConst(const V3NetId&);
      void createClock(const V3NetId&);
      //const bool extendNetId(const V3NetId&);
      void setInput(const V3NetId&, const V3InputVec&);
      void createGate(const V3GateType&, const V3NetId&);
      // Ntk Reconstruction Functions
      void replaceFanin(const V3RepIdHash&);
      void replaceOutput(const uint32_t&, const V3NetId&);
      // Ntk Destructive Functions  (NOT Recommended)
      inline void freeNetId(const V3NetId&);
      inline void removeLastInput() { _IOList[0].pop_back(); }
      inline void removeLastOutput() { _IOList[1].pop_back(); }
      inline void removeLastInout() { _IOList[2].pop_back(); }
      inline void removeLastLatch() { _FFList.pop_back(); }
      // Ntk Structure Functions
      virtual const uint32_t getNetWidth(const V3NetId&) const;
      inline const V3GateType getGateType(const V3NetId&) const;
      inline const uint32_t getNetSize() const { return _inputData.size(); }
      inline const uint32_t getModuleSize() const { return _ntkModule.size(); }
      inline const uint32_t getInputSize() const { return _IOList[0].size(); }
      inline const uint32_t getOutputSize() const { return _IOList[1].size(); }
      inline const uint32_t getInoutSize() const { return _IOList[2].size(); }
      inline const uint32_t getLatchSize() const { return _FFList.size(); }
      inline const uint32_t getConstSize() const { return _ConstList.size(); }
      inline const V3NetId& getInput(const uint32_t& i) const { assert (i < getInputSize()); return _IOList[0][i]; }
      inline const V3NetId& getOutput(const uint32_t& i) const { assert (i < getOutputSize()); return _IOList[1][i]; }
      inline const V3NetId& getInout(const uint32_t& i) const { assert (i < getInoutSize()); return _IOList[2][i]; }
      inline const V3NetId& getLatch(const uint32_t& i) const { assert (i < getLatchSize()); return _FFList[i]; }
      inline const V3NetId& getConst(const uint32_t& i) const { assert (i < getConstSize()); return _ConstList[i]; }
      inline const V3NetId& getClock() const { return _globalClk; }
      inline V3NtkModule* const getModule(const uint32_t&) const;
      inline V3NtkModule* const getModule(const V3NetId&) const;
      // Ntk Traversal Functions
      inline const uint32_t getInputNetSize(const V3NetId&) const;
      inline const V3NetId& getInputNetId(const V3NetId&, const uint32_t&) const;
      // Ntk Misc Data Functions
      inline void newMiscData() { assert (_globalMisc < V3MiscType(0, V3NtkUD).misc); ++_globalMisc; }
      inline const bool isLatestMiscData(const V3NetId&) const;
      inline void setLatestMiscData(const V3NetId&);
      // Ntk Cut Signal Functions
      inline void setCutSignals(const V3NetVec& cut) { _cutSignals = cut; }
      inline void clearCutSignals() { _cutSignals.clear(); }
      inline const uint32_t getCutSize() const { return _cutSignals.size(); }
      inline const V3NetId& getCutSignal(const uint32_t& i) const { return _cutSignals[i]; }
      // Ntk Construction Validation Report Functions
      inline const bool validNetId(const V3NetId& id) const { return (id.id) < getNetSize(); }
      const bool reportInvertingNet(const V3NetId&) const;
      const bool reportMultipleDrivenNet(const V3GateType&, const V3NetId&) const;
      const bool reportUnexpectedFaninSize(const V3GateType&, const V3NetId&) const;
      const bool reportUnexpectedNetWidth(const V3NetId&, const uint32_t&, const string&) const;
      const bool reportNetWidthInconsistency(const V3NetId&, const V3NetId&, const string&) const;
   protected : 
      // V3 Network Private Members
      V3NetVec       _IOList[3];    // V3NetId of PI / PO / PIO
      V3NetVec       _FFList;       // V3NetId of FF
      V3NetVec       _ConstList;    // V3NetId of BV_CONST (AIG_FALSE)
      V3TypeVec      _typeMisc;     // GateType with Misc Data
      V3InputTable   _inputData;    // Fanin Table for V3NetId   (V3NetId, V3BVXId, V3BusId)
      V3NetVec       _cutSignals;   // Signals on Current Cut
      uint32_t       _globalMisc;   // Global Misc Data for V3NetId in Ntk
      // V3 Special Handling Members
      V3NetId        _globalClk;    // Global Clock Signal (Specified in RTL)
      V3NtkModuleVec _ntkModule;    // Module Instance for Hierarchical Ntk
};

// V3AigNtk : V3 AIG Network
typedef V3Ntk     V3AigNtk;

// V3BvNtk : V3 BV Network
class V3BvNtk : public V3Ntk
{
   public : 
      // Constructors for BV Network
      V3BvNtk();
      V3BvNtk(const V3BvNtk&);
      ~V3BvNtk();
      // Ntk Construction Functions
      void initialize();
      const V3NetId createNet(const uint32_t& = 1);
      // Ntk Structure Functions
      const uint32_t getNetWidth(const V3NetId&) const;
      // Ntk for BV Gate Functions
      static const V3BVXId hashV3ConstBitVec(const string&);
      static const V3BusId hashV3BusId(const uint32_t&, const uint32_t&);
      const uint32_t getConstWidth(const V3BVXId&) const;
      const uint32_t getSliceWidth(const V3BusId&) const;
      const V3BitVecX getConstValue(const V3BVXId&) const;
      const uint32_t getSliceBit(const V3BusId&, const bool&) const;
      const V3BitVecX getInputConstValue(const V3NetId&) const;
      const uint32_t getInputSliceBit(const V3NetId&, const bool&) const;
      // Ntk Reconstruction Functions
      void resetNetWidth(const V3NetId&, const uint32_t&);
   private : 
      // Static Members for BV Gates
      static V3ConstHash   _V3ConstHash;  // Global Hash for V3BitVecX of V3Const for All V3BvNtk
      static V3BusIdHash   _V3BusIdHash;  // Global Hash for V3BusId of V3Slice for All V3BvNtk
      static V3BitVecXVec  _V3ValueVec;   // Global Vec for V3BVXId to V3BitVecX  Mapping
      static V3BusPairVec  _V3BusIdVec;   // Global Vec for V3BusId to (MSB, LSB) Mapping
      // BV Ntk Extended Data Member
      V3UI32Vec            _netWidth;     // Width of Bit-Vector V3NetId
};

// Inline Function Implementation of Ntk Destructive Functions
inline void V3Ntk::freeNetId(const V3NetId& id) {
   assert (validNetId(id)); _inputData[id.id].clear(); _typeMisc[id.id].type = V3_PI; }
// Inline Function Implementations of Ntk Structure Functions
inline const V3GateType V3Ntk::getGateType(const V3NetId& id) const {
   assert (validNetId(id)); return (V3GateType)_typeMisc[id.id].type; }
inline V3NtkModule* const V3Ntk::getModule(const uint32_t& i) const {
   assert (i < getModuleSize()); return _ntkModule[i]; }
inline V3NtkModule* const V3Ntk::getModule(const V3NetId& id) const {
   assert (V3_MODULE == getGateType(id)); return getModule(_inputData[id.id][0].value); }
// Inline Function Implementations of Ntk Traversal Functions
inline const uint32_t V3Ntk::getInputNetSize(const V3NetId& id) const {
   assert (validNetId(id)); return _inputData[id.id].size(); }
inline const V3NetId& V3Ntk::getInputNetId(const V3NetId& id, const uint32_t& i) const {
   assert (i < getInputNetSize(id)); return _inputData[id.id][i].id; }
// Inline Function Implementations of Ntk Misc Data Functions
inline const bool V3Ntk::isLatestMiscData(const V3NetId& id) const {
   assert (validNetId(id)); return _globalMisc == _typeMisc[id.id].misc; }
inline void V3Ntk::setLatestMiscData(const V3NetId& id) {
   assert (validNetId(id)); _typeMisc[id.id].misc = _globalMisc; assert (isLatestMiscData(id)); }

#endif

