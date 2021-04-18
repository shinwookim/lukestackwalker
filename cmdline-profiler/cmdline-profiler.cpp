// cmdline-profiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include "..\sampledata.h"
#include "..\profilersettings.h"


void LogMessage(bool bError, const wchar_t *format, ...) {
  wchar_t buffer[10240];
  va_list args;
  va_start (args, format);
  _vsnwprintf_s (buffer, _countof(buffer), _TRUNCATE, format, args);  
  va_end (args);  
  if (bError) {    
    fwprintf(stderr, buffer);
    if (buffer[wcslen(buffer)-1] != '\n')
      fwprintf(stderr, L"\n");
      fflush(stderr);
  } else {
    fwprintf(stdout, buffer);
    if (buffer[wcslen(buffer)-1] != '\n')
      fwprintf(stdout, L"\n");
      fflush(stdout);
  }
  
}


static HANDLE s_hMapFile = 0;
static LPCTSTR s_pBuf = 0;


ProfilerProgressStatus *OpenSharedMemory(const wchar_t *name) {
       
  s_hMapFile = OpenFileMapping(
                   FILE_MAP_ALL_ACCESS,   // read/write access
                   FALSE,                 // do not inherit the name
                   name);               // name of mapping object 

   if (s_hMapFile == NULL) { 
      _tprintf(TEXT("Could not open file mapping object (%d).\n"),  GetLastError());
      return 0;
   } 

   s_pBuf = (LPTSTR) MapViewOfFile(s_hMapFile, // handle to map object
               FILE_MAP_ALL_ACCESS,  // read/write permission
               0,                    
               0,                    
               sizeof(ProfilerProgressStatus));                   

   if (s_pBuf == NULL) { 
     _tprintf(TEXT("Could not map view of file (%d).\n"),  GetLastError()); 
     CloseHandle(s_hMapFile);
     return 0;
   }

   return (ProfilerProgressStatus *)s_pBuf; 
}

void CloseSharedMemory() {
  UnmapViewOfFile(s_pBuf);
  CloseHandle(s_hMapFile);
}


bool Profile(const wchar_t *settingsName, const wchar_t *resultFileName, const wchar_t *sharedMemFile, int pid) {
  ProfilerSettings ps;
  if (!ps.Load(settingsName)) {
    printf("Loading settings file [%S] failed.\n", settingsName);
    return false;
  }
  ProfilerProgressStatus *status;
  ProfilerProgressStatus unusedStatus;
  if (sharedMemFile) {
    status = OpenSharedMemory(sharedMemFile);    
  } else {
    status = &unusedStatus;
  }  
  if (!SampleProcess(&ps, status, pid)) {
    printf("Profiling failed\n");
    return false;
  }  
  if (!SaveSampleData(resultFileName)) {
    printf("Saving sample data to [%S] failed.\n", resultFileName);
    status->bFinishedSampling = true;
    return false;
  }
  status->bFinishedSampling = true;
  if (sharedMemFile) {
    CloseSharedMemory();
  }  
  return true;
}



int _tmain(int argc, _TCHAR* argv[])
{
	wchar_t *inputfile = 0;
	wchar_t *outputfile = 0;
  wchar_t *sharedMemFile = 0;
  int pid = 0;
	for (int i = 1; i < argc; i++) {
		if (!_wcsicmp(argv[i], L"-in")) {
			if (argc > i+1) {
			  inputfile = argv[i+i];
			  i++;
			  continue;
			}
		}
		if (!_wcsicmp(argv[i], L"-out")) {
			if (argc > i+1) {
			  outputfile = argv[i+1];
			  i++;
			  continue;
			}
		}
    if (!_wcsicmp(argv[i], L"-shm")) {
			if (argc > i+1) {
			  sharedMemFile = argv[i+1];
			  i++;
			  continue;
			}
		}
    if (!_wcsicmp(argv[i], L"-pid")) {
			if (argc > i+1) {
			  pid = _wtoi(argv[i+1]);
			  i++;
			  continue;
			}
		}
	}
	if (!inputfile) {
		printf("No input file (project settings, *.lsp) defined use:'-in <file>.\n");		
	}
  
  if (!outputfile) {
	  printf("No output file (profile data, *.lsd) defined use:'-out <file>.\n");		
	}

  if (!outputfile || !inputfile) {
    return -1;
  }

	
	Profile(inputfile, outputfile, sharedMemFile, pid);
	return 0;
}



