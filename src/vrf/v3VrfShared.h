/****************************************************************************
  FileName     [ v3VrfShared.h ]
  PackageName  [ v3/src/vrf ]
  Synopsis     [ Shared Information Among Verification Engines. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_VRF_SHARED_H
#define V3_VRF_SHARED_H

#include "v3Usage.h"
#include "v3SvrBase.h"
#include "v3Property.h"
#include "v3NtkHandler.h"

#include <pthread.h>
#include <sys/time.h>

// Defines
#define getTimeUsed(i, e)  ((double)(e.tv_sec - i.tv_sec) + (0.000001 * (double)(e.tv_usec - i.tv_usec)))

// Defines
typedef V3Vec<V3VrfResult>::Vec  V3VrfResultVec;

// class V3VrfShared : Base class for Verification Shared Data
class V3VrfShared
{
   public :
      // Constructor and Destructor
      V3VrfShared() { pthread_mutex_init(&_mutex, NULL); }
      virtual ~V3VrfShared() {}
      // Public Member Functions
      static inline void printLock() { pthread_mutex_lock(&_printMutex); }
      static inline void printUnlock() { pthread_mutex_unlock(&_printMutex); }
   protected :
      // Protected Member Functions
      inline void lock() { pthread_mutex_lock(&_mutex); }
      inline void unlock() { pthread_mutex_unlock(&_mutex); }
      // Protected Data Members
      pthread_mutex_t         _mutex;        // Mutex for a Specific Type of Shared Data
      static pthread_mutex_t  _printMutex;   // Mutex for Output Messages
};

// class V3VrfSharedBound : Deep Bound Data for Each Property
class V3VrfSharedBound : public V3VrfShared
{
   public :
      // Constructor and Destructor
      V3VrfSharedBound(const uint32_t&, const bool& = false);
      ~V3VrfSharedBound();
      // Public Member Functions
      const bool updateBound(const uint32_t&, const uint32_t&);
      const uint32_t getBound(const uint32_t&);
      inline uint32_t getUnsolvedSize() const { return _unsvd; }
      inline void getBound(V3UI32Vec& bound) { lock(); bound = _bound; unlock(); }
   private :
      // Private Data Members
      const bool        _print;     // Enable Deep Bound Printing (Only Valid for Single Property)
      V3UI32Vec         _bound;     // The Deep Bound of Each Property (V3NtkUD for Verified)
      uint32_t          _unsvd;     // Number of Unsolved Properties
};

// class V3VrfSharedInv : Cube Invariants for Checkers
class V3VrfSharedInv : public V3VrfShared
{
   public : 
      // Constructor and Destructor
      V3VrfSharedInv();
      ~V3VrfSharedInv();
      // Public Member Functions
      void pushInv(V3SvrBase* const, V3Ntk* const, const uint32_t&);
      inline void getInv(V3NetTable& inv) { lock(); inv = _inv; unlock(); }
      inline void updateInv(const V3NetTable& i) { lock(); _inv = i; unlock(); }
      inline void updateInv(const V3NetVec& i) { lock(); _inv.push_back(i); unlock(); }
   private : 
      // Private Data Members
      V3NetTable        _inv;       // Cube Invariants (in terms of latch indices)
};

// class V3VrfSharedNtk : Optimized Network for Checkers
class V3VrfSharedNtk : public V3VrfShared
{
   public :
      // Constructor and Destructor
      V3VrfSharedNtk();
      ~V3VrfSharedNtk();
      // Public Member Functions
      const uint32_t getNetSize();
      void updateNtk(V3Ntk* const, const V3NetTable&);
      void releaseNtk(V3NtkHandler* const);
      V3NtkHandler* const getNtk(V3NtkHandler* const);
      V3NtkHandler* const getNtk(V3NtkHandler* const, V3NetTable&);
   private :
      struct V3VrfSharedNtkData
      {
         uint32_t          _refCount;     // Reference Count
         V3NtkHandler*     _handler;      // Network Handler
         V3NetTable        _constr;       // Constraints
      };
      // Private Data Members
      V3Vec<V3VrfSharedNtkData>::Vec   _data;   // Verification Data for Optimized Networks
};

// class V3VrfSharedMem : Memory Control Among Checkers
class V3VrfSharedMem : public V3VrfShared
{
   public : 
      // Constructor and Destructor
      V3VrfSharedMem(const double& m) : V3VrfShared() { _maxMemory = m; }
      ~V3VrfSharedMem() {}
      // Public Member Functions
      inline void updateMaxMemory(const double& m) { lock(); _maxMemory = m; unlock(); }
      inline const double getMaxMemory() { lock(); const double m = _maxMemory; unlock(); return m; }
      inline const double getMemory() { lock(); const double m = v3Usage.getMemUsage(); unlock(); return m; }
      inline const bool isMemValid() { lock(); const double m = v3Usage.getMemUsage(); unlock(); return _maxMemory > m; }
   private : 
      // Private Data Members
      double               _maxMemory;    // Memory Bound
};

#endif

