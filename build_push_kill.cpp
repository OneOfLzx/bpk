#include "build_push_kill.h"

BuildPushKill::BuildPushKill()
{

}

BuildPushKill::~BuildPushKill()
{

}

int BuildPushKill::CleanTempFile()
{
    int                 result      = 0;
    string              bpkTmpDir   = BuildPushKill::GetBPKTempDir();
    DIR*                pDirStream  = nullptr;
    struct dirent*      pDirEntry   = nullptr;
    unsigned            pathLen     = 0;
    char*               newPath     = nullptr;
    string              cmd         = "";

    LOG("清空缓存");

    if (nullptr != (pDirStream = opendir(bpkTmpDir.c_str())))
    {
        while (nullptr != (pDirEntry = readdir(pDirStream)))
        {
            if (0 == strcmp(".", pDirEntry->d_name) ||
                0 == strcmp("..", pDirEntry->d_name))
            {
                continue;
            }
            else if (DT_DIR == pDirEntry->d_type)
            {
                pathLen = bpkTmpDir.length() + strlen(pDirEntry->d_name) + 2;
                newPath = new char[pathLen];
                sprintf(newPath, "%s/%s", bpkTmpDir.c_str(), pDirEntry->d_name);

                Utils::RemoveDir(newPath);

                delete[] newPath;
            }
        }

        closedir(pDirStream);
    }

    return 0;
}

int BuildPushKill::ResetBuildTime()
{
    int                         result              = 0;
    unsigned long long          timestamp           = 0;
    string                      curPath             = Utils::GetCurPath();
    string                      androidRootPath     = Utils::GetAndroidRootPath();
    string                      targetsOutPath      = Utils::GetTargetsOutPath();
    string                      targetProductName   = Utils::GetProductName();
    string                      tempDirBPK          = BuildPushKill::GetBPKTempDir();
    hash<string>                hash;
    unsigned long long          dirName             = hash(androidRootPath + "/" + targetProductName);
    string                      projectConfigPath   = tempDirBPK + "/" + to_string(dirName) + "/" + string(PROJECT_CONFIG_FILE_NAME);
    JsonStream                  jsonStream;

    if (0 == androidRootPath.length() || 0 == targetsOutPath.length())
    {
        LOG("安卓编译环境还未配置，请先运行envsetup.sh & lunch");
        result = -1;
    }

    if (nullptr == strstr(curPath.c_str(), androidRootPath.c_str()))
    {
        LOG("请在安卓项目根目录及其子目录下运行");
        result = -1;
    }

    if (0 == result)
    {
        if (false == CheckAndMakeTempDir(to_string(dirName)))
        {
            result = -1;
        }
    }

    if (0 == result)
    {
        if (false == jsonStream.Open(projectConfigPath))
        {
            result = -1;
        }
    }

    if (0 == result)
    {
        timestamp = Utils::GetCurrentSecCounts();
        jsonStream.GetRoot()[CONFIG_NAME_RUN_BPK_TIMESTAMP] = (Json::UInt64)timestamp;
    }

    if (0 == result)
    {
        jsonStream.UpdateTofile();
        LOG("更新扫描文件起始时间戳");
    }

    return result;
}

