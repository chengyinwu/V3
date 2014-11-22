/****************************************************************************
  FileName     [ v3Usage.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ Report Run Time and Memory Usage. ]
  Author       [ Chung-Yang (Ric) Huang, Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_USAGE_H
#define V3_USAGE_H

#include "v3Msg.h"
#include "v3IntType.h"

#include <ctime>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iomanip>

#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>

#undef   MYCLK_TCK
#define  MYCLK_TCK sysconf(_SC_CLK_TCK)

class V3Usage;
class V3Stat;
extern V3Usage v3Usage;

class V3Usage 
{
   public:
      V3Usage() {
         reset();
      }
      void reset() {
         _initMem = checkMem(); _peakMem = _totalMem = _currentMem = 0;
         _currentTick =  checkTick(); _periodUsedTime = _totalUsedTime = 0.0;
      }
      void report(const bool& repTime, const bool& repMem) {
         if (repTime) {
            setTimeUsage();
            Msg(MSG_IFO) << "Period time used : " << setprecision(4) << _periodUsedTime << " seconds" << endl;
            Msg(MSG_IFO) << "Total time used  : " << setprecision(4) << _totalUsedTime << " seconds" << endl;
         }
         if (repMem) {
            setMemUsage();
            Msg(MSG_IFO) << "Peak memory used   : " << setprecision(4) << _peakMem << " M Bytes" << endl;
            Msg(MSG_IFO) << "Total memory used  : " << setprecision(4) << _totalMem << " M Bytes" << endl;
            Msg(MSG_IFO) << "Current memory used: " << setprecision(4) << _currentMem << " M Bytes" << endl;
         }
      }
      const double getMemUsage() { setMemUsage(); return _currentMem; }
   private:
      // Private Functions
      const double checkMem() const {
         ifstream inf("/proc/self/status");
         if (!inf) { Msg(MSG_ERR) << "Cannot get memory usage" << endl; return 0.0; }
         const size_t bufSize = 128;
         char bufStr[bufSize];
         while (!inf.eof()) {
            inf.getline(bufStr, bufSize);
            if (strncmp(bufStr, "VmSize", 6) == 0) {
               long memSizeLong = atol(strchr(bufStr, ' '));
               return ((double)memSizeLong / 1024.0);
            }
         }
         return 0.0;
      }
      const double checkTick() const {
         tms tBuffer; times(&tBuffer);
         return tBuffer.tms_utime;
      }
      void setMemUsage() {
         // Total Memory Usage
         _totalMem = checkMem() - _initMem; _currentMem = 0;
         // Peak Memory Usage
         struct rusage rusage; getrusage(RUSAGE_SELF, &rusage);
         _peakMem = (size_t)(rusage.ru_maxrss * 1024L) / 1048576.0;
         // Current Memory Usage
         unsigned long vsize; long rss; string ignore;
         ifstream input("/proc/self/stat");
         input >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
               >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
               >> ignore >> ignore >> vsize >> rss; input.close();
         _currentMem = vsize / 1048576.0;  //resident_set = rss * page_size_kb;
         /*
         FILE* file = fopen("/proc/self/statm", "r");
         if (file) {
            long memory = 0;
            if (1 == fscanf(file, "%*s%ld", &memory))
               _currentMem = (memory * sysconf(_SC_PAGESIZE)) / 1048576.0;
            fclose(file);
         }
         */
      }
      void setTimeUsage() {
         double thisTick = checkTick();
         _periodUsedTime = (thisTick - _currentTick) / double(MYCLK_TCK);
         _totalUsedTime += _periodUsedTime; _currentTick = thisTick;
      }
      // Memory usage
      double   _initMem;
      double   _peakMem;
      double   _totalMem;
      double   _currentMem;
      // CPU time usage
      double   _currentTick;
      double   _periodUsedTime;
      double   _totalUsedTime;
};

class V3Stat
{
   public : 
      // Constructor and Destructor
      V3Stat(const string& name, V3Stat* const ref = 0) : _statName(name), _refStat(ref) {
         _timeAccum = 0; _isStarted = 0; _numCalled = 0;
      }
      ~V3Stat() {}
      // Inline Member Functions
      inline void start() { assert (!_isStarted); _startTime = clock(); ++_numCalled; _isStarted = 1; }
      inline void end() { assert (_isStarted); _timeAccum += (clock() - _startTime) / CLOCKS_PER_SEC; _isStarted = 0; }
      inline const uint32_t getNumCalled() const { assert (!_isStarted); return _numCalled; }
      inline const double getAccumTime() const { assert (!_isStarted); return _timeAccum; }
      inline const double getAvgTime() const { return getAccumTime() / (double)(getNumCalled()); }
      inline const double getAccumToRef() const { return _refStat ? getAccumTime() / _refStat->getAccumTime() : 0; }
      // Operator Overloads
      friend ostream& operator << (ostream& os, const V3Stat& stat) {
         os << left << fixed << setw(20) << stat._statName << "["
            << "Avg Time = " << setprecision(5) << stat.getAvgTime() << "; "
            << "Total Called = " << setprecision(8) << stat.getNumCalled() << ", "
            << "Time = " << setprecision(6) << stat.getAccumTime();
         if (stat._refStat) os << "  (" << setprecision(5) << (100.00 * stat.getAccumToRef()) << " %)";
         os << "]"; return os;
      }
   private : 
      // Private Members
      const string   _statName;
      V3Stat* const  _refStat;
      double         _timeAccum;
      double         _startTime;
      uint32_t       _isStarted :  1;
      uint32_t       _numCalled : 31;
};

#endif

