Release 1.0.8 March 25th, 2023
- added disassembly window that shows samples in top of callstack functions using the Zydis disassembler library

Release 1.0.7 March 19th, 2023
- move temporary files for the 64-bit command line profiler from \program files(x86)\Luke Stackwalker\ to %TEMP% because windows no longer allows writing in the original location.

Release 1.0.6 March 12th, 2023
-ctrl-c copies selected function details OR source edit control selection, if something is selected and the edit control has focus

Release 1.0.5 January 22nd, 2023
-better multi-format EOL support (can now handle cr+lf,cr and lf all in the same file)
-detection of UTF-8 BOM and separate loading path for UTF-8 encoded files
-tested / fixed loading of japanese characters in shift-JIS encoding
-restore compiler optimizations in the GUI and 32-bit profiler -> faster sampling (they were never removerd from the x64 profiler)
-add visual studio 2022 project file to version control

Release 1.0.4, January 16th, 2023
-Source code view auto-adapts between unix,mac and windows style line endings

Release 1.0.3, December 15th, 2022
-Load last used profiler settings when launched, so that your default settings are loaded automatically
-Exporting the call graph view as a .png image using the File/Save call graph view as .png... menu command
-Copy selected function (=last clicked in profile or call graph view) info to clipboard as text with Ctrl-c or the Edit/Copy command
-Allow configuring the symbol server cache directory in the profiler settings wizard step 2
-Make also the last profiled binary name remembered persistently
-Performance optimization in profiling,boost sampling speed a little

Release 1.0.2, November 20th, 2022
- fix additional debug info directories from project configuration not being passed to dbghelp.dll
- fix crash with very long >1k character symbol names (thanks, Qt ;-P)
- attach to process-dialog remembers the name of the previously profiled program (within one run of luke stackwalker) and pre-selects that


Release 1.0.1, May 1st 2021
-fix window splitters to work with display scaling, bug report by Jani Kesänen 

Release 1.0.0, April 22th 2021
Time to declare the long beta period over and promote Luke Stackwalker version 1.0 - thanks t0st! 
- ported to visual studio 2019
- using graphwiz 2.47.0 & wxwidgets 3.14
- updated dbghelp.dll to latest version from Microsoft
- fixed ~10 bugs found by new compiler version
- support for loading widechar source files

Release 0.9.9, Dec 22th 2010
- Fix crash when clicking on a call-graph node with source code
- Fix crash when having one profile loaded, source code view displaying source data and then loading another profile
- Prevent application main window from opening off-screen
- Updated manual/help

Release 0.9.8, Dec 12th 2010
- DLL hell fix (Again... shoot me!)
- Support for profiling 64-bit programs
- Several small GUI tweaks
- Several bug fixes

Release 0.9.7, Apr 19th 2010
- A sexier display of samples per source line
- An option in the view menu to display samples either as sample counts or percentage of total samples


Release 0.9.6, Feb 2nd 2010
- Added Edit/Find - command that searches a function name containing a given string in the top-of-stack profile.
- Display the sample count of the selected (top-of-stack) function in all threads in the program status bar
- Politely refuse to attach to 'self' - this would not work anyway.

Release 0.9.5, Jan 9th 2010
- Bug fix: By default, abort call stack traversing if PC is not in any known module. Not doing
  this used to get a lot of bogus call stack data when the call stack could not be walked properly.
  Getting a lot of bogus addresses could make the call graph view to take a very long time to draw
  and thus to make the program to appear to have hung.  
- Bug fix: The source window would sometimes stay blank and not load the correct source file.
- Bug fix: Don't allow moving the selection in the profile view below the bottom of the grid view with arrow keys

Release 0.9.4, Nov 19th 2009
- Bug fix: DLL hell had struck me again, dependency to a version  8.0.50727.4053 of the vc runtime had creeped in to the program via windows update.
  fixed according to : http://tedwvc.wordpress.com/2009/08/10/avoiding-problems-with-vc2005-sp1-security-update-kb971090/
- Separate tool bar buttons for saving profile data and project settings  

Release 0.9.3, Nov 8th 2009
- Doctor, heal yourself: profiled luke stackwalker -> improved sampling speed by up to 3x
- Bug fix: fixed a logging-related crash when loading a sample data file and have not run a profile before
- Added profile data file name to window title bar text

Release 0.9.2, Oct 29th 2009
- Bug fix: stop sampling with an error message if no threads can be sampled
- Bug fix: complain only once for each callstack address that cannot be resolved to a function name / source line.
  Before this could flood the log view to the point where the program would hang.
- Check if the process has loaded new modules during sampling and load debug info for the new modules
- Log window enchancements: log scrolls when new text is written, more errors in red
- Bug fix: spin controls in project settings wizard now allow typing into them (fixed by updating wxWidgets to 2.8.10)
- Updated debuggin tools for windows redistributables to currently latest available version 6.11.1.404 

Release 0.9.1, Oct 20th 2009
- Bug fix: fixed a crash when displaying call graph with very long (STL-style) function names 
- More sensible default project settings
- Project settings dialog suggests stack sampling depth of 1 for best accuracy, 0 for getting a good call graph
- Sorting of processes according to executable name in "Select process to profile"-dialog
- Displaying of sum of CPU time used in thread select combo box when multiple threads are selected

Release 0.9.0, June 5th 2009
- Bug Fix : profiling an already running program, "attach to process"  was broken in 0.8.9
- canceling profiling during module loading works now
- sample collection can be paused and continued
- limited horizontal splitter movement - cannot make left side bigger than size of data grid

Release 0.8.9, May 18th 2009
- Properly install MSVC runtime (the program has probably not worked before for people without the MSVC 2005 SP1 runtime installed)
- sort threads by CPU time used in toolbar thread selection list
- debug info loading log line in red colour if debug info was not found for a module
- display number of sampled top-level functions correctly after loading sample data from a file 
- detect if the profiled program exits immediately (it's probably missing a DLL or something)

Release 0.8.8, March 31 2009
- Show user manual from Help menu
- Profiling of an already running program

Release 0.8.7, Jan 27 2009
- Display(+Save/Load) of CPU time used by each thread in the thread 

Release 0.8.6, Jan 11 2009
- PDF user manual included
- Progress dialog shows loading of debug info
- Prompting for saving of profile data before closing program/profiling again/loading a profile from file
- Crash fix when clicking on profile view column headers
- Installer improvements

ReleaSE 0.8.5
- Initial release