int BuildPushKill::Run(deque<string>& rTargetsList)
{
    int                         result              = 0;
    string                      curPath             = Utils::GetCurPath();
    string                      androidRootPath     = Utils::GetAndroidRootPath();
    string                      targetsOutPath      = Utils::GetTargetsOutPath();
    string                      targetProductName   = Utils::GetProductName();
    string                      tempDirBPK          = BuildPushKill::GetBPKTempDir();
    hash<string>                hash;
    unsigned long long          dirName             = hash(androidRootPath + "/" + targetProductName);
    string                      projectConfigPath   = tempDirBPK + "/" + to_string(dirName) + "/" + string(PROJECT_CONFIG_FILE_NAME);
    bool                        isAutoScanTarget    = true;
    set<string>                 targetsModuleList;
    deque<ModulePathInfo>       modulePushList;
    unsigned long long          curTimestamp        = 0;
    deque<string>               modifiedFileList;
    mutex                       modifiedFileListLock;
    string                      bpkConfigPath       = tempDirBPK + "/" + BPK_CONFIG_FILE_NAME;
    JsonStream                  bpkConfigJsonStream;
    set<string>                 extShellBeforeBuild;
    set<string>                 extShellAfterBuild;
    set<string>                 ignoreDirList;
    modifiedFileList.clear();
    targetsModuleList.clear();
    modulePushList.clear();
    extShellBeforeBuild.clear();
    extShellAfterBuild.clear();
    ignoreDirList.clear();

    if (0 == result)
    {
        if (false ==CheckAndMakeBPKTempDir())
        {
            LOG("创建临时文件目录失败");
            result = -1;
        }
    }

    if (0 == result)
    {
        if (false == bpkConfigJsonStream.Open(bpkConfigPath))
        {
            result = -1;
        }
    }

    if (0 == androidRootPath.length() || 0 == targetsOutPath.length())
    {
        LOG("安卓编译环境还未配置，请先运行envsetup.sh & lunch");
        result = -1;
    }

    if (nullptr == strstr(curPath.c_str(), androidRootPath.c_str()))
    {
        LOG("请在安卓项目根目录及其子目录下运行");
        result = -1;
    }

    if (0 == result)
    {
        if (0 < rTargetsList.size())
        {
            isAutoScanTarget    = false;
            unsigned targetsNum = rTargetsList.size();
            for (unsigned i = 0; i < targetsNum; i++)
            {
                targetsModuleList.insert(rTargetsList[i]);
            }
        }
        else
        {
            isAutoScanTarget = true;

            JsonStream                      projectJsonStream;
            unsigned long long              scanTimestamp           = 0;
            deque<shared_ptr<ObjDepsInfo>>  objDepsInfoList;
            objDepsInfoList.clear();

            LOG("正在扫描被修改的文件...");

            if (0 == result)
            {
                if (false == CheckAndMakeTempDir(to_string(dirName)))
                {
                    result = -1;
                }
            }

            if (0 == result)
            {
                if (false == projectJsonStream.Open(projectConfigPath))
                {
                    result = -1;
                }
            }

            if (0 == result)
            {
                if (true == projectJsonStream.GetRoot().isMember(CONFIG_NAME_RUN_BPK_TIMESTAMP) &&
                    0 < projectJsonStream.GetRoot()[CONFIG_NAME_RUN_BPK_TIMESTAMP].asUInt64())
                {
                    scanTimestamp = projectJsonStream.GetRoot()[CONFIG_NAME_RUN_BPK_TIMESTAMP].asUInt64();
                }
                else
                {
                    scanTimestamp = Utils::GetCurrentSecCounts() - 30 * 60;
                }
                projectJsonStream.GetRoot()[CONFIG_NAME_RUN_BPK_TIMESTAMP] = (Json::UInt64)Utils::GetCurrentSecCounts();
            }

            auto scanStartT = Utils::GetCurrentMilliSecCounts();
            if (0 == result)
            {
                ThreadPool  getDepsTreeTP(1);
                getDepsTreeTP.enqueue(GetDepsTree, &projectJsonStream, &objDepsInfoList);

                ThreadPool  scanFileTP(SCAN_MODIFIED_FILE_THREAD_NUM, true);
                if (true == bpkConfigJsonStream.GetRoot().isMember(CONFOG_NAME_IGNORE_MODIFIED_FILE_DIR) &&
                    true == bpkConfigJsonStream.GetRoot()[CONFOG_NAME_IGNORE_MODIFIED_FILE_DIR].isArray())
                {
                    Json::Value&    ignoreDirJson   = bpkConfigJsonStream.GetRoot()[CONFOG_NAME_IGNORE_MODIFIED_FILE_DIR];
                    unsigned        ignoreNum       = ignoreDirJson.size();
                    for (unsigned igIdx = 0; igIdx < ignoreNum; igIdx++)
                    {
                        if (0 != ignoreDirJson[igIdx].asString().compare(""))
                        {
                            ignoreDirList.insert(androidRootPath + "/" + ignoreDirJson[igIdx].asString());
                        }
                    }
                }
                scanFileTP.enqueue(ScanModifiedFile, curPath, scanTimestamp, &ignoreDirList,
                    0, &scanFileTP, &modifiedFileList, &modifiedFileListLock);
            }
            auto scanEndT = Utils::GetCurrentMilliSecCounts();
            auto scanCost = scanEndT - scanStartT;

            if (0 == result)
            {
                GetModifiedModule(objDepsInfoList, modifiedFileList, targetsModuleList);
            }

            if (0 == result)
            {
                projectJsonStream.UpdateTofile();
            }

            LOG("扫描完成（耗时: %llums）", scanCost);
            LOG("");
        }
    }

    if (0 == result &&
        (MAX_MODIFIED_FILE_NUM <= modifiedFileList.size() ||
        MAX_MODIFIED_MODULE_NUM <= targetsModuleList.size()))
    {
        LOG("被修改的文件数：%lu，需要编译的模块数：%lu，是否要继续(y/N)？",
            modifiedFileList.size(), targetsModuleList.size());
        char input[128] = { 0 };
        cin.get(input, 128);
        if (0 != strcmp(input, "y") &&
            0 != strcmp(input, "Y"))
        {
            LOG("放弃编译")
            result = -1;
        }
    }

    if (0 == result &&
        true == bpkConfigJsonStream.GetRoot().isMember(CONFOG_NAME_EXT_ACTION) &&
        true == bpkConfigJsonStream.GetRoot()[CONFOG_NAME_EXT_ACTION].isArray())
    {
        Json::Value&    extActionListJson   = bpkConfigJsonStream.GetRoot()[CONFOG_NAME_EXT_ACTION];
        unsigned        extNum              = extActionListJson.size();
        for (unsigned extIdx = 0; extIdx < extNum; extIdx++)
        {
            Json::Value& extAction = extActionListJson[extIdx];
            for (auto iter = modifiedFileList.begin(); iter != modifiedFileList.end(); iter++)
            {
                if (nullptr != strstr((*iter).c_str(), extAction["Dir"].asString().c_str()))
                {
                    if (0 != extAction["RelatedModule"].asString().compare(""))
                    {
                        targetsModuleList.insert(extAction["RelatedModule"].asString());
                    }
                    if (0 != extAction["ShellBeforeBuild"].asString().compare(""))
                    {
                        extShellBeforeBuild.insert(extAction["ShellBeforeBuild"].asString());
                    }
                    if (0 != extAction["ShellAfterBuild"].asString().compare(""))
                    {
                        extShellAfterBuild.insert(extAction["ShellAfterBuild"].asString());
                    }
                }
            }
        }
    }

    if (0 == result)
    {
        if (false == isAutoScanTarget)
        {
            LOG("指定编译模块:");
            for (auto iter = targetsModuleList.begin(); iter != targetsModuleList.end(); iter++)
            {
                LOG("    %s", (*iter).c_str());
            }
            LOG("================================================================");
            LOG("");
        }
        else
        {
            if (0 < modifiedFileList.size())
            {
                LOG("被修改的文件:");
                unsigned fileNum = modifiedFileList.size();
                for (unsigned i = 0; i < fileNum; i++)
                {
                    LOG("    %s", modifiedFileList[i].c_str());
                }
                LOG("");
                LOG("需要编译的模块:");
                for (auto iter = targetsModuleList.begin(); iter != targetsModuleList.end(); iter++)
                {
                    LOG("    %s", (*iter).c_str());
                }
                LOG("================================================================");
                LOG("");
            }
            else
            {
                LOG("未扫描到已修改文件");
            }
        }
    }

    if (0 == result)
    {
        if (0 >= targetsModuleList.size() &&
            0 >= extShellBeforeBuild.size() &&
            0 >= extShellAfterBuild.size())
        {
            result = -1;
        }
    }

    if (0 == result)
    {
        string  bpkScrptDir = tempDirBPK + "/" + BPK_SCRIPT_DIR_NAME + "/";
        string  scriptPath;
        string  cmd;
        for (auto iter = extShellBeforeBuild.begin(); iter != extShellBeforeBuild.end(); iter++)
        {
            scriptPath  = bpkScrptDir + *iter;
            cmd         = SET_SHELL_SCRIPT_PERMISSIONS_CMD + scriptPath + "; source " + scriptPath;
            system(cmd.c_str());
        }
    }

    if (0 == result && 0 < targetsModuleList.size())
    {
        curTimestamp = Utils::GetCurrentSecCounts();

        string cmd = androidRootPath + BUILD_MODULE_CMD;
        for (auto iter = targetsModuleList.begin(); iter != targetsModuleList.end(); iter++)
        {
            cmd += string(" ") + *iter;
        }

        if (-1 == system(cmd.c_str()))
        {
            LOG("执行编译指令错误");
            result = -1;
        }
    }

    if (0 == result)
    {
        string  bpkScrptDir = tempDirBPK + "/" + BPK_SCRIPT_DIR_NAME + "/";
        string  scriptPath;
        string  cmd;
        for (auto iter = extShellAfterBuild.begin(); iter != extShellAfterBuild.end(); iter++)
        {
            scriptPath  = bpkScrptDir + *iter;
            cmd         = SET_SHELL_SCRIPT_PERMISSIONS_CMD + scriptPath + "; source " + scriptPath;
            system(cmd.c_str());
        }
    }

    if (0 == result)
    {
        pid_t pId = fork();
        if (0 == pId)
        {
            JsonStream jsonStream;
            jsonStream.Open(projectConfigPath);
            GetDepsTree(&jsonStream, nullptr);
            jsonStream.UpdateTofile();
        }
        else
        {
            LOG("");
            LOG("================================================================");

            if (0 == result && true == Utils::ADBRR())
            {
                GetModulePath(targetsModuleList, curTimestamp, modulePushList);

                string      cmd         = "";
                unsigned    targetsNum  = modulePushList.size();
                for (unsigned i = 0; i < targetsNum; i++)
                {
                    cmd = "adb push " + modulePushList[i].modulePath + " " + modulePushList[i].modulePathInPhone;
                    system(cmd.c_str());
                }
            }
            else
            {
                LOG("adb roo失败");
                result = -1;
            }

            LOG("================================================================");
            LOG("");

            if (0 == result)
            {
                BuildPushKill::KillProcess();
            }
        }
    }

    return result;
}

