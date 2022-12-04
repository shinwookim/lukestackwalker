; Script generated by the HM NIS Edit Script Wizard.


!searchparse /file lukesw-version.h `#define STRINGVERSION "` VER_MAJOR `.` VER_MINOR `.` VER_BUGFIX `"`

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Luke Stackwalker"
!define PRODUCT_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_BUGFIX}"
!define PRODUCT_PUBLISHER "Sami Sallinen."
!define PRODUCT_WEB_SITE "http://lukestackwalker.sourceforge.net/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\luke_sw.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "bitmaps\installer.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "license.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\luke_sw.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "LukeStackWalkerSetup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\Luke Stackwalker"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "luke_sw.exe"
  CreateDirectory "$SMPROGRAMS\Luke Stackwalker"
  CreateDirectory "$SMPROGRAMS\Luke Stackwalker\cmdline-profiler"
  CreateShortCut "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker.lnk" "$INSTDIR\luke_sw.exe"
  CreateShortCut "$DESKTOP\Luke Stackwalker.lnk" "$INSTDIR\luke_sw.exe"
  File "ANN.dll"
  File "brotlicommon.dll"
  File "brotlidec.dll"
  File "bz2.dll"
  File "cairo-2.dll"
  File "cairo.dll"
  File "cdt.dll"
  File "cgraph.dll"
  File "dbghelp.dll"
  File "expat.dll"
  File "fontconfig-1.dll"
  File "fontconfig.dll"
  File "freeglut.dll"
  File "freetype.dll"
  File "getopt.dll"
  File "glib-2.dll"
  File "gobject-2.dll"
  File "gvc.dll"
  File "gvplugin_core.dll"
  File "gvplugin_dot_layout.dll"
  File "gvplugin_gd.dll"
  File "gvplugin_gdiplus.dll"
  File "gvplugin_neato_layout.dll"
  File "gvplugin_pango.dll"
  File "gvprlib.dll"
  File "jpeg62.dll"
  File "lab_gamut.dll"
  File "libatk-1.0-0.dll"
  File "libexpat.dll"
  File "libgd.dll"
  File "libgtk-win32-2.0-0.dll"
  File "libharfbuzz-0.dll"
  File "liblzma.dll"
  File "libpng16.dll"
  File "msvcp140.dll"
  File "msvcp_win.dll"
  File "pango-1.dll"
  File "pango2-1.dll"
  File "pangocairo-1.dll"
  File "pangoft2-1.dll"
  File "pangowin32-1.dll"
  File "Pathplan.dll"
  File "pcre2-16.dll"
  File "pixman-1-0.dll"
  File "pixman-1.dll"
  File "srcsrv.dll"
  File "symsrv.dll"
  File "tiff.dll"
  File "vcruntime140.dll"
  File "wxbase32u_vc.dll"
  File "wxmsw32u_core_vc.dll"
  File "wxmsw32u_stc_vc.dll"
  File "xdot.dll"
  File "zlib1.dll"
  
  
  File "config6"
  File "props.txt"
  File "bitmaps\lsp.ico"
  File "bitmaps\lsd.ico"
  File "manual\luke stackwalker manual.pdf"
  File "readme.txt"
  SetOutPath "$INSTDIR\cmdline-profiler\"
  File "cmdline-profiler\cmdline-profiler.exe"
  File "cmdline-profiler\symsrv.dll"
  File "cmdline-profiler\dbghelp.dll"
  File "cmdline-profiler\wxbase316u_vc_custom.dll"
  File "cmdline-profiler\zlib1.dll"
  File "cmdline-profiler\pcre2-16.dll"
  File "cmdline-profiler\vcruntime140.dll"
  File "cmdline-profiler\vcruntime140_1.dll"
  File "cmdline-profiler\msvcp140.dll"
  File "cmdline-profiler\msvcp_win.dll"
  

  
SectionEnd

Function RefreshShellIcons
  !define SHCNE_ASSOCCHANGED 0x08000000
  !define SHCNF_IDLIST 0
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Luke Stackwalker\Uninstall.lnk" "$INSTDIR\uninst.exe"
  CreateShortCut "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker Manual.lnk" "$INSTDIR\luke stackwalker manual.pdf"
  
  WriteRegStr HKCR ".lsp" "" "LukeStackwalker.Project"
  WriteRegStr HKCR ".lsd" "" "LukeStackwalker.Data"
  WriteRegStr HKCR "LukeStackwalker.Project" ""  "Luke Stackwalker project settings"
  WriteRegStr HKCR "LukeStackwalker.Project\DefaultIcon" ""  "$INSTDIR\lsp.ico"

  WriteRegStr HKCR "LukeStackwalker.Data" ""  "Luke Stackwalker profile data"
  WriteRegStr HKCR "LukeStackwalker.Data\DefaultIcon" ""  "$INSTDIR\lsd.ico"

  WriteRegStr HKCR "LukeStackwalker.Project\shell\open\command" "" '"$INSTDIR\luke_sw.exe" "%1"'
  WriteRegStr HKCR "LukeStackwalker.Data\shell\open\command" "" '"$INSTDIR\luke_sw.exe" "%1"'

  Call RefreshShellIcons

  
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\luke_sw.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\luke_sw.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "Luke Stackwalker was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove Luke Stackwalker and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
 
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\config6"
  Delete "$INSTDIR\props.txt"
  Delete "$INSTDIR\bitmaps\lsp.ico"
  Delete "$INSTDIR\bitmaps\lsd.ico"
  Delete "$INSTDIR\lsp.ico"
  Delete "$INSTDIR\lsd.ico"
  Delete "$INSTDIR\luke stackwalker manual.pdf"
  Delete "$INSTDIR\manual\luke stackwalker manual.pdf"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\symsrv.yes"
  Delete "$INSTDIR\luke_sw.exe"
  
  Delete "$INSTDIR\cmdline-profiler\cmdline-profiler.exe"
  Delete "$INSTDIR\cmdline-profiler\*.dll"  


  Delete "$SMPROGRAMS\Luke Stackwalker\Uninstall.lnk"
  Delete "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker Website.lnk"
  Delete "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker.lnk"
  Delete "$SMPROGRAMS\Luke Stackwalker\Luke Stackwalker Manual.lnk"

  Delete "$DESKTOP\Luke Stackwalker.lnk"

  RMDir "$SMPROGRAMS\Luke Stackwalker"
  RMDir "$INSTDIR\cmdline-profiler"
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd