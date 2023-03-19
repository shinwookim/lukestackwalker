/**********************************************************************
 * 
 * StackWalker.cpp
 *
 *
 * History:
 *  2005-07-27   v1    - First public release on http://www.codeproject.com/
 *                       http://www.codeproject.com/threads/StackWalker.asp
 *  2005-07-28   v2    - Changed the params of the constructor and ShowCallstack
 *                       (to simplify the usage)
 *  2005-08-01   v3    - Changed to use 'CONTEXT_FULL' instead of CONTEXT_ALL 
 *                       (should also be enough)
 *                     - Changed to compile correctly with the PSDK of VC7.0
 *                       (GetFileVersionInfoSizeA and GetFileVersionInfoA is wrongly defined:
 *                        it uses LPSTR instead of LPCSTR as first paremeter)
 *                     - Added declarations to support VC5/6 without using 'dbghelp.h'
 *                     - Added a 'pUserData' member to the ShowCallstack function and the 
 *                       PReadProcessMemoryRoutine declaration (to pass some user-defined data, 
 *                       which can be used in the readMemoryFunction-callback)
 *  2005-08-02   v4    - OnSymInit now also outputs the OS-Version by default
 *                     - Added example for doing an exception-callstack-walking in main.cpp
 *                       (thanks to owillebo: http://www.codeproject.com/script/profile/whos_who.asp?id=536268)
 *  2005-08-05   v5    - Removed most Lint (http://www.gimpel.com/) errors... thanks to Okko Willeboordse!
 *
 **********************************************************************/

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


#include <windows.h>
#include "dbghelp.h"

#include <tchar.h>
#include <stdio.h>
#pragma comment(lib, "version.lib")  // for "VerQueryValue"

#include "StackWalker.h"
#include <set>
#include <string>
#include <tlhelp32.h>
#include <wx/string.h>
#include <Psapi.h>

// Normally it should be enough to use 'CONTEXT_FULL' (better would be 'CONTEXT_ALL')
#define USED_CONTEXT_FLAGS CONTEXT_FULL


class StackWalkerInternal
{
public:
  std::set<DWORD64> m_unknownAddresses;
  std::set<std::wstring> m_alreadyLoadedModules;
  StackWalkerInternal(StackWalker *parent, HANDLE hProcess)
  {
    m_parent = parent;
    m_hProcess = hProcess;
  }
  ~StackWalkerInternal()
  {
    SymCleanup(m_hProcess);
    m_parent = NULL;
  }
  BOOL Init(const wchar_t *szSymPath)
  {
    if (m_parent == NULL)
      return FALSE;

  
    // SymInitialize
    m_szSymPath = szSymPath;
    if (SymInitializeW(m_hProcess, m_szSymPath.c_str(), FALSE) == FALSE)
      this->m_parent->OnDbgHelpErr("SymInitializeW", GetLastError(), 0);
      
    DWORD symOptions = SymGetOptions();  // SymGetOptions
    symOptions |= SYMOPT_LOAD_LINES;
    symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
    //symOptions |= SYMOPT_NO_PROMPTS;
    // SymSetOptions
    symOptions = SymSetOptions(symOptions);

    wchar_t buf[StackWalker::STACKWALK_MAX_NAMELEN] = {0};
    
    if (SymGetSearchPathW(m_hProcess, buf, StackWalker::STACKWALK_MAX_NAMELEN) == FALSE)
      this->m_parent->OnDbgHelpErr("SymGetSearchPath", GetLastError(), 0);
    
    wchar_t szUserName[1024] = {0};
    DWORD dwSize = 1024;
    GetUserNameW(szUserName, &dwSize);
    this->m_parent->OnSymInit(buf, symOptions, szUserName);

    return TRUE;
  }

  StackWalker *m_parent;

  HANDLE m_hProcess;
  std::wstring m_szSymPath;  
  
 


private:
  // **************************************** ToolHelp32 ************************