void BuildPushKill::ScanModifiedFile(const string dir, const unsigned long long timestamp,
                            const set<string>* pIgnoreDirList, unsigned step, ThreadPool* pThreadPool,
                            deque<string>* pModifiedFileList, mutex* pModifiedFileListLock)
{
    DIR*                pDirStream      = nullptr;
    struct dirent*      pDirEntry       = nullptr;
    struct stat         statInfo        = { };
    unsigned            pathLen         = 0;
    char*               newPath         = nullptr;
    char*               dirPath         = nullptr;
    bool                isIgnore        = false;
    deque<char*>        dirPathList;
    deque<string>       modifiedFileList;
    mutex               modifiedFileListLock;
    dirPathList.clear();
    modifiedFileList.clear();

    pathLen     = dir.length() + 1;
    newPath     = new char[pathLen];
    sprintf(newPath, "%s", dir.c_str());
    dirPathList.push_back(newPath);

    while (0 < dirPathList.size())
    {
        dirPath     = dirPathList.front();
        isIgnore    = false;
        for (auto iter = pIgnoreDirList->begin(); iter != pIgnoreDirList->end(); iter++)
        {
            if (0 == (*iter).compare(dirPath))
            {
                isIgnore = true;
                break;
            }
        }

        if (false == isIgnore && nullptr != (pDirStream = opendir(dirPath)))
        {
            while (nullptr != (pDirEntry = readdir(pDirStream)))
            {
                if (0 == strcmp(".", pDirEntry->d_name) ||
                    0 == strcmp("..", pDirEntry->d_name) ||
                    0 == strcmp(".git", pDirEntry->d_name) ||
                    0 == strcmp(".repo", pDirEntry->d_name) ||
                    (0 == step && 0 == strcmp("out", pDirEntry->d_name)))
                {
                    continue;
                }
                else if (DT_DIR == pDirEntry->d_type)
                {
                    pathLen = strlen(dirPath) + strlen(pDirEntry->d_name) + 2;
                    newPath = new char[pathLen];
                    sprintf(newPath, "%s/%s", dirPath, pDirEntry->d_name);

                    dirPathList.push_back(newPath);
                }
                else if (DT_REG == pDirEntry->d_type)
                {
                    pathLen = strlen(dirPath) + strlen(pDirEntry->d_name) + 2;
                    newPath = new char[pathLen];
                    sprintf(newPath, "%s/%s", dirPath, pDirEntry->d_name);

                    stat(newPath, &statInfo);
                    if (statInfo.st_mtim.tv_sec >= timestamp)
                    {
                        modifiedFileList.emplace_back(string(newPath));
                    }

                    delete[] newPath;
                    newPath = nullptr;
                }
            }

            closedir(pDirStream);
        }

        delete[] dirPath;
        dirPathList.pop_front();

        step++;
        unsigned dirNum = dirPathList.size();
        for (unsigned i = 0; i < dirNum; i++)
        {
            pThreadPool->enqueue(ScanModifiedFile, string(dirPathList[i]), timestamp,
                pIgnoreDirList, step, pThreadPool, pModifiedFileList, pModifiedFileListLock);
            delete[] dirPathList[i];
            dirPathList[i] = nullptr;

        }
        dirPathList.clear();
    }

    pModifiedFileListLock->lock();
    unsigned fNum = modifiedFileList.size();
    for (unsigned i = 0; i < fNum; i++)
    {
        pModifiedFileList->push_back(modifiedFileList[i]);
    }
    pModifiedFileListLock->unlock();
}

