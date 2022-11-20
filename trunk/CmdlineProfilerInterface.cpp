#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "profilersettings.h"
#include "CmdlineProfilerInterface.h"
#include <wx/process.h>
#include <wx/txtstrm.h>
#include "sampledata.h"
#include <wx/filename.h>
#include <filesystem>

static  HANDLE s_hMapFile = INVALID_HANDLE_VALUE;
static char *s_pBuf = 0;
static wxString s_sharedMemFileName;



ProfilerProgressStatus *PrepareStatusInSharedMemory() {
  if (s_pBuf != 0) {
    CloseSharedMemory();
  }

  const DWORD id = GetCurrentProcessId();
  wchar_t buf[256];
  swprintf(buf, _countof(buf), L"Local\\LukeStackWalkerStatus-%d", id);
  s_sharedMemFileName = buf;

   s_hMapFile = CreateFileMappingW(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD) 
                 sizeof(ProfilerProgressStatus),   // maximum object size (low-order DWORD)  
                 buf);                 // name of mapping object
 
   if (s_hMapFile == NULL)  { 
      _tprintf(TEXT("Could not create file mapping object (%d).\n"), 
             GetLastError());
      return 0;
   }
   s_pBuf = (char *) MapViewOfFile(s_hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,                   
                        0,                   
                        sizeof(ProfilerProgressStatus));           
 
   if (s_pBuf == NULL)  { 
      _tprintf(TEXT("Could not map view of file (%d).\n"), 
             GetLastError()); 
	    CloseHandle(s_hMapFile);
      return 0;
   }
   memset(s_pBuf, 0, sizeof(ProfilerProgressStatus));
   return (ProfilerProgressStatus *)s_pBuf;
};

class MyPipedProcess : public wxProcess {
public:
    bool m_bRunning;
    MyPipedProcess() : wxProcess(0) {
       Redirect();
       m_bRunning = false;
    }

    virtual void OnTerminate(int /*pid*/, int /*status*/) noexcept override {
      m_bRunning = false;
    }

    virtual bool HasInput() {
       bool hasInput = false;

      if ( IsInputAvailable() ) {
        wxTextInputStream tis(*GetInputStream());

        // this assumes that the output is always line buffered
        wxString msg;
        msg << tis.ReadLine();
        LogMessage(false, msg.wc_str());
        hasInput = true;
      }
      if ( IsErrorAvailable() ) {
          wxTextInputStream tis(*GetErrorStream());
          // this assumes that the output is always line buffered
          wxString msg;
          msg << tis.ReadLine();
          LogMessage(true, msg.wc_str());
          hasInput = true;
      }
      return hasInput;

    }
};


static MyPipedProcess s_process;
static wxString s_resultsName;
static wxString s_settingsName;


bool SampleWithCommandLineProfiler(ProfilerSettings *settings, unsigned int processId) {
  wchar_t exeFileName[MAX_PATH];
  GetModuleFileName(0, exeFileName, _countof(exeFileName));
  wxFileName exeName(exeFileName);
  wxString exeDir = exeName.GetPath(wxPATH_GET_VOLUME|| wxPATH_GET_SEPARATOR);
  const DWORD id = GetCurrentProcessId();
  wchar_t buf[2048];
  swprintf(buf, _countof(buf), L"%d-tmp", id);
  // save settings to temp file
  wxString subdir = L"cmdline-profiler\\";
  s_settingsName = exeDir + subdir + wxString(buf) + wxString(L".lsp");
  s_resultsName = exeDir + subdir + wxString(buf) + wxString(L".lsd");
#ifdef _DEBUG
  wxString cmdLineProfiler = exeDir + subdir + L"cmdline-profilerD.exe";
#else
  wxString cmdLineProfiler = exeDir + subdir + L"cmdline-profiler.exe";
#endif
  _wunlink(s_resultsName.wc_str()); 
  _wunlink(s_settingsName.wc_str());
  Sleep(200);
  if (!settings->SaveAs(s_settingsName.wc_str())) {
    LogMessage(true, L"Failed to save settings file [%s] for command line profiler!", s_settingsName.wc_str());
    return false;
  }

  swprintf(buf, _countof(buf), L"%s -in \"%s\" -out \"%s\" -shm %s -pid %d", cmdLineProfiler.wc_str(), s_settingsName.wc_str(), s_resultsName.wc_str(), s_sharedMemFileName.wc_str(), processId);
  long pid = wxExecute(buf, wxEXEC_ASYNC, &s_process);
  if (pid) {
    s_process.m_bRunning = true;
  } else {
    LogMessage(true, L"Failed to execute command line profiler [%s]!", buf);
  }
  return !!pid;
}


bool HandleCommandLineProfilerOutput() {  
  while (s_process.HasInput()) {};      
  return s_process.m_bRunning;
}

void CloseSharedMemory() noexcept {
  UnmapViewOfFile(s_pBuf);
  CloseHandle(s_hMapFile);
  s_pBuf = 0;
}



bool FinishCmdLineProfiling() {
  wxBusyCursor wait;
  for (int i = 0; i < 30; i++) {
    if (!s_process.m_bRunning)
      break;
    if (!wxProcess::Exists(s_process.GetPid()))
      break;
    HandleCommandLineProfilerOutput();
    Sleep(500);
  }
  for (int i = 0; i < 10; i++) {
    std::error_code ec;
    if (std::filesystem::exists(s_resultsName.wc_str(), ec))
      break;
    Sleep(500);
  }

  bool bRet = true;
  if (!LoadSampleData(s_resultsName)) {
   LogMessage(true, L"Failed to load profile data from command line profiler!");
   bRet = false;
  } else {
    _wunlink(s_resultsName.wc_str());
  }
  _wunlink(s_settingsName.wc_str());
  return bRet;
}