  BOOL GetModuleListTH32(HANDLE hProcess, DWORD pid)
  {
    // CreateToolhelp32Snapshot()
    typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
    // Module32First()
    typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);
    // Module32Next()
    typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);

    // try both dlls...
    const TCHAR * const dllname[] = { _T("kernel32.dll"), _T("tlhelp32.dll") };
    HINSTANCE hToolhelp = NULL;
    tCT32S pCT32S = NULL;
    tM32F pM32F = NULL;
    tM32N pM32N = NULL;

    HANDLE hSnap;
    MODULEENTRY32W me = { 0 };
    me.dwSize = sizeof(me);
    BOOL keepGoing;
    size_t i;

    for (i = 0; i<(sizeof(dllname) / sizeof(dllname[0])); i++ )
    {
      hToolhelp = LoadLibrary( dllname[i] );
      if (hToolhelp == NULL)
        continue;
      pCT32S = (tCT32S) GetProcAddress(hToolhelp, "CreateToolhelp32Snapshot");
      pM32F = (tM32F) GetProcAddress(hToolhelp, "Module32FirstW");
      pM32N = (tM32N) GetProcAddress(hToolhelp, "Module32NextW");
      if ( (pCT32S != NULL) && (pM32F != NULL) && (pM32N != NULL) )
        break; // found the functions!
      FreeLibrary(hToolhelp);
      hToolhelp = NULL;
    }

    if (hToolhelp == NULL)
      return FALSE;

    hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
    if (hSnap == (HANDLE) -1)
      return FALSE;

    keepGoing = !!pM32F( hSnap, &me );
    int moduleCount = 0;
    while (keepGoing) {      
      moduleCount++;
      keepGoing = !!pM32N( hSnap, &me );
    }

    keepGoing = !!pM32F( hSnap, &me );
    int cnt = 0;
    while (keepGoing)
    {
      DWORD ret = 0;
      if (m_alreadyLoadedModules.find(me.szExePath) == m_alreadyLoadedModules.end()) {
        ret = this->LoadModule(hProcess, me.szExePath, me.szModule, (DWORD64) me.modBaseAddr, me.modBaseSize, moduleCount, cnt+1);
        m_alreadyLoadedModules.insert(me.szExePath);
        if (ret == ERROR_CANCELLED)
          break;
      }      
      cnt++;
      keepGoing = !!pM32N( hSnap, &me );
    }
    CloseHandle(hSnap);
    FreeLibrary(hToolhelp);
    if (cnt <= 0)
      return FALSE;
    return TRUE;
  }  // GetModuleListTH32

  // **************************************** PSAPI ************************
  /*typedef struct _MODULEINFO {
      LPVOID lpBaseOfDll;
      DWORD SizeOfImage;
      LPVOID EntryPoint;
  } MODULEINFO, *LPMODULEINFO;
  */
  BOOL GetModuleListPSAPI(HANDLE hProcess)
  {
    DWORD i;
    //ModuleEntry e;
    DWORD cbNeeded;
    MODULEINFO mi;
    HMODULE *hMods = 0;
    const SIZE_T TTBUFLEN = 8096;
    wchar_t tt[TTBUFLEN] = { 0 };
    wchar_t tt2[TTBUFLEN] = { 0 };

    int cnt = 0;

    hMods = (HMODULE*) malloc(sizeof(HMODULE) * (TTBUFLEN / sizeof HMODULE));
    if (hMods == NULL)
      goto cleanup;

    if ( !EnumProcessModules( hProcess, hMods, TTBUFLEN, &cbNeeded ) ) {
      //_ftprintf(fLogFile, _T("%lu: EPM failed, GetLastError = %lu\n"), g_dwShowCount, gle );
      goto cleanup;
    }

    if ( cbNeeded > TTBUFLEN ) {
      //_ftprintf(fLogFile, _T("%lu: More than %lu module handles. Huh?\n"), g_dwShowCount, lenof( hMods ) );
      goto cleanup;
    }

    for ( i = 0; i < cbNeeded / sizeof hMods[0]; i++ )
    {
      // base address, size
      GetModuleInformation(hProcess, hMods[i], &mi, sizeof mi );
      // image file name
      tt[0] = 0;
      GetModuleFileNameExW(hProcess, hMods[i], tt, TTBUFLEN );
      // module name
      tt2[0] = 0;
      GetModuleBaseName(hProcess, hMods[i], tt2, TTBUFLEN );

      DWORD dwRes = this->LoadModule(hProcess, tt, tt2, (DWORD64) mi.lpBaseOfDll, mi.SizeOfImage, cbNeeded / sizeof hMods[0], i);
      if (dwRes == ERROR_CANCELLED)
        break;
      if (dwRes != ERROR_SUCCESS)
        this->m_parent->OnDbgHelpErr("LoadModule", dwRes, 0);
      cnt++;
    }

  cleanup:
    if (hMods != NULL) free(hMods);

    return cnt != 0;
  }  // GetModuleListPSAPI

  DWORD LoadModule(HANDLE hProcess, const wchar_t *img, const wchar_t *mod, DWORD64 baseAddr, DWORD size, int totalModules, int currentModule)
  {
    DWORD result = ERROR_SUCCESS;
    
    if (SymLoadModuleExW(hProcess, 0, img, mod, baseAddr, size, nullptr, 0) == 0)
      result = GetLastError();
    
    ULONGLONG fileVersion = 0;
    if ( (m_parent != NULL) && (img != NULL) )
    {
      // try to retrive the file-version:
      if ( (this->m_parent->m_options & StackWalker::RetrieveFileVersion) != 0)
      {
        VS_FIXEDFILEINFO *fInfo = NULL;
        DWORD dwHandle;
        DWORD dwSize = GetFileVersionInfoSizeW(img, &dwHandle);
        if (dwSize > 0)
        {
          LPVOID vData = malloc(dwSize);
          if (vData != NULL)
          {
            if (GetFileVersionInfoW(img, dwHandle, dwSize, vData) != 0)
            {
              UINT len;
              const TCHAR szSubBlock[] = _T("\\");
              if (VerQueryValue(vData, szSubBlock, (LPVOID*) &fInfo, &len) == 0)
                fInfo = NULL;
              else
              {
                fileVersion = ((ULONGLONG)fInfo->dwFileVersionLS) + ((ULONGLONG)fInfo->dwFileVersionMS << 32);
              }
            }
            free(vData);
          }
        }
      }

      // Retrive some additional-infos about the module
      IMAGEHLP_MODULEW64 Module = { 0 };
      const wchar_t *szSymType = L"-unknown-";      
      Module.LoadedPdbName[0] = 0;
      if (this->GetModuleInfo(hProcess, baseAddr, &Module) != FALSE)
      {
        switch(Module.SymType)
        {
          case SymNone:
            szSymType = L"-nosymbols-";
            break;
          case SymCoff:
            szSymType = L"COFF";
            break;
          case SymCv:
            szSymType = L"CV";
            break;
          case SymPdb:
            szSymType = L"PDB";
            break;
          case SymExport:
            szSymType = L"-exported-";
            break;
          case SymDeferred:
            szSymType = L"-deferred-";
            break;
          case SymSym:
            szSymType = L"SYM";
            break;
          case 8: //SymVirtual:
            szSymType = L"Virtual";
            break;
          case 9: // SymDia:
            szSymType = L"DIA";
            break;
        }
      }
      if (!this->m_parent->OnLoadModule(img, mod, baseAddr, size, result, szSymType, Module.LoadedPdbName, fileVersion, totalModules, currentModule)) {
        result = ERROR_CANCELLED;
      }
    }
    return result;
  }