void BuildPushKill::GetDepsTree(JsonStream* pJsonStream, deque<shared_ptr<ObjDepsInfo>>* pObjDepsInfoList)
{
    bool                result              = true;
    unsigned long long  curDepsTS           = 0;
    unsigned long long  newDepsTs           = 0;
    string              androidRootPath     = Utils::GetAndroidRootPath();
    string              targetProductName   = Utils::GetProductName();
    string              ninjaFilePath       = androidRootPath + "/out/combined-" + targetProductName + ".ninja";
    string              checkMtimeFile      = androidRootPath + "/out/build-" + targetProductName + ".ninja";
    hash<string>        hash;
    unsigned long long  tmpDirName          = hash(androidRootPath + "/" + targetProductName);
    string              tmpDirPath          = BuildPushKill::GetBPKTempDir() + "/" + to_string(tmpDirName);
    string              depsInfoFile        = tmpDirPath + "/" + DEPS_INFO_FILE_NAME;
    FILE*               pFileLock           = nullptr;
    string              fileLockPath        = BuildPushKill::GetBPKTempDir() + "/" + BPK_FILELOCK_FILE_NAME;

    {
        if (nullptr != (pFileLock = fopen(fileLockPath.c_str(), "r")))
        {
            if (0 != flock(pFileLock->_fileno, LOCK_EX))
            {
                LOG("锁定文件锁失败!");
                result = false;
            }
        }
        else
        {
            LOG("获取文件锁失败!");
            result = false;
        }
    }

    if (nullptr == pJsonStream)
    {
        result = false;
    }

    if (true == result)
    {
        result = pJsonStream->IsOpen();
    }

    if (true == result)
    {
        result = CheckAndMakeTempDir(to_string(tmpDirName));
    }

    if (true == result)
    {
        newDepsTs = Utils::GetFileMTimeSec(checkMtimeFile.c_str());
    
        if (true == pJsonStream->GetRoot().isMember(CONFIG_NAME_DEPS_FILE_TIMESTAMP))
        {
            curDepsTS = pJsonStream->GetRoot()[CONFIG_NAME_DEPS_FILE_TIMESTAMP].asUInt64();
        }

        if (newDepsTs > curDepsTS)
            pJsonStream->GetRoot()[CONFIG_NAME_DEPS_FILE_TIMESTAMP] = (Json::UInt64)newDepsTs;
    }

    if (true == result && (newDepsTs > curDepsTS || false == Utils::IsFileExist(depsInfoFile.c_str())))
    {
        string cmd  = "cd " + androidRootPath + ";";
        cmd         += androidRootPath + "/prebuilts/build-tools/linux-x86/bin/ninja -f " + ninjaFilePath
                    + " -t deps > " + depsInfoFile.c_str();

        if (-1 == system(cmd.c_str()))
        {
            LOG("创建依赖树失败");
            result = false;
        }
    }

    if (true == result && nullptr != pObjDepsInfoList)
    {
        ifstream reader;
        reader.open(depsInfoFile, ios::in);
        if (true == reader.is_open())
        {
            const unsigned          maxLineLen      = 2048;
            char*                   txtLine         = new char[maxLineLen];
            bool                    isLineValid     = false;
            unsigned                blankEndOff     = 0;
            unsigned                slipOff         = 0;
            unsigned                txtLen          = 0;
            string                  depPath         = "";
            shared_ptr<ObjDepsInfo> pObjDepinfo = { };

            while (!reader.eof())
            {
                reader.getline(txtLine, maxLineLen);

                isLineValid = false;
                txtLen      = strlen(txtLine);

                if (2 <= txtLen && ' ' == txtLine[0] && ' ' == txtLine[1] &&
                    nullptr != pObjDepinfo.get())
                {
                    for (unsigned i = 0; i < txtLen; i++)
                    {
                        if (' ' != txtLine[i])
                        {
                            isLineValid = true;
                            blankEndOff = i;
                            break;
                        }
                    }

                    if (true == isLineValid)
                    {
                        depPath = txtLine;
                        depPath = depPath.substr(blankEndOff, depPath.length() - blankEndOff);
                        pObjDepinfo->depsPathList.push_back(depPath);
                    }
                }
                else if (7 <= txtLen)
                {
                    for (unsigned i = 0; i < txtLen; i++)
                    {
                        if (':' == txtLine[i] &&
                            (i + 1 < txtLen && ' ' == txtLine[i + 1]) &&
                            (i + 2 < txtLen && '#' == txtLine[i + 2]) &&
                            (i + 3 < txtLen && 'd' == txtLine[i + 3]) &&
                            (i + 4 < txtLen && 'e' == txtLine[i + 4]) &&
                            (i + 5 < txtLen && 'p' == txtLine[i + 5]) &&
                            (i + 6 < txtLen && 's' == txtLine[i + 6]))
                        {
                            isLineValid = true;
                            slipOff     = i;
                            break;
                        }
                    }

                    if (true == isLineValid)
                    {
                        if (nullptr != pObjDepinfo.get())
                        {
                            pObjDepinfo->depsNum = pObjDepinfo->depsPathList.size();
                            pObjDepsInfoList->push_back(pObjDepinfo);
                        }

                        pObjDepinfo             = shared_ptr<ObjDepsInfo>(new ObjDepsInfo());
                        pObjDepinfo->objPath    = txtLine;
                        pObjDepinfo->objPath    = pObjDepinfo->objPath.substr(0, slipOff);
                    }
                }
            }

            pObjDepinfo->depsNum = pObjDepinfo->depsPathList.size();
            pObjDepsInfoList->push_back(pObjDepinfo);
            reader.close();
        }
    }

    if (nullptr != pFileLock)
    {
        if (0 != flock(pFileLock->_fileno, LOCK_UN))
        {
            LOG("释放文件锁失败!");
            result = false;
        }
        fclose(pFileLock);
    }
}

