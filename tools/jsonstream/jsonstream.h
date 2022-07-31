#ifndef JSON_STREAM_H
#define JSON_STREAM_H

#include <iostream>
#include <string>
#include <fstream>
#include "../jsoncpp/include/json/reader.h"
#include "../jsoncpp/include/json/writer.h"
#include "../utils/utils.h"

using namespace std;

class JsonStream
{
public:
    JsonStream();
    ~JsonStream();
    bool Open(string path);
    bool UpdateTofile();
    inline Json::Value& GetRoot()
    {
        return m_jsonValueRoot;
    }
    inline bool IsOpen()
    {
        return m_isOpen;
    }
private:
    bool                m_isOpen;
    string              m_path;
    Json::Reader        m_jsonReader;
    Json::FastWriter    m_jsonWriter;
    Json::Value         m_jsonValueRoot;
};








#endif  //  JSON_STREAM_H