#include "utils.h"

string Utils::GetAndroidRootPath()
{
    FILE*   pF                  = nullptr;
    string  androidRootPath     = "";
    char*   pAndroidRootPath    = new char[MAX_PATH_LEN];
    memset(pAndroidRootPath, 0, MAX_PATH_LEN);

    pF = popen("echo $ANDROID_BUILD_TOP", "r");
    fgets(pAndroidRootPath, MAX_PATH_LEN, pF);
    pclose(pF);

    unsigned strLen = strlen(pAndroidRootPath);
    for (unsigned i = 0; i < strLen; i++)
        if ('\r' == pAndroidRootPath[i] || '\n' == pAndroidRootPath[i])
            pAndroidRootPath[i] = '\0';
    androidRootPath = pAndroidRootPath;
    delete[] pAndroidRootPath;
    return androidRootPath;
}

string Utils::GetCurPath()
{
    string  path    = "";
    char*   pPath   = new char[MAX_PATH_LEN];
    memset(pPath, 0, MAX_PATH_LEN);
    getcwd(pPath, MAX_PATH_LEN);

    path = pPath;
    delete[] pPath;
    return path;
}

string Utils::GetTargetsOutPath()
{
    FILE*   pF          = nullptr;
    string  outPath     = "";
    char*   pOutPath    = new char[MAX_PATH_LEN];
    memset(pOutPath, 0, MAX_PATH_LEN);

    pF = popen("echo $ANDROID_PRODUCT_OUT", "r");
    fgets(pOutPath, MAX_PATH_LEN, pF);
    pclose(pF);

    unsigned strLen = strlen(pOutPath);
    for (unsigned i = 0; i < strLen; i++)
        if ('\r' == pOutPath[i] || '\n' == pOutPath[i])
            pOutPath[i] = '\0';
    outPath = pOutPath;
    delete[] pOutPath;
    return outPath;
}

string Utils::GetProductName()
{
    FILE*   pF              = nullptr;
    string  productName     = "";
    char*   pProductName    = new char[MAX_PATH_LEN];
    memset(pProductName, 0, MAX_PATH_LEN);

    pF = popen("echo $TARGET_PRODUCT", "r");
    fgets(pProductName, MAX_PATH_LEN, pF);
    pclose(pF);

    unsigned strLen = strlen(pProductName);
    for (unsigned i = 0; i < strLen; i++)
        if ('\r' == pProductName[i] || '\n' == pProductName[i])
            pProductName[i] = '\0';
    productName = pProductName;
    delete[] pProductName;
    return productName;
}

bool Utils::IsDirExist(const char* pDir)
{
    if (nullptr != pDir && nullptr != opendir(pDir))
    {
        return true;
    }
    return false;
}

bool Utils::IsFileExist(const char* pFilePath)
{
    struct stat statBuf = { };
    if (nullptr != pFilePath && 0 == stat(pFilePath, &statBuf))
    {
        return true;
    }
    return false;
}

unsigned long long Utils::GetCurrentSecCounts()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

unsigned long long Utils::GetCurrentMilliSecCounts()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

unsigned long long Utils::GetCurrentMicroSecCounts()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

unsigned long long Utils::GetFileMTimeSec(const char* pPath)
{
    struct stat         statInfo    = { };
    unsigned long long  mtime       = 0;

    if (0 == stat(pPath, &statInfo))
    {
        mtime = statInfo.st_mtim.tv_sec;
    }

    return mtime;
}

bool Utils::WriteFile(const char* pPath, const char* pBuf, unsigned size)
{
    bool        result  = true;
    ofstream    writer;

    if (true == result)
    {
        if (nullptr == pPath || nullptr == pBuf || 0 == size)
        {
            result = false;
        }
    }

    if (true == result)
    {
        writer.open(pPath, ios::out | ios::binary);
        if (true == writer.is_open())
        {
            writer.write(pBuf, size);
            writer.close();
        }
        else
        {
            result = false;
        }
    }

    return result;
}

char* Utils::ReadFile(const char* pPath)
{
    char*       pBuf = nullptr;
    ifstream    reader;

    reader.open(pPath, ios::in | ios::binary);
    if (true == reader.is_open())
    {
        reader.seekg(0, ios::end);
        unsigned fileSize = reader.tellg();
        reader.seekg(0, ios::beg);

        pBuf = new char[fileSize];
        if (nullptr != pBuf)
        {
            reader.read(pBuf, fileSize);
        }
    
        reader.close();
    }

    return pBuf;
}