void BuildPushKill::GetModifiedModule(
                            const deque<shared_ptr<ObjDepsInfo>>& rObjDepsInfoList,
                            const deque<string>& rModifiedFileList,
                            set<string>& rModifiedModuleList)
{
    string          androidRootPath     = Utils::GetAndroidRootPath() + "/";
    string          targetsOutPath      = Utils::GetTargetsOutPath();
    unsigned        objNum              = rObjDepsInfoList.size();
    unsigned        mFileNum            = rModifiedFileList.size();
    unsigned        depsNum             = 0;
    string          mFilePath;
    bool            isFound             = false;
    set<string>     modifiedObjList;
    modifiedObjList.clear();

    const int       fileNumPreThread        = SEARCH_MODIFIED_MODULE_NUM_PRE_THREAD;
    const int       threadNum               = (mFileNum / fileNumPreThread + 1) < std::thread::hardware_concurrency() ?
                                            (mFileNum / fileNumPreThread + 1) : std::thread::hardware_concurrency();
    deque<string>*  subModifiedFileList     = new deque<string>[threadNum];
    unsigned        subModifiedFilePushNum  = 0;
    unsigned        subModifiedFileNum      = 0;
    set<string>*    subModifiedObjList      = new set<string>[threadNum];
    {
        ThreadPool  threadPool(threadNum);
        for (unsigned i = 0; i < threadNum; i++)
        {
            if (mFileNum <= subModifiedFilePushNum) break;

            subModifiedFileNum = (mFileNum - subModifiedFilePushNum) < fileNumPreThread ?
                                (mFileNum - subModifiedFilePushNum) : fileNumPreThread;
            for (unsigned fIdx = 0; fIdx < subModifiedFileNum; fIdx++)
            {
                subModifiedFileList[i].push_back(rModifiedFileList[subModifiedFilePushNum + fIdx]);
            }
            subModifiedFilePushNum = subModifiedFilePushNum + subModifiedFileNum;

            threadPool.enqueue(GetModifiedModuleAsync, &rObjDepsInfoList,
                    &(subModifiedFileList[i]), &(subModifiedObjList[i]));
        }
    }
    for (unsigned i = 0; i < threadNum; i++)
    {
        for (auto iter = subModifiedObjList[i].begin(); iter != subModifiedObjList[i].end(); iter++)
        {
            modifiedObjList.insert(*iter);
        }
    }
    delete[] subModifiedFileList;
    delete[] subModifiedObjList;

    for (auto iter = modifiedObjList.begin(); iter != modifiedObjList.end(); iter++)
    {
        rModifiedModuleList.insert(GetModuleNameBySubObjectPath(*iter));
    }
}

