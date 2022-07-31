#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <chrono>
#include <fstream>
#include <deque>
#include <mutex>
#include "../threadpool/threadpool.h"
using namespace std;

#define MAX_PATH_LEN                1024
#define MAX_SHELL_OUT_SIZE          2048
#define LOG(fmt, ...)               printf(fmt"\n", ##__VA_ARGS__);

class Utils
{
public:
    static string GetAndroidRootPath();
    static string GetCurPath();
    static string GetTargetsOutPath();
    static string GetProductName();
    static bool IsDirExist(const char* pDir);
    static bool IsFileExist(const char* pFilePath);
    static unsigned long long GetCurrentSecCounts();
    static unsigned long long GetCurrentMilliSecCounts();
    static unsigned long long GetCurrentMicroSecCounts();
    static unsigned long long GetFileMTimeSec(const char* pPath);
    static bool WriteFile(const char* pPath, const char* pBuf, unsigned size);
    static char* ReadFile(const char* pPath);
    static void FreeFileBuf(char* pBuf);
    static string GetUserPath();
    static string FindFile(const string absoluteDir, const string fileName);
    static void FindFileFuzzy(const string absoluteDir, const string fileName, deque<string>& rResultPath);
    static bool RemoveDir(const char* pDir);
    static bool ADBKillProcessByNameRelated(const char* name);
    static bool ADBKillProcessByName(const char* name);
    static string GetFileNameNoSuffix(const string fileName);
    static bool ADBStopAPK(const string packageName);
    static bool ADBOpenAPK(const string packageName, const string activityName);
    static bool ADBRR();
    static string GetShellOutput(string cmd);
    static bool ShellCMDNoLog(string cmd);

private:
    static void FindFileFuzzyAsync(const string absoluteDir, const string fileName,
                    ThreadPool* pThreadPool, deque<string>* pResultPath, mutex* pResultPathLock);
};


#endif  //  UTILS_H