public:
  BOOL LoadModules(HANDLE hProcess, DWORD dwProcessId)
  {
    // first try toolhelp32
    if (GetModuleListTH32(hProcess, dwProcessId))
      return true;
    // then try psapi
    return GetModuleListPSAPI(hProcess);
  }


  BOOL GetModuleInfo(HANDLE hProcess, DWORD64 baseAddr, IMAGEHLP_MODULEW64 *pModuleInfo)
  {
    pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULEW64);
    void *pData = malloc(4096); // reserve enough memory, so the bug in v6.3.5.1 does not lead to memory-overwrites...
    if (pData == NULL)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    memcpy(pData, pModuleInfo, sizeof(IMAGEHLP_MODULEW64));
    if (SymGetModuleInfoW64(hProcess, baseAddr, (IMAGEHLP_MODULEW64*) pData) != FALSE)
    {
      // only copy as much memory as is reserved...
      memcpy(pModuleInfo, pData, sizeof(IMAGEHLP_MODULEW64));
      pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULEW64);
      free(pData);
      return TRUE;
    }
    free(pData);
    SetLastError(ERROR_DLL_INIT_FAILED);
    return FALSE;
  }
};

// #############################################################
StackWalker::StackWalker(DWORD dwProcessId, HANDLE hProcess, LPCWSTR szSymPath, LPCWSTR szSymServerCachePath)
{
  m_options = OptionsAll;
  m_modulesLoaded = FALSE;
  m_hProcess = hProcess;
  m_sw = new StackWalkerInternal(this, this->m_hProcess);
  m_dwProcessId = dwProcessId;
  if (szSymServerCachePath) {
    m_symbolServerCachePath = szSymServerCachePath;
  }
  if (szSymPath != NULL)
  {
    m_szSymPath = szSymPath;
    m_options |= SymBuildPath;
  }
}