void Utils::FreeFileBuf(char* pBuf)
{
    if (nullptr != pBuf)
    {
        delete[] pBuf;
    }
}

string Utils::GetUserPath()
{
    FILE*   pF      = nullptr;
    string  str     = "";
    char*   pStr    = new char[MAX_PATH_LEN];
    memset(pStr, 0, MAX_PATH_LEN);

    pF = popen("cd ~;echo $(pwd)", "r");
    fgets(pStr, MAX_PATH_LEN, pF);
    pclose(pF);

    unsigned strLen = strlen(pStr);
    for (unsigned i = 0; i < strLen; i++)
        if ('\r' == pStr[i] || '\n' == pStr[i])
            pStr[i] = '\0';
    str = pStr;
    delete[] pStr;
    return str;
}

string Utils::FindFile(const string absoluteDir, const string fileName)
{
    string              filePath        = "";
    bool                isFound         = false;
    DIR*                pDirStream      = nullptr;
    struct dirent*      pDirEntry       = nullptr;
    unsigned            pathLen         = 0;
    char*               newPath         = nullptr;
    char*               dirPath         = nullptr;
    deque<char*>        dirPathList;
    dirPathList.clear();

    pathLen     = absoluteDir.length() + 1;
    newPath     = new char[pathLen];
    sprintf(newPath, "%s", absoluteDir.c_str());
    dirPathList.push_back(newPath);

    while (0 < dirPathList.size())
    {
        dirPath = dirPathList.front();

        if (nullptr != (pDirStream = opendir(dirPath)))
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
                    pathLen = strlen(dirPath) + strlen(pDirEntry->d_name) + 2;
                    newPath = new char[pathLen];
                    sprintf(newPath, "%s/%s", dirPath, pDirEntry->d_name);

                    dirPathList.push_back(newPath);
                }
                else if (DT_REG == pDirEntry->d_type)
                {
                    if (0 == strcmp(fileName.c_str(), pDirEntry->d_name))
                    {
                        filePath    = dirPath + string("/") + fileName;
                        isFound     = true;
                        break;
                    }
                }
            }

            closedir(pDirStream);
        }

        delete[] dirPath;
        dirPathList.pop_front();

        if (true == isFound)
        {
            break;
        }
    }

    unsigned restNum = dirPathList.size();
    for (unsigned i = 0; i < restNum; i++)
    {
        if (nullptr != dirPathList[i])
            delete[] dirPathList[i];
    }
    dirPathList.clear();

    return filePath;
}

void Utils::FindFileFuzzy(const string absoluteDir, const string fileName, deque<string>& rResultPath)
{
    ThreadPool      threadPool(std::thread::hardware_concurrency(), true);
    mutex           resultPathLock;
    rResultPath.clear();

    threadPool.enqueue(FindFileFuzzyAsync, absoluteDir, fileName, &threadPool,
                    &rResultPath, &resultPathLock);
}

