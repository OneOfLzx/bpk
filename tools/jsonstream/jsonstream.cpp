#include "jsonstream.h"

JsonStream::JsonStream()
    : m_isOpen(false)
    , m_path("")
    , m_jsonReader()
    , m_jsonWriter()
    , m_jsonValueRoot()
{

}

JsonStream::~JsonStream()
{

}

bool JsonStream::Open(string path)
{
    bool    result  = true;
    char*   jsonBuf = nullptr;

    if (true == result)
    {
        ifstream in(path, ios::in);
        if (false == in.is_open())
        {
            ofstream out(path, ios::out | ios::binary);
            out.close();
            m_isOpen = true;
        }
        else
        {
            if (true == result)
            {
                jsonBuf = Utils::ReadFile(path.c_str());
                if (nullptr == jsonBuf)
                {
                    result = false;
                }
            }

            if (true == result)
            {
                result = m_jsonReader.parse(jsonBuf, m_jsonValueRoot);
            }

            if (nullptr != jsonBuf)
            {
                Utils::FreeFileBuf(jsonBuf);
            }

            if (true == result)
            {
                m_isOpen = true;
            }
        }
    }

    if (true == result && true == m_isOpen)
    {
        m_path = path;
    }

    return result;
}

bool JsonStream::UpdateTofile()
{
    bool result = true;

    if (false == m_isOpen)
    {
        result = false;
    }

    if (true == result)
    {
        string jsonBuf = m_jsonWriter.write(m_jsonValueRoot);
        result = Utils::WriteFile(m_path.c_str(), jsonBuf.c_str(), jsonBuf.length());
    }

    return result;
}