StackWalker::StackWalker(int options, LPCWSTR szSymPath, LPCWSTR szSymServerCachePath, DWORD dwProcessId, HANDLE hProcess)
{
  m_options = options;
  m_modulesLoaded = FALSE;
  m_hProcess = hProcess;
  m_sw = new StackWalkerInternal(this, this->m_hProcess);
  m_dwProcessId = dwProcessId;
  if (szSymPath != NULL) {
    m_szSymPath = szSymPath;
    m_options |= SymBuildPath;
  } 
  if (szSymServerCachePath) {
    m_symbolServerCachePath = szSymServerCachePath;
  }
}

StackWalker::~StackWalker() {
  if (m_sw != NULL)
    delete m_sw;
  m_sw = NULL;
}

BOOL StackWalker::LoadModules()
{
  if (m_sw == NULL)
  {
    SetLastError(ERROR_DLL_INIT_FAILED);
    return FALSE;
  }
  if (m_modulesLoaded != FALSE) {
    return m_sw->LoadModules(this->m_hProcess, this->m_dwProcessId);    
  }

  // Build the sym-path:
  std::wstring szSymPath;
  if ( (m_options & SymBuildPath) != 0)
  {
   
    // Now first add the (optional) provided sympath:
    if (!m_szSymPath.empty())
    {
      szSymPath = m_szSymPath;
      szSymPath += L";";
    }

    szSymPath += L".;";

    const size_t nTempLen = 1024;
    wchar_t szTemp[nTempLen];
    // Now add the current directory:
    if (GetCurrentDirectoryW(nTempLen, szTemp) > 0)
    {
      szTemp[nTempLen-1] = 0;
      szSymPath += szTemp;
      szSymPath += L";";
    }

    // Now add the path for the main-module:
    if (GetModuleFileNameW(NULL, szTemp, nTempLen) > 0)
    {
      szTemp[nTempLen-1] = 0;
      for (wchar_t *p = (szTemp+wcslen(szTemp)-1); p >= szTemp; --p)
      {
        // locate the rightmost path separator
        if ( (*p == L'\\') || (*p == L'/') || (*p == L':') )
        {
          *p = 0;
          break;
        }
      }  // for (search for path separator...)
      if (wcslen(szTemp) > 0)
      {
        szSymPath += szTemp;
        szSymPath += L";";
      }
    }
    if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", szTemp, nTempLen) > 0)
    {
      szTemp[nTempLen-1] = 0;
      szSymPath += szTemp;
      szSymPath += L";";
    }
    if (GetEnvironmentVariableW(L"_NT_ALTERNATE_SYMBOL_PATH", szTemp, nTempLen) > 0)
    {
      szTemp[nTempLen-1] = 0;
      szSymPath+= szTemp;
      szSymPath+= L";";
    }
    if (GetEnvironmentVariableW(L"SYSTEMROOT", szTemp, nTempLen) > 0)
    {
      szTemp[nTempLen-1] = 0;
      szSymPath+= szTemp;
      szSymPath += L";";
      // also add the "system32"-directory:
      szSymPath += szTemp;
      szSymPath += L"\\system32;";
    }

    if ( (m_options & SymBuildPath) != 0) {
      if (m_options & SymUseSymSrv) {        
        szSymPath+= L"SRV*";        
        szSymPath += m_symbolServerCachePath;
        szSymPath += L"*http://msdl.microsoft.com/download/symbols;";        
      } else {
        szSymPath += m_symbolServerCachePath;
        szSymPath += L";";        
      }
    }
  }

  // First Init the whole stuff...
  BOOL bRet = m_sw->Init(szSymPath.c_str());
  if (bRet == FALSE)
  {
    OnDbgHelpErr("Error while initializing dbghelp.dll", 0, 0);
    SetLastError(ERROR_DLL_INIT_FAILED);
    return FALSE;
  }

  bRet = m_sw->LoadModules(this->m_hProcess, this->m_dwProcessId);
  if (bRet != FALSE)
    m_modulesLoaded = TRUE;
  return bRet;
}