void BuildPushKill::GetModifiedModuleAsync(
                const deque<shared_ptr<ObjDepsInfo>>* pObjDepsInfoList,
                const deque<string>* pSubModifiedFileList,
                set<string>* pSubModifiedObjList)
{
    string      androidRootPath     = Utils::GetAndroidRootPath() + "/";
    bool        isFound             = false;
    unsigned    objNum              = pObjDepsInfoList->size();
    unsigned    mFileNum            = pSubModifiedFileList->size();
    unsigned    depsNum             = 0;
    string      mFilePath;
    for (unsigned mIdx = 0; mIdx < mFileNum; mIdx++)
    {
        mFilePath   = (*pSubModifiedFileList)[mIdx].substr(androidRootPath.length());
        for (unsigned oIdx = 0; oIdx < objNum; oIdx++)
        {
            const shared_ptr<ObjDepsInfo>& objDepsInfo  = (*pObjDepsInfoList)[oIdx];
            depsNum                                     = objDepsInfo->depsNum;
            for (unsigned dIdx = 0; dIdx < depsNum; dIdx++)
            {
                if (0 == mFilePath.compare(objDepsInfo->depsPathList[dIdx]))
                {
                    pSubModifiedObjList->insert(objDepsInfo->objPath);
                    break;
                }
            }
        }
    }
}

bool BuildPushKill::GetModulePath(const set<string>& rModuleNameList, const unsigned long long timestamp,
                        deque<ModulePathInfo>& rModulePathList)
{
    bool            result          = true;
    JsonStream      jsonStream;
    string          bpkConfigPath   = BuildPushKill::GetBPKTempDir() + "/" + BPK_CONFIG_FILE_NAME;
    string          targetsOutPath  = Utils::GetTargetsOutPath();
    set<string>     scanDirList;
    deque<string>   modulePathList;
    unsigned        modulePathNum   = 0;
    ModulePathInfo  pathInfo        = { };
    modulePathList.clear();

    if (true == jsonStream.Open(bpkConfigPath) &&
        true == jsonStream.GetRoot().isMember(CONFIG_NAME_TARGETS_OUT_DIR_SCAN) &&
        jsonStream.GetRoot()[CONFIG_NAME_TARGETS_OUT_DIR_SCAN].isArray())
    {
        unsigned dirNum = jsonStream.GetRoot()[CONFIG_NAME_TARGETS_OUT_DIR_SCAN].size();
        for (unsigned i = 0; i < dirNum; i++)
        {
            scanDirList.insert(targetsOutPath + "/" + jsonStream.GetRoot()[CONFIG_NAME_TARGETS_OUT_DIR_SCAN][i].asString());
        }
    }

    if (true == result)
    {
        for (auto mIter = rModuleNameList.begin(); mIter != rModuleNameList.end(); mIter++)
        {
            for (auto dirIter = scanDirList.begin(); dirIter != scanDirList.end(); dirIter++)
            {
                Utils::FindFileFuzzy(*dirIter, *mIter, modulePathList);

                modulePathNum = modulePathList.size();
                for (unsigned pIdx = 0; pIdx < modulePathNum; pIdx++)
                {
                    if (timestamp < Utils::GetFileMTimeSec(modulePathList[pIdx].c_str()))
                    {
                        pathInfo.modulePath         = modulePathList[pIdx];
                        pathInfo.modulePathInPhone  = pathInfo.modulePath.substr(targetsOutPath.length() < pathInfo.modulePath.length() ?
                                                                    targetsOutPath.length() : pathInfo.modulePath.length());
                        if (0 != pathInfo.modulePath.compare("") &&
                            0 != pathInfo.modulePathInPhone.compare(""))
                        {
                            rModulePathList.push_back(pathInfo);
                        }
                    }
                }
            }
        }
    }

    return result;
}

