/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>

#include "../include/dll.h"
#include "dds.h"

#include "System.h"
#include "SolveBoard.h"

#define DDS_SYSTEM_THREAD_BASIC 0
#define DDS_SYSTEM_THREAD_WINAPI 1
#define DDS_SYSTEM_THREAD_OPENMP 2
#define DDS_SYSTEM_THREAD_GCD 3
#define DDS_SYSTEM_THREAD_BOOST 4
#define DDS_SYSTEM_THREAD_SIZE 5

const vector<string> DDS_SYSTEM_PLATFORM =
{
  "",
  "Windows",
  "Cygwin",
  "Linux",
  "Apple"
};

const vector<string> DDS_SYSTEM_COMPILER =
{
  "",
  "Microsoft Visual C++",
  "MinGW",
  "GNU g++",
  "clang"
};

const vector<string> DDS_SYSTEM_CONSTRUCTOR =
{
  "",
  "DllMain",
  "Unix-style"
};

const vector<string> DDS_SYSTEM_THREADING =
{
  "None",
  "Windows",
  "OpenMP",
  "GCD",
  "Boost"
};


System::System()
{
  System::Reset();
}


System::~System()
{
}


void System::Reset()
{
  runCat = DDS_SYSTEM_SOLVE;
  numThreads = 1;
  preferredSystem = DDS_SYSTEM_THREAD_BASIC;

  availableSystem.resize(DDS_SYSTEM_THREAD_SIZE);

  availableSystem[DDS_SYSTEM_THREAD_BASIC] = true;

#ifdef _MSC_VER
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_WINAPI] = false;
#endif

#ifdef _OPENMP
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_OPENMP] = false;
#endif

#if (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED))
  availableSystem[DDS_SYSTEM_THREAD_GCD] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_GCD] = false;
#endif

#ifdef BOOST_VERSION
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = true;
#else
  availableSystem[DDS_SYSTEM_THREAD_BOOST] = false;
#endif
  
  InitPtrList.resize(DDS_SYSTEM_THREAD_SIZE);
  InitPtrList[DDS_SYSTEM_THREAD_BASIC] = &System::InitThreadsBasic; 
  InitPtrList[DDS_SYSTEM_THREAD_WINAPI] = &System::InitThreadsWinAPI; 
  InitPtrList[DDS_SYSTEM_THREAD_OPENMP] = &System::InitThreadsOpenMP; 
  InitPtrList[DDS_SYSTEM_THREAD_GCD] = &System::InitThreadsGCD; 
  InitPtrList[DDS_SYSTEM_THREAD_BOOST] = &System::InitThreadsBoost; 
  
  RunPtrList.resize(DDS_SYSTEM_THREAD_SIZE);
  RunPtrList[DDS_SYSTEM_THREAD_BASIC] = &System::RunThreadsBasic; 
  RunPtrList[DDS_SYSTEM_THREAD_WINAPI] = &System::RunThreadsWinAPI; 
  RunPtrList[DDS_SYSTEM_THREAD_OPENMP] = &System::RunThreadsOpenMP; 
  RunPtrList[DDS_SYSTEM_THREAD_GCD] = &System::RunThreadsGCD; 
  RunPtrList[DDS_SYSTEM_THREAD_BOOST] = &System::RunThreadsBoost; 

  // TODO Correct functions
  CallbackSimpleList.resize(DDS_SYSTEM_SIZE);
  CallbackSimpleList[DDS_SYSTEM_SOLVE] = SolveChunkCommon;
  CallbackSimpleList[DDS_SYSTEM_CALC] = SolveChunkCommon;
  CallbackSimpleList[DDS_SYSTEM_PLAY] = SolveChunkCommon;

  CallbackComplexList.resize(DDS_SYSTEM_SIZE);
  CallbackComplexList[DDS_SYSTEM_SOLVE] = SolveChunkDDtableCommon;
  CallbackComplexList[DDS_SYSTEM_CALC] = SolveChunkDDtableCommon;
  CallbackComplexList[DDS_SYSTEM_PLAY] = SolveChunkDDtableCommon;
}


int System::Register(
  const unsigned code,
  const int nThreads)
{
  if (code >= DDS_SYSTEM_SIZE)
    return RETURN_THREAD_MISSING; // Not quite right;

  runCat = code;

  if (nThreads < 1 || nThreads >= MAXNOOFTHREADS)
    return RETURN_THREAD_INDEX;

  numThreads = nThreads;
  return RETURN_NO_FAULT;
}