// The following is used to pass the "userData"-Pointer to the user-provided readMemoryFunction
// This has to be done due to a problem with the "hProcess"-parameter in x64...
// Because this class is in no case multi-threading-enabled (because of the limitations 
// of dbghelp.dll) it is "safe" to use a static-variable
static StackWalker::PReadProcessMemoryRoutine s_readMemoryFunction = NULL;
static LPVOID s_readMemoryFunction_UserData = NULL;

BOOL StackWalker::ShowCallstack(HANDLE hThread, int maxDepth, const CONTEXT *context, PReadProcessMemoryRoutine readMemoryFunction, LPVOID pUserData)
{
  CONTEXT c;
  CallstackEntry csEntry = { 0 };
  PSYMBOL_INFOW pSym = NULL;
  IMAGEHLP_MODULEW64 Module;
  IMAGEHLP_LINEW64 Line;
  int frameNum;

  if (m_modulesLoaded == FALSE)
    this->LoadModules();  // ignore the result...


  s_readMemoryFunction = readMemoryFunction;
  s_readMemoryFunction_UserData = pUserData;

  if (context == NULL)
  {
    // If no context is provided, capture the context
    /*if (hThread == GetCurrentThread())
    {
      GET_CURRENT_CONTEXT(c, USED_CONTEXT_FLAGS);
    }
    else*/
    {
      SuspendThread(hThread);
      memset(&c, 0, sizeof(CONTEXT));
      c.ContextFlags = USED_CONTEXT_FLAGS;
      if (!GetThreadContext(hThread, &c)) {
        ResumeThread(hThread);
        return FALSE;
      }
    }
  }
  else {
    c = *context;
  }
  // init STACKFRAME for first call
  STACKFRAME64 s; // in/out stackframe
  memset(&s, 0, sizeof(s));
  DWORD imageType;
#ifdef _M_IX86
  // normally, call ImageNtHeader() and use machine info from PE header
  imageType = IMAGE_FILE_MACHINE_I386;
  s.AddrPC.Offset = c.Eip;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.Ebp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.Esp;
  s.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  imageType = IMAGE_FILE_MACHINE_AMD64;
  s.AddrPC.Offset = c.Rip;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.Rsp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.Rsp;
  s.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

  if (m_symbol_info_buffer.size() < sizeof(SYMBOL_INFOW) + 4 * STACKWALK_MAX_NAMELEN) {
    m_symbol_info_buffer.resize(sizeof(SYMBOL_INFOW) + 4 * STACKWALK_MAX_NAMELEN);
  }
  pSym = (SYMBOL_INFOW *) &m_symbol_info_buffer[0];
  memset(pSym, 0, sizeof(SYMBOL_INFOW) + 2);
  pSym->SizeOfStruct = sizeof(SYMBOL_INFOW);
  pSym->MaxNameLen = 2*STACKWALK_MAX_NAMELEN-1;

  memset(&Line, 0, sizeof(Line));
  Line.SizeOfStruct = sizeof(Line);

  memset(&Module, 0, sizeof(Module));
  Module.SizeOfStruct = sizeof(Module);

  for (frameNum = 0; (!maxDepth || (frameNum < maxDepth)); ++frameNum )
  {
    // get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
    // if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
    // assume that either you are done, or that the stack is so hosed that the next
    // deeper frame could not be found.
    // CONTEXT need not to be suplied if imageTyp is IMAGE_FILE_MACHINE_I386!
    if ( !StackWalk64(imageType, this->m_hProcess, hThread, &s, &c, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, NULL) )
    {
      this->OnDbgHelpErr("StackWalk64", GetLastError(), s.AddrPC.Offset);
      break;
    }

    csEntry.offset = s.AddrPC.Offset;
    csEntry.name[0] = 0;
    csEntry.undName[0] = 0;
    csEntry.undFullName[0] = 0;
    csEntry.offsetFromSmybol = 0;
    csEntry.offsetFromLine = 0;
    csEntry.lineFileName[0] = 0;
    csEntry.lineNumber = 0;
    csEntry.loadedImageName[0] = 0;
    csEntry.moduleName[0] = 0;
    if (s.AddrPC.Offset == s.AddrReturn.Offset)
    {
      this->OnDbgHelpErr("StackWalk64-Endless-Callstack!", 0, s.AddrPC.Offset);
      break;
    }
    if (s.AddrPC.Offset != 0)
    {
      // we seem to have a valid PC
      // show procedure info (SymGetSymFromAddr64())
      if (SymFromAddrW(this->m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromSmybol), pSym) != FALSE)
      {
        wcscpy_s(csEntry.name, pSym->Name);
        auto it = m_knownUndecorations.find(csEntry.name);
        if (it == m_knownUndecorations.end()) {
          UnDecorateSymbolNameW(csEntry.name, csEntry.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY);
          UnDecorateSymbolNameW(csEntry.name, csEntry.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE);
          m_knownUndecorations[csEntry.name] = std::pair<std::wstring, std::wstring> (csEntry.undName, csEntry.undFullName);
        } else {
          wcscpy_s(csEntry.undName, it->second.first.c_str());
          wcscpy_s(csEntry.undFullName, it->second.second.c_str());
        }
      }
      else
      {
        this->OnDbgHelpErr("SymFromAddrW", GetLastError(), s.AddrPC.Offset);
      }

      // show line number info, NT5.0-method (SymGetLineFromAddr64())
      if (m_sw->m_unknownAddresses.find(s.AddrPC.Offset) == m_sw->m_unknownAddresses.end()) {
        if (SymGetLineFromAddrW64(this->m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromLine), &Line) != FALSE) {
          csEntry.lineNumber = Line.LineNumber;
          // TODO: Mache dies sicher...!
          wcscpy_s(csEntry.lineFileName, Line.FileName);
        } else {
          m_sw->m_unknownAddresses.insert(s.AddrPC.Offset);
        }
      }
      
      // show module info (SymGetModuleInfo64())
      if (this->m_sw->GetModuleInfo(this->m_hProcess, s.AddrPC.Offset, &Module ) != FALSE)
      { // got module info OK
        switch ( Module.SymType )
        {
        case SymNone:
          csEntry.symTypeString = "-nosymbols-";
          break;
        case SymCoff:
          csEntry.symTypeString = "COFF";
          break;
        case SymCv:
          csEntry.symTypeString = "CV";
          break;
        case SymPdb:
          csEntry.symTypeString = "PDB";
          break;
        case SymExport:
          csEntry.symTypeString = "-exported-";
          break;
        case SymDeferred:
          csEntry.symTypeString = "-deferred-";
          break;
        case SymSym:
          csEntry.symTypeString = "SYM";
          break;
#if API_VERSION_NUMBER >= 9
        case SymDia:
          csEntry.symTypeString = "DIA";
          break;
#endif
        case 8: //SymVirtual:
          csEntry.symTypeString = "Virtual";
          break;
        default:
          //_snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
          csEntry.symTypeString = NULL;
          break;
        }

        // TODO: Mache dies sicher...!
        wcscpy_s(csEntry.moduleName, Module.ModuleName);
        csEntry.baseOfImage = Module.BaseOfImage;
        wcscpy_s(csEntry.loadedImageName, Module.LoadedImageName);
      } // got module info OK
      else
      {
        this->OnDbgHelpErr("SymGetModuleInfo64", GetLastError(), s.AddrPC.Offset);        
      }

      if ((csEntry.moduleName[0] == 0) && m_bAbortWhenPCOutsideKnownModules)
          break;

    } // we seem to have a valid PC

    CallstackEntryType et = nextEntry;
    if (frameNum == 0)
      et = firstEntry;
    this->OnCallstackEntry(et, csEntry);
    
    if (s.AddrReturn.Offset == 0)
    {
      this->OnCallstackEntry(lastEntry, csEntry);
      SetLastError(ERROR_SUCCESS);
      break;
    }   
  } // for ( frameNum )

  if (context == NULL)
    ResumeThread(hThread);

  return TRUE;
}