bool BuildPushKill::KillProcess()
{
    bool                    result                      = true;
    string                  bpkConfigPath               = BuildPushKill::GetBPKTempDir() + "/" + BPK_CONFIG_FILE_NAME;
    JsonStream              jsonStream;
    deque<string>           killProcessName;
    deque<string>           killProcessNameRelated;
    unsigned                processNum                  = 0;
    deque<APKACtivityInfo>  restartAPKList;
    unsigned                apkNum                      = 0;
    unsigned long long      restartAPKIntervalMilliSec  = 0;
    killProcessName.clear();
    killProcessNameRelated.clear();
    restartAPKList.clear();

    if (true != jsonStream.Open(bpkConfigPath))
    {
        result = false;
    }

    if (true == result)
    {
        if (true == jsonStream.GetRoot().isMember(CONFOG_NAME_KILL_PROCESS_NAME) &&
            jsonStream.GetRoot()[CONFOG_NAME_KILL_PROCESS_NAME].isArray())
        {
            unsigned num = jsonStream.GetRoot()[CONFOG_NAME_KILL_PROCESS_NAME].size();
            for (unsigned i = 0; i < num; i++)
            {
                Json::Value& rJV = jsonStream.GetRoot()[CONFOG_NAME_KILL_PROCESS_NAME][i];
                if (true == rJV.isMember("AccurateSearch") && 0 < rJV["AccurateSearch"].asString().length() &&
                    true == rJV.isMember("ProcessName") && 0 < rJV["ProcessName"].asString().length())
                {
                    if (0 == rJV["AccurateSearch"].asString().compare("true"))
                    {
                        killProcessName.push_back(rJV["ProcessName"].asString());
                    }
                    else
                    {
                        killProcessNameRelated.push_back(rJV["ProcessName"].asString());
                    }
                }
            }
        }

        if (true == jsonStream.GetRoot().isMember(CONFOG_NAME_RESTART_APK_NAME) &&
            jsonStream.GetRoot()[CONFOG_NAME_RESTART_APK_NAME].isArray())
        {
            unsigned num = jsonStream.GetRoot()[CONFOG_NAME_RESTART_APK_NAME].size();
            for (unsigned i = 0; i < num; i++)
            {
                Json::Value& rJV = jsonStream.GetRoot()[CONFOG_NAME_RESTART_APK_NAME][i];
                if (true == rJV.isMember("PackageName") && 0 < rJV["PackageName"].asString().length() &&
                    true == rJV.isMember("ActivityName") && 0 < rJV["ActivityName"].asString().length())
                {
                    APKACtivityInfo info    = { };
                    info.packageName        = rJV["PackageName"].asString();
                    info.activityName       = rJV["ActivityName"].asString();
                    info.restartInterval    = rJV["RestartAPKInterval"].asUInt64();
                    if (0 != info.packageName.compare("") &&
                        0 != info.activityName.compare(""))
                    {
                        restartAPKList.push_back(info);
                    }
                }
            }
        }
    }

    if (true == result)
    {
        result = Utils::ADBRR();
    }

    if (true == result)
    {
        apkNum = restartAPKList.size();
        for (unsigned i = 0; i < processNum; i++)
        {
            Utils::ADBStopAPK(restartAPKList[i].packageName);
        }
    }

    if (true == result &&
        (0 < killProcessName.size() || 0 < killProcessNameRelated.size()))
    {
        LOG("关闭相关进程...");
        processNum = killProcessName.size();
        for (unsigned i = 0; i < processNum; i++)
        {
            Utils::ADBKillProcessByName(killProcessName[i].c_str());
        }

        processNum = killProcessNameRelated.size();
        for (unsigned i = 0; i < processNum; i++)
        {
            Utils::ADBKillProcessByNameRelated(killProcessNameRelated[i].c_str());
        }
    }

    if (true == result && 0 < restartAPKList.size())
    {
        LOG("等待相关服务注册完成...");

        apkNum = restartAPKList.size();
        for (unsigned i = 0; i < apkNum; i++)
        {
            if (restartAPKList[i].restartInterval > restartAPKIntervalMilliSec)
            {
                restartAPKIntervalMilliSec = restartAPKList[i].restartInterval;
            }
        }

        usleep(restartAPKIntervalMilliSec * 1000);

        for (unsigned i = 0; i < apkNum; i++)
        {
            Utils::ADBOpenAPK(restartAPKList[i].packageName, restartAPKList[i].activityName);
        }
    }

    return result;
}

