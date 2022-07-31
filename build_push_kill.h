#ifndef BUILD_PUSH_KILL_H
#define BUILD_PUSH_KILL_H

#include <iostream>
#include <dirent.h>
#include <string.h>
#include <deque>
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <memory>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <mutex>
#include <set>
#include <stdio.h>
#include "tools/jsoncpp/include/json/reader.h"
#include "tools/jsoncpp/include/json/writer.h"
#include "tools/utils/utils.h"
#include "tools/threadpool/threadpool.h"
#include "tools/jsonstream/jsonstream.h"
#include "config/bpk_common_config.h"
using namespace std;

#define BPK_TEMP_DIR_NAME                       ".BPKTemp"
#define BPK_SCRIPT_DIR_NAME                     "script"
#define BPK_CONFIG_FILE_NAME                    "config.json"
#define BPK_FILELOCK_FILE_NAME                  "bpk_file_lock"
#define BUILD_MODULE_CMD                        "/build/soong/soong_ui.bash --make-mode"
#define PROJECT_CONFIG_FILE_NAME                "project_config.json"
#define DEPS_INFO_FILE_NAME                     "deps.txt"
#define MODULE_NAME_MARK                        "_intermediates"
#define SET_SHELL_SCRIPT_PERMISSIONS_CMD        "chmod 777 "

#define CONFIG_NAME_TARGETS_OUT_DIR_SCAN        "TargetsOutDirScan"
#define CONFOG_NAME_KILL_PROCESS_NAME           "KillProcess"
#define CONFOG_NAME_RESTART_APK_NAME            "RestartAPK"
#define CONFOG_NAME_RESTART_APK_INTERVAL        "RestartAPKInterval"
#define CONFOG_NAME_IGNORE_MODIFIED_FILE_DIR    "IgnoreModifiedFileDir"
#define CONFOG_NAME_EXT_ACTION                  "ExtAction"
#define CONFIG_NAME_DEPS_FILE_TIMESTAMP         "DepsFileTimestamp"
#define CONFIG_NAME_RUN_BPK_TIMESTAMP           "RunBPKTimestamp"

#define SCAN_MODIFIED_FILE_THREAD_NUM           std::thread::hardware_concurrency()
#define SEARCH_MODIFIED_MODULE_NUM_PRE_THREAD   60u
#define MAX_MODIFIED_FILE_NUM                   1000u
#define MAX_MODIFIED_MODULE_NUM                 100u


struct ObjDepsInfo
{
    string          objPath;
    unsigned        depsNum;
    deque<string>   depsPathList;
};

struct AndroidProjectConfigInfo
{
    unsigned long long buildTimestamp;
    unsigned long long depsFileTimestamp;
};

struct ModulePathInfo
{
    string  modulePath;
    string  modulePathInPhone;
};

struct APKACtivityInfo
{
    string              packageName;
    string              activityName;
    unsigned long long  restartInterval;
};


class BuildPushKill
{
public:
    BuildPushKill();
    ~BuildPushKill();
    static int Run(deque<string>& rTargetsList);
    static int ResetBuildTime();
    static int CleanTempFile();
private:
    static void GetDepsTree(JsonStream* pJsonStream, deque<shared_ptr<ObjDepsInfo>>* pObjDepsInfoList);
    static void ScanModifiedFile(const string dir, const unsigned long long timestamp,
                            const set<string>* pIgnoreDirList, unsigned step, ThreadPool* pThreadPool,
                            deque<string>* pModifiedFileList, mutex* pModifiedFileListLock);
    static void GetModifiedModule(
                    const deque<shared_ptr<ObjDepsInfo>>& rObjDepsInfoList,
                    const deque<string>& rModifiedFileList,
                    set<string>& rModifiedModuleList);
    static void GetModifiedModuleAsync(
                    const deque<shared_ptr<ObjDepsInfo>>* pObjDepsInfoList,
                    const deque<string>* pSubModifiedFileList,
                    set<string>* pSubModifiedObjList);
    static bool GetModulePath(const set<string>& rModuleNameList, const unsigned long long timestamp,
                    deque<ModulePathInfo>& rModulePathList);
    static bool KillProcess();

    static string GetBPKTempDir();
    static bool CheckAndMakeBPKTempDir();
    static bool CheckAndMakeTempDir(string tempDirName);
    static string GetModuleNameBySubObjectPath(string path);
    static string GetModulePathByModuleName(
                    const deque<shared_ptr<ObjDepsInfo>>& rObjDepsInfoList, 
                    const deque<string>& rSearchDirList,
                    string name,
                    string* pNameWithSuffix);
};

#endif  //  BUILD_PUSH_KILL_H