bool StackWalker::OnLoadModule(LPCWSTR, LPCWSTR, DWORD64, DWORD, DWORD, LPCWSTR, LPCWSTR, ULONGLONG, int, int) {
  return true;
}

void StackWalker::OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry)
{
  wchar_t buffer[STACKWALK_MAX_NAMELEN];
  if ( (eType != lastEntry) && (entry.offset != 0) )
  {
    if (entry.name[0] == 0)
      wcscpy(entry.name, L"(function-name not available)");
    if (entry.undName[0] != 0)
      wcscpy(entry.name, entry.undName);
    if (entry.undFullName[0] != 0)
      wcscpy(entry.name, entry.undFullName);
    if (entry.lineFileName[0] == 0)
    {
      wcscpy(entry.lineFileName, L"(filename not available)");
      if (entry.moduleName[0] == 0)
        wcscpy(entry.moduleName, L"(module-name not available)");
      _snwprintf_s(buffer, STACKWALK_MAX_NAMELEN, L"%p (%s): %s: %s+%lld\n", (LPVOID) entry.offset, entry.moduleName, entry.lineFileName, entry.name, entry.offsetFromSmybol);
    }
    else
      _snwprintf_s(buffer, STACKWALK_MAX_NAMELEN, L"%s (%d): %s +%lld\n", entry.lineFileName, entry.lineNumber, entry.name, entry.offsetFromSmybol);
    OnOutput(buffer);
  }
}

void StackWalker::OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
{
  wchar_t buffer[STACKWALK_MAX_NAMELEN];
  _snwprintf_s(buffer, STACKWALK_MAX_NAMELEN, L"ERROR: %S, GetLastError: %d (Address: %p)", szFuncName, gle, (LPVOID) addr);
  OnOutput(buffer);
}

void StackWalker::OnSymInit(LPCWSTR szSearchPath, DWORD symOptions, LPCWSTR szUserName)
{
  wchar_t buffer[STACKWALK_MAX_NAMELEN];
  _snwprintf_s(buffer, STACKWALK_MAX_NAMELEN, L"SymInit: Symbol-SearchPath: '%s', symOptions: %d, UserName: '%s'\n", szSearchPath, symOptions, szUserName);
  OnOutput(buffer);
}

void StackWalker::OnOutput(LPCWSTR buffer)
{
  OutputDebugStringW(buffer);
}