bool BuildPushKill::CheckAndMakeBPKTempDir()
{
    bool        result          = true;
    string      bpkTempDir      = BuildPushKill::GetBPKTempDir();
    struct stat statBuf         = { };
    string      filePath        = "";

    if (true == result && nullptr == opendir(bpkTempDir.c_str()))
    {
        int res = ::mkdir(bpkTempDir.c_str(), 0755);
        if (0 != res)
        {
            result = false;
        }
    }

    filePath = bpkTempDir + "/" + BPK_SCRIPT_DIR_NAME;
    if (true == result && nullptr == opendir(filePath.c_str()))
    {
        int res = ::mkdir(filePath.c_str(), 0755);
        if (0 != res)
        {
            result = false;
        }
    }

    statBuf     = { };
    filePath    = bpkTempDir + "/" + BPK_CONFIG_FILE_NAME;
    if (true == result && 0 != stat(filePath.c_str(), &statBuf))
    {
        ofstream writer;
        writer.open(filePath, ios::out | ios::binary);
        if (true == writer.is_open())
        {
            writer.write(BPKCommonConfig, sizeof(BPKCommonConfig) - 1);
        }
        else
        {
            result = false;
        }
        writer.close();
    }

    statBuf     = { };
    filePath    = bpkTempDir + "/" + BPK_FILELOCK_FILE_NAME;
    if (true == result && 0 != stat(filePath.c_str(), &statBuf))
    {
        ofstream writer;
        writer.open(filePath, ios::out | ios::binary);
        if (true != writer.is_open())
        {
            result = false;
        }
        writer.close();
    }

    return result;
}

bool BuildPushKill::CheckAndMakeTempDir(string tempDirName)
{
    bool    result          = true;
    string  bpkTempDir      = BuildPushKill::GetBPKTempDir();
    string  tmpDir          = bpkTempDir + "/" + tempDirName;

    if (true == result && nullptr == opendir(bpkTempDir.c_str()))
    {
        int res = ::mkdir(bpkTempDir.c_str(), 0755);
        if (0 != res)
        {
            result = false;
        }
    }

    if (true == result && nullptr == opendir(tmpDir.c_str()))
    {
        int res = ::mkdir(tmpDir.c_str(), 0755);
        if (0 != res)
        {
            result = false;
        }
    }

    return result;
}

string BuildPushKill::GetBPKTempDir()
{
    static string bpkTmpDir = Utils::GetUserPath() + "/" + BPK_TEMP_DIR_NAME;
    return bpkTmpDir;
}

string BuildPushKill::GetModuleNameBySubObjectPath(string path)
{
    string      moduleName  = "";
    char*       pSaveBuf    = nullptr;
    char*       pStr        = nullptr;
    char*       pTmp        = nullptr;
    unsigned    pathLen     = path.length();
    char*       pPath       = new char[pathLen + 1];
    memcpy(pPath, path.c_str(), pathLen + 1);

    pStr = strtok_r(pPath, "/", &pSaveBuf);
    while (nullptr != pStr)
    {
        if (nullptr != (pTmp = strstr(pStr, MODULE_NAME_MARK)))
        {
            moduleName = pStr;
            moduleName = moduleName.substr(0, pTmp - pStr);
            break;
        }

        pStr = strtok_r(nullptr, "/", &pSaveBuf);
    }

    return moduleName;
}

string BuildPushKill::GetModulePathByModuleName(
                            const deque<shared_ptr<ObjDepsInfo>>& rObjDepsInfoList, 
                            const deque<string>& rSearchDirList,
                            string name,
                            string* pNameWithSuffix)
{
    string      path        = "";
    bool        result      = true;
    unsigned    objNum      = rObjDepsInfoList.size();
    bool        isFound     = false;

    if (0 == name.compare("") || 0 == rObjDepsInfoList.size() || 0 == rSearchDirList.size())
    {
        result = false;
    }

    if (nullptr != pNameWithSuffix) *pNameWithSuffix = "";

    if (true == result)
    {
        name = name + MODULE_NAME_MARK + "/" + name;
        for (unsigned i = 0; i < objNum; i++)
        {
            if (nullptr != strstr(rObjDepsInfoList[i]->objPath.c_str(), name.c_str()))
            {
                const char*     pObjPath            = rObjDepsInfoList[i]->objPath.c_str();
                unsigned        objPathLen          = rObjDepsInfoList[i]->objPath.length();
                unsigned        moduleNameStartOff  = 0;
                unsigned        moduleNameLen       = 0;
                for (int j = objPathLen - 1; j >= 0; j--)
                {
                    if ('/' == pObjPath[j])
                    {
                        moduleNameStartOff  = j + 1;
                        moduleNameLen       = objPathLen - moduleNameStartOff;
                        break;
                    }
                }

                name    = rObjDepsInfoList[i]->objPath.substr(moduleNameStartOff);
                isFound = true;
                break;
            }
        }

        if (true == isFound)
        {
            isFound         = false;
            unsigned dirNum = rSearchDirList.size();
            for (unsigned i = 0; i < dirNum; i++)
            {
                path = Utils::FindFile(rSearchDirList[i], name);
                if (0 != path.compare(""))
                {
                    isFound = true;
                    if (nullptr != pNameWithSuffix) *pNameWithSuffix = name;
                    break;
                }
            }
        }
    }

    if (true != result || true != isFound)
    {
        path = "";
    }

    return path;
}

