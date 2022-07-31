#include "build_push_kill.h"

#define RESET_TIMESTAMP_FLAG    "-r"
#define CLEAN_TEMP_FLIE_FLAG    "-c"

int main(int argc, char** argv)
{
    int             result = 0;
    deque<string>   buildTargets;
    buildTargets.clear();

    for (unsigned i = 0; i < argc; i++)
    {
        if (nullptr != argv[i] && 0 == strcmp(argv[i], CLEAN_TEMP_FLIE_FLAG))
        {
            result = BuildPushKill::CleanTempFile();
            return result;
        }
    }

    for (unsigned i = 0; i < argc; i++)
    {
        if (nullptr != argv[i] && 0 == strcmp(argv[i], RESET_TIMESTAMP_FLAG))
        {
            result = BuildPushKill::ResetBuildTime();
            return result;
        }
    }

    for (unsigned i = 1; i < argc; i++)
    {
        if (nullptr != argv[i])
        {
            buildTargets.push_back(argv[i]);
        }
    }
    result = BuildPushKill::Run(buildTargets);

    return result;
}