void Utils::FindFileFuzzyAsync(const string absoluteDir, const string fileName,
                ThreadPool* pThreadPool, deque<string>* pResultPath, mutex* pResultPathLock)
{
    string              filePath        = "";
    bool                isFound         = false;
    DIR*                pDirStream      = nullptr;
    struct dirent*      pDirEntry       = nullptr;
    unsigned            pathLen         = 0;
    char*               newPath         = nullptr;
    char*               dirPath         = nullptr;
    deque<char*>        dirPathList;
    deque<string>       resultPath;
    dirPathList.clear();
    resultPath.clear();

    pathLen     = absoluteDir.length() + 1;
    newPath     = new char[pathLen];
    sprintf(newPath, "%s", absoluteDir.c_str());
    dirPathList.push_back(newPath);

    while (0 < dirPathList.size())
    {
        dirPath = dirPathList.front();

        if (nullptr != (pDirStream = opendir(dirPath)))
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
                    pathLen = strlen(dirPath) + strlen(pDirEntry->d_name) + 2;
                    newPath = new char[pathLen];
                    sprintf(newPath, "%s/%s", dirPath, pDirEntry->d_name);

                    dirPathList.push_back(newPath);
                }
                else if (DT_REG == pDirEntry->d_type)
                {
                    if (nullptr != strstr(pDirEntry->d_name, fileName.c_str()))
                    {
                        resultPath.emplace_back(dirPath + string("/") + pDirEntry->d_name);
                    }
                }
            }

            closedir(pDirStream);
        }

        delete[] dirPath;
        dirPath = nullptr;
        dirPathList.pop_front();

        unsigned dirNum = dirPathList.size();
        for (unsigned i = 0; i < dirNum; i++)
        {
            pThreadPool->enqueue(FindFileFuzzyAsync, string(dirPathList[i]), fileName, pThreadPool,
                            pResultPath, pResultPathLock);
            delete[] dirPathList[i];
            dirPathList[i] = nullptr;

        }
        dirPathList.clear();
    }

    unsigned restNum = dirPathList.size();
    for (unsigned i = 0; i < restNum; i++)
    {
        if (nullptr != dirPathList[i])
            delete[] dirPathList[i];
    }
    dirPathList.clear();

    pResultPathLock->lock();
    unsigned resultNum = resultPath.size();
    for (unsigned i = 0; i < resultNum; i++)
    {
        pResultPath->push_back(resultPath[i]);
    }
    pResultPathLock->unlock();
}

bool Utils::RemoveDir(const char* pDir)
{
    bool    result  = true;
    string  cmd     = string("rm -fr ") + pDir;
    if (-1 == system(cmd.c_str()))
    {
        result = false;
    }
    return result;
}

bool Utils::ADBKillProcessByNameRelated(const char* name)
{
    if (0 == strcmp(name, "") ||
        0 == strcmp(name, " "))
    {
        return false;
    }

    string cmd = "adb shell kill -9 $(adb shell ps -A|grep " + string(name) + "|awk '{print $2}')";
    return ShellCMDNoLog(cmd);
}

bool Utils::ADBKillProcessByName(const char* name)
{
    string cmd = "adb shell kill -9 $(adb shell pidof " + string(name) + ")";
    return ShellCMDNoLog(cmd);
}

string Utils::GetFileNameNoSuffix(const string fileName)
{
    string      nameNoSuffix    = "";
    unsigned    nameLen         = fileName.length();
    const char* pFileName       = fileName.c_str();

    for (int i = nameLen - 1; i >= 1; i--)
    {
        if ('.' == pFileName[i])
        {
            nameNoSuffix = fileName.substr(0, i);
            break;
        }
    }

    return nameNoSuffix;
}

bool Utils::ADBStopAPK(const string packageName)
{
    string cmd = "adb shell am force-stop " + packageName;
    return ShellCMDNoLog(cmd);
}

bool Utils::ADBOpenAPK(const string packageName, const string activityName)
{
    string cmd = "adb shell am start " + packageName + "/" + packageName + "." + activityName;
    return ShellCMDNoLog(cmd);
}

bool Utils::ADBRR()
{
    bool    result  = true;
    string  output  = "";

    if (true == result)
    {
        output = GetShellOutput("adb root;adb remount");
        if (nullptr == strstr(output.c_str(), "as root") ||
            nullptr == strstr(output.c_str(), "remount succeeded"))
        {
            result = false;
        }
    }

    return result;
}

string Utils::GetShellOutput(string cmd)
{
    FILE*   pF              = nullptr;
    string  output          = "";
    char*   pOutput         = new char[MAX_SHELL_OUT_SIZE];
    memset(pOutput, 0, MAX_SHELL_OUT_SIZE);

    pF = popen(cmd.c_str(), "r");
    if (nullptr != pF)
    {
        fread(pOutput, MAX_SHELL_OUT_SIZE, MAX_SHELL_OUT_SIZE, pF);
        pclose(pF);
    }

    output = pOutput;
    delete[] pOutput;
    return output;
}

bool Utils::ShellCMDNoLog(string cmd)
{
    bool    result  = true;
    FILE*   pF      = nullptr;

    if (nullptr != (pF = popen(cmd.c_str(), "r")))
    {
        pclose(pF);
    }
    else
    {
        result = false;
    }

    return result;
}