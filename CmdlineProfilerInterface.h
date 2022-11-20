
ProfilerProgressStatus *PrepareStatusInSharedMemory();
bool SampleWithCommandLineProfiler(ProfilerSettings *settings, unsigned int processId);
bool HandleCommandLineProfilerOutput();
void CloseSharedMemory() noexcept;
bool FinishCmdLineProfiling();
 