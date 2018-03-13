/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SYSTEM_H
#define DDS_SYSTEM_H

/*
   This class encapsulates all(?) the system-dependent stuff.
 */

#include <string>
#include <vector>

// Boost: Disable some header warnings.

#ifdef BOOST_VERSION
  #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4061 4191 4365 4571 4619 4625 4626 5026 5027 50    31)
  #endif

  #include <boost/thread.hpp>

  #ifdef _MSC_VER
    #pragma warning(pop)
  #endif
#endif

#include "dds.h"

using namespace std;

#define DDS_SYSTEM_SOLVE 0
#define DDS_SYSTEM_CALC 1
#define DDS_SYSTEM_PLAY 2
#define DDS_SYSTEM_SIZE 3


class System
{
  private:

    unsigned runCat; // SOLVE / CALC / PLAY

    int numThreads;

    unsigned preferredSystem;

    vector<bool> availableSystem;

    typedef void (*fptrType)(const int thid);
    vector<fptrType> CallbackSimpleList;
    vector<fptrType> CallbackComplexList;

    typedef int (System::*InitPtr)();
    vector<InitPtr> InitPtrList;

    typedef int (System::*RunPtr)();
    vector<RunPtr> RunPtrList;

    fptrType fptr;

#ifdef BOOST_VERSION
    using boost::thread;
    vector<thread *> threads;
#endif

#ifdef _MSC_VER
    HANDLE solveAllEvents[MAXNOOFTHREADS];
    LONG threadIndex;
    DWORD CALLBACK WinCallback(void *);
#endif

    int InitThreadsBasic();
    int InitThreadsBoost();
    int InitThreadsOpenMP();
    int InitThreadsGCD();
    int InitThreadsWinAPI();
  
    int RunThreadsBasic();
    int RunThreadsBoost();
    int RunThreadsOpenMP();
    int RunThreadsGCD();
    int RunThreadsWinAPI();


  public:
    System();

    ~System();

    void Reset();

    int Register(
      const unsigned code,
      const int noOfThreads = 1);

    int PreferThreading(const unsigned code);

    int InitThreads();

    int RunThreads(const int chunkSize);

    string str(DDSInfo * info) const;
};

#endif