int System::PreferThreading(const unsigned code)
{
  if (code >= DDS_SYSTEM_THREAD_SIZE)
    return RETURN_THREAD_MISSING;

  if (! availableSystem[code])
    return RETURN_THREAD_MISSING;

  preferredSystem = code;
  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Basic                                  //
//////////////////////////////////////////////////////////////////////

int System::InitThreadsBasic()
{
  return RETURN_NO_FAULT;
}


int System::RunThreadsBasic()
{
  (*fptr)(0);
  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           WinAPI                                 //
//////////////////////////////////////////////////////////////////////

int System::InitThreadsWinAPI()
{
#ifdef _WIN32
  threadIndex = -1;
  for (int k = 0; k < numThreads; k++)
  {
    solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }
#endif

  return RETURN_NO_FAULT;
}


#ifdef _WIN32
DWORD CALLBACK System::WinCallback(void *)
{
  int thid;
  thid = InterlockedIncrement(&threadIndex);
  (*fptr)(thid);

  if (SetEvent(solveAllEvents[thid]) == 0)
    return 0;

  return 1;
}
#endif


int System::RunThreadsWinAPI()
{
#ifdef _WIN32
  
  for (int k = 0; k < numThreads; k++)
  {
    int res = QueueUserWorkItem(&System::WinCallback, &k, 
      WT_EXECUTELONGFUNCTION);
    if (res != 1)
      return res;
  }

  DWORD solveAllWaitResult;
  solveAllWaitResult = WaitForMultipleObjects(
    static_cast<unsigned>(numThreads), solveAllEvents, TRUE, INFINITE);

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < noOfThreads; k++)
    CloseHandle(solveAllEvents[k]);
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           OpenMP                                 //
//////////////////////////////////////////////////////////////////////

int System::InitThreadsOpenMP()
{
  // Added after suggestion by Dirk Willecke.
#ifdef _OPENMP
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(numThreads);
#endif

  return RETURN_NO_FAULT;
}


int System::RunThreadsOpenMP()
{
#ifdef _OPENMP
  int thid;
  #pragma omp parallel default(none) private(thid)
  {
    #pragma omp while schedule(dynamic, chunk)
    while (1)
    {
      thid = omp_get_thread_num();
      (*fptr)(thid);
    }
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                            GCD                                   //
//////////////////////////////////////////////////////////////////////

int System::InitThreadsGCD()
{
  return RETURN_NO_FAULT;
}


int System::RunThreadsGCD()
{
#if (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED))
  dispatch_apply(static_cast<size_t>(noOfThreads),
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
    ^(size_t t)
  {
    thid = omp_get_thread_num();
    (*fptr)(thid);
  }
#endif

  return RETURN_NO_FAULT;
}


//////////////////////////////////////////////////////////////////////
//                           Boost                                  //
//////////////////////////////////////////////////////////////////////

int System::InitThreadsBoost()
{
#ifdef BOOST_VERSION
  threads.resize(static_cast<unsigned>(numThreads));
#endif
  return RETURN_NO_FAULT;
}


int System::RunThreadsBoost()
{
#ifdef BOOST_VERSION
  const unsigned nu = static_cast<unsigned>(numThreads);
  for (unsigned k = 0; k < nu; k++)
    threads[k] = new thread(fptr, k);

  for (unsigned k = 0; k < nu; k++)
  {
    threads[k]->join();
    delete threads[k];
  }
#endif

  return RETURN_NO_FAULT;
}


int System::InitThreads()
{
  return (this->*InitPtrList[preferredSystem])();
}


int System::RunThreads(const int chunkSize)
{
  // TODO Add timing on the caller side, not here in System

  fptr = (chunkSize == 1 ? 
    CallbackSimpleList[runCat] : CallbackComplexList[runCat]);

  return (this->*RunPtrList[preferredSystem])();
}


string System::str(DDSInfo * info) const
{
  info->major = DDS_VERSION / 10000;
  info->minor = (DDS_VERSION - info->major * 10000) / 100;
  info->patch = DDS_VERSION % 100;

  sprintf(info->versionString, "%d.%d.%d",
    info->major, info->minor, info->patch);

  info->system = 0;
  info->compiler = 0;
  info->constructor = 0;
  info->threading = 0;
  info->noOfThreads = numThreads;

#if defined(_WIN32)
  info->system = 1;
#elif defined(__CYGWIN__)
  info->system = 2;
#elif defined(__linux)
  info->system = 3;
#elif defined(__APPLE__)
  info->system = 4;
#endif

  stringstream ss;
  ss << "DDS DLL\n-------\n";
  ss << left << setw(13) << "System" <<
    setw(20) << right << 
      DDS_SYSTEM_PLATFORM[static_cast<unsigned>(info->system)] << "\n";

#if defined(_MSC_VER)
  info->compiler = 1;
#elif defined(__MINGW32__)
  info->compiler = 2;
#elif defined(__GNUC__)
  info->compiler = 3;
#elif defined(__clang__)
  info->compiler = 4;
#endif
  ss << left << setw(13) << "Compiler" <<
    setw(20) << right << 
      DDS_SYSTEM_COMPILER[static_cast<unsigned>(info->compiler)] << "\n";

#if defined(USES_DLLMAIN)
  info->constructor = 1;
#elif defined(USES_CONSTRUCTOR)
  info->constructor = 2;
#endif
  ss << left << setw(13) << "Constructor" <<
    setw(20) << right << 
      DDS_SYSTEM_CONSTRUCTOR[static_cast<unsigned>(info->constructor)] << 
      "\n";

  ss << left << setw(9) << "Threading";
  string sy = "";
  for (unsigned k = 0; k < DDS_SYSTEM_THREAD_SIZE; k++)
  {
    if (availableSystem[k])
    {
      sy += " " + DDS_SYSTEM_THREADING[k];
      if (k == preferredSystem)
        sy += "(*)";
    }
  }
  ss << setw(24) << right << sy << "\n";

  strcpy(info->systemString, ss.str().c_str());
  return ss.str();
}

