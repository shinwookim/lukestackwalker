#include <string>
#include <map>
#include <list>
#include <map>
#include <limits>
#include <vector>
#include "profilersettings.h"
#include "graphviz\gvc.h"

void LogMessage(bool bError, const wchar_t *format, ...);

struct FunctionSample;


struct Caller;

struct Call {
  Call() noexcept {
    m_target = 0;
    m_count = 0;
    m_graphEdge = 0;
  }
  // call graph edge
  Caller *m_target;
  int m_count;
  Agedge_t *m_graphEdge;
};


struct Caller {
  // call graph node
  Caller()  noexcept {
    m_functionSample = 0;
    m_sampleCount = 0;
    m_lineNumber = -1;
    m_graphNode = 0;
    m_ordinalForSaving = 0;
  }
  FunctionSample *m_functionSample;
  std::list<Call>m_callsFromHere;
  int m_sampleCount;
  int m_lineNumber;  
  Agnode_t *m_graphNode;
  int m_ordinalForSaving;
};

struct LineInfo {
  LineInfo() noexcept {
    m_sampleCount = 0;
  }
  int m_sampleCount;
  //std::list<Caller> m_callers;
};

struct FileLineInfo {
  std::wstring m_fileName;
  std::vector<LineInfo> m_lineSamples;
};

#undef max
struct FunctionSample { // data struct for top-of-stack samples
  FunctionSample() {
    m_sampleCount = 0;
    m_maxLine = -1;
    m_minLine = std::numeric_limits<int>::max();
    m_bIgnoredFromDisplay = false;
  }
  std::wstring m_functionName;
  std::wstring m_fileName;
  std::wstring m_moduleName;

  int m_minLine;
  int m_maxLine;

  int m_sampleCount; // times the stack has been sampled while this function is on the top of the call stack
  std::list<Caller> m_callgraph;  // caller info for the whole function - only updated when function is at the top of stack
  bool m_bIgnoredFromDisplay;
};

struct ThreadSampleInfo {
  std::map<std::wstring, FunctionSample> m_functionSamples;
  std::map<std::wstring, FileLineInfo> m_lineSamples;
  std::list<FunctionSample *> m_sortedFunctionSamples;
  int m_totalSamples;
  __int64 m_kernelTimeStart;
  __int64 m_userTimeStart;
  __int64 m_kernelTimeEnd;
  __int64 m_userTimeEnd;
  unsigned m_firstTickCount;
  unsigned m_lastTickCount;
  bool m_bFirstSample;
  bool m_bSelectedForDisplay;
  ThreadSampleInfo() noexcept {
    m_totalSamples = 0;
    m_bSelectedForDisplay = false;
    m_kernelTimeStart = 0;
    m_userTimeStart = 0;
    m_kernelTimeEnd = 0;
    m_userTimeEnd = 0;
    m_firstTickCount = 0;
    m_lastTickCount = 0;
    m_bFirstSample = true;
  }
  int GetRunningTime_ms() noexcept {return (int)(m_lastTickCount - m_firstTickCount);}
  int GetKernelTime_ms() noexcept {return (int)((m_kernelTimeEnd - m_kernelTimeStart) / 10000);}
  int GetUserTime_ms() noexcept  {return (int)((m_userTimeEnd - m_userTimeStart) / 10000);}
  int GetCPUTime_ms() noexcept {return GetKernelTime_ms() + GetUserTime_ms();}
  int GetIgnoredSamples() noexcept;
};




extern bool g_bNewProfileData;

extern std::map<unsigned int, ThreadSampleInfo> g_threadSamples;

bool SampleProcess(ProfilerSettings *settings, ProfilerProgressStatus *status, unsigned int processId);

void SelectThreadForDisplay(unsigned int threadId, bool bSelect = true) noexcept;
void ProduceDisplayData();

extern ThreadSampleInfo *g_displayedSampleInfo;

bool LoadSampleData(const wxString &fn);
bool SaveSampleData(const wxString &fn);