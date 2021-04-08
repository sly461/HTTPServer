#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <string>
#include <string.h>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"

class HTTPResponse {
public:
    HTTPResponse();
    ~HTTPResponse();

    void Init(const std::string& rootDir, std::string& path, bool isKeepAlive, int code);
    void MakeResponse(Buffer& buffer);
    void UnmapFile();

private:
    int m_code;
    bool m_isKeepAlive;

    std::string m_path;
    std::string m_rootDir;

    char * m_mmFile;
    struct stat m_mmFileStat;
    
};